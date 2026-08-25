#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include "portal/restorestore.hpp"

using wormhole::portal::RestoreEntry;
using wormhole::portal::RestoreStore;

namespace {

void setStateHome(const QString& path) {
    qputenv("XDG_STATE_HOME", path.toUtf8());
}

QString statePath(const QTemporaryDir& dir) {
    return dir.path();
}

RestoreEntry monitorEntry(const QString& appId, const QString& output, bool durable = true) {
    RestoreEntry entry;
    entry.appId = appId;
    entry.outputName = output;
    entry.fps = 60;
    entry.durable = durable;
    return entry;
}

RestoreEntry windowEntry(const QString& appId, const QString& identifier, bool durable = true) {
    RestoreEntry entry;
    entry.appId = appId;
    entry.isWindow = true;
    entry.windowAppId = QStringLiteral("org.app.Foo");
    entry.windowTitle = QStringLiteral("Window Title");
    entry.toplevelIdentifier = identifier;
    entry.durable = durable;
    return entry;
}

} // namespace

class TestRestoreStore : public QObject {
    Q_OBJECT

private slots:
    void stableTokenPerIdentity();
    void distinctTokensPerSource();
    void takeMatchesWildcardAppId();
    void takeRejectsWrongAppId();
    void durableEntriesPersistAcrossInstances();
    void volatileEntriesDoNotPersist();
    void removePersists();
    void revokeAppOnlyAffectsApp();
    void revokeAllClearsEverything();
    void perAppCapEnforced();
    void invalidEntriesRejected();
    void legacyDuplicateHealing();
};

void TestRestoreStore::stableTokenPerIdentity() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    auto* store = RestoreStore::createForTesting();
    const QString token1 = store->addOrReplace(monitorEntry(QStringLiteral("firefox"), QStringLiteral("DP-1")));
    QVERIFY(!token1.isEmpty());

    RestoreEntry updated = monitorEntry(QStringLiteral("firefox"), QStringLiteral("DP-1"));
    updated.fps = 144;
    const QString token2 = store->addOrReplace(updated);
    QCOMPARE(token2, token1);

    RestoreEntry out;
    QVERIFY(store->take(token1, QStringLiteral("firefox"), out));
    QCOMPARE(out.fps, 144);
    delete store;
}

void TestRestoreStore::distinctTokensPerSource() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    auto* store = RestoreStore::createForTesting();
    const QString t1 = store->addOrReplace(monitorEntry(QStringLiteral("firefox"), QStringLiteral("DP-1")));
    const QString t2 = store->addOrReplace(monitorEntry(QStringLiteral("firefox"), QStringLiteral("DP-2")));
    const QString t3 = store->addOrReplace(monitorEntry(QStringLiteral("obs"), QStringLiteral("DP-1")));
    QVERIFY(t1 != t2);
    QVERIFY(t1 != t3);
    QVERIFY(t2 != t3);
    delete store;
}

void TestRestoreStore::takeMatchesWildcardAppId() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    auto* store = RestoreStore::createForTesting();
    RestoreEntry entry = monitorEntry(QString(), QStringLiteral("eDP-1"));
    entry.appId.clear();
    const QString token = store->addOrReplace(entry);

    RestoreEntry out;
    QVERIFY(store->take(token, QStringLiteral("any-app"), out));
    QCOMPARE(out.outputName, QStringLiteral("eDP-1"));
    delete store;
}

void TestRestoreStore::takeRejectsWrongAppId() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    auto* store = RestoreStore::createForTesting();
    const QString token = store->addOrReplace(monitorEntry(QStringLiteral("firefox"), QStringLiteral("DP-1")));

    RestoreEntry out;
    QVERIFY(!store->take(token, QStringLiteral("chromium"), out));
    QVERIFY(store->take(token, QStringLiteral("firefox"), out));
    delete store;
}

void TestRestoreStore::durableEntriesPersistAcrossInstances() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    const QString identifier = QStringLiteral("hyprland:0x1234abcd");
    QString token;
    {
        auto* store = RestoreStore::createForTesting();
        token = store->addOrReplace(windowEntry(QStringLiteral("app-one"), identifier));
        delete store;
    }

    QVERIFY(QFile::exists(statePath(dir) + QStringLiteral("/wormhole/screencast.json")));

    auto* store2 = RestoreStore::createForTesting();
    RestoreEntry out;
    QVERIFY(store2->take(token, QStringLiteral("app-one"), out));
    QCOMPARE(out.toplevelIdentifier, identifier);
    QCOMPARE(out.isWindow, true);

    const QString storeFilePath = statePath(dir) + QStringLiteral("/wormhole/screencast.json");
    const QFileDevice::Permissions perms = QFileInfo(storeFilePath).permissions();
    QVERIFY(!(perms & QFileDevice::ReadGroup));
    QVERIFY(!(perms & QFileDevice::ReadOther));

    delete store2;
}

void TestRestoreStore::volatileEntriesDoNotPersist() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    QString token;
    {
        auto* store = RestoreStore::createForTesting();
        token = store->addOrReplace(monitorEntry(QStringLiteral("obs"), QStringLiteral("HDMI-A-1"), false));
        delete store;
    }
    QVERIFY(!QFile::exists(statePath(dir) + QStringLiteral("/wormhole/screencast.json")));

    auto* store2 = RestoreStore::createForTesting();
    RestoreEntry out;
    QVERIFY(!store2->take(token, QStringLiteral("obs"), out));
    delete store2;
}

void TestRestoreStore::removePersists() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    auto* store = RestoreStore::createForTesting();
    const QString keepToken = store->addOrReplace(monitorEntry(QStringLiteral("a"), QStringLiteral("DP-1")));
    const QString killToken = store->addOrReplace(monitorEntry(QStringLiteral("b"), QStringLiteral("DP-2")));
    store->remove(killToken);

    auto* store2 = RestoreStore::createForTesting();
    RestoreEntry out;
    QVERIFY(store2->take(keepToken, QStringLiteral("a"), out));
    QVERIFY(!store2->take(killToken, QStringLiteral("b"), out));
    delete store2;
    delete store;
}

void TestRestoreStore::revokeAppOnlyAffectsApp() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    auto* store = RestoreStore::createForTesting();
    store->addOrReplace(monitorEntry(QStringLiteral("firefox"), QStringLiteral("DP-1")));
    store->addOrReplace(monitorEntry(QStringLiteral("firefox"), QStringLiteral("DP-2")));
    store->addOrReplace(monitorEntry(QStringLiteral("obs"), QStringLiteral("DP-1")));

    QCOMPARE(store->revokeApp(QStringLiteral("firefox")), 2);
    QCOMPARE(store->listEntries().size(), 1);

    const auto remaining = store->listEntries();
    QCOMPARE(remaining.first().entry.appId, QStringLiteral("obs"));
    delete store;
}

void TestRestoreStore::revokeAllClearsEverything() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    auto* store = RestoreStore::createForTesting();
    store->addOrReplace(monitorEntry(QStringLiteral("a"), QStringLiteral("DP-1")));
    store->addOrReplace(monitorEntry(QStringLiteral("b"), QStringLiteral("DP-2")));
    store->revokeAll();
    QCOMPARE(store->listEntries().size(), 0);

    auto* store2 = RestoreStore::createForTesting();
    QCOMPARE(store2->listEntries().size(), 0);
    delete store2;
    delete store;
}

void TestRestoreStore::perAppCapEnforced() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    auto* store = RestoreStore::createForTesting();
    for (int i = 0; i < 25; ++i) {
        store->addOrReplace(monitorEntry(QStringLiteral("app"), QStringLiteral("OUT-%1").arg(i)));
    }

    int appCount = 0;
    for (const auto& listing : store->listEntries()) {
        if (listing.entry.appId == QStringLiteral("app")) {
            ++appCount;
        }
    }
    QVERIFY(appCount <= 17);
    delete store;
}

void TestRestoreStore::invalidEntriesRejected() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    auto* store = RestoreStore::createForTesting();
    RestoreEntry badMonitor;
    badMonitor.appId = QStringLiteral("x");
    badMonitor.durable = true;

    RestoreEntry badWindow;
    badWindow.appId = QStringLiteral("x");
    badWindow.isWindow = true;
    badWindow.durable = true;

    QVERIFY(store->addOrReplace(badMonitor).isEmpty());
    QVERIFY(store->addOrReplace(badWindow).isEmpty());
    QCOMPARE(store->listEntries().size(), 0);
    delete store;
}

void TestRestoreStore::legacyDuplicateHealing() {
    QTemporaryDir dir;
    setStateHome(statePath(dir));

    const QString storeFile = statePath(dir) + QStringLiteral("/wormhole/screencast.json");
    QDir().mkpath(QFileInfo(storeFile).absolutePath());

    QJsonObject root;
    QJsonObject stale;
    stale.insert(QStringLiteral("app_id"), QStringLiteral("firefox"));
    stale.insert(QStringLiteral("output"), QStringLiteral("DP-1"));
    stale.insert(QStringLiteral("last_used"), static_cast<qint64>(1000));
    root.insert(QStringLiteral("stale-token"), stale);

    QJsonObject fresh;
    fresh.insert(QStringLiteral("app_id"), QStringLiteral("firefox"));
    fresh.insert(QStringLiteral("output"), QStringLiteral("DP-1"));
    fresh.insert(QStringLiteral("last_used"),
                 static_cast<qint64>(QDateTime::currentSecsSinceEpoch()));
    root.insert(QStringLiteral("fresh-token"), fresh);

    QFile file(storeFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(root).toJson());
    file.close();

    auto* store = RestoreStore::createForTesting();
    QCOMPARE(store->listEntries().size(), 1);

    RestoreEntry out;
    QVERIFY(store->take(QStringLiteral("fresh-token"), QStringLiteral("firefox"), out));
    QVERIFY(!store->take(QStringLiteral("stale-token"), QStringLiteral("firefox"), out));
    delete store;
}

QTEST_GUILESS_MAIN(TestRestoreStore)
#include "test_restorestore.moc"
