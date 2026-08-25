#include "restorestore.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

namespace wormhole::portal {

RestoreStore* RestoreStore::instance() {
    static RestoreStore store;
    return &store;
}

RestoreStore* RestoreStore::createForTesting() {
    return new RestoreStore();
}

RestoreStore::RestoreStore() {
    load();
}

QString RestoreStore::filePath() {
    QString base = qEnvironmentVariable("XDG_STATE_HOME");
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/.local/state");
    }
    return base + QStringLiteral("/wormhole/screencast.json");
}

bool RestoreStore::isValid(const RestoreEntry& entry) const {
    if (entry.isWindow) {
        return !entry.toplevelIdentifier.isEmpty() || !entry.windowAddress.isEmpty() || !entry.windowAppId.isEmpty();
    }
    return !entry.outputName.isEmpty();
}

QString RestoreStore::findMatchingToken(const RestoreEntry& entry) const {
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        const RestoreEntry& e = it.value();
        if (e.appId != entry.appId) {
            continue;
        }
        if (e.isWindow != entry.isWindow) {
            continue;
        }
        if (entry.isWindow) {
            if (!entry.toplevelIdentifier.isEmpty() && e.toplevelIdentifier == entry.toplevelIdentifier) {
                return it.key();
            }
            if (!entry.windowAddress.isEmpty() && e.windowAddress == entry.windowAddress) {
                return it.key();
            }
            if (entry.toplevelIdentifier.isEmpty() && entry.windowAddress.isEmpty() &&
                e.windowAppId == entry.windowAppId && e.windowTitle == entry.windowTitle) {
                return it.key();
            }
        } else {
            if (e.outputName == entry.outputName) {
                return it.key();
            }
        }
    }
    return QString();
}

void RestoreStore::enforcePerAppCap(const QString& appId) {
    if (appId.isEmpty()) return;

    QList<QPair<QString, qint64>> appEntries;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (it.value().appId == appId) {
            appEntries.append({ it.key(), it.value().lastUsed });
        }
    }

    if (appEntries.size() > kMaxEntriesPerApp) {
        std::sort(appEntries.begin(), appEntries.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

        int toRemove = appEntries.size() - kMaxEntriesPerApp;
        for (int i = 0; i < toRemove; ++i) {
            m_entries.remove(appEntries.at(i).first);
        }
    }
}

void RestoreStore::load() {
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject root = doc.object();
    QHash<QString, QPair<QString, qint64>> identityToToken;

    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        const QJsonObject obj = it.value().toObject();

        RestoreEntry entry;
        entry.appId = obj.value(QStringLiteral("app_id")).toString();
        entry.isWindow = obj.value(QStringLiteral("is_window")).toBool(false);
        entry.outputName = obj.value(QStringLiteral("output")).toString();
        entry.windowAppId = obj.value(QStringLiteral("window_app_id")).toString();
        entry.windowTitle = obj.value(QStringLiteral("window_title")).toString();
        entry.windowAddress = obj.value(QStringLiteral("window_address")).toString();
        entry.toplevelIdentifier = obj.value(QStringLiteral("toplevel_id")).toString();
        entry.x = obj.value(QStringLiteral("x")).toInt(0);
        entry.y = obj.value(QStringLiteral("y")).toInt(0);
        entry.fps = obj.value(QStringLiteral("fps")).toInt(60);
        entry.durable = true;
        entry.lastUsed = obj.value(QStringLiteral("last_used")).toVariant().toLongLong();
        if (entry.lastUsed == 0) {
            entry.lastUsed = QDateTime::currentSecsSinceEpoch();
        }

        if (!isValid(entry)) {
            continue;
        }

        QString identityKey = QStringLiteral("%1:%2:%3:%4").arg(
            entry.appId,
            QString::number(entry.isWindow ? 1 : 0),
            entry.isWindow ? (entry.toplevelIdentifier.isEmpty() ? entry.windowAddress : entry.toplevelIdentifier) : entry.outputName,
            entry.windowAppId
        );

        if (identityToToken.contains(identityKey)) {
            const auto existing = identityToToken.value(identityKey);
            if (entry.lastUsed > existing.second) {
                m_entries.remove(existing.first);
                m_entries.insert(it.key(), entry);
                identityToToken.insert(identityKey, { it.key(), entry.lastUsed });
            }
        } else {
            identityToToken.insert(identityKey, { it.key(), entry.lastUsed });
            m_entries.insert(it.key(), entry);
        }
    }
}

void RestoreStore::save() const {
    const QString path = filePath();
    bool hasDurable = false;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (it.value().durable) {
            hasDurable = true;
            break;
        }
    }

    if (!hasDurable) {
        if (QFile::exists(path)) {
            QFile::remove(path);
        }
        return;
    }

    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonObject root;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (!it.value().durable) {
            continue;
        }

        QJsonObject obj;
        obj.insert(QStringLiteral("app_id"), it.value().appId);
        obj.insert(QStringLiteral("is_window"), it.value().isWindow);
        obj.insert(QStringLiteral("output"), it.value().outputName);
        obj.insert(QStringLiteral("window_app_id"), it.value().windowAppId);
        obj.insert(QStringLiteral("window_title"), it.value().windowTitle);
        obj.insert(QStringLiteral("window_address"), it.value().windowAddress);
        obj.insert(QStringLiteral("toplevel_id"), it.value().toplevelIdentifier);
        obj.insert(QStringLiteral("x"), it.value().x);
        obj.insert(QStringLiteral("y"), it.value().y);
        obj.insert(QStringLiteral("fps"), it.value().fps);
        obj.insert(QStringLiteral("last_used"), it.value().lastUsed);

        root.insert(it.key(), obj);
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

QString RestoreStore::addOrReplace(const RestoreEntry& entry) {
    if (!isValid(entry)) {
        return QString();
    }

    RestoreEntry newEntry = entry;
    newEntry.lastUsed = QDateTime::currentSecsSinceEpoch();

    QString existingToken = findMatchingToken(newEntry);
    QString token = existingToken.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : existingToken;

    m_entries.insert(token, newEntry);
    enforcePerAppCap(newEntry.appId);

    if (newEntry.durable) {
        save();
    }
    return token;
}

bool RestoreStore::take(const QString& token, const QString& appId, RestoreEntry& entry) {
    if (token.isEmpty()) {
        return false;
    }

    const auto it = m_entries.find(token);
    if (it == m_entries.end()) {
        return false;
    }

    if (!it.value().appId.isEmpty() && !appId.isEmpty() && it.value().appId != appId) {
        return false;
    }

    it.value().lastUsed = QDateTime::currentSecsSinceEpoch();
    entry = it.value();
    return true;
}

void RestoreStore::remove(const QString& token) {
    const auto it = m_entries.find(token);
    if (it == m_entries.end()) {
        return;
    }

    const bool wasDurable = it.value().durable;
    m_entries.erase(it);
    if (wasDurable) {
        save();
    }
}

int RestoreStore::revokeApp(const QString& appId) {
    if (appId.isEmpty()) return 0;

    int count = 0;
    bool hadDurable = false;
    for (auto it = m_entries.begin(); it != m_entries.end(); ) {
        if (it.value().appId == appId) {
            if (it.value().durable) hadDurable = true;
            it = m_entries.erase(it);
            ++count;
        } else {
            ++it;
        }
    }

    if (hadDurable) {
        save();
    }
    return count;
}

void RestoreStore::revokeAll() {
    bool hadDurable = false;
    for (const auto& e : std::as_const(m_entries)) {
        if (e.durable) {
            hadDurable = true;
            break;
        }
    }
    m_entries.clear();
    if (hadDurable) {
        save();
    }
}

QList<RestoreStore::Listing> RestoreStore::listEntries() const {
    QList<Listing> list;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        list.append({ it.key(), it.value() });
    }
    return list;
}

} // namespace wormhole::portal
