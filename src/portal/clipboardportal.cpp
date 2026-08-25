#include "clipboardportal.hpp"
#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>

namespace wormhole::portal {

ClipboardPortal::ClipboardPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

uint ClipboardPortal::RequestClipboard(const QDBusObjectPath& /*handle*/,
                                      const QDBusObjectPath& session_handle,
                                      const QString& app_id,
                                      const QVariantMap& /*options*/,
                                      QVariantMap& /*results*/) {
    auto* sessionObj = new PortalSession(session_handle.path(), app_id, this);
    Q_UNUSED(sessionObj);
    return 0;
}

uint ClipboardPortal::SetSelection(const QDBusObjectPath& /*session_handle*/,
                                  const QVariantMap& /*options*/,
                                  const QVariantMap& /*mime_types*/,
                                  QVariantMap& /*results*/) {
    return 0;
}

} // namespace wormhole::portal
