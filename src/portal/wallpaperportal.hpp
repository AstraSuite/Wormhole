#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QVariantMap>
#include "request.hpp"

namespace wormhole::portal {

class WallpaperPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Wallpaper")

public:
    explicit WallpaperPortal(QObject* parent = nullptr);
    ~WallpaperPortal() override = default;

public slots:
    Q_SCRIPTABLE void SetWallpaperURI(const QDBusObjectPath& handle,
                                     const QString& app_id,
                                     const QString& parent_window,
                                     const QString& uri,
                                     const QVariantMap& options,
                                     const QDBusMessage& message);

    Q_SCRIPTABLE void SetWallpaperFile(const QDBusObjectPath& handle,
                                      const QString& app_id,
                                      const QString& parent_window,
                                      const QDBusUnixFileDescriptor& fd,
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
