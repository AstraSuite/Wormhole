#include "portaldaemon.hpp"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDebug>
#include "appcontroller.hpp"

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

    const QString serviceName = QStringLiteral("org.freedesktop.impl.portal.Desktop.wormhole");
    if (!bus.registerService(serviceName)) {
        QDBusError error = bus.lastError();
        qCritical() << "Failed to register D-Bus service" << serviceName << ":" << error.message();
        return false;
    }

    const QString objectPath = QStringLiteral("/org/freedesktop/portal/desktop");

    m_access = new AccessPortal(this);
    m_account = new AccountPortal(this);
    m_appChooser = new AppChooserPortal(this);
    m_background = new BackgroundPortal(this);
    m_camera = new CameraPortal(this);
    m_clipboard = new ClipboardPortal(this);
    m_dynamicLauncher = new DynamicLauncherPortal(this);
    m_email = new EmailPortal(this);
    m_fileChooser = new FileChooserPortal(this);
    m_globalShortcuts = new GlobalShortcutsPortal(this);
    m_inhibit = new InhibitPortal(this);
    m_notifInhibit = new NotificationInhibitionPortal(this);
    m_inputCapture = new InputCapturePortal(this);
    m_location = new LocationPortal(this);
    m_lockdown = new LockdownPortal(this);
    m_notification = new NotificationPortal(this);
    m_print = new PrintPortal(this);
    m_realtime = new RealtimePortal(this);
    m_remoteDesktop = new RemoteDesktopPortal(this);
    m_screenCast = new ScreenCastPortal(this);
    m_screenshot = new ScreenshotPortal(this);
    m_secret = new SecretPortal(this);
    m_settings = new SettingsPortal(this);
    m_usb = new UsbPortal(this);
    m_wallpaper = new WallpaperPortal(this);

    if (!bus.registerObject(objectPath, this, QDBusConnection::ExportAdaptors)) {
        QDBusError error = bus.lastError();
        qCritical() << "Failed to register root portal object on" << objectPath << ":" << error.message();
        return false;
    }

    auto* ctrl = wormhole::core::AppController::instance();

    connect(m_fileChooser, &FileChooserPortal::openFileRequested,
            ctrl, [ctrl](const QString& handle, const QString& title,
                         const QStringList& filters, const QString& filterLabel,
                         bool directoryOnly, bool multiple, const QString& initialDir) {
        ctrl->setDialogMode(wormhole::core::AppController::DialogMode::FileChooser);
        ctrl->setPortalHandle(handle);
        ctrl->setTitle(title);
        ctrl->setFilters(filters);
        ctrl->setFilterLabel(filterLabel);
        ctrl->setDirectoryOnly(directoryOnly);
        ctrl->setInitialDirectory(initialDir);
        ctrl->setSaveMode(false);
    });

    connect(m_fileChooser, &FileChooserPortal::saveFileRequested,
            ctrl, [ctrl](const QString& handle, const QString& title,
                         const QStringList& filters, const QString& filterLabel,
                         const QString& suggestedName, const QString& initialDir) {
        ctrl->setDialogMode(wormhole::core::AppController::DialogMode::FileChooser);
        ctrl->setPortalHandle(handle);
        ctrl->setTitle(title);
        ctrl->setFilters(filters);
        ctrl->setFilterLabel(filterLabel);
        ctrl->setSuggestedName(suggestedName);
        ctrl->setInitialDirectory(initialDir);
        ctrl->setSaveMode(true);
    });

    connect(m_fileChooser, &FileChooserPortal::saveFilesRequested,
            ctrl, [ctrl](const QString& handle, const QString& title,
                         const QStringList& fileList, const QString& initialDir) {
        Q_UNUSED(fileList)
        ctrl->setDialogMode(wormhole::core::AppController::DialogMode::FileChooser);
        ctrl->setPortalHandle(handle);
        ctrl->setTitle(title);
        ctrl->setInitialDirectory(initialDir);
        ctrl->setDirectoryOnly(true);
        ctrl->setSaveMode(true);
    });

    connect(ctrl, &wormhole::core::AppController::portalRequestFinished,
            m_fileChooser, &FileChooserPortal::finishRequest);

    connect(m_fileChooser, &FileChooserPortal::openFileRequested,
            this, [](const QString& handle) {
        qDebug() << "openFileRequested received in PortalDaemon: handle=" << handle;
    });

    qInfo() << "Wormhole portal daemon registered successfully on" << serviceName << "at" << objectPath;
    return true;
}

} // namespace wormhole::portal
