#pragma once
#include <QJsonObject>
#include <QString>
#include <filesystem>

// ─────────────────────────────────────────────────────────────────────────────
// Config
// Singleton that locates and caches config.json.
//
// Resolution order (first directory containing config.json wins):
//   1. FINDASH_CONFIG_DIR environment variable
//   2. config_dir.txt pointer file in the application directory
//   3. ~/OneDrive/FinDash/  and  ~/Library/CloudStorage/OneDrive-*/FinDash/
//   4. Application directory (dev fallback)
// ─────────────────────────────────────────────────────────────────────────────
class Config
{
public:
    static Config &instance();

    // Returns the resolved config directory.
    const std::filesystem::path &configDir() const;

    // Returns the full path to config.json.
    std::filesystem::path configFilePath()     const;

    // Returns the full path to daily_rep_upd_def.json.
    std::filesystem::path reportDefFilePath()  const;

    // Returns the 'cloudbeds' section of config.json.
    QJsonObject cloudbedsConfig() const;

    // Returns the 'lsk' section of config.json.
    QJsonObject lskConfig() const;

    // Returns the full parsed config object.
    QJsonObject fullConfig()      const;

private:
    Config();                                    // private — use instance()
    static std::filesystem::path findConfigDir();

    std::filesystem::path m_configDir;
    mutable QJsonObject   m_cache;
    mutable bool          m_loaded = false;

    void ensureLoaded() const;
};
