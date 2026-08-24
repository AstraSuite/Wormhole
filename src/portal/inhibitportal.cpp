#include "inhibitportal.hpp"
#include <QDebug>

namespace wormhole::portal {

InhibitPortal::InhibitPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

NotificationInhibitionPortal::NotificationInhibitionPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void InhibitPortal::Inhibit(const QDBusObjectPath& /*handle*/,
                           const QString& /*app_id*/,
                           const QString& /*parent_window*/,
                           const QString& /*reason*/,
                           uint /*flags*/,
                           const QVariantMap& /*options*/,
                           const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

void InhibitPortal::CreateMonitor(const QDBusObjectPath& /*handle*/,
                                 const QString& /*app_id*/,
                                 const QDBusObjectPath& /*session_handle*/,
                                 const QVariantMap& /*options*/,
                                 const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

void InhibitPortal::QueryEndResponse(const QDBusObjectPath& /*handle*/,
                                    const QVariantMap& /*options*/) {
}

void NotificationInhibitionPortal::InhibitNotifications(const QDBusObjectPath& /*handle*/,
                                                      const QString& /*app_id*/,
                                                      const QString& /*reason*/,
                                                      const QVariantMap& /*options*/,
                                                      const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

void NotificationInhibitionPortal::UninhibitNotifications(const QDBusObjectPath& /*handle*/,
                                                        const QString& /*app_id*/,
                                                        const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
