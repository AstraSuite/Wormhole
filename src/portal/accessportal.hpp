#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QVariantMap>
#include "request.hpp"

namespace wormhole::portal {

class AccessPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Access")

public:
    explicit AccessPortal(QObject* parent = nullptr);
    ~AccessPortal() override = default;

public slots:
    Q_SCRIPTABLE void AccessDialog(const QDBusObjectPath& handle,
                                  const QString& app_id,
                                  const QString& parent_window,
                                  const QString& title,
                                  const QString& subtitle,
                                  const QString& body,
                                  const QVariantMap& options,
                                  const QDBusMessage& message);

private:
    struct PendingRequest {
        QDBusMessage message;
        QProcess* process = nullptr;
        PortalRequest* requestObject = nullptr;
    };

    QMap<QString, PendingRequest> m_requests;
};

} // namespace wormhole::portal
