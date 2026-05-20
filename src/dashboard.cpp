#include "dashboard.h"
#include "export_xlsx.h"
#include <QCalendarWidget>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollArea>
#include <QSplitter>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────


DashboardWindow::DashboardWindow(CloudbedsApiClient *cbClient,
                                 LskApiClient       *lskClient,
                                 QWidget            *parent)
    : QMainWindow(parent)
    , m_cbClient(cbClient)
    , m_lskClient(lskClient)
    , m_useMock(false)
{
    buildUi();
    QTimer::singleShot(0, this, &DashboardWindow::refresh);
}

DashboardWindow::DashboardWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_useMock(true)
{
    buildUi();
    QTimer::singleShot(0, this, &DashboardWindow::refresh);
}

// ─────────────────────────────────────────────────────────────────────────────
// UI construction
// ─────────────────────────────────────────────────────────────────────────────

void DashboardWindow::buildUi()
{
    setWindowTitle("Financial Dashboard");
    resize(1300, 900);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setSpacing(6);
    root->setContentsMargins(16, 12, 16, 8);

    root->addLayout(buildHeader());
    root->addWidget(makeHLine());
    root->addWidget(buildBody(), 1);

    m_statusBar = new QStatusBar(this);
    setStatusBar(m_statusBar);
}

QLayout *DashboardWindow::buildHeader()
{
    auto *header = new QHBoxLayout();

    auto *title = new QLabel("Financial Dashboard");
    title->setStyleSheet("font-size:18px; font-weight:bold;");

    m_datePicker = new QDateEdit(this);
    m_datePicker->setCalendarPopup(true);
    m_datePicker->blockSignals(true);
    m_datePicker->setDate(QDate::currentDate().addDays(-1));
    m_datePicker->blockSignals(false);
    m_datePicker->setDisplayFormat("ddd, MMM d yyyy");
    m_datePicker->setFixedWidth(185);
    connect(m_datePicker->calendarWidget(), &QCalendarWidget::clicked,
            this, [this](const QDate &) { refresh(); });

    m_refreshBtn = new QPushButton("⟳  Refresh", this);
    m_refreshBtn->setFixedWidth(120);
    connect(m_refreshBtn, &QPushButton::clicked, this, &DashboardWindow::refresh);

    m_exportBtn = new QPushButton("⬇  Export XLSX", this);
    m_exportBtn->setFixedWidth(130);
    m_exportBtn->setEnabled(false);
    connect(m_exportBtn, &QPushButton::clicked, this, &DashboardWindow::exportXlsx);

    m_modeBadge = new QLabel("MOCK DATA", this);
    m_modeBadge->setStyleSheet(
        "font-size:11px; color:white; background:#6c757d;"
        "border-radius:4px; padding:2px 8px;"
    );

    header->addWidget(title);
    header->addStretch();
    header->addWidget(new QLabel("Date:"));
    header->addWidget(m_datePicker);
    header->addSpacing(8);
    header->addWidget(m_refreshBtn);
    header->addSpacing(4);
    header->addWidget(m_exportBtn);
    header->addSpacing(8);
    header->addWidget(m_modeBadge);
    return header;
}

QWidget *DashboardWindow::buildBody()
{
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(4);
    splitter->setChildrenCollapsible(false);

    // ── Left panel (scrollable) ───────────────────────────────────────────────
    auto *leftScroll = new QScrollArea();
    leftScroll->setWidgetResizable(true);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setFrameShape(QFrame::NoFrame);

    auto *leftContent = new QWidget();
    auto *leftLayout  = new QVBoxLayout(leftContent);
    leftLayout->setSpacing(8);
    leftLayout->setContentsMargins(0, 4, 8, 4);

    leftLayout->addWidget(makeSectionLabel("REVENUE COMPARISON"));
    m_comparison = new ComparisonPanel(leftContent);
    leftLayout->addWidget(m_comparison);

    leftLayout->addSpacing(6);
    leftLayout->addWidget(makeHLine());

    leftLayout->addWidget(makeSectionLabel("ROOMS  ·  CLOUDBEDS"));
    m_roomPanel = new RoomOccupancyPanel(leftContent);
    leftLayout->addWidget(m_roomPanel);

    leftLayout->addSpacing(6);
    leftLayout->addWidget(makeHLine());

    leftLayout->addWidget(makeSectionLabel("REVENUE  ·  LIGHTSPEED"));
    m_grid = new FinancialGrid(leftContent);
    leftLayout->addWidget(m_grid, 1);
    leftLayout->addStretch();

    leftScroll->setWidget(leftContent);

    // ── Right panel ───────────────────────────────────────────────────────────
    auto *rightWidget = new QWidget();
    rightWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setSpacing(8);
    rightLayout->setContentsMargins(8, 4, 0, 4);

    m_bankingPanel = new BankingPanel(rightWidget);
    rightLayout->addWidget(m_bankingPanel);

    rightLayout->addSpacing(6);
    rightLayout->addWidget(makeHLine());

    m_gpPanel = new GuestPrivilegePanel(rightWidget);
    rightLayout->addWidget(m_gpPanel);
    rightLayout->addStretch();

    splitter->addWidget(leftScroll);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({900, 360});
    return splitter;
}

// ─────────────────────────────────────────────────────────────────────────────
// Data flow
// ─────────────────────────────────────────────────────────────────────────────

QDate DashboardWindow::selectedDate() const
{
    return m_datePicker->date();
}

void DashboardWindow::refresh()
{
    if (m_fetchPending) return;
    m_fetchPending = true;
    m_refreshBtn->setEnabled(false);
    m_statusBar->showMessage("Fetching data…");

    if (m_useMock)
        m_thread = new CombinedFetchThread(selectedDate(), this);
    else
        m_thread = new CombinedFetchThread(selectedDate(), m_cbClient, m_lskClient, this);

    connect(m_thread, &CombinedFetchThread::finished,
            this,     &DashboardWindow::onData);
    connect(m_thread, &CombinedFetchThread::error,
            this,     &DashboardWindow::onError);
    connect(m_thread, &CombinedFetchThread::finished,
            m_thread, &QObject::deleteLater);
    m_thread->start();
}

void DashboardWindow::onData(FinancialDashboard dash, CloudbedsMetrics cb)
{
    m_lastDash = dash;
    m_lastCb   = cb;

    m_comparison->populate(dash, cb);
    m_roomPanel->populate(cb);
    m_grid->populate(dash, cb);
    m_gpPanel->populate(dash);

    bool live = !m_useMock;
    m_modeBadge->setText(live ? "LIVE" : "MOCK DATA");
    m_modeBadge->setStyleSheet(
        QString("font-size:11px; color:white; background:%1;"
                "border-radius:4px; padding:2px 8px;")
            .arg(live ? "#28a745" : "#6c757d")
    );
    m_statusBar->showMessage(
        QString("Updated %1  •  %2")
            .arg(QTime::currentTime().toString("HH:mm:ss"))
            .arg(dash.as_of.toString("dddd, dd MMMM yyyy")),
        8000
    );
    m_fetchPending = false;
    m_refreshBtn->setEnabled(true);
    m_exportBtn->setEnabled(true);
}

void DashboardWindow::onError(const QString &message)
{
    m_fetchPending = false;
    m_statusBar->showMessage("Error: " + message);
    m_refreshBtn->setEnabled(true);
}

void DashboardWindow::exportXlsx()
{
    if (!m_lastDash) return;
    try {
        m_exportBtn->setEnabled(false);
        m_statusBar->showMessage("Exporting…");
        auto outPath = exportDailyReport(*m_lastDash, m_lastCb);
        m_statusBar->showMessage(
            QString("Saved → %1").arg(QString::fromStdString(outPath.string())), 8000
        );
    } catch (const std::exception &ex) {
        QMessageBox::critical(this, "Export failed", ex.what());
        m_statusBar->showMessage(QString("Export error: %1").arg(ex.what()));
    }
    m_exportBtn->setEnabled(true);
}
