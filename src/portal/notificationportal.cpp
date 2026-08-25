#include "notificationportal.hpp"
#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

namespace wormhole::portal {

NotificationPortal::NotificationPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("ActionInvoked"),
        this,
        SLOT(onFdoActionInvoked(uint, QString))
    );

    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("NotificationClosed"),
        this,
        SLOT(onFdoNotificationClosed(uint, uint))
    );
}

void NotificationPortal::AddNotification(const QString& app_id,
                                        const QString& id,
                                        const QVariantMap& notification) {
    QString title = notification.value(QStringLiteral("title")).toString();
    QString body = notification.value(QStringLiteral("body")).toString();
    QString icon = notification.value(QStringLiteral("icon")).toString();
    QString priority = notification.value(QStringLiteral("priority"), QStringLiteral("normal")).toString();

    QStringList actions;
    if (notification.contains(QStringLiteral("default-action"))) {
        actions << QStringLiteral("default") << QStringLiteral("Open");
    }

    if (notification.contains(QStringLiteral("buttons"))) {
        const QDBusArgument arg = notification.value(QStringLiteral("buttons")).value<QDBusArgument>();
        arg.beginArray();
        while (!arg.atEnd()) {
            QVariantMap btn;
            arg >> btn;
            QString action = btn.value(QStringLiteral("action")).toString();
            QString label = btn.value(QStringLiteral("label")).toString();
            if (!action.isEmpty()) {
                actions << action << (label.isEmpty() ? action : label);
            }
        }
        arg.endArray();
    }

    QVariantMap hints;
    hints.insert(QStringLiteral("desktop-entry"), app_id);
    if (priority == QLatin1String("urgent") || priority == QLatin1String("high")) {
        hints.insert(QStringLiteral("urgency"), static_cast<uchar>(2));
    } else if (priority == QLatin1String("low")) {
        hints.insert(QStringLiteral("urgency"), static_cast<uchar>(0));
    } else {
        hints.insert(QStringLiteral("urgency"), static_cast<uchar>(1));
    }

    uint replacesId = m_portalToFdo.value(id, 0);

    QDBusInterface fdoNotif(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QDBusConnection::sessionBus()
    );

    QDBusReply<uint> reply = fdoNotif.call(
        QStringLiteral("Notify"),
        app_id,
        replacesId,
        icon,
        title,
        body,
        actions,
        hints,
        -1
    );

    if (reply.isValid()) {
        uint fdoId = reply.value();
        m_portalToFdo.insert(id, fdoId);
        m_fdoToPortal.insert(fdoId, id);
    }
}

void NotificationPortal::RemoveNotification(const QString& /*app_id*/,
                                           const QString& id) {
    if (m_portalToFdo.contains(id)) {
        uint fdoId = m_portalToFdo.take(id);
        m_fdoToPortal.remove(fdoId);

        QDBusInterface fdoNotif(
            QStringLiteral("org.freedesktop.Notifications"),
            QStringLiteral("/org/freedesktop/Notifications"),
            QStringLiteral("org.freedesktop.Notifications"),
            QDBusConnection::sessionBus()
        );
        fdoNotif.call(QStringLiteral("CloseNotification"), fdoId);
    }
}

void NotificationPortal::onFdoActionInvoked(uint id, const QString& actionKey) {
    if (m_fdoToPortal.contains(id)) {
        QString portalId = m_fdoToPortal.value(id);
        emit ActionInvoked(QString(), portalId, actionKey, QVariantList());
    }
}

void NotificationPortal::onFdoNotificationClosed(uint id, uint /*reason*/) {
    if (m_fdoToPortal.contains(id)) {
        QString portalId = m_fdoToPortal.take(id);
        m_portalToFdo.remove(portalId);
    }
}

} // namespace wormhole::portal
