#pragma once

#include <QObject>
#include <QDBusConnection>
#include "filechooserportal.hpp"
#include "screencastportal.hpp"
#include "screenshotportal.hpp"
#include "appchooserportal.hpp"
#include "accessportal.hpp"
#include "accountportal.hpp"
#include "dynamiclauncherportal.hpp"
#include "wallpaperportal.hpp"
#include "inhibitportal.hpp"
#include "settingsportal.hpp"

namespace wormhole::portal {

class PortalDaemon : public QObject {
    Q_OBJECT

public:
    explicit PortalDaemon(QObject* parent = nullptr);
    ~PortalDaemon() override = default;

    bool start();

private:
    FileChooserPortal* m_fileChooser = nullptr;
    ScreenCastPortal* m_screenCast = nullptr;
    ScreenshotPortal* m_screenshot = nullptr;
    AppChooserPortal* m_appChooser = nullptr;
    AccessPortal* m_access = nullptr;
    AccountPortal* m_account = nullptr;
    DynamicLauncherPortal* m_dynamicLauncher = nullptr;
    WallpaperPortal* m_wallpaper = nullptr;
    InhibitPortal* m_inhibit = nullptr;
    NotificationInhibitionPortal* m_notifInhibit = nullptr;
    SettingsPortal* m_settings = nullptr;
};

} // namespace wormhole::portal
