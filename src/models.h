#pragma once
#include <QDate>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <optional>

// ─────────────────────────────────────────────────────────────────────────────
// GroupTotals
// Revenue totals for one LSK accounting group in one period.
// ─────────────────────────────────────────────────────────────────────────────
struct GroupTotals
{
    double total_amount = 0.0;
    double tax = 0.0;
    double service_charge = 0.0;
    double discount = 0.0;
    int transaction_count = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// PeriodData
// All group totals for one time period (today / MTD / etc.)
// ─────────────────────────────────────────────────────────────────────────────
struct PeriodData
{
    QMap<QString, GroupTotals> groups;
    double total_amount = 0.0;
    double tax = 0.0;
    double service_charge = 0.0;
    int transaction_count = 0;

    // Parse a JSON response from the LSK API into a PeriodData.
    static PeriodData fromLskResponse(const QJsonObject& json);

    // Return total_amount for a group, 0.0 if not present.
    double groupAmount(const QString& key) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// FinancialDashboard
// Four comparison periods returned together from the LSK API.
// ─────────────────────────────────────────────────────────────────────────────
struct FinancialDashboard
{
    QDate as_of;
    PeriodData today;
    PeriodData same_day_last_year;
    PeriodData mtd;
    PeriodData mtd_last_year;

    double mtd_variance_amt(const QString& group) const;
    std::optional<double> mtd_variance_pct(const QString& group) const;
    std::optional<double> mtd_total_variance_pct() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// CloudbedsMetrics
// Occupancy and revenue KPIs from Cloudbeds for a single date.
// ─────────────────────────────────────────────────────────────────────────────
struct CloudbedsMetrics
{
    QDate date;
    int arrivals = 0;
    int occupied_rooms = 0;
    int total_rooms = 0;

    double room_revenue = 0.0;
    double room_revenue_othr = 0.0;
    double adr = 0.0;
    double revpar = 0.0;
    double occupancy_pct = 0.0;

    // MTD
    double mtd_revenue = 0.0;
    double mtd_revenue_othr = 0.0;
    double mtd_revpar = 0.0;
    double mtd_adr = 0.0;

    // Last year comparisons
    int ly_date_occupied = 0;
    int ly_date_capacity = 0;
    int ly_month_occupied = 0;
    int ly_month_capacity = 0;
    double ly_mtd_revenue = 0.0;
    double ly_mtd_revenue_othr = 0.0;
    double ly_mtd_revpar = 0.0;
    double ly_mtd_adr = 0.0;
};
