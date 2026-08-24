#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
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

class DynamicLauncherPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.DynamicLauncher")

public:
    explicit DynamicLauncherPortal(QObject* parent = nullptr);
    ~DynamicLauncherPortal() override = default;

public slots:
    Q_SCRIPTABLE void PrepareInstall(const QDBusObjectPath& handle,
                                     const QString& app_id,
                                     const QString& parent_window,
                                     const QString& name,
                                     const QDBusVariant& icon,
                                     const QVariantMap& options,
                                     const QDBusMessage& message);

    Q_SCRIPTABLE uint Install(const QString& app_id,
                              const QString& token,
                              const QString& desktop_file_id,
                              const QVariantMap& options);

    Q_SCRIPTABLE uint Uninstall(const QString& app_id,
                                const QString& desktop_file_id,
                                const QVariantMap& options);

    Q_SCRIPTABLE uint Launch(const QString& app_id,
                             const QString& desktop_file_id,
                             const QVariantMap& options);

private:
    struct PendingRequest {
        QDBusMessage message;
        QProcess* process = nullptr;
        PortalRequest* requestObject = nullptr;
    };

    QMap<QString, PendingRequest> m_requests;
};

} // namespace wormhole::portal
