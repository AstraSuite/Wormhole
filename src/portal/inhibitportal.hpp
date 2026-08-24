#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include "request.hpp"

namespace wormhole::portal {

class InhibitPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Inhibit")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit InhibitPortal(QObject* parent = nullptr);
    ~InhibitPortal() override = default;

    uint version() const { return 3; }

public slots:
    Q_SCRIPTABLE void Inhibit(const QDBusObjectPath& handle,
                             const QString& app_id,
                             const QString& parent_window,
                             const QString& reason,
                             uint flags,
                             const QVariantMap& options,
                             const QDBusMessage& message);

    Q_SCRIPTABLE void CreateMonitor(const QDBusObjectPath& handle,
                                   const QString& app_id,
                                   const QDBusObjectPath& session_handle,
                                   const QVariantMap& options,
                                   const QDBusMessage& message);

    Q_SCRIPTABLE void QueryEndResponse(const QDBusObjectPath& handle,
                                      const QVariantMap& options);
};

class NotificationInhibitionPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.NotificationInhibition")

public:
    explicit NotificationInhibitionPortal(QObject* parent = nullptr);
    ~NotificationInhibitionPortal() override = default;

public slots:
    Q_SCRIPTABLE void InhibitNotifications(const QDBusObjectPath& handle,
                                          const QString& app_id,
                                          const QString& reason,
                                          const QVariantMap& options,
                                          const QDBusMessage& message);

    Q_SCRIPTABLE void UninhibitNotifications(const QDBusObjectPath& handle,
                                            const QString& app_id,
                                            const QDBusMessage& message);
};

} // namespace wormhole::portal
