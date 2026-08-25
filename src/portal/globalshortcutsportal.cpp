#include "globalshortcutsportal.hpp"
#include <QDBusMetaType>
#include <QDebug>

namespace wormhole::portal {

GlobalShortcutsPortal::GlobalShortcutsPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
    qDBusRegisterMetaType<GlobalShortcutItem>();
    qDBusRegisterMetaType<GlobalShortcutList>();
}

uint GlobalShortcutsPortal::CreateSession(const QDBusObjectPath& /*handle*/,
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

void GlobalShortcutsPortal::BindShortcuts(const QDBusObjectPath& /*handle*/,
                                         const QDBusObjectPath& session_handle,
                                         const wormhole::portal::GlobalShortcutList& shortcuts,
                                         const QString& /*parent_window*/,
                                         const QVariantMap& /*options*/,
                                         const QDBusMessage& message) {
    if (!m_sessions.contains(session_handle.path())) {
        QDBusMessage reply = message.createReply();
        reply << static_cast<uint>(2) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
        return;
    }

    m_sessions[session_handle.path()].shortcuts = shortcuts;

    QVariantMap results;
    results.insert(QStringLiteral("shortcuts"), QVariant::fromValue(shortcuts));

    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << results;
    QDBusConnection::sessionBus().send(reply);
}

uint GlobalShortcutsPortal::ListShortcuts(const QDBusObjectPath& /*handle*/,
                                        const QDBusObjectPath& session_handle,
                                        QVariantMap& results) {
    if (!m_sessions.contains(session_handle.path())) {
        return 2;
    }

    const auto& items = m_sessions.value(session_handle.path()).shortcuts;
    results.insert(QStringLiteral("shortcuts"), QVariant::fromValue(items));
    return 0;
}

} // namespace wormhole::portal
