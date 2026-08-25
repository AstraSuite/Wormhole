#include "remotedesktopportal.hpp"
#include <QDebug>

namespace wormhole::portal {

RemoteDesktopPortal::RemoteDesktopPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

uint RemoteDesktopPortal::CreateSession(const QDBusObjectPath& /*handle*/,
                                       const QDBusObjectPath& session_handle,
                                       const QString& app_id,
                                       const QVariantMap& /*options*/,
                                       QVariantMap& /*results*/) {
    SessionData data;
    data.appId = app_id;
    m_sessions.insert(session_handle.path(), data);

    auto* sessionObj = new PortalSession(session_handle.path(), app_id, this);
    connect(sessionObj, &PortalSession::closed, this, [this, session_handle]() {
        m_sessions.remove(session_handle.path());
    });

    return 0;
}

uint RemoteDesktopPortal::SelectDevices(const QDBusObjectPath& /*handle*/,
                                       const QDBusObjectPath& session_handle,
                                       const QString& /*app_id*/,
                                       const QVariantMap& options,
                                       QVariantMap& /*results*/) {
    if (!m_sessions.contains(session_handle.path())) {
        return 2;
    }

    uint types = options.value(QStringLiteral("types"), Keyboard | Pointer).toUInt();
    m_sessions[session_handle.path()].devices = types;
    return 0;
}

void RemoteDesktopPortal::Start(const QDBusObjectPath& /*handle*/,
                               const QDBusObjectPath& session_handle,
                               const QString& /*app_id*/,
                               const QString& /*parent_window*/,
                               const QVariantMap& /*options*/,
                               const QDBusMessage& message) {
    if (!m_sessions.contains(session_handle.path())) {
        QDBusMessage reply = message.createReply();
        reply << static_cast<uint>(2) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
        return;
    }

    QVariantMap results;
    results.insert(QStringLiteral("devices"), m_sessions.value(session_handle.path()).devices);

    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << results;
    QDBusConnection::sessionBus().send(reply);
}

void RemoteDesktopPortal::NotifyPointerMotion(const QDBusObjectPath& /*session_handle*/,
                                             const QVariantMap& /*options*/,
                                             double /*dx*/,
                                             double /*dy*/) {
}

void RemoteDesktopPortal::NotifyPointerMotionAbsolute(const QDBusObjectPath& /*session_handle*/,
                                                     const QVariantMap& /*options*/,
                                                     uint /*stream*/,
                                                     double /*x*/,
                                                     double /*y*/) {
}

void RemoteDesktopPortal::NotifyPointerButton(const QDBusObjectPath& /*session_handle*/,
                                             const QVariantMap& /*options*/,
                                             int /*button*/,
                                             uint /*state*/) {
}

void RemoteDesktopPortal::NotifyPointerAxis(const QDBusObjectPath& /*session_handle*/,
                                           const QVariantMap& /*options*/,
                                           double /*dx*/,
                                           double /*dy*/) {
}

void RemoteDesktopPortal::NotifyPointerAxisDiscrete(const QDBusObjectPath& /*session_handle*/,
                                                   const QVariantMap& /*options*/,
                                                   uint /*axis*/,
                                                   int /*steps*/) {
}

void RemoteDesktopPortal::NotifyKeyboardKeycode(const QDBusObjectPath& /*session_handle*/,
                                               const QVariantMap& /*options*/,
                                               int /*keycode*/,
                                               uint /*state*/) {
}

void RemoteDesktopPortal::NotifyKeyboardKeysym(const QDBusObjectPath& /*session_handle*/,
                                              const QVariantMap& /*options*/,
                                              int /*keysym*/,
                                              uint /*state*/) {
}

void RemoteDesktopPortal::NotifyTouchDown(const QDBusObjectPath& /*session_handle*/,
                                         const QVariantMap& /*options*/,
                                         uint /*stream*/,
                                         uint /*slot*/,
                                         double /*x*/,
                                         double /*y*/) {
}

void RemoteDesktopPortal::NotifyTouchMotion(const QDBusObjectPath& /*session_handle*/,
                                           const QVariantMap& /*options*/,
                                           uint /*stream*/,
                                           uint /*slot*/,
                                           double /*x*/,
                                           double /*y*/) {
}

void RemoteDesktopPortal::NotifyTouchUp(const QDBusObjectPath& /*session_handle*/,
                                       const QVariantMap& /*options*/,
                                       uint /*slot*/) {
}

} // namespace wormhole::portal
