#pragma once

#include <QObject>
#include <QDBusConnection>
#include "accessportal.hpp"
#include "accountportal.hpp"
#include "appchooserportal.hpp"
#include "backgroundportal.hpp"
#include "cameraportal.hpp"
#include "clipboardportal.hpp"
#include "dynamiclauncherportal.hpp"
#include "emailportal.hpp"
#include "filechooserportal.hpp"
#include "globalshortcutsportal.hpp"
#include "inhibitportal.hpp"
#include "inputcaptureportal.hpp"
#include "locationportal.hpp"
#include "lockdownportal.hpp"
#include "notificationportal.hpp"
#include "printportal.hpp"
#include "realtimeportal.hpp"
#include "remotedesktopportal.hpp"
#include "screencastportal.hpp"
#include "screenshotportal.hpp"
#include "secretportal.hpp"
#include "settingsportal.hpp"
#include "usbportal.hpp"
#include "wallpaperportal.hpp"

namespace wormhole::portal {

class PortalDaemon : public QObject {
    Q_OBJECT

public:
    explicit PortalDaemon(QObject* parent = nullptr);
    ~PortalDaemon() override = default;

    bool start();

private:
    AccessPortal* m_access = nullptr;
    AccountPortal* m_account = nullptr;
    AppChooserPortal* m_appChooser = nullptr;
    BackgroundPortal* m_background = nullptr;
    CameraPortal* m_camera = nullptr;
    ClipboardPortal* m_clipboard = nullptr;
    DynamicLauncherPortal* m_dynamicLauncher = nullptr;
    EmailPortal* m_email = nullptr;
    FileChooserPortal* m_fileChooser = nullptr;
    GlobalShortcutsPortal* m_globalShortcuts = nullptr;
    InhibitPortal* m_inhibit = nullptr;
    NotificationInhibitionPortal* m_notifInhibit = nullptr;
    InputCapturePortal* m_inputCapture = nullptr;
    LocationPortal* m_location = nullptr;
    LockdownPortal* m_lockdown = nullptr;
    NotificationPortal* m_notification = nullptr;
    PrintPortal* m_print = nullptr;
    RealtimePortal* m_realtime = nullptr;
    RemoteDesktopPortal* m_remoteDesktop = nullptr;
    ScreenCastPortal* m_screenCast = nullptr;
    ScreenshotPortal* m_screenshot = nullptr;
    SecretPortal* m_secret = nullptr;
    SettingsPortal* m_settings = nullptr;
    UsbPortal* m_usb = nullptr;
    WallpaperPortal* m_wallpaper = nullptr;
};

} // namespace wormhole::portal
