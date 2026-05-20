#include "export_xlsx.h"
#include "config.h"
#include <QDebug>
#include <QDate>
#include <QColor>
#include <stdexcept>

// QXlsx - Native Qt Excel library
// #include "xlsxwriter.h"
#include <xlsxdocument.h>
#include <xlsxformat.h>

namespace fs = std::filesystem;

// ── LSK group key → spreadsheet row ──────────────────────────────────────────
static const QList<QPair<QString,int>> LSK_ROW_MAP = {
    { "Cafe Bar",    31 },
    { "Cafe Food",   32 },
    { "Health Club", 37 },
    { "Laundry",     41 },
    { "Misc",        44 },
    { "Retail",      50 },
// { "Cafe Bar",    0 },
// { "Cafe Food",   1 },
// { "Health Club", 2 },
// { "Laundry",     3 },
// { "Misc",        4 },
// { "Retail",      5 },
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
    fs::path tmpltFile = outDir / "template.xlsx";

    QString dateStr = dash.as_of.toString("yyyy-MM-dd");
    fs::path outPath = outDir / ("DailyReport_" + dateStr.toStdString() + ".xlsx");

    // Create output directory if it doesn't exist
    if (!fs::exists(outDir)) {
        fs::create_directories(outDir);
    }

    // Create Excel workbook using QXlsx
    QXlsx::Document xlsx(tmpltFile.c_str());

    // Set column widths
    xlsx.setColumnWidth(1, 25);  // Column A - labels
    xlsx.setColumnWidth(2, 15);  // Column B - Today
    xlsx.setColumnWidth(3, 15);  // Column C - MTD
    xlsx.setColumnWidth(4, 15);  // Column D - MTD-LY
    xlsx.setColumnWidth(5, 15);  // Column E - Variance $
    xlsx.setColumnWidth(6, 15);  // Column F - Variance %

    // Create formats
    QXlsx::Format titleFormat;
    titleFormat.setFontBold(true);
    titleFormat.setFontSize(14);

    QXlsx::Format boldFormat;
    boldFormat.setFontBold(true);

    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setPatternBackgroundColor(QColor(211, 211, 211));  // Light gray

    QXlsx::Format currencyFormat;
    currencyFormat.setNumberFormat("$#,##0.00");

    QXlsx::Format percentFormat;
    percentFormat.setNumberFormat("0.0%");

    int dataRow = 1;

    // Title (row 1)
    // xlsx.write(dataRow, 1, "Financial Dashboard Report", titleFormat);

    // Date (row 2)
    dataRow = 3;
    // xlsx.write(dataRow, 1, "Date:", boldFormat);
    xlsx.write(dataRow, 1, dateStr);

    // Cloudbeds Occupancy Stats (if available)
    if (cb.has_value()) {
        dataRow = 15;  // Add spacing

        // xlsx.write(dataRow, 1, "Room Revenue (Today):", boldFormat);
        xlsx.write(dataRow, 2, cb->room_revenue, currencyFormat);

        dataRow = 16;
        QString occ_rooms = QString("%1 of %2").arg(cb->occupied_rooms).arg(cb->total_rooms);
        // xlsx.write(dataRow, 1, "Occupied Rooms:", boldFormat);
        xlsx.write(dataRow, 2, occ_rooms);

        // dataRow++;
        // xlsx.write(dataRow, 1, "Total Rooms:", boldFormat);
        // xlsx.write(dataRow, 2, cb->total_rooms);
        dataRow = 17;

        // xlsx.write(dataRow, 1, "ADR (Today):", boldFormat);
        xlsx.write(dataRow, 2, cb->adr, currencyFormat);

        dataRow = 18;
        xlsx.write(dataRow, 2, cb->mtd_adr, currencyFormat);

        dataRow = 19;
        xlsx.write(dataRow, 2, cb->ly_mtd_adr, currencyFormat);

        dataRow = 20;
        // xlsx.write(dataRow, 1, "RevPAR (Today):", boldFormat);
        xlsx.write(dataRow, 2, cb->revpar, currencyFormat);

        dataRow = 21;
        // xlsx.write(dataRow, 1, "MTD Revenue:", boldFormat);
        xlsx.write(dataRow, 2, cb->mtd_revpar, currencyFormat);
        dataRow = 22;
        xlsx.write(dataRow, 2, cb->ly_mtd_revpar, currencyFormat);
    }


    // Headers (row 26)
    int headerRow = 26;
    // xlsx.write(headerRow, 1, "Revenue Category", headerFormat);
    // xlsx.write(headerRow, 2, "Today", headerFormat);
    // xlsx.write(headerRow, 3, "MTD", headerFormat);
    // xlsx.write(headerRow, 4, "MTD-LY", headerFormat);
    // xlsx.write(headerRow, 5, "Variance $", headerFormat);
    // xlsx.write(headerRow, 6, "Variance %", headerFormat);

    // LSK Revenue by group (rows starts at 31)
    dataRow = 27;
    xlsx.write(dataRow, 2, cb->room_revenue, currencyFormat);
    xlsx.write(dataRow, 3, cb->mtd_revenue, currencyFormat);
    xlsx.write(dataRow, 4, cb->ly_mtd_revenue, currencyFormat);




    dataRow = 31;
    for (const auto &[groupName, idx] : LSK_ROW_MAP) {
        // xlsx.write(dataRow, 1, groupName);
        xlsx.write(idx, 2, groupAmt(dash.today, groupName), currencyFormat);
        xlsx.write(idx, 3, groupAmt(dash.mtd, groupName), currencyFormat);
        xlsx.write(idx, 4, groupAmt(dash.mtd_last_year, groupName), currencyFormat);
        // xlsx.write(dataRow, 2, groupAmt(dash.today, groupName), currencyFormat);
        // xlsx.write(dataRow, 3, groupAmt(dash.mtd, groupName), currencyFormat);
        // xlsx.write(dataRow, 4, groupAmt(dash.mtd_last_year, groupName), currencyFormat);

        // // Variance $ formula: MTD - MTD-LY
        // QString varFormula = QString("=C%1-D%1").arg(dataRow);
        // xlsx.write(dataRow, 5, varFormula, currencyFormat);
        // //
        // // Variance % formula: (MTD - MTD-LY) / MTD-LY with zero protection
        // QString varPctFormula = QString("=IF(D%1=0,0,(C%1-D%1)/D%1)").arg(dataRow);
        // xlsx.write(dataRow, 6, varPctFormula, percentFormat);

        dataRow++;
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
