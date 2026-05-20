#pragma once
#include "models.h"
#include <QColor>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QWidget>
#include <optional>

// ─────────────────────────────────────────────────────────────────────────────
// Shared palette
// ─────────────────────────────────────────────────────────────────────────────
namespace Palette {
    inline const QColor POS    { "#d4edda" };
    inline const QColor NEG    { "#f8d7da" };
    inline const QColor NA     { "#e9ecef" };
    inline const QColor ALT    { "#f8f9fa" };
    inline const QColor NEG_FG { "#c0392b" };
    inline const QColor DARK   { "#2c3e50" };
    inline const QColor WHITE  { "#ffffff" };
    inline const QColor ROOM   { "#dce8f5" };
    inline const QColor LABEL  { "#eef2f7" };
}

// ─────────────────────────────────────────────────────────────────────────────
// Formatting helpers
// ─────────────────────────────────────────────────────────────────────────────
QString fmtMoney   (double v);                          // "$1,234.56" or "—" if zero
QString fmtVariance(std::optional<double> pct);         // "▲ 4.2%" / "▼ 1.1%" / "—"
QColor  varColor   (std::optional<double> pct);         // green / red / grey

// ─────────────────────────────────────────────────────────────────────────────
// InfoTable
// Compact, non-scrolling, spreadsheet-style QTableWidget.
// ─────────────────────────────────────────────────────────────────────────────
class InfoTable : public QTableWidget
{
    Q_OBJECT
public:
    static constexpr int ROW_H = 24;

    explicit InfoTable(const QStringList &colLabels, QObject *parent = nullptr);
    explicit InfoTable(int nCols, QObject *parent = nullptr);

    void setColWidths(std::initializer_list<int> widths);

    int addSectionRow(const QString &text);
    int addDataRow   (const QString &label,
                      const QStringList &values = {},
                      bool shade = false,
                      const QColor &labelBg = QColor());
    int addEmptyRow  ();

    void seal();   // lock height to content

private:
    QTableWidgetItem *makeItem(const QString &text,
                               const QColor  &bg    = QColor(),
                               const QColor  &fg    = QColor(),
                               bool           bold  = false,
                               bool           right = false);
};

// ─────────────────────────────────────────────────────────────────────────────
// ComparisonPanel   (template rows 6-11)
// ─────────────────────────────────────────────────────────────────────────────
class ComparisonPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ComparisonPanel(QWidget *parent = nullptr);
    void populate(const FinancialDashboard &dash, const CloudbedsMetrics &cb);

private:
    InfoTable *m_table;
    int m_rToday, m_rTwoWeeks, m_r12Months;
    void setRow(int row, std::optional<double> current, std::optional<double> ly);
};

// ─────────────────────────────────────────────────────────────────────────────
// RoomOccupancyPanel   (template rows 14-22)
// ─────────────────────────────────────────────────────────────────────────────
class RoomOccupancyPanel : public QWidget
{
    Q_OBJECT
public:
    explicit RoomOccupancyPanel(QWidget *parent = nullptr);
    void populate(const CloudbedsMetrics &cb);

private:
    InfoTable *m_table;
    int m_rRooms, m_rOcc;
    int m_rAdrTd, m_rAdrMtd, m_rAdrLy;
    int m_rRevTd, m_rRevMtd, m_rRevLy;

    void set(int row, int col, const QString &text, const QColor &bg = QColor());
};

// ─────────────────────────────────────────────────────────────────────────────
// FinancialGrid   (template rows 26-58)
// ─────────────────────────────────────────────────────────────────────────────
class FinancialGrid : public QTableWidget
{
    Q_OBJECT
public:
    explicit FinancialGrid(QWidget *parent = nullptr);
    void populate(const FinancialDashboard &dash, const CloudbedsMetrics &cb);

private:
    void cell(int row, int col, const QString &text,
              const QColor &bg = QColor(), const QColor &fg = QColor(),
              bool bold = false, bool left = false);

    void naRow(int row, const QString &label, const QColor &bg);

    static const QStringList COLS;
    static const QList<QPair<QString,QString>> GROUPS;  // {lsk key, display name}
};

// ─────────────────────────────────────────────────────────────────────────────
// BankingPanel   (right column, rows 4-13)
// ─────────────────────────────────────────────────────────────────────────────
class BankingPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BankingPanel(QWidget *parent = nullptr);
    void setValue(const QString &label, const QString &value);

private:
    InfoTable *m_table;
    QList<int> m_rows;
};

// ─────────────────────────────────────────────────────────────────────────────
// GuestPrivilegePanel   (right column, rows 14-22)
// ─────────────────────────────────────────────────────────────────────────────
class GuestPrivilegePanel : public QWidget
{
    Q_OBJECT
public:
    explicit GuestPrivilegePanel(QWidget *parent = nullptr);
    void populate(const FinancialDashboard &dash);

private:
    InfoTable *m_table;
    int m_rVisits, m_rPasses, m_rRevenue, m_rMtdRev, m_r14Day;
    void set(int row, const QString &text);
};

// ─────────────────────────────────────────────────────────────────────────────
// Layout helpers
// ─────────────────────────────────────────────────────────────────────────────
QFrame *makeHLine ();
QLabel *makeSectionLabel(const QString &text);
