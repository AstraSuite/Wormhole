#include "backgroundportal.hpp"
#include <QDebug>

namespace wormhole::portal {

BackgroundPortal::BackgroundPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void BackgroundPortal::GetAppState(const QDBusMessage& message) {
    QVariantMap states;
    QDBusMessage reply = message.createReply();
    reply << states;
    QDBusConnection::sessionBus().send(reply);
}

void BackgroundPortal::NotifyBackground(const QDBusObjectPath& /*handle*/,
                                       const QString& /*app_id*/,
                                       const QString& /*name*/,
                                       const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

void BackgroundPortal::SetStatus(const QDBusObjectPath& /*handle*/,
                                const QString& /*app_id*/,
                                const QVariantMap& /*options*/,
                                const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
