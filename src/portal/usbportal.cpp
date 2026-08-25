#include "usbportal.hpp"
#include <QDebug>

namespace wormhole::portal {

UsbPortal::UsbPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void UsbPortal::ClaimDevice(const QDBusObjectPath& /*handle*/,
                            const QString& /*app_id*/,
                            const QString& /*parent_window*/,
                            const QVariantMap& /*options*/,
                            const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

void UsbPortal::OpenDevice(const QDBusObjectPath& /*handle*/,
                           const QString& /*app_id*/,
                           const QString& /*parent_window*/,
                           const QVariantMap& /*options*/,
                           const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(1) << QVariant::fromValue(QDBusUnixFileDescriptor()) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

void UsbPortal::ReleaseDevice(const QDBusObjectPath& /*handle*/,
                             const QString& /*app_id*/,
                             const QVariantMap& /*options*/,
                             const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
