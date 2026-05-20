#pragma once

#include "models.h"
#include "lsk_tokens.h"
#include <QDate>
#include <memory>

class QNetworkAccessManager;

// ─────────────────────────────────────────────────────────────────────────────
// LSK API Client
//
// Fetches financial data from Lightspeed API using OAuth2 tokens.
// Converts API responses into FinancialDashboard structure.
// ─────────────────────────────────────────────────────────────────────────────

class LskApi
{
public:
    // Initialize with credentials and business location ID
    explicit LskApi(const QString &clientId,
                   const QString &clientSecret,
                   const QString &businessLocationId,
                   const QString &tokenFilePath);

    // Fetch daily sales data
    QJsonObject getDailyTotals(const QDate &date);

    // Fetch MTD (month-to-date) sales data
    QJsonObject getMtdTotals(const QDate &date);

    // Fetch LY-MTD (month-to-date) sales data
    QJsonObject getLyMtdTotals(const QDate &date);

    // Convert API response to FinancialDashboard
    static PeriodData parseGroupData(const QJsonObject &groupData);
    static FinancialDashboard buildDashboard(const QDate &date,
                                             const QJsonObject &daily,
                                             const QJsonObject &mtd,
                                             const QJsonObject &ly_mtd);

private:
    // Date formatting helpers
    static QString formatDate(const QDate &d);  // "YYYY-MM-DD"
    static QString formatDateTime(const QDate &d, int hour, int minute, int second);  // ISO 8601 with TZ

    // HTTP helper
    QJsonObject httpGet(const QString &url, const QMap<QString,QString> &params);

    // Members
    std::unique_ptr<LskTokenManager> m_tokenManager;
    QString m_businessLocationId;

    static constexpr const char *API_BASE = "https://api.lsk.lightspeed.app/f/finance";
    static constexpr const char *REPORT_ID = "aggregatedSales";
};
