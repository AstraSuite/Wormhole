#include "request.hpp"

namespace wormhole::portal {

PortalRequest::PortalRequest(const QString& path, QObject* parent)
    : QObject(parent), m_path(path) {
    QDBusConnection::sessionBus().registerObject(m_path, this, QDBusConnection::ExportAllSlots);
}

PortalRequest::~PortalRequest() {
    QDBusConnection::sessionBus().unregisterObject(m_path);
}

void PortalRequest::Close() {
    emit closeRequested();
}

} // namespace wormhole::portal
