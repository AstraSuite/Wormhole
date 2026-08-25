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

class AccountPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Account")

public:
    explicit AccountPortal(QObject* parent = nullptr);
    ~AccountPortal() override = default;

public slots:
    Q_SCRIPTABLE void GetUserInformation(const QDBusObjectPath& handle,
                                         const QString& app_id,
                                         const QString& parent_window,
                                         const QVariantMap& options,
                                         const QDBusMessage& message);

private:
    struct PendingRequest {
        QDBusMessage message;
        QProcess* process = nullptr;
        PortalRequest* requestObject = nullptr;
        QString appId;
    };

    QMap<QString, PendingRequest> m_requests;
};

} // namespace wormhole::portal
