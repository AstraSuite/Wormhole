#pragma once

#include <QHash>
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
    int x = 0;
    int y = 0;
    int fps = 60;
    bool durable = false;
};

class RestoreStore {
public:
    static RestoreStore* instance();

    QString add(const RestoreEntry& entry);
    bool take(const QString& token, const QString& appId, RestoreEntry& entry) const;
    void remove(const QString& token);

private:
    RestoreStore();

    void load();
    void save() const;
    static QString filePath();

    QHash<QString, RestoreEntry> m_entries;
};

} // namespace wormhole::portal
