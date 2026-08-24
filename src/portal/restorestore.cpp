#include "restorestore.hpp"

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
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        const QJsonObject obj = it.value().toObject();

        RestoreEntry entry;
        entry.appId = obj.value(QStringLiteral("app_id")).toString();
        entry.isWindow = obj.value(QStringLiteral("is_window")).toBool();
        entry.outputName = obj.value(QStringLiteral("output")).toString();
        entry.windowAppId = obj.value(QStringLiteral("window_app_id")).toString();
        entry.windowTitle = obj.value(QStringLiteral("window_title")).toString();
        entry.windowAddress = obj.value(QStringLiteral("window_address")).toString();
        entry.x = obj.value(QStringLiteral("x")).toInt();
        entry.y = obj.value(QStringLiteral("y")).toInt();
        entry.fps = obj.value(QStringLiteral("fps")).toInt(60);
        entry.durable = true;

        m_entries.insert(it.key(), entry);
    }
}

void RestoreStore::save() const {
    const QString path = filePath();
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
        obj.insert(QStringLiteral("x"), it.value().x);
        obj.insert(QStringLiteral("y"), it.value().y);
        obj.insert(QStringLiteral("fps"), it.value().fps);

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

QString RestoreStore::add(const RestoreEntry& entry) {
    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_entries.insert(token, entry);
    if (entry.durable) {
        save();
    }
    return token;
}

bool RestoreStore::take(const QString& token, const QString& appId, RestoreEntry& entry) const {
    if (token.isEmpty()) {
        return false;
    }

    const auto it = m_entries.constFind(token);
    if (it == m_entries.constEnd() || it.value().appId != appId) {
        return false;
    }

    entry = it.value();
    return true;
}

void RestoreStore::remove(const QString& token) {
    const auto it = m_entries.constFind(token);
    if (it == m_entries.constEnd()) {
        return;
    }

    const bool wasDurable = it.value().durable;
    m_entries.erase(it);
    if (wasDurable) {
        save();
    }
}

} // namespace wormhole::portal
