#include "settingsportal.hpp"
#include <QColor>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>

#pragma push_macro("signals")
#undef signals
#include <glib.h>
#include <gio/gio.h>
#pragma pop_macro("signals")

namespace wormhole::portal {

SettingsPortal::SettingsPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
    m_gsettings = g_settings_new("org.gnome.desktop.interface");
    if (m_gsettings) {
        g_signal_connect(m_gsettings, "changed::color-scheme",
                         G_CALLBACK(onGSettingsChanged), this);
    }
}

SettingsPortal::~SettingsPortal() {
    if (m_gsettings) {
        g_signal_handlers_disconnect_by_data(m_gsettings, this);
        g_object_unref(m_gsettings);
    }
}

void SettingsPortal::onGSettingsChanged(GSettings* /*settings*/, const gchar* /*key*/, gpointer user_data) {
    auto* self = static_cast<SettingsPortal*>(user_data);
    const QString ns = QStringLiteral("org.freedesktop.appearance");

    QVariantMap values;
    values.insert(QStringLiteral("color-scheme"), self->readKey(ns, QStringLiteral("color-scheme")));
    values.insert(QStringLiteral("accent-color"), self->readKey(ns, QStringLiteral("accent-color")));
    values.insert(QStringLiteral("contrast"), self->readKey(ns, QStringLiteral("contrast")));

    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        emit self->SettingChanged(ns, it.key(), QDBusVariant(it.value()));
    }
}

QVariant SettingsPortal::readKey(const QString& namesp, const QString& key) {
    if (namesp == QLatin1String("org.freedesktop.appearance")) {
        if (key == QLatin1String("color-scheme")) {
            // 0 = no preference, 1 = prefer dark, 2 = prefer light
            if (m_gsettings) {
                gchar* scheme = g_settings_get_string(m_gsettings, "color-scheme");
                QString value = QString::fromUtf8(scheme);
                g_free(scheme);

                if (value == QLatin1String("prefer-light")) {
                    return QVariant::fromValue(static_cast<uint>(2));
                }
                if (value == QLatin1String("prefer-dark")) {
                    return QVariant::fromValue(static_cast<uint>(1));
                }
                // "default" or unknown — fall back to dark
                return QVariant::fromValue(static_cast<uint>(1));
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
