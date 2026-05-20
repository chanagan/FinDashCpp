#include "models.h"
#include <QJsonArray>
#include <QJsonValue>

// ── PeriodData ────────────────────────────────────────────────────────────────

PeriodData PeriodData::fromLskResponse(const QJsonObject &json)
{
    PeriodData p;
    QJsonObject groupsJson = json["groups"].toObject();
    for (const QString &key : groupsJson.keys()) {
        QJsonObject g = groupsJson[key].toObject();
        GroupTotals gt;
        gt.total_amount      = g["total_amount"].toDouble();
        gt.tax               = g["tax"].toDouble();
        gt.service_charge    = g["service_charge"].toDouble();
        gt.discount          = g["discount"].toDouble();
        gt.transaction_count = g["transaction_count"].toInt();
        p.groups[key] = gt;
    }
    p.total_amount      = json["total_amount"].toDouble();
    p.tax               = json["tax"].toDouble();
    p.service_charge    = json["service_charge"].toDouble();
    p.transaction_count = json["transaction_count"].toInt();
    return p;
}

double PeriodData::groupAmount(const QString &key) const
{
    return groups.value(key).total_amount;
}

// ── FinancialDashboard ────────────────────────────────────────────────────────

double FinancialDashboard::mtd_variance_amt(const QString &group) const
{
    return mtd.groupAmount(group) - mtd_last_year.groupAmount(group);
}

std::optional<double> FinancialDashboard::mtd_variance_pct(const QString &group) const
{
    double ly = mtd_last_year.groupAmount(group);
    if (ly == 0.0) return std::nullopt;
    return (mtd.groupAmount(group) - ly) / std::abs(ly) * 100.0;
}

std::optional<double> FinancialDashboard::mtd_total_variance_pct() const
{
    if (mtd_last_year.total_amount == 0.0) return std::nullopt;
    return (mtd.total_amount - mtd_last_year.total_amount)
           / std::abs(mtd_last_year.total_amount) * 100.0;
}
