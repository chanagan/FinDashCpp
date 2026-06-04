#include "lsk_api.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QTimeZone>
#include <QThread>
#include <QDebug>
#include <stdexcept>

LskApi::LskApi(const QString &clientId,
               const QString &clientSecret,
               const QString &businessLocationId,
               const QString &tokenFilePath)
    : m_businessLocationId(businessLocationId)
{
    m_tokenManager = std::make_unique<LskTokenManager>(
        clientId, clientSecret, tokenFilePath.toStdString()
    );
}

QString LskApi::formatDate(const QDate &d)
{
    return d.toString("yyyy-MM-dd");
}

QString LskApi::formatDateTime(const QDate &d, int hour, int minute, int second)
{
    // Format as ISO 8601 with America/New_York timezone
    QDateTime dt(d, QTime(hour, minute, second));
    QTimeZone tz("America/New_York");
    dt.setTimeZone(tz);

    // Format: "2026-02-01T00:00:00-05:00"
    return dt.toString("yyyy-MM-ddThh:mm:ssZ");
}

QJsonObject LskApi::httpGet(const QString &url, const QMap<QString,QString> &params)
{
    // Build URL with parameters
    QString token = m_tokenManager->getAccessToken();
    QUrl fullUrl{url};
    QUrlQuery query;
    for (auto it = params.cbegin(); it != params.cend(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    fullUrl.setQuery(query);

    QNetworkRequest req{fullUrl};
    req.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    req.setRawHeader("Accept", "application/json");

    // Create network manager with explicit thread affinity
    QNetworkAccessManager network;
    network.moveToThread(QThread::currentThread());

    QNetworkReply *reply = network.get(req);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        QByteArray responseBody = reply->readAll();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();

        qWarning() << "[LSK] HTTP error (status" << statusCode << "):" << errorMsg << "Response:" << QString::fromUtf8(responseBody);
        throw std::runtime_error("LSK API HTTP " + std::to_string(statusCode) + ": " + errorMsg.toStdString());
    }

    QByteArray responseBody = reply->readAll();
    reply->deleteLater();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(responseBody, &err);

    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[LSK] JSON parse error:" << err.errorString() << "Response:" << QString::fromUtf8(responseBody);
        throw std::runtime_error("LSK API response JSON parse error: " + err.errorString().toStdString());
    }

    return doc.object();
}

QJsonObject LskApi::getDailyTotals(const QDate &date)
{
    QString url = QString("%1/%2/%3")
        .arg(API_BASE)
        .arg(m_businessLocationId)
        .arg(REPORT_ID);

    QMap<QString,QString> params;
    params["groupBy"] = "accountingGroup";
    params["date"] = formatDate(date);

    qInfo() << "[LSK] Fetching daily totals for" << date.toString(Qt::ISODate);
    QJsonObject response = httpGet(url, params);
    QByteArray responseBody = QJsonDocument(response).toJson();
    qDebug() << "[LSK] Daily response:" << QJsonDocument(response).toJson();
    return response;
}

QJsonObject LskApi::getMtdTotals(const QDate &date)
{
    QString url = QString("%1/%2/%3")
        .arg(API_BASE)
        .arg(m_businessLocationId)
        .arg(REPORT_ID);

    // From start of month to end of day
    QDate monthStart(date.year(), date.month(), 1);
    QString fromDate = formatDateTime(monthStart, 0, 0, 0);
    QString toDate = formatDateTime(date, 23, 59, 59);

    QMap<QString,QString> params;
    params["groupBy"] = "accountingGroup";
    params["from"] = fromDate;
    params["to"] = toDate;

    qInfo() << "[LSK] Fetching MTD totals for" << date.toString(Qt::ISODate);
    QJsonObject response = httpGet(url, params);
    qDebug() << R"([LSK] MTD response:)" << QJsonDocument(response).toJson();

    return response;
}

QJsonObject LskApi::getLyMtdTotals(const QDate &date)
{
    QString url = QString("%1/%2/%3")
        .arg(API_BASE)
        .arg(m_businessLocationId)
        .arg(REPORT_ID);

    // fix the date so points to last year
    int currentYear = date.year();
    int lastYear = currentYear - 1;
    QDate thisDateLastYear(lastYear, date.month(), date.day());
    // From start of month to end of day
    QDate monthStart(lastYear, date.month(), 1);
    // QDate monthStart(date.year(), date.month(), 1);
    QString fromDate = formatDateTime(monthStart, 0, 0, 0);
    QString toDate = formatDateTime(thisDateLastYear, 23, 59, 59);
    // QString toDate = formatDateTime(date, 23, 59, 59);

    QMap<QString,QString> params;
    params["groupBy"] = "accountingGroup";
    params["from"] = fromDate;
    params["to"] = toDate;

    qInfo() << "[LSK] Fetching LY-MTD totals for" << thisDateLastYear.toString(Qt::ISODate);
    QJsonObject response = httpGet(url, params);
    qDebug() << R"([LSK] LY-MTD response:)" << QJsonDocument(response).toJson();

    return response;
}

PeriodData LskApi::parseGroupData(const QJsonObject &response)
{
    // response structure:
    // - children[0].children[] - array of accounting groups
    // - each group has: groupByValue, totalAmount, totalTaxAmount, serviceCharge, numberOfSales
    PeriodData period;

    // Navigate: response -> children[0] -> children[]
    QJsonArray children = response["children"].toArray();
    if (children.isEmpty()) {
        qWarning() << "[LSK] No children in response";
        return period;
    }

    QJsonObject firstChild = children[0].toObject();
    QJsonArray groups = firstChild["children"].toArray();

    for (const QJsonValue &groupVal : groups) {
        QJsonObject groupObj = groupVal.toObject();

        QString groupName = groupObj["groupByValue"].toString();
        double totalAmount = groupObj["totalAmount"].toString().toDouble();
        double taxAmount = groupObj["totalTaxAmount"].toString().toDouble();
        double serviceCharge = groupObj["serviceCharge"].toString().toDouble();
        int transactionCount = groupObj["numberOfSales"].toInt();

        GroupTotals gt;
        gt.total_amount = totalAmount;
        gt.tax = taxAmount;
        gt.service_charge = serviceCharge;
        gt.transaction_count = transactionCount;

        period.groups[groupName] = gt;
        period.total_amount += totalAmount;
    }

    return period;
}

FinancialDashboard LskApi::buildDashboard(const QDate &date,
                                          const QJsonObject &daily,
                                          const QJsonObject &mtd,
                                          const QJsonObject &ly_mtd)
{
    FinancialDashboard dashboard;
    dashboard.as_of = date;

    // Parse daily data
    dashboard.today = parseGroupData(daily);

    // MTD data - need to calculate from first of month to specified date
    dashboard.mtd = parseGroupData(mtd);

    // LY-MTD - need to calculate from first of month to specified date - last year
    dashboard.mtd_last_year = parseGroupData(ly_mtd);

    // Same day last year - would need separate call or mock for now
    // For now, create empty/zero structure
    dashboard.same_day_last_year = PeriodData();
    // dashboard.mtd_last_year = PeriodData();

    qInfo() << "[LSK] Dashboard built: today total =" << dashboard.today.total_amount
    << "  MTD total =" << dashboard.mtd.total_amount
    << "  MTD-LY total =" << dashboard.mtd_last_year.total_amount;

    return dashboard;
}
