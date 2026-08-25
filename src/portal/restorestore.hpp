#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

namespace wormhole::portal {

struct RestoreEntry {
    QString appId;
    bool isWindow = false;
    QString outputName;
    QString windowAppId;
    QString windowTitle;
    QString windowAddress;
    QString toplevelIdentifier;
    int x = 0;
    int y = 0;
    int fps = 60;
    bool durable = false;
    qint64 lastUsed = 0;
};

class RestoreStore {
public:
    struct Listing {
        QString token;
        RestoreEntry entry;
    };

    static RestoreStore* instance();
    static RestoreStore* createForTesting();

    RestoreStore();
    ~RestoreStore() = default;

    QString addOrReplace(const RestoreEntry& entry);
    QString add(const RestoreEntry& entry) { return addOrReplace(entry); }
    bool take(const QString& token, const QString& appId, RestoreEntry& entry);
    void remove(const QString& token);
    int revokeApp(const QString& appId);
    void revokeAll();
    QList<Listing> listEntries() const;

private:
    bool isValid(const RestoreEntry& entry) const;
    QString findMatchingToken(const RestoreEntry& entry) const;
    void enforcePerAppCap(const QString& appId);
    void load();
    void save() const;
    static QString filePath();

    QHash<QString, RestoreEntry> m_entries;
    static constexpr int kMaxEntriesPerApp = 16;
};

} // namespace wormhole::portal
