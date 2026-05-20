#include "threads.h"
#include <QDebug>
#include <stdexcept>

CombinedFetchThread::CombinedFetchThread(const QDate        &targetDate,
                                         CloudbedsApiClient *cbClient,
                                         LskApiClient       *lskClient,
                                         QObject            *parent)
    : QThread(parent)
    , m_date(targetDate)
    , m_cbClient(cbClient)
    , m_lskClient(lskClient)
    , m_useMock(false)
{}

CombinedFetchThread::CombinedFetchThread(const QDate &targetDate, QObject *parent)
    : QThread(parent)
    , m_date(targetDate)
    , m_useMock(true)
{}

void CombinedFetchThread::run()
{
    try {
        // Fetch both data sources sequentially in this QThread
        // (No std::async to avoid std::thread threading issues with Qt networking)

        FinancialDashboard dash;
        if (m_useMock) {
            dash = MockLskClient{}.fetchDashboard(m_date);
        } else {
            dash = m_lskClient->fetchDashboard(m_date);
        }

        CloudbedsMetrics cb;
        if (m_useMock) {
            cb = MockCloudbedsClient{}.fetchMetrics(m_date);
        } else {
            cb = m_cbClient->fetchMetrics(m_date);
        }

        emit finished(dash, cb);
    }
    catch (const std::exception &ex) {
        qWarning() << "[FetchThread] Error:" << ex.what();
        emit error(QString::fromStdString(ex.what()));
    }
}
