#pragma once
#include "clients.h"
#include "models.h"
#include "threads.h"
#include "widgets.h"
#include <QDateEdit>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QStatusBar>
#include <optional>

// ─────────────────────────────────────────────────────────────────────────────
// DashboardWindow
// ─────────────────────────────────────────────────────────────────────────────
class DashboardWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Live mode — pass real clients
    explicit DashboardWindow(CloudbedsApiClient *cbClient,
                             LskApiClient       *lskClient,
                             QWidget            *parent = nullptr);

    // Mock mode
    explicit DashboardWindow(QWidget *parent = nullptr);

private slots:
    void refresh();
    void onData (FinancialDashboard dash, CloudbedsMetrics cb);
    void onError(const QString &message);
    void exportXlsx();

private:
    void buildUi();
    QLayout *buildHeader();
    QWidget *buildBody();
    QDate    selectedDate() const;

    // ── Widgets ───────────────────────────────────────────────────────────────
    QDateEdit          *m_datePicker   = nullptr;
    QPushButton        *m_refreshBtn   = nullptr;
    QPushButton        *m_exportBtn    = nullptr;
    QLabel             *m_modeBadge    = nullptr;
    QStatusBar         *m_statusBar    = nullptr;

    ComparisonPanel    *m_comparison   = nullptr;
    RoomOccupancyPanel *m_roomPanel    = nullptr;
    FinancialGrid      *m_grid         = nullptr;
    BankingPanel       *m_bankingPanel = nullptr;
    GuestPrivilegePanel*m_gpPanel      = nullptr;

    // ── State ─────────────────────────────────────────────────────────────────
    bool                          m_fetchPending = false;
    CombinedFetchThread          *m_thread       = nullptr;
    std::optional<FinancialDashboard> m_lastDash;
    std::optional<CloudbedsMetrics>   m_lastCb;

    CloudbedsApiClient *m_cbClient  = nullptr;
    LskApiClient       *m_lskClient = nullptr;
    bool                m_useMock   = false;
};
