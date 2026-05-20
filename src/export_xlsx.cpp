#include "export_xlsx.h"
#include "config.h"
#include <QDebug>
#include <QDate>
#include <QColor>
#include <stdexcept>

// QXlsx - Native Qt Excel library
#include <xlsxdocument.h>
#include <xlsxformat.h>

namespace fs = std::filesystem;

// ── LSK group key → spreadsheet row ──────────────────────────────────────────
// These row numbers should match your template layout
static const QList<QPair<QString,int>> LSK_ROW_MAP = {
    { "Cafe Bar",    31 },
    { "Cafe Food",   32 },
    { "Health Club", 37 },
    { "Laundry",     41 },
    { "Misc",        44 },
    { "Retail",      50 },
};

static double groupAmt(const PeriodData &p, const QString &key)
{
    auto it = p.groups.find(key);
    if (it != p.groups.end()) {
        return it.value().total_amount;
    }
    return 0.0;
}

fs::path exportDailyReport(
    const FinancialDashboard          &dash,
    const std::optional<CloudbedsMetrics> &cb,
    const fs::path                    &templatePath,
    const fs::path                    &outputDir)
{
    // Resolve output path
    auto &cfg = Config::instance();
    fs::path outDir = outputDir.empty() ? cfg.configDir() : outputDir;

    // Use provided template or look for default
    fs::path tmpl = templatePath.empty() ? (outDir / "template.xlsx") : templatePath;

    QString dateStr = dash.as_of.toString("yyyy-MM-dd");
    fs::path outPath = outDir / ("DailyReport_" + dateStr.toStdString() + ".xlsx");

    // Create output directory if it doesn't exist
    if (!fs::exists(outDir)) {
        fs::create_directories(outDir);
    }

    // Load template if it exists, otherwise create new
    QXlsx::Document xlsx(fs::exists(tmpl) ? QString::fromStdString(tmpl.string()) : "");

    if (fs::exists(tmpl)) {
        qInfo() << "[Export] Loaded template from:" << QString::fromStdString(tmpl.string());
    } else {
        qWarning() << "[Export] Template not found at:" << QString::fromStdString(tmpl.string());
        qInfo() << "[Export] Creating report without template";
    }

    // Create formats for data we'll write
    QXlsx::Format titleDateFormat;
    titleDateFormat.setPatternBackgroundColor(Qt::yellow);

    QXlsx::Format currencyFormat;
    currencyFormat.setNumberFormat("$#,##0.00");

    QXlsx::Format percentFormat;
    percentFormat.setNumberFormat("0.0%");

    // Write date in designated cell (adjust if needed for your template)
    xlsx.write("B2", dash.as_of.toString("MMM dd, yyyy"), titleDateFormat);

    // Write LSK Revenue data to template rows
    // Adjust column letters (B, C, D, E, F) based on your template layout
    for (const auto &[groupName, row] : LSK_ROW_MAP) {
        double today = groupAmt(dash.today, groupName);
        double mtd = groupAmt(dash.mtd, groupName);
        double mtd_ly = groupAmt(dash.mtd_last_year, groupName);

        // Write to specific cells (B=col2, C=col3, D=col4, E=col5, F=col6)
        // Adjust column letters to match your template
        xlsx.write(row, 2, today, currencyFormat);    // Column B
        xlsx.write(row, 3, mtd, currencyFormat);      // Column C
        xlsx.write(row, 4, mtd_ly, currencyFormat);   // Column D

        // Variance calculations
        double variance = mtd - mtd_ly;
        double variancePct = (mtd_ly != 0) ? (mtd - mtd_ly) / mtd_ly : 0.0;

        xlsx.write(row, 5, variance, currencyFormat); // Column E
        xlsx.write(row, 6, variancePct, percentFormat); // Column F
    }

    // Write Cloudbeds data to template (adjust rows/columns as needed)
    if (cb.has_value()) {
        // Example: adjust these row numbers to match your template
        xlsx.write(15, 2, cb->room_revenue, currencyFormat);
        xlsx.write(16, 2, QString("%1 of %2").arg(cb->occupied_rooms).arg(cb->total_rooms));
        xlsx.write(17, 2, cb->adr, currencyFormat);
        xlsx.write(18, 2, cb->revpar, currencyFormat);
        xlsx.write(19, 2, cb->mtd_revenue, currencyFormat);
    }

    // Save the workbook
    QString outPathQt = QString::fromStdString(outPath.string());
    bool success = xlsx.saveAs(outPathQt);

    if (!success) {
        qWarning() << "[Export] Failed to save Excel file:" << outPathQt;
        throw std::runtime_error("Failed to save Excel file: " + outPath.string());
    }

    qInfo() << "[Export] Saved →" << outPathQt;
    return outPath;
}
