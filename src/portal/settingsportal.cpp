#include "settingsportal.hpp"
#include <QColor>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>

namespace wormhole::portal {

SettingsPortal::SettingsPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

QVariant SettingsPortal::readKey(const QString& namesp, const QString& key) {
    if (namesp == QLatin1String("org.freedesktop.appearance")) {
        if (key == QLatin1String("color-scheme")) {
            // 0 = no preference, 1 = prefer dark, 2 = prefer light
            QString scheme = qEnvironmentVariable("DARK_MODE");
            if (scheme == QLatin1String("0") || scheme == QLatin1String("false") || scheme == QLatin1String("light")) {
                return QVariant::fromValue(static_cast<uint>(2));
            }
            return QVariant::fromValue(static_cast<uint>(1));
        }
        if (key == QLatin1String("accent-color")) {
            // Material 3 Primary Accent
            QDBusArgument arg;
            arg.beginStructure();
            arg << 0.815 << 0.745 << 0.957; // M3 Iris / Lavender accent
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
    if (!val.isValid()) {
        QDBusMessage err = message.createErrorReply(
            QStringLiteral("org.freedesktop.portal.Error.NotFound"),
            QStringLiteral("Setting %1.%2 not found").arg(namesp, key));
        QDBusConnection::sessionBus().send(err);
        return;
    }

    QDBusMessage reply = message.createReply();
    reply << QVariant::fromValue(QDBusVariant(val));
    QDBusConnection::sessionBus().send(reply);
}

void SettingsPortal::ReadAll(const QStringList& namespaces,
                            const QDBusMessage& message) {
    QMap<QString, QVariantMap> result;

    bool handleAppearance = namespaces.isEmpty();
    for (const QString& rawNs : namespaces) {
        if (rawNs.contains(QLatin1String("org.freedesktop.appearance"))) {
            handleAppearance = true;
            break;
        }
    }

    if (handleAppearance) {
        const QString ns = QStringLiteral("org.freedesktop.appearance");
        QVariantMap appMap;
        appMap.insert(QStringLiteral("color-scheme"), QVariant::fromValue(QDBusVariant(readKey(ns, QStringLiteral("color-scheme")))));
        appMap.insert(QStringLiteral("accent-color"), QVariant::fromValue(QDBusVariant(readKey(ns, QStringLiteral("accent-color")))));
        appMap.insert(QStringLiteral("contrast"), QVariant::fromValue(QDBusVariant(readKey(ns, QStringLiteral("contrast")))));
        result.insert(ns, appMap);
    }

    QDBusMessage reply = message.createReply();
    reply << QVariant::fromValue(result);
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
