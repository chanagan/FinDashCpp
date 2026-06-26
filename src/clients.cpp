#include "clients.h"
#include "config.h"
#include "lsk_api.h"

#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QUrlQuery>
#include <stdexcept>
#include <fstream>

struct PeriodValues
{
    double today = 0.0;
    double mtd = 0.0;
    double mtdLY = 0.0;

    PeriodValues& operator+=(const PeriodValues& other) {
        today += other.today;
        mtd   += other.mtd;
        mtdLY += other.mtdLY;
        return *this;
    }
};

PeriodValues parseJsonValues(const QJsonObject &revenueType)
{
    double yest    = revenueType["Today"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    double mtd    = revenueType["MTD"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    double mtdLY    = revenueType["MTD-LY"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();

    return PeriodValues{yest, mtd, mtdLY};
};

// all these roll together for 'Other Charges
QStringList otherCharges = {
    "Other Room Charges Cancelation Fee",
    "Other Room Charges Cleaning Fee",
    "Other Room Charges Damages",
    "Other Room Charges Early Check-Out Fee",
    "Other Room Charges Forfeit",
    "Other Room Charges Guest Laundry",
    "Other Room Charges Hair Dryer",
};
// -------------
// getRevenueValues - get yesterday / mtd / ly-mtd for a revenue type
//
std::tuple <double, double, double> getRevenueValues(QJsonObject revenueType)
{
    double yest    = revenueType["Today"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    double mtd    = revenueType["MTD"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    double mtdLY    = revenueType["MTD-LY"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    return std::make_tuple(yest, mtd, mtdLY);
}


// ─────────────────────────────────────────────────────────────────────────────
// CloudbedsApiClient — construction
// ─────────────────────────────────────────────────────────────────────────────

CloudbedsApiClient::CloudbedsApiClient(const QString &apiKey,
                                       const QString &propertyId,
                                       QObject       *parent)
    : QObject(parent)
    , m_propertyId(propertyId)
    , m_authHeader("Bearer " + apiKey.toUtf8())
{
    auto &cfg       = Config::instance();
    m_occCacheFile  = cfg.configDir() / ".cloudbeds_occ_cache.json";
    m_reportDefFile = cfg.reportDefFilePath();
    loadOccCache();
}

// ─────────────────────────────────────────────────────────────────────────────
// HTTP helpers
// All block the calling thread using a local QEventLoop — safe inside QThread.
// ─────────────────────────────────────────────────────────────────────────────

QJsonObject CloudbedsApiClient::httpGet(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", m_authHeader);
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("X-PROPERTY-ID", m_propertyId.toUtf8());

    QNetworkAccessManager network;
    network.moveToThread(QThread::currentThread());

    QNetworkReply *reply = network.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        QByteArray responseBody = reply->readAll();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qWarning() << "[Cloudbeds] HTTP error (status" << statusCode << "):" << errorMsg
                   << "Response:" << QString::fromUtf8(responseBody);
        throw std::runtime_error("HTTP " + std::to_string(statusCode) + ": " + errorMsg.toStdString());
    }

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(reply->readAll(), &err);
    reply->deleteLater();
    if (err.error != QJsonParseError::NoError)
        throw std::runtime_error("JSON parse: " + err.errorString().toStdString());
    return doc.object();
}

QJsonObject CloudbedsApiClient::httpPut(const QUrl &url, const QJsonObject &body)
{
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", m_authHeader);
    req.setRawHeader("Content-Type", "application/json");
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("X-PROPERTY-ID", m_propertyId.toUtf8());

    QNetworkAccessManager network;
    network.moveToThread(QThread::currentThread());

    QNetworkReply *reply = network.put(req, QJsonDocument(body).toJson());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        QByteArray responseBody = reply->readAll();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qWarning() << "[Cloudbeds] HTTP error (status" << statusCode << "):" << errorMsg
                   << "Response:" << QString::fromUtf8(responseBody);
        throw std::runtime_error("HTTP " + std::to_string(statusCode) + ": " + errorMsg.toStdString());
    }

    reply->deleteLater();
    return {};   // PUT response body not needed
}

QJsonObject CloudbedsApiClient::apiGet(const QString &endpoint,
                                       const QMap<QString,QString> &params)
{
    QUrl url(QString(BASE_URL) + "/" + endpoint);
    QUrlQuery q;
    q.addQueryItem("propertyID", m_propertyId);
    for (auto it = params.cbegin(); it != params.cend(); ++it)
        q.addQueryItem(it.key(), it.value());
    url.setQuery(q);
    return httpGet(url);
}

QJsonObject CloudbedsApiClient::reportGet(const QString &reportId,
                                          const QMap<QString,QString> &params)
{
    QUrl url(QString(BASE_REP_URL) + "/" + reportId + "/data");
    QUrlQuery q;
    q.addQueryItem("propertyID", m_propertyId);
    for (auto it = params.cbegin(); it != params.cend(); ++it)
        q.addQueryItem(it.key(), it.value());
    url.setQuery(q);
    return httpGet(url);
}

QJsonObject CloudbedsApiClient::reportPut(const QString &reportId,
                                          const QJsonObject &body)
{
    QUrl url(QString(BASE_REP_URL) + "/" + reportId);
    httpPut(url, body);
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// Date helpers
// ─────────────────────────────────────────────────────────────────────────────

QString CloudbedsApiClient::isoDate(const QDate &d)
{
    return d.toString("yyyy-MM-dd");
}

QString CloudbedsApiClient::isoDateTime(const QDate &d)
{
    return d.toString("yyyy-MM-dd") + "T00:00:00.000Z";
}

// ─────────────────────────────────────────────────────────────────────────────
// Report definition update
// ─────────────────────────────────────────────────────────────────────────────

QJsonArray CloudbedsApiClient::buildPeriods(const QDate &d) const
{
    QDate tomorrow    = d.addDays(1);
    QDate monthStart  = QDate(d.year(), d.month(), 1);
    QDate lyDate      = d.addYears(-1);
    QDate lyTomorrow  = lyDate.addDays(1);
    QDate lyMonthStart = QDate(lyDate.year(), lyDate.month(), 1);

    auto makePeriod = [&](const QString &name,
                          const QDate  &start,
                          const QDate  &end) -> QJsonObject {
        return QJsonObject{
            {"cdf", QJsonObject{{"type","default"},{"column","service_date"}}},
            {"name", name},
            {"start", isoDateTime(start)},
            {"end",   isoDateTime(end)},
            {"start_relative_to_end", false}
        };
    };

    return QJsonArray{
        makePeriod("Today",  d,           tomorrow),
        makePeriod("MTD",    monthStart,  tomorrow),
        makePeriod("MTD-LY", lyMonthStart,lyTomorrow),
    };
}

void CloudbedsApiClient::updateReportDef(const QString &reportId, const QDate &d)
{
    // Load the definition template from disk
    QFile f(QString::fromStdString(m_reportDefFile.string()));
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error("Cannot open report def: " + m_reportDefFile.string());

    QJsonObject def = QJsonDocument::fromJson(f.readAll()).object();
    def["periods"]  = buildPeriods(d);

    reportPut(reportId, def);
    qInfo() << "[Cloudbeds] Report" << reportId << "updated → Today ="
            << isoDate(d) << "  MTD from" << isoDate(QDate(d.year(), d.month(), 1));
}

// ─────────────────────────────────────────────────────────────────────────────
// Occupancy cache
// ─────────────────────────────────────────────────────────────────────────────

void CloudbedsApiClient::loadOccCache()
{
    QFile f(QString::fromStdString(m_occCacheFile.string()));
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    for (const QString &key : obj.keys()) {
        QJsonObject v = obj[key].toObject();
        m_occCache[key] = { v["occupied"].toInt(), v["capacity"].toInt() };
    }
}

void CloudbedsApiClient::saveOccCache() const
{
    QJsonObject obj;
    for (auto it = m_occCache.cbegin(); it != m_occCache.cend(); ++it)
        obj[it.key()] = QJsonObject{{"occupied", it->occupied}, {"capacity", it->capacity}};

    QFile f(QString::fromStdString(m_occCacheFile.string()));
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(obj).toJson());
}

std::optional<CloudbedsApiClient::OccEntry>
CloudbedsApiClient::occCached(const QDate &d) const
{
    if (d >= QDate::currentDate()) return std::nullopt;   // today may still change
    auto it = m_occCache.find(isoDate(d));
    if (it == m_occCache.end()) return std::nullopt;
    return *it;
}

void CloudbedsApiClient::occCache(const QDate &d, int occupied, int capacity)
{
    if (d >= QDate::currentDate()) return;
    m_occCache[isoDate(d)] = { occupied, capacity };
    saveOccCache();
}

// ─────────────────────────────────────────────────────────────────────────────
// getRoomsOccupied — accumulates daily getDashboard for current + LY month
// ─────────────────────────────────────────────────────────────────────────────

CloudbedsApiClient::OccupancyCounts
CloudbedsApiClient::getRoomsOccupied(const QDate &targetDate)
{
    OccupancyCounts result;

    auto accumulate = [&](const QDate &target, bool isLY) {
        QDate first(target.year(), target.month(), 1);
        QDate cur = first;
        while (cur <= target) {
            int occupied = 0, capacity = 0;
            if (auto cached = occCached(cur)) {
                occupied = cached->occupied;
                capacity = cached->capacity;
            } else {
                auto payload = apiGet("getDashboard", {{"date", isoDate(cur)}});
                auto data    = payload["data"].toObject();
                occupied     = data["roomsOccupied"].toInt();
                capacity     = data["capacity"].toInt();
                occCache(cur, occupied, capacity);
            }
            if (!isLY) {
                result.this_month_occupied += occupied;
                result.this_month_capacity += capacity;
                if (cur == target) {
                    result.this_date_occupied = occupied;
                    result.this_date_capacity = capacity;
                }
            } else {
                result.ly_month_occupied += occupied;
                result.ly_month_capacity += capacity;
                if (cur == target) {
                    result.ly_date_occupied = occupied;
                    result.ly_date_capacity = capacity;
                }
            }
            cur = cur.addDays(1);
        }
    };

    accumulate(targetDate,          false);
    accumulate(targetDate.addYears(-1), true);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// fetchMetrics — main entry point
// ─────────────────────────────────────────────────────────────────────────────

CloudbedsMetrics CloudbedsApiClient::fetchMetrics(const QDate &targetDate)
{
    qInfo() << "[Cloudbeds] fetchMetrics →" << targetDate.toString(Qt::ISODate);

    // Step 0: update report period dates
    updateReportDef(REPORT_ID, targetDate);

    // Step 1: occupancy counts
    auto occ = getRoomsOccupied(targetDate);

    // Step 2: revenue from report (Today / MTD / MTD-LY all in one response)
    auto payload = reportGet(REPORT_ID, {{"format", "raw"},
                                         {"date",   isoDate(targetDate)}});
    QByteArray responseBody = QJsonDocument(payload).toJson();

    // Navigate: records → "Room Rate" → strip-whitespace key → period → metric → sum
    auto stripKeys = [](const QJsonObject &obj) {
        QJsonObject out;
        for (const QString &k : obj.keys())
            out[k.trimmed()] = obj[k];
        return out;
    };

    auto roomRate  = stripKeys(payload["records"].toObject()["Room Rate"].toObject())["Room Rate"].toObject();
    auto revYest   = roomRate["Today"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    auto revMtd    = roomRate["MTD"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    auto revLyMtd  = roomRate["MTD-LY"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();

    qInfo() << "[Cloudbeds clients] revenue today=" << revYest << "  MTD=" << revMtd << "  MTD-LY=" << revLyMtd;

    auto itmsSvcs = stripKeys(payload["records"].toObject()["Items & Services"].toObject());
    // auto canxFee = stripKeys(itmsSvcs["Other Room Charges Cancelation Fee"].toObject());

    PeriodValues tot;
    PeriodValues pv;
    for (const QString& s : otherCharges) {
        auto otherFee = stripKeys(itmsSvcs[s].toObject());
        pv = parseJsonValues(otherFee);
        tot += pv;
        qInfo() << "[Cloudbeds clients] " << s << ": " << pv.today << "  MTD=" << pv.mtd << "  MTD-LY=" << pv.mtdLY;
    }
    qInfo() << "[Cloudbeds clients] Total Other: " << tot.today << "  MTD=" << tot.mtd << "  MTD-LY=" << tot.mtdLY;

    // PeriodValues r = parseJsonValues(canxFee);
    // qInfo() << "[Cloudbeds clients] canx revenue today=" << r.today << "  MTD=" << r.mtd << "  MTD-LY=" << r.mtdLY;
    //
    //
    //
    //
    // auto [yester, mtd, mtdLY] = getRevenueValues(canxFee);
    // // what if i do that again with the auto - maybe an issue
    // // - make the function so it get all the other revenue and returns it here - makes better sense for where we are
    // qInfo() << "[Cloudbeds] canx revenue today=" << yester << "  MTD=" << mtd << "  MTD-LY=" << mtdLY;
    // // auto canxYestr    = canxFee["Today"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    // // auto canxMtd    = canxFee["MTD"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    // // auto canxLyMtd    = canxFee["MTD-LY"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    //
    // // auto itmsSvcsCanxFee  = stripKeys(payload["records"].toObject()["Items & Services"].toObject())["Other Room Charges Cancelation Fee"].toObject();
    // // auto canxYes    = itmsSvcsCanxFee["Today"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    // // auto canxMtd    = itmsSvcsCanxFee["MTD"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();
    // // auto canxLyMtd  = itmsSvcsCanxFee["MTD-LY"].toObject()["balance_due_amount_converted_rate"].toObject()["sum"].toDouble();


    // Step 3: assemble KPIs
    CloudbedsMetrics cb;
    cb.date           = targetDate;
    cb.occupied_rooms = occ.this_date_occupied;
    cb.total_rooms    = occ.this_date_capacity;
    cb.room_revenue   = revYest;
    cb.room_revenue_othr   = tot.today;
    cb.revpar         = occ.this_date_capacity  ? revYest / occ.this_date_capacity  : 0.0;
    cb.adr            = occ.this_date_occupied  ? revYest / occ.this_date_occupied  : 0.0;

    cb.mtd_revenue    = revMtd;
    cb.mtd_revenue_othr    = tot.mtd;
    cb.mtd_revpar     = occ.this_month_capacity ? revMtd  / occ.this_month_capacity : 0.0;
    cb.mtd_adr        = occ.this_month_occupied ? revMtd  / occ.this_month_occupied : 0.0;

    cb.ly_date_occupied  = occ.ly_date_occupied;
    cb.ly_date_capacity  = occ.ly_date_capacity;
    cb.ly_month_occupied = occ.ly_month_occupied;
    cb.ly_month_capacity = occ.ly_month_capacity;

    cb.ly_mtd_revenue    = revLyMtd;
    cb.ly_mtd_revenue_othr    = tot.mtdLY;
    cb.ly_mtd_revpar     = occ.ly_month_capacity ? revLyMtd / occ.ly_month_capacity : 0.0;
    cb.ly_mtd_adr        = occ.ly_month_occupied ? revLyMtd / occ.ly_month_occupied : 0.0;

    return cb;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mock clients
// ─────────────────────────────────────────────────────────────────────────────

CloudbedsMetrics MockCloudbedsClient::fetchMetrics(const QDate &targetDate)
{
    CloudbedsMetrics cb;
    cb.date           = targetDate;
    cb.occupied_rooms = 35;
    cb.total_rooms    = 39;
    cb.room_revenue   = 8050.0;
    cb.adr            = cb.room_revenue / cb.occupied_rooms;
    cb.revpar         = cb.room_revenue / cb.total_rooms;
    cb.mtd_revenue    = 142000.0;
    cb.mtd_adr        = 195.0;
    cb.mtd_revpar     = 175.0;
    cb.ly_mtd_revenue = 131000.0;
    cb.ly_mtd_adr     = 180.0;
    cb.ly_mtd_revpar  = 162.0;
    return cb;
}

FinancialDashboard LskApiClient::fetchDashboard(const QDate &targetDate)
{
    try {
        auto &cfg = Config::instance();
        QJsonObject lskCfg = cfg.lskConfig();

        QString clientId = lskCfg["client_id"].toString();
        QString clientSecret = lskCfg["client_secret"].toString();
        QString businessLocId = lskCfg["business_location_id"].toString();

        if (clientId.isEmpty() || clientSecret.isEmpty() || businessLocId.isEmpty()) {
            throw std::runtime_error("LSK credentials not configured in config.json");
        }

        // Token file path in config directory
        auto tokenFilePath = cfg.configDir() / ".lsk_tokens.json";

        // Create API client and fetch data
        LskApi api(clientId, clientSecret, businessLocId, QString::fromStdString(tokenFilePath.string()));
        QJsonObject daily = api.getDailyTotals(targetDate);
        QJsonObject mtd = api.getMtdTotals(targetDate);
        QJsonObject ly_mtd = api.getLyMtdTotals(targetDate);

        FinancialDashboard dashboard = LskApi::buildDashboard(targetDate, daily, mtd, ly_mtd);

        qInfo() << "[LSK] Dashboard fetched successfully for" << targetDate.toString(Qt::ISODate);
        return dashboard;

    } catch (const std::exception &ex) {
        qCritical() << "[LSK] Error fetching dashboard:" << ex.what();
        throw;
    }
}

FinancialDashboard MockLskClient::fetchDashboard(const QDate &targetDate)
{
    auto makeGroup = [](double amt, double tax, double svc, int txns) {
        GroupTotals g;
        g.total_amount = amt; g.tax = tax; g.service_charge = svc; g.transaction_count = txns;
        return g;
    };
    auto makePeriod = [&](double bar, double food, double health, double retail) {
        PeriodData p;
        p.groups["Cafe Bar"]    = makeGroup(bar,    bar*0.075,    bar*0.006,    qMax(1,(int)(bar/4.2)));
        p.groups["Cafe Food"]   = makeGroup(food,   food*0.074,   food*0.007,   qMax(1,(int)(food/5.0)));
        p.groups["Health Club"] = makeGroup(health, health*0.074, health*0.013, qMax(1,(int)(health/45)));
        p.groups["Retail"]      = makeGroup(retail, retail*0.075, 0, qMax(1,(int)(retail/33)));
        p.total_amount = bar + food + health + retail;
        return p;
    };
    FinancialDashboard d;
    d.as_of              = targetDate;
    d.today              = makePeriod(2391, 2082, 592, 33);
    d.same_day_last_year = makePeriod(2100, 1850, 520, 25);
    d.mtd                = makePeriod(48000, 41000, 9500, 680);
    d.mtd_last_year      = makePeriod(43000, 37000, 8800, 600);
    return d;
}
