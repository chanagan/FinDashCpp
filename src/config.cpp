#include "config.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDateTime>
#include <QStandardPaths>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

// ── Helpers ───────────────────────────────────────────────────────────────────

static fs::path homeDir()
{
#ifdef _WIN32
    const char *home = std::getenv("USERPROFILE");
#else
    const char *home = std::getenv("HOME");
#endif
    return home ? fs::path(home) : fs::current_path();
}

static bool hasConfig(const fs::path &dir)
{
    return fs::exists(dir / "config.json");
}

// ── Config::findConfigDir ─────────────────────────────────────────────────────

fs::path Config::findConfigDir()
{
    const std::string subfolder = "FinDash";

#ifdef Q_OS_WIN
    qDebug() << "Looking for Windows config";
    // Windows: OneDrive path with space
    // "C:\Users\{username}\OneDrive - Island House Key West\FinDash\Config.json"
    QString homeDirStr = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString configPath = homeDirStr + "/OneDrive - Island House Key West/FinDash";
    // QString configPath = homeDirStr + "/OneDrive - Island House Key West/FinDash/Config.json";
    qDebug() << "Looking for config at (Windows):" << configPath;
    fs::path p (configPath.toStdString());
    return p;

#endif

    // 1. Environment variable override
    qInfo() << "[Config] - 1 - try environment variable";
    if (const char *env = std::getenv("FINDASH_CONFIG_DIR")) {
        fs::path p{env};
        if (hasConfig(p)) {
            qInfo() << "[Config]" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
                    << "Using FINDASH_CONFIG_DIR:" << QString::fromStdString(p.string());
            return p;
        }
    }

    // 2. Pointer file (covers SharePoint and any custom path)
    fs::path appDir = fs::path(
        QCoreApplication::applicationDirPath().toStdString()
    );
    fs::path pointerFile = appDir / "config_dir.txt";
    qInfo() << "[Config] - 2 - try pointer file";
    if (fs::exists(pointerFile)) {
        std::ifstream f(pointerFile);
        std::string line;
        std::getline(f, line);
        // trim trailing whitespace / CR
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        fs::path p{line};
        if (hasConfig(p)) {
            qInfo() << "[Config]" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
                    << "Using config_dir.txt:" << QString::fromStdString(p.string());
            return p;
        }
    }

    // 3. OneDrive personal (macOS + Windows)
    qInfo() << "[Config] - 3 - oneDrive personal";
    fs::path home = homeDir();
    std::vector<fs::path> candidates = {
        home / "OneDrive" / subfolder,
        home / "OneDrive - Personal" / subfolder,
    };
    // macOS modern OneDrive sync: ~/Library/CloudStorage/OneDrive-*/FinDash
    fs::path cloudStorage = home / "Library" / "CloudStorage";
    if (fs::is_directory(cloudStorage)) {
        for (const auto &entry : fs::directory_iterator(cloudStorage)) {
            if (entry.path().filename().string().starts_with("OneDrive")) {
                candidates.push_back(entry.path() / subfolder);
            }
        }
    }
    for (const auto &p : candidates) {
        if (hasConfig(p)) {
            qInfo() << "[Config]" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
                    << "Using OneDrive path:" << QString::fromStdString(p.string());
            return p;
        }
    }

    // 4. Application directory (dev fallback)
    qInfo() << "[Config] - 4 - application directory";
    qWarning() << "[Config]" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
               << "config.json not found in any candidate — using app dir.";
    return appDir;
}

// ── Config singleton ──────────────────────────────────────────────────────────

Config::Config()
    : m_configDir(findConfigDir())
{}

Config &Config::instance()
{
    static Config inst;
    return inst;
}

const fs::path &Config::configDir() const       { return m_configDir; }
fs::path Config::configFilePath()    const       { return m_configDir / "config.json"; }
fs::path Config::reportDefFilePath() const       { return m_configDir / "daily_rep_upd_def.json"; }

void Config::ensureLoaded() const
{
    if (m_loaded) return;
    QString fileName = QString::fromStdString(configFilePath().string());
    QFile f(fileName);
    // QFile f(QString::fromStdString(configFilePath().string()));
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[Config] Cannot open" << f.fileName();
        m_loaded = true;
        return;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[Config] JSON parse error:" << err.errorString();
    } else {
        m_cache = doc.object();
    }
    m_loaded = true;
}

QJsonObject Config::fullConfig() const
{
    ensureLoaded();
    return m_cache;
}

QJsonObject Config::cloudbedsConfig() const
{
    return fullConfig()["cloudbeds"].toObject();
}

QJsonObject Config::lskConfig() const
{
    return fullConfig()["lsk"].toObject();
}
