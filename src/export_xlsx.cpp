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
// static const QList<QPair<QString,int>> LSK_ROW_MAP = {
//     { "Cafe Bar",    31 },
//     { "Cafe Food",   32 },
//     { "Health Club", 37 },
//     { "Laundry",     41 },
//     { "Misc",        44 },
//     { "Retail",      50 },
// };
static const QList<QPair<QString,int>> LSK_ROW_MAP = {
    { "Cafe Bar",    31 },
    { "Cafe Food",   32 },
    { "Health Club", 37 },
    { "Massage",     39 },
    { "Guest Relations Exp",     40 },
    { "Laundry",     41 },
    { "Misc",        44 },
    { "Retail",      38 },
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

    // going to use this to allow use of template-based formatting for some cells
    QXlsx::Format originalFormat;

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
    QString asOfDate = dash.as_of.toString("MMM dd, yyyy");
    // originalFormat = xlsx.cellAt(3,1)->format();
    // xlsx.write("A3", asOfDate);
    xlsx.write("A3", dash.as_of);

    // Write LSK Revenue data to template rows
    // Adjust column letters (B, C, D, E, F) based on your template layout
    for (const auto &[groupName, row] : LSK_ROW_MAP) {
        double today = groupAmt(dash.today, groupName);
        double mtd = groupAmt(dash.mtd, groupName);
        double mtd_ly = groupAmt(dash.mtd_last_year, groupName);

        // Write to specific cells (B=col2, C=col3, D=col4, E=col5, F=col6)
        // Adjust column letters to match your template
        xlsx.write(row, 2, today);    // Column B
        xlsx.write(row, 3, mtd);      // Column C
        xlsx.write(row, 4, mtd_ly);   // Column D

        // Variance calculations
        double variance = mtd - mtd_ly;
        double variancePct = (mtd_ly != 0) ? (mtd - mtd_ly) / mtd_ly : 0.0;

        xlsx.write(row, 5, variance); // Column E
        xlsx.write(row, 6, variancePct); // Column F
    }

    // Write Cloudbeds data to template (adjust rows/columns as needed)
    if (cb.has_value()) {
        // double variance;
        // Example: adjust these row numbers to match your template
        xlsx.write("B15", cb->room_revenue);
        xlsx.write("B16", QString("%1 of %2").arg(cb->occupied_rooms).arg(cb->total_rooms));

        // ADR
        xlsx.write("B17", cb->adr);
        xlsx.write("B18", cb->mtd_adr);
        xlsx.write("B19", cb->ly_mtd_adr);
        // variance = cb->mtd_adr - cb->ly_mtd_adr;
        // originalFormat = xlsx.cellAt(19,3)->format();
        // xlsx.write(19,3, variance, originalFormat);
        xlsx.write("C19", "=(B18-B19)");

        // RevPar
        xlsx.write("B20", cb->revpar);
        xlsx.write("B21", cb->mtd_revpar);
        xlsx.write("B22",cb->ly_mtd_revpar);
        // variance = cb->mtd_revpar - cb->ly_mtd_revpar;
        // originalFormat = xlsx.cellAt(22,3)->format();
        // xlsx.write(22,3, variance, originalFormat);
        xlsx.write("C22", "=(B21-B22)");

        // Revenue
        // xlsx.write(27,2,cb->room_revenue, currencyFormat);
        xlsx.write("B27", cb->room_revenue);
        xlsx.write("C27",cb->mtd_revenue);
        xlsx.write("D27",cb->ly_mtd_revenue);
        xlsx.write("E27", "=+C27-D27");
        xlsx.write("F27", "=+C27/D27-1");

        // Revenue - Other Row 29
        // xlsx.write(29,2,cb->room_revenue, currencyFormat);
        xlsx.write("B29", cb->room_revenue_othr);
        xlsx.write("C29",cb->mtd_revenue_othr);
        xlsx.write("D29",cb->ly_mtd_revenue_othr);
        xlsx.write("E29", "=+C29-D29");
        xlsx.write("F29", "=+C29/D29-1");

    }

    xlsx.write("E58", "=+C58-D58");
    xlsx.write("F58", "=+C58/D58-1");

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
