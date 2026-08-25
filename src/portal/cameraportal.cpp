#include "cameraportal.hpp"
#include <QDebug>
#include <unistd.h>

namespace wormhole::portal {

CameraPortal::CameraPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void CameraPortal::AccessCamera(const QDBusObjectPath& /*handle*/,
                                const QString& /*app_id*/,
                                const QString& /*parent_window*/,
                                const QVariantMap& /*options*/,
                                const QDBusMessage& message) {
    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

void CameraPortal::OpenPipeWireRemote(const QDBusObjectPath& /*handle*/,
                                     const QString& /*app_id*/,
                                     const QVariantMap& /*options*/,
                                     const QDBusMessage& message) {
    int fd = screencast::PipeWireStreamManager::instance()->getRemoteFd();
    if (fd < 0) {
        QDBusMessage reply = message.createReply();
        reply << static_cast<uint>(1) << QVariant::fromValue(QDBusUnixFileDescriptor()) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
        return;
    }

    QDBusUnixFileDescriptor unixFd;
    unixFd.setFileDescriptor(fd);
    close(fd);

    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariant::fromValue(unixFd) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
