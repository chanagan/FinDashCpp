#pragma once
#include "models.h"
#include <filesystem>
#include <optional>

// ─────────────────────────────────────────────────────────────────────────────
// exportDailyReport
//
// Creates a new Excel file from scratch using QXlsx (native Qt library) and writes:
//   • LSK revenue data by accounting group (Today / MTD / MTD-LY)
//   • Variance calculations with formulas ($ and %)
//   • Cloudbeds metrics (room revenue, occupancy, ADR, RevPAR, MTD revenue)
//
// Features:
//   - Formatted headers with gray background
//   - Currency and percentage number formats
//   - Excel formulas for automatic variance calculations
//   - Proper column widths for readability
//   - No external dependencies (pure Qt)
//
// The file is saved as DailyReport_YYYY-MM-DD.xlsx in the config directory
// (or the specified outputDir if provided).
//
// Returns the filesystem path of the saved file.
// ─────────────────────────────────────────────────────────────────────────────
std::filesystem::path exportDailyReport(
    const FinancialDashboard          &dash,
    const std::optional<CloudbedsMetrics> &cb      = std::nullopt,
    const std::filesystem::path       &templatePath = {},   // defaults to template.xlsx beside exe
    const std::filesystem::path       &outputDir    = {}    // defaults to template dir
);
