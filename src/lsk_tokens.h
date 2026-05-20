#pragma once

#include <QString>
#include <QJsonObject>
#include <filesystem>
#include <optional>

// ─────────────────────────────────────────────────────────────────────────────
// LSK Token Manager
//
// Handles OAuth2 token refresh for Lightspeed API using client credentials flow.
// Tokens are persisted to disk and refreshed automatically when expired.
// ─────────────────────────────────────────────────────────────────────────────

class LskTokenManager
{
public:
    // Initialize with credentials and token storage path
    explicit LskTokenManager(const QString &clientId,
                            const QString &clientSecret,
                            const std::filesystem::path &tokenStorePath);

    // Get valid access token (refreshes if needed)
    QString getAccessToken();

    // Manually refresh tokens (also happens automatically in getAccessToken)
    void refreshTokens();

    // Check if tokens exist and are valid
    bool hasValidTokens() const;

private:
    struct TokenData {
        QString accessToken;
        QString refreshToken;
        qint64 expiresAt = 0;  // Unix timestamp
    };

    // Token storage
    void loadTokens();
    void saveTokens();

    // HTTP operations
    TokenData fetchNewTokens(const QString &refreshToken);
    QString encodeBasicAuth(const QString &clientId, const QString &clientSecret);

    // Members
    QString m_clientId;
    QString m_clientSecret;
    std::filesystem::path m_tokenStorePath;
    TokenData m_tokens;
    bool m_tokensLoaded = false;

    static constexpr const char *TOKEN_URL = "https://api.lsk.lightspeed.app/oauth/token";
    static constexpr int SKEW_SECONDS = 60;  // Refresh 1 minute early
};
