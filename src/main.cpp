#include "clients.h"
#include "config.h"
#include "dashboard.h"
#include <QApplication>
#include <QStyleHints>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);

    QApplication app(argc, argv);
    app.setApplicationName("FinDash");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption mockOpt("mock", "Use mock data (no API calls)");
    parser.addOption(mockOpt);
    parser.process(app);

    DashboardWindow *window = nullptr;

    if (parser.isSet(mockOpt)) {
        window = new DashboardWindow();   // mock mode
    } else {
        auto cbCfg   = Config::instance().cloudbedsConfig();
        auto apiKey  = cbCfg["api_key"].toString();
        auto propId  = cbCfg["property_id"].toString();

        if (apiKey.isEmpty()) {
            qCritical() << "[Main] No Cloudbeds API key found in config.json — use --mock to run offline.";
            return 1;
        }

        auto *cbClient  = new CloudbedsApiClient(apiKey, propId);
        auto *lskClient = new LskApiClient();
        window = new DashboardWindow(cbClient, lskClient);
    }

    window->show();
    return app.exec();
}
