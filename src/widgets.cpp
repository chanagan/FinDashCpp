#include "widgets.h"
#include <QFont>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QBrush>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Formatting helpers
// ─────────────────────────────────────────────────────────────────────────────

QString fmtMoney(double v)
{
    if (v == 0.0) return QStringLiteral("—");
    return QString("$%L1").arg(v, 0, 'f', 2);
}

QString fmtVariance(std::optional<double> pct)
{
    if (!pct) return QStringLiteral("—");
    QChar arrow = (*pct >= 0) ? QChar(0x25B2) : QChar(0x25BC);   // ▲ / ▼
    return QString("%1 %2%").arg(arrow).arg(std::abs(*pct ), 0, 'f', 2);
}

QColor varColor(std::optional<double> pct)
{
    if (!pct) return Palette::NA;
    return (*pct >= 0) ? Palette::POS : Palette::NEG;
}

static QFont boldFont()
{
    QFont f; f.setBold(true); return f;
}

// ─────────────────────────────────────────────────────────────────────────────
// InfoTable
// ─────────────────────────────────────────────────────────────────────────────

InfoTable::InfoTable(const QStringList &colLabels, QObject *parent)
    : QTableWidget(0, colLabels.size(), qobject_cast<QWidget*>(parent))
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setEditTriggers(NoEditTriggers);
    setSelectionMode(NoSelection);
    verticalHeader()->setVisible(false);
    setShowGrid(true);
    setStyleSheet(
        "QTableWidget { gridline-color:#dee2e6; font-size:12px; border:1px solid #dee2e6; }"
        "QTableWidget::item { padding:2px 8px; }"
    );
    setHorizontalHeaderLabels(colLabels);
    auto *hh = horizontalHeader();
    hh->setFont(boldFont());
    hh->setStyleSheet(
        "QHeaderView::section { background:#2c3e50; color:white; font-size:11px;"
        "  padding:4px 8px; border:none; font-weight:bold; }"
    );
    for (int c = 0; c < columnCount(); ++c)
        hh->setSectionResizeMode(c, QHeaderView::Fixed);
}

InfoTable::InfoTable(int nCols, QObject *parent)
    : InfoTable(QStringList(nCols), parent)
{
    horizontalHeader()->setVisible(false);
}

void InfoTable::setColWidths(std::initializer_list<int> widths)
{
    int i = 0;
    for (int w : widths) { if (i < columnCount()) setColumnWidth(i++, w); }
}

QTableWidgetItem *InfoTable::makeItem(const QString &text, const QColor &bg,
                                      const QColor &fg, bool bold, bool right)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(right ? Qt::AlignRight|Qt::AlignVCenter
                                 : Qt::AlignLeft |Qt::AlignVCenter);
    if (bold) item->setFont(boldFont());
    if (bg.isValid()) item->setBackground(QBrush(bg));
    if (fg.isValid()) item->setForeground(QBrush(fg));
    return item;
}

int InfoTable::addSectionRow(const QString &text)
{
    int r = rowCount(); insertRow(r);
    setRowHeight(r, ROW_H);
    for (int c = 0; c < columnCount(); ++c)
        setItem(r, c, makeItem(c == 0 ? text : QString(), Palette::DARK,
                               QColor(Qt::white), true));
    return r;
}

int InfoTable::addDataRow(const QString &label, const QStringList &values,
                          bool shade, const QColor &labelBg)
{
    int r = rowCount(); insertRow(r);
    setRowHeight(r, ROW_H);
    QColor rowBg = shade ? Palette::ALT : Palette::WHITE;
    setItem(r, 0, makeItem(label, labelBg.isValid() ? labelBg : rowBg,
                           QColor(), labelBg.isValid()));
    for (int c = 0; c < values.size() && (c + 1) < columnCount(); ++c) {
        QColor fg = values[c].startsWith("-$") ? Palette::NEG_FG : QColor();
        setItem(r, c + 1, makeItem(values[c], rowBg, fg, false, true));
    }
    return r;
}

int InfoTable::addEmptyRow()
{
    int r = rowCount(); insertRow(r);
    setRowHeight(r, ROW_H / 2);
    for (int c = 0; c < columnCount(); ++c)
        setItem(r, c, makeItem(QString(), Palette::WHITE));
    return r;
}

void InfoTable::seal()
{
    int h = horizontalHeader()->isVisible() ? horizontalHeader()->height() : 0;
    for (int r = 0; r < rowCount(); ++r) h += rowHeight(r);
    setFixedHeight(h + 2);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

// ─────────────────────────────────────────────────────────────────────────────
// ComparisonPanel
// ─────────────────────────────────────────────────────────────────────────────

ComparisonPanel::ComparisonPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new InfoTable({"", "Current", "Last Year", "Var %"}, this);
    m_table->setColWidths({175, 105, 105, 80});

    m_rToday     = m_table->addDataRow("Today",      {}, false, Palette::LABEL);
    m_rTwoWeeks  = m_table->addDataRow("Two Weeks",  {}, true);
    m_r12Months  = m_table->addDataRow("12 Months",  {}, false, Palette::LABEL);

    m_table->seal();
    layout->addWidget(m_table);
}

void ComparisonPanel::setRow(int row, std::optional<double> current, std::optional<double> ly)
{
    std::optional<double> pct;
    if (current && ly && *ly != 0.0)
        pct = (*current - *ly) / std::abs(*ly) * 100.0;

    if (auto *item = m_table->item(row, 1))
        item->setText(current ? fmtMoney(*current) : "—");
    if (auto *item = m_table->item(row, 2))
        item->setText(ly      ? fmtMoney(*ly)      : "—");
    if (auto *varItem = m_table->item(row, 3)) {
        varItem->setText(fmtVariance(pct));
        varItem->setBackground(QBrush(varColor(pct)));
    }
}

void ComparisonPanel::populate(const FinancialDashboard &dash, const CloudbedsMetrics &cb)
{
    double todayTotal = dash.today.total_amount + cb.room_revenue;
    double lyTotal    = dash.same_day_last_year.total_amount;
    setRow(m_rToday, todayTotal, lyTotal);
    // Two Weeks / 12 Months require additional fetches — remain "—"
}

// ─────────────────────────────────────────────────────────────────────────────
// RoomOccupancyPanel
// ─────────────────────────────────────────────────────────────────────────────

RoomOccupancyPanel::RoomOccupancyPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new InfoTable(3, this);
    // m_table->setColumnCount(4);
    m_table->setColWidths({175, 105, 90});

    m_table->addSectionRow("Room Occupancy Stats");
    m_rRooms  = m_table->addDataRow("Rooms",            {"—"});
    m_rOcc    = m_table->addDataRow("Occupied Rooms",   {"—"}, true);
    m_table->addSectionRow("ADR");
    m_rAdrTd  = m_table->addDataRow("Today's ADR",      {"—"});
    m_rAdrMtd = m_table->addDataRow("MTD ADR",          {"—"}, true);
    m_rAdrLy  = m_table->addDataRow("LY Month ADR",     {"—", "—"});
    m_table->addSectionRow("RevPar");
    m_rRevTd  = m_table->addDataRow("Today's RevPar",   {"—"});
    m_rRevMtd = m_table->addDataRow("MTD RevPar",       {"—"}, true);
    m_rRevLy  = m_table->addDataRow("LY Month RevPar",  {"—", "—"});

    m_table->seal();
    layout->addWidget(m_table);
}

void RoomOccupancyPanel::set(int row, int col, const QString &text, const QColor &bg)
{
    if (auto *item = m_table->item(row, col)) {
        item->setText(text);
        if (bg.isValid()) item->setBackground(QBrush(bg));
    }
}

void RoomOccupancyPanel::populate(const CloudbedsMetrics &cb)
{
    auto negFg = [](double v) { return v < 0 ? Palette::NEG_FG : QColor(); };
    QTableWidgetItem *item;

    double variance;
    set(m_rRooms,  1, fmtMoney(cb.room_revenue));
    set(m_rOcc,    1, cb.total_rooms
                      ? QString("%1 / %2").arg(cb.occupied_rooms).arg(cb.total_rooms)
                      : QStringLiteral("—"));
    // adr - today - this month - this month last year
    set(m_rAdrTd,  1, fmtMoney(cb.adr));
    set(m_rAdrMtd, 1, fmtMoney(cb.mtd_adr));
    set(m_rAdrLy,  1, fmtMoney(cb.ly_mtd_adr));

    variance = cb.mtd_adr - cb.ly_mtd_adr;
    set(m_rAdrLy, 2, fmtMoney(variance));
    item = m_table->item(m_rAdrLy, 2);
    if (variance < 0) item->setForeground(Qt::red);

    // revpar - today - this month - this month last year
    set(m_rRevTd,  1, fmtMoney(cb.revpar));
    set(m_rRevMtd, 1, fmtMoney(cb.mtd_revpar));
    set(m_rRevLy,  1, fmtMoney(cb.ly_mtd_revpar));

    variance = cb.mtd_revpar - cb.ly_mtd_revpar;
    set(m_rRevLy, 2, fmtMoney(variance));
    item = m_table->item(m_rRevLy, 2);
    if (variance < 0) item->setForeground(Qt::red);

}

// ─────────────────────────────────────────────────────────────────────────────
// FinancialGrid
// ─────────────────────────────────────────────────────────────────────────────

const QStringList FinancialGrid::COLS = {
    "Revenue", "Today", "MTD", "MTD-LY", "Variance $", "Variance %"
};

const QList<QPair<QString,QString>> FinancialGrid::GROUPS = {
    {"Cafe Bar",    "Café Bar"},
    {"Cafe Food",   "Café Food"},
    {"Health Club", "Guest Privilege"},
    {"Laundry",     "Laundry"},
    {"Misc",        "Misc Adjust"},
    {"Retail",      "Retail"},
};

FinancialGrid::FinancialGrid(QWidget *parent)
    : QTableWidget(0, COLS.size(), parent)
{
    setHorizontalHeaderLabels(COLS);
    setEditTriggers(NoEditTriggers);
    setSelectionBehavior(SelectRows);
    verticalHeader()->setVisible(false);
    setShowGrid(true);
    setStyleSheet(
        "QTableWidget { gridline-color:#dee2e6; font-size:13px; }"
        "QTableWidget::item { padding:4px 10px; }"
    );
    auto *hh = horizontalHeader();
    hh->setFont(boldFont());
    hh->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    for (int c = 1; c < COLS.size(); ++c)
        hh->setSectionResizeMode(c, QHeaderView::Stretch);
}

void FinancialGrid::cell(int row, int col, const QString &text,
                         const QColor &bg, const QColor &fg, bool bold, bool left)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(left ? Qt::AlignLeft|Qt::AlignVCenter
                                : Qt::AlignRight|Qt::AlignVCenter);
    if (bold) item->setFont(boldFont());
    if (bg.isValid()) item->setBackground(QBrush(bg));
    if (fg.isValid()) item->setForeground(QBrush(fg));
    setItem(row, col, item);
}

void FinancialGrid::naRow(int row, const QString &label, const QColor &bg)
{
    cell(row, 0, label, Palette::ROOM, QColor(), true, true);
    for (int c = 1; c < COLS.size(); ++c)
        cell(row, c, "—", bg);
}

void FinancialGrid::populate(const FinancialDashboard &dash, const CloudbedsMetrics &cb)
{
    setRowCount(3 + GROUPS.size() + 1);
    int row = 0;
    auto negFg = [](double v) { return v < 0 ? Palette::NEG_FG : QColor(); };

    // Room revenue rows (Cloudbeds)
    cell(row, 0, "Room Revenue",        Palette::ROOM, QColor(), true, true);
    cell(row, 1, fmtMoney(cb.room_revenue), Palette::ROOM);
    cell(row, 2, fmtMoney(cb.mtd_revenue), Palette::ROOM);
    cell(row, 3, fmtMoney(cb.ly_mtd_revenue), Palette::ROOM);
    double variance = cb.mtd_revenue - cb.ly_mtd_revenue;
    double variance_pct = (variance / cb.ly_mtd_revenue) * 100;
    cell(row, 4, fmtMoney(variance), Palette::ROOM, negFg(variance));
    cell(row, 5, fmtVariance(variance_pct), varColor(variance_pct),QColor());

    // for (int c = 4; c < COLS.size(); ++c) cell(row, c, "—", Palette::ROOM);
    ++row;
    naRow(row++, "Room Revenue (TE)",      Palette::ALT);
    naRow(row++, "Room Revenue Forfeit",   Palette::WHITE);

    // LSK groups
    for (int i = 0; i < GROUPS.size(); ++i, ++row) {
        const auto &[key, name] = GROUPS[i];
        QColor bg = (i % 2 == 0) ? Palette::ALT : Palette::WHITE;
        double today   = dash.today.groupAmount(key);
        double mtd     = dash.mtd.groupAmount(key);
        double mtdLy   = dash.mtd_last_year.groupAmount(key);
        double varAmt  = dash.mtd_variance_amt(key);
        auto   varPct  = dash.mtd_variance_pct(key);

        // auto negFg = [](double v) { return v < 0 ? Palette::NEG_FG : QColor(); };
        cell(row, 0, name,              Palette::LABEL, QColor(), true, true);
        cell(row, 1, fmtMoney(today),   bg, negFg(today));
        cell(row, 2, fmtMoney(mtd),     bg, negFg(mtd));
        cell(row, 3, fmtMoney(mtdLy),   bg, negFg(mtdLy));
        cell(row, 4, fmtMoney(varAmt),  bg, negFg(varAmt));
        cell(row, 5, fmtVariance(varPct), varColor(varPct), QColor(), true);
    }

    // Total row
    double totToday  = dash.today.total_amount + cb.room_revenue;
    double totMtd    = dash.mtd.total_amount + cb.mtd_revenue;
    double totMtdLy  = dash.mtd_last_year.total_amount + cb.ly_mtd_revenue;
    double totVarAmt = totMtd - totMtdLy;
    auto   totVarPct = dash.mtd_total_variance_pct();

    cell(row, 0, "TOTAL REVENUE",          Palette::DARK, QColor(Qt::white), true, true);
    cell(row, 1, fmtMoney(totToday),        Palette::DARK, QColor(Qt::white), true);
    cell(row, 2, fmtMoney(totMtd),          Palette::DARK, QColor(Qt::white), true);
    cell(row, 3, fmtMoney(totMtdLy),        Palette::DARK, QColor(Qt::white), true);
    cell(row, 4, fmtMoney(totVarAmt),       Palette::DARK, QColor(Qt::white), true);
    cell(row, 5, fmtVariance(totVarPct),    Palette::DARK, QColor(Qt::white), true);

    resizeRowsToContents();
}

// ─────────────────────────────────────────────────────────────────────────────
// BankingPanel
// ─────────────────────────────────────────────────────────────────────────────

BankingPanel::BankingPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new InfoTable(2, this);
    m_table->setColWidths({210, 120});
    m_table->addSectionRow("Banking");

    static const QStringList labels = {
        "Today's Deposit", "Deposit This Month", "Operating Expenses",
        "First State Bank (Operations)", "First State Bank (Holding)",
        "Debt on LOC", "Marina Berry – 2 Stickney Ln"
    };
    for (int i = 0; i < labels.size(); ++i)
        m_rows << m_table->addDataRow(labels[i], {"—"}, i % 2 != 0);

    m_table->seal();
    layout->addWidget(m_table);
}

void BankingPanel::setValue(const QString &label, const QString &value)
{
    for (int r : m_rows) {
        if (auto *item = m_table->item(r, 0); item && item->text() == label)
            if (auto *val = m_table->item(r, 1)) { val->setText(value); break; }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GuestPrivilegePanel
// ─────────────────────────────────────────────────────────────────────────────

GuestPrivilegePanel::GuestPrivilegePanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new InfoTable(2, this);
    m_table->setColWidths({210, 120});
    m_table->addSectionRow("Guest Privilege");
    m_rVisits  = m_table->addDataRow("Daily Visits",       {"—"});
    m_rPasses  = m_table->addDataRow("Multi-Passes Today", {"—"}, true);
    m_rRevenue = m_table->addDataRow("Revenue",            {"—"});
    m_rMtdRev  = m_table->addDataRow("MTD Revenue",        {"—"}, true);
    m_r14Day   = m_table->addDataRow("14 Day Increase",    {"—"});
    m_table->seal();
    layout->addWidget(m_table);
}

void GuestPrivilegePanel::set(int row, const QString &text)
{
    if (auto *item = m_table->item(row, 1)) item->setText(text);
}

void GuestPrivilegePanel::populate(const FinancialDashboard &dash)
{
    auto hcToday = dash.today.groups.value("Health Club");
    auto hcMtd   = dash.mtd.groups.value("Health Club");

    set(m_rRevenue, fmtMoney(hcToday.total_amount));
    set(m_rMtdRev,  fmtMoney(hcMtd.total_amount));
    // Daily visits / multi-passes not available from LSK aggregated data
}

// ─────────────────────────────────────────────────────────────────────────────
// Layout helpers
// ─────────────────────────────────────────────────────────────────────────────

QFrame *makeHLine()
{
    auto *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    return line;
}

QLabel *makeSectionLabel(const QString &text)
{
    auto *lbl = new QLabel(text);
    lbl->setStyleSheet(
        "font-size:10px; font-weight:bold; color:#6c757d; letter-spacing:1.5px;"
    );
    return lbl;
}
