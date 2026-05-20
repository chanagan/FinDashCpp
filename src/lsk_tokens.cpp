#include "lsk_tokens.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QThread>
#include <QDebug>
#include <QDateTime>
#include <ctime>
#include <stdexcept>

LskTokenManager::LskTokenManager(const QString &clientId,
                                 const QString &clientSecret,
                                 const std::filesystem::path &tokenStorePath)
    : m_clientId(clientId)
    , m_clientSecret(clientSecret)
    , m_tokenStorePath(tokenStorePath)
{
    loadTokens();
}

QString LskTokenManager::getAccessToken()
{
    // Check if we have valid tokens
    qint64 now = QDateTime::currentSecsSinceEpoch();
    if (!m_tokens.accessToken.isEmpty() && m_tokens.expiresAt > (now + SKEW_SECONDS)) {
        return m_tokens.accessToken;
    }

    // Refresh if we have a refresh token
    if (!m_tokens.refreshToken.isEmpty()) {
        refreshTokens();
        return m_tokens.accessToken;
    }

    throw std::runtime_error("No valid access token and no refresh token available");
}

void LskTokenManager::refreshTokens()
{
    if (m_tokens.refreshToken.isEmpty()) {
        throw std::runtime_error("Cannot refresh: no refresh_token stored");
    }

    try {
        TokenData newTokens = fetchNewTokens(m_tokens.refreshToken);
        m_tokens = newTokens;
        saveTokens();
        qInfo() << "[LSK] Tokens refreshed successfully";
    } catch (const std::exception &ex) {
        qCritical() << "[LSK] Token refresh failed:" << ex.what();
        throw;
    }
}

bool LskTokenManager::hasValidTokens() const
{
    if (m_tokens.accessToken.isEmpty()) return false;
    qint64 now = QDateTime::currentSecsSinceEpoch();
    return m_tokens.expiresAt > now;
}

void LskTokenManager::loadTokens()
{
    if (m_tokensLoaded) return;
    m_tokensLoaded = true;

    if (!std::filesystem::exists(m_tokenStorePath)) {
        qInfo() << "[LSK] No token file found at" << QString::fromStdString(m_tokenStorePath.string());
        return;
    }

    QFile f(QString::fromStdString(m_tokenStorePath.string()));
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[LSK] Cannot open token file:" << f.fileName();
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();

    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[LSK] JSON parse error in token file:" << err.errorString();
        return;
    }

    QJsonObject obj = doc.object();
    m_tokens.accessToken = obj["access_token"].toString();
    m_tokens.refreshToken = obj["refresh_token"].toString();
    m_tokens.expiresAt = obj["expires_at"].toInt();

    qInfo() << "[LSK] Tokens loaded from file";
}

void LskTokenManager::saveTokens()
{
    QJsonObject obj;
    obj["access_token"] = m_tokens.accessToken;
    obj["refresh_token"] = m_tokens.refreshToken;
    obj["expires_at"] = static_cast<int>(m_tokens.expiresAt);

    QFile f(QString::fromStdString(m_tokenStorePath.string()));
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "[LSK] Cannot write token file:" << f.fileName();
        return;
    }

    f.write(QJsonDocument(obj).toJson());
    f.close();
    qInfo() << "[LSK] Tokens saved to file";
}

QString LskTokenManager::encodeBasicAuth(const QString &clientId, const QString &clientSecret)
{
    QString credentials = clientId + ":" + clientSecret;
    QByteArray raw = credentials.toUtf8();
    return "Basic " + raw.toBase64();
}

LskTokenManager::TokenData LskTokenManager::fetchNewTokens(const QString &refreshToken)
{
    QNetworkAccessManager network;
    network.moveToThread(QThread::currentThread());

    QNetworkRequest req{QUrl(TOKEN_URL)};

    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    req.setRawHeader("Authorization", encodeBasicAuth(m_clientId, m_clientSecret).toUtf8());

    // Prepare request body
    QUrlQuery query;
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", refreshToken);

    QNetworkReply *reply = network.post(req, query.toString().toUtf8());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString msg = reply->errorString();
        reply->deleteLater();
        throw std::runtime_error("Token refresh HTTP error: " + msg.toStdString());
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &err);
    reply->deleteLater();

    if (err.error != QJsonParseError::NoError) {
        throw std::runtime_error("Token response JSON parse error: " + err.errorString().toStdString());
    }

    QJsonObject obj = doc.object();
    TokenData result;
    result.accessToken = obj["access_token"].toString();
    result.refreshToken = obj.contains("refresh_token")
                          ? obj["refresh_token"].toString()
                          : refreshToken;  // Keep old refresh token if not returned
    result.expiresAt = QDateTime::currentSecsSinceEpoch() + obj["expires_in"].toInt(3600);

    if (result.accessToken.isEmpty()) {
        throw std::runtime_error("No access_token in response");
    }

    return result;
}
