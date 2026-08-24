#include "settingsportal.hpp"
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDebug>
#include <QDir>
#include <QFile>

namespace wormhole::portal {

SettingsPortal::SettingsPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

QVariant SettingsPortal::readKey(const QString& namesp, const QString& key) {
    if (namesp == QLatin1String("org.freedesktop.appearance")) {
        if (key == QLatin1String("color-scheme")) {
            // 1 = prefer dark, 2 = prefer light, 0 = no preference
            return QVariant::fromValue(static_cast<uint>(1));
        }
        if (key == QLatin1String("accent-color")) {
            QDBusArgument arg;
            arg.beginStructure();
            arg << 0.8 << 0.6 << 1.0;
            arg.endStructure();
            return QVariant::fromValue(arg);
        }
        if (key == QLatin1String("contrast")) {
            return QVariant::fromValue(static_cast<uint>(0));
        }
    }
    return {};
}

void SettingsPortal::Read(const QString& namesp,
                         const QString& key,
                         const QDBusMessage& message) {
    QVariant val = readKey(namesp, key);
    QDBusMessage reply = message.createReply();

    if (val.isValid()) {
        reply << static_cast<uint>(0) << QVariant::fromValue(QDBusVariant(val));
    } else {
        reply << static_cast<uint>(1) << QVariant::fromValue(QDBusVariant(QVariant()));
    }
    QDBusConnection::sessionBus().send(reply);
}

void SettingsPortal::ReadAll(const QStringList& namespaces,
                            const QDBusMessage& message) {
    QVariantMap result;

    QStringList nsList = namespaces;
    if (nsList.isEmpty()) {
        nsList << QStringLiteral("org.freedesktop.appearance");
    }

    for (const QString& ns : nsList) {
        if (ns == QLatin1String("org.freedesktop.appearance")) {
            QVariantMap appMap;
            appMap.insert(QStringLiteral("color-scheme"), QVariant::fromValue(QDBusVariant(static_cast<uint>(1))));
            appMap.insert(QStringLiteral("contrast"), QVariant::fromValue(QDBusVariant(static_cast<uint>(0))));
            result.insert(ns, appMap);
        }
    }

    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << result;
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
