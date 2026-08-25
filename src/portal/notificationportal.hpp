#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace wormhole::portal {

class NotificationPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Notification")

public:
    explicit NotificationPortal(QObject* parent = nullptr);
    ~NotificationPortal() override = default;

public slots:
    Q_SCRIPTABLE void AddNotification(const QString& app_id,
                                      const QString& id,
                                      const QVariantMap& notification);

    Q_SCRIPTABLE void RemoveNotification(const QString& app_id,
                                         const QString& id);

signals:
    Q_SCRIPTABLE void ActionInvoked(const QString& app_id,
                                    const QString& id,
                                    const QString& action,
                                    const QVariantList& parameter);

private slots:
    void onFdoActionInvoked(uint id, const QString& actionKey);
    void onFdoNotificationClosed(uint id, uint reason);

private:
    QMap<QString, uint> m_portalToFdo;
    QMap<uint, QString> m_fdoToPortal;
};

} // namespace wormhole::portal
