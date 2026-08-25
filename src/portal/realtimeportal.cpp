#include "realtimeportal.hpp"
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

namespace wormhole::portal {

RealtimePortal::RealtimePortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

uint RealtimePortal::MakeThreadRealtimeWithPID(qulonglong process, qulonglong thread, uint priority) {
    QDBusInterface rtkit(
        QStringLiteral("org.freedesktop.RealtimeKit1"),
        QStringLiteral("/org/freedesktop/RealtimeKit1"),
        QStringLiteral("org.freedesktop.RealtimeKit1"),
        QDBusConnection::systemBus()
    );

    if (rtkit.isValid()) {
        QDBusReply<void> reply = rtkit.call(QStringLiteral("MakeThreadRealtimeWithPID"), process, thread, priority);
        if (reply.isValid()) {
            return 0;
        }
    }
    return 1;
}

uint RealtimePortal::MakeThreadHighPriorityWithPID(qulonglong process, qulonglong thread, int priority) {
    QDBusInterface rtkit(
        QStringLiteral("org.freedesktop.RealtimeKit1"),
        QStringLiteral("/org/freedesktop/RealtimeKit1"),
        QStringLiteral("org.freedesktop.RealtimeKit1"),
        QDBusConnection::systemBus()
    );

    if (rtkit.isValid()) {
        QDBusReply<void> reply = rtkit.call(QStringLiteral("MakeThreadHighPriorityWithPID"), process, thread, priority);
        if (reply.isValid()) {
            return 0;
        }
    }
    return 1;
}

} // namespace wormhole::portal
