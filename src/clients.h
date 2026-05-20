#pragma once
#include "models.h"
#include <QDate>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <filesystem>
#include <optional>

// ─────────────────────────────────────────────────────────────────────────────
// CloudbedsApiClient
//
// All HTTP calls are made synchronously (via a local QEventLoop) so they can
// be called from a worker QThread without any async callback plumbing.
//
// Key methods:
//   fetchMetrics(date)       — main entry; updates report def then fetches data
//   getRoomsOccupied(date)   — getDashboard day-by-day, cached to disk
// ─────────────────────────────────────────────────────────────────────────────
class CloudbedsApiClient : public QObject
{
    Q_OBJECT

public:
    explicit CloudbedsApiClient(const QString &apiKey,
                                const QString &propertyId,
                                QObject       *parent = nullptr);

    CloudbedsMetrics fetchMetrics(const QDate &targetDate);

private:
    // ── HTTP helpers ──────────────────────────────────────────────────────────
    // All return the parsed JSON body; throw std::runtime_error on failure.
    QJsonObject httpGet (const QUrl &url);
    QJsonObject httpPut (const QUrl &url, const QJsonObject &body);

    QJsonObject apiGet    (const QString &endpoint,   const QMap<QString,QString> &params = {});
    QJsonObject reportGet (const QString &reportId,   const QMap<QString,QString> &params = {});
    QJsonObject reportPut (const QString &reportId,   const QJsonObject &body);

    // ── Occupancy cache ───────────────────────────────────────────────────────
    struct OccEntry { int occupied = 0; int capacity = 0; };
    std::optional<OccEntry> occCached(const QDate &d) const;
    void                    occCache (const QDate &d, int occupied, int capacity);
    void                    loadOccCache();
    void                    saveOccCache() const;

    // ── Report definition ─────────────────────────────────────────────────────
    QJsonArray  buildPeriods(const QDate &targetDate) const;
    void        updateReportDef(const QString &reportId, const QDate &targetDate);

    // ── Occupancy fetch ───────────────────────────────────────────────────────
    struct OccupancyCounts {
        int this_date_occupied  = 0;  int this_date_capacity  = 0;
        int this_month_occupied = 0;  int this_month_capacity = 0;
        int ly_date_occupied    = 0;  int ly_date_capacity    = 0;
        int ly_month_occupied   = 0;  int ly_month_capacity   = 0;
    };
    OccupancyCounts getRoomsOccupied(const QDate &targetDate);

    // ── Formatting helpers ────────────────────────────────────────────────────
    static QString isoDate(const QDate &d);        // "YYYY-MM-DD"
    static QString isoDateTime(const QDate &d);    // "YYYY-MM-DDT00:00:00.000Z"

    // ── Members ───────────────────────────────────────────────────────────────
    QString                m_propertyId;
    QByteArray             m_authHeader;           // "Bearer <key>"

    QMap<QString, OccEntry> m_occCache;            // key = "YYYY-MM-DD"
    std::filesystem::path   m_occCacheFile;
    std::filesystem::path   m_reportDefFile;

    static constexpr const char *BASE_URL     = "https://api.cloudbeds.com/api/v1.1";
    static constexpr const char *BASE_REP_URL = "https://api.cloudbeds.com/datainsights/v1.1/reports";
    static constexpr const char *REPORT_ID    = "71673";
};

// ─────────────────────────────────────────────────────────────────────────────
// LskApiClient
//
// Thin wrapper around the GetLSK shared library / subprocess.
// (Adapt fetchDashboard() to however GetLSK is exposed in C++.)
// ─────────────────────────────────────────────────────────────────────────────
class LskApiClient
{
public:
    FinancialDashboard fetchDashboard(const QDate &targetDate);
};

// ─────────────────────────────────────────────────────────────────────────────
// Mock clients — return static data for UI development / offline testing
// ─────────────────────────────────────────────────────────────────────────────
class MockCloudbedsClient
{
public:
    CloudbedsMetrics fetchMetrics(const QDate &targetDate);
};

class MockLskClient
{
public:
    FinancialDashboard fetchDashboard(const QDate &targetDate);
};
