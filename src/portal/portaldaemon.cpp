#include "portaldaemon.hpp"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDebug>

namespace wormhole::portal {

PortalDaemon::PortalDaemon(QObject* parent)
    : QObject(parent) {
}

bool PortalDaemon::start() {
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCritical() << "Cannot connect to the D-Bus session bus.";
        return false;
    }

    const QString serviceName = QStringLiteral("org.freedesktop.impl.portal.desktop.wormhole");
    auto* iface = bus.interface();
    if (iface) {
        auto reply = iface->registerService(serviceName,
                                            QDBusConnectionInterface::ReplaceExistingService,
                                            QDBusConnectionInterface::AllowReplacement);
        if (reply != QDBusConnectionInterface::ServiceRegistered &&
            reply != QDBusConnectionInterface::ServiceQueued) {
            qCritical() << "Failed to register D-Bus service" << serviceName;
            return false;
        }
    } else if (!bus.registerService(serviceName)) {
        QDBusError error = bus.lastError();
        qCritical() << "Failed to register D-Bus service" << serviceName << ":" << error.message();
        return false;
    }

    const QString objectPath = QStringLiteral("/org/freedesktop/portal/desktop");

    m_fileChooser = new FileChooserPortal(this);
    m_screenCast = new ScreenCastPortal(this);
    m_screenshot = new ScreenshotPortal(this);
    m_appChooser = new AppChooserPortal(this);
    m_access = new AccessPortal(this);
    m_account = new AccountPortal(this);
    m_dynamicLauncher = new DynamicLauncherPortal(this);
    m_wallpaper = new WallpaperPortal(this);
    m_inhibit = new InhibitPortal(this);
    m_notifInhibit = new NotificationInhibitionPortal(this);
    m_settings = new SettingsPortal(this);

    if (!bus.registerObject(objectPath, this, QDBusConnection::ExportAdaptors)) {
        QDBusError error = bus.lastError();
        qCritical() << "Failed to register root portal object on" << objectPath << ":" << error.message();
        return false;
    }

    qInfo() << "Wormhole portal daemon registered successfully on" << serviceName << "at" << objectPath;
    return true;
}

} // namespace wormhole::portal
