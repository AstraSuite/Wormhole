#include "session.hpp"

namespace wormhole::portal {

PortalSession::PortalSession(const QString& path, const QString& appId, QObject* parent)
    : QObject(parent), m_path(path), m_appId(appId) {
    QDBusConnection::sessionBus().registerObject(m_path, this, QDBusConnection::ExportAllSlots);
}

PortalSession::~PortalSession() {
    QDBusConnection::sessionBus().unregisterObject(m_path);
}

void PortalSession::Close() {
    emit closed();
}

} // namespace wormhole::portal
