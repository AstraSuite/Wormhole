#include "locationportal.hpp"
#include <QDebug>

namespace wormhole::portal {

LocationPortal::LocationPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

uint LocationPortal::CreateSession(const QDBusObjectPath& /*handle*/,
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

uint LocationPortal::SelectAccuracy(const QDBusObjectPath& /*handle*/,
                                   const QDBusObjectPath& session_handle,
                                   const QString& /*app_id*/,
                                   const QVariantMap& options,
                                   QVariantMap& /*results*/) {
    if (!m_sessions.contains(session_handle.path())) {
        return 2;
    }

    m_sessions[session_handle.path()].accuracy = options.value(QStringLiteral("accuracy"), 0).toUInt();
    return 0;
}

void LocationPortal::Start(const QDBusObjectPath& /*handle*/,
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

    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
