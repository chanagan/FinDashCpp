#pragma once
#include "clients.h"
#include "models.h"
#include <QThread>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
// CombinedFetchThread
//
// Fetches LSK and Cloudbeds data in parallel using two std::threads, then
// emits finished() on success or error() on failure.
//
// Usage:
//   auto *t = new CombinedFetchThread(date, lskClient, cbClient, this);
//   connect(t, &CombinedFetchThread::finished, this, &Dashboard::onData);
//   connect(t, &CombinedFetchThread::error,    this, &Dashboard::onError);
//   t->start();
// ─────────────────────────────────────────────────────────────────────────────
class CombinedFetchThread : public QThread
{
    Q_OBJECT

public:
    // Live clients
    CombinedFetchThread(const QDate             &targetDate,
                        CloudbedsApiClient      *cbClient,
                        LskApiClient            *lskClient,
                        QObject                 *parent = nullptr);

    // Mock clients
    CombinedFetchThread(const QDate             &targetDate,
                        QObject                 *parent = nullptr);

signals:
    void finished(FinancialDashboard dash, CloudbedsMetrics cb);
    void error(QString message);

protected:
    void run() override;

private:
    QDate               m_date;
    CloudbedsApiClient *m_cbClient  = nullptr;   // may be null → use mock
    LskApiClient       *m_lskClient = nullptr;   // may be null → use mock
    bool                m_useMock   = false;
};
