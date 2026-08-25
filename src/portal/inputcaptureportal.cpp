#include "inputcaptureportal.hpp"
#include <QDebug>

namespace wormhole::portal {

InputCapturePortal::InputCapturePortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

uint InputCapturePortal::CreateSession(const QDBusObjectPath& /*handle*/,
                                      const QDBusObjectPath& session_handle,
                                      const QString& app_id,
                                      const QVariantMap& options,
                                      QVariantMap& results) {
    SessionData data;
    data.appId = app_id;
    data.capabilities = options.value(QStringLiteral("capabilities"), Keyboard | Pointer).toUInt();
    m_sessions.insert(session_handle.path(), data);

    auto* sessionObj = new PortalSession(session_handle.path(), app_id, this);
    connect(sessionObj, &PortalSession::closed, this, [this, session_handle]() {
        m_sessions.remove(session_handle.path());
    });

    results.insert(QStringLiteral("capabilities"), data.capabilities);
    return 0;
}

uint InputCapturePortal::GetZones(const QDBusObjectPath& session_handle,
                                 const QVariantMap& /*options*/,
                                 QVariantMap& results) {
    if (!m_sessions.contains(session_handle.path())) {
        return 2;
    }

    results.insert(QStringLiteral("zones"), QVariantList());
    results.insert(QStringLiteral("zone_set"), static_cast<uint>(1));
    return 0;
}

uint InputCapturePortal::SetPointerBarriers(const QDBusObjectPath& session_handle,
                                           const QVariantMap& /*options*/,
                                           const QVariantList& /*barriers*/,
                                           uint /*zone_set*/,
                                           QVariantMap& results) {
    if (!m_sessions.contains(session_handle.path())) {
        return 2;
    }

    results.insert(QStringLiteral("failed_barriers"), QVariantList());
    return 0;
}

uint InputCapturePortal::Enable(const QDBusObjectPath& session_handle,
                               const QVariantMap& /*options*/,
                               QVariantMap& /*results*/) {
    if (!m_sessions.contains(session_handle.path())) {
        return 2;
    }

    m_sessions[session_handle.path()].enabled = true;
    return 0;
}

uint InputCapturePortal::Disable(const QDBusObjectPath& session_handle,
                                const QVariantMap& /*options*/,
                                QVariantMap& /*results*/) {
    if (!m_sessions.contains(session_handle.path())) {
        return 2;
    }

    m_sessions[session_handle.path()].enabled = false;
    return 0;
}

uint InputCapturePortal::Release(const QDBusObjectPath& session_handle,
                                const QVariantMap& /*options*/,
                                QVariantMap& /*results*/) {
    if (!m_sessions.contains(session_handle.path())) {
        return 2;
    }

    return 0;
}

} // namespace wormhole::portal
