#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QProcess>
#include <QSettings>
#include <QSurfaceFormat>
#include <QUrl>
#include <iostream>

#include "config/colours.hpp"
#include "config/tokens.hpp"
#include "core/appcontroller.hpp"
#include "core/iconprovider.hpp"
#include "core/fileutils.hpp"
#include "core/placesmodel.hpp"
#include "core/drivemanager.hpp"
#include "core/thumbnailprovider.hpp"
#include "screencast/previewprovider.hpp"
#include "portal/portaldaemon.hpp"
#include "portal/filechooserportal.hpp"
#include "screencast/pipewirestream.hpp"
#include "screencast/sourcesmodel.hpp"

namespace {
void setupIconTheme() {
    // 1. Ensure all standard Freedesktop icon search directories are registered
    QStringList searchPaths = QIcon::themeSearchPaths();
    const QString home = QDir::homePath();
    const QStringList extraPaths = {
        home + QStringLiteral("/.local/share/icons"),
        home + QStringLiteral("/.icons"),
        QStringLiteral("/usr/local/share/icons"),
        QStringLiteral("/usr/share/icons")
    };
    for (const QString& p : extraPaths) {
        if (QDir(p).exists() && !searchPaths.contains(p)) {
            searchPaths.prepend(p);
        }
    }
    QIcon::setThemeSearchPaths(searchPaths);

    // 2. Detect configured GTK/system icon theme if Qt did not auto-discover one
    QString detectedTheme;
    const QString gtk3Config = home + QStringLiteral("/.config/gtk-3.0/settings.ini");
    if (QFile::exists(gtk3Config)) {
        QSettings gtkSettings(gtk3Config, QSettings::IniFormat);
        detectedTheme = gtkSettings.value(QStringLiteral("Settings/gtk-icon-theme-name")).toString();
    }

    if (detectedTheme.isEmpty()) {
        const QString xsettingsConfig = home + QStringLiteral("/.config/xsettingsd/xsettingsd.conf");
        if (QFile::exists(xsettingsConfig)) {
            QFile file(xsettingsConfig);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                while (!file.atEnd()) {
                    QString line = QString::fromUtf8(file.readLine()).trimmed();
                    if (line.startsWith(QLatin1String("Net/IconThemeName"))) {
                        int startQuote = line.indexOf('"');
                        int endQuote = line.lastIndexOf('"');
                        if (startQuote != -1 && endQuote > startQuote) {
                            detectedTheme = line.mid(startQuote + 1, endQuote - startQuote - 1);
                            break;
                        }
                    }
                }
            }
        }
    }

    // 3. Apply detected theme or search for top installed system themes
    if (!detectedTheme.isEmpty()) {
        QIcon::setThemeName(detectedTheme);
    } else if (QIcon::themeName().isEmpty() || QIcon::themeName() == QLatin1String("hicolor")) {
        const QStringList candidateThemes = {
            QStringLiteral("Papirus-Dark"),
            QStringLiteral("Papirus"),
            QStringLiteral("breeze-dark"),
            QStringLiteral("breeze"),
            QStringLiteral("Adwaita"),
            QStringLiteral("elementary")
        };
        for (const QString& candidate : candidateThemes) {
            if (QIcon::hasThemeIcon(QStringLiteral("folder-download")) ||
                QFile::exists(QStringLiteral("/usr/share/icons/") + candidate) ||
                QFile::exists(home + QStringLiteral("/.local/share/icons/") + candidate)) {
                QIcon::setThemeName(candidate);
                break;
            }
        }
    }
}
} // namespace

int main(int argc, char* argv[]) {
    qputenv("QT_NO_XDG_DESKTOP_PORTAL", "1");

    // Check if running in headless daemon mode
    bool isDaemonMode = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--daemon") == 0 || strcmp(argv[i], "-d") == 0) {
            isDaemonMode = true;
            break;
        }
    }

    if (isDaemonMode) {
        QGuiApplication app(argc, argv);
        app.setApplicationName("wormhole");
        app.setApplicationDisplayName("Wormhole Portal Daemon");
        app.setOrganizationName("astra-wormhole");
        app.setApplicationVersion("1.0.0");

        setupIconTheme();

        QFontDatabase::addApplicationFont(":/qt/qml/wormhole/assets/fonts/GoogleSansFlex.ttf");
        QFontDatabase::addApplicationFont(":/qt/qml/wormhole/assets/fonts/MaterialSymbolsRounded.ttf");

        auto* controller = wormhole::core::AppController::instance();
        auto* colours = new wormhole::config::ColoursSingleton(&app);
        auto* tokens = wormhole::config::TokensSingleton::instance();

        QQmlApplicationEngine engine;
        engine.addImageProvider(QStringLiteral("icon"), new wormhole::core::IconImageProvider());
        engine.addImageProvider(QStringLiteral("monitor"), new wormhole::screencast::ScreenPreviewProvider());
        engine.addImageProvider(QStringLiteral("thumb"), new wormhole::core::ThumbnailImageProvider());

        auto* fileUtils = new wormhole::core::FileUtils(&app);
        auto* driveManager = new wormhole::core::DriveManager(&app);

        engine.rootContext()->setContextProperty(QStringLiteral("Colours"), colours);
        engine.rootContext()->setContextProperty(QStringLiteral("Tokens"), tokens);
        engine.rootContext()->setContextProperty(QStringLiteral("AppController"), controller);
        engine.rootContext()->setContextProperty(QStringLiteral("FileUtils"), fileUtils);
        engine.rootContext()->setContextProperty(QStringLiteral("DriveManager"), driveManager);

        engine.load(QUrl(QStringLiteral("qrc:/qt/qml/wormhole/qml/main.qml")));

        wormhole::screencast::PipeWireStreamManager::instance()->initialize();

        wormhole::portal::PortalDaemon daemon;
        if (!daemon.start()) {
            return 1;
        }

        return app.exec();
    }

    // Explicitly configure 32-bit RGBA surface format
    QSurfaceFormat format;
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setAlphaBufferSize(8);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);
    app.setApplicationName("wormhole");
    app.setApplicationDisplayName("Wormhole");
    app.setOrganizationName("astra-wormhole");
    app.setApplicationVersion("1.0.0");

    setupIconTheme();

    // Load fonts
    QFontDatabase::addApplicationFont(":/qt/qml/wormhole/assets/fonts/GoogleSansFlex.ttf");
    QFontDatabase::addApplicationFont(":/qt/qml/wormhole/assets/fonts/MaterialSymbolsRounded.ttf");

    QCommandLineParser parser;
    parser.setApplicationDescription("Wormhole: Material 3 Expressive XDG Desktop Portal");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption daemonOpt(QStringList{ "d", "daemon" }, "Run as D-Bus portal daemon");
    QCommandLineOption screencastOpt("screencast", "Open ScreenCast source chooser");
    QCommandLineOption screenshotOpt("screenshot", "Open Screenshot capture overlay");
    QCommandLineOption fileChooserOpt(QStringList{ "file-chooser", "filechooser" }, "Open embedded File Chooser dialog");
    QCommandLineOption pickColorOpt("pick-color", "Open Color Picker");
    QCommandLineOption appChooserOpt(QStringList{ "appchooser", "app-chooser" }, "Open Application Chooser");
    QCommandLineOption accessOpt("access", "Open Permission Access dialog");
    QCommandLineOption accountOpt("account", "Open User Account dialog");
    QCommandLineOption dynamicLauncherOpt("dynamic-launcher", "Open Dynamic Launcher installer");
    QCommandLineOption wallpaperOpt("wallpaper", "Open Wallpaper preview dialog");

    QCommandLineOption typesOpt(QStringList{ "types", "source-types" }, "Available source types bitmask", "types");
    QCommandLineOption titleOpt("title", "Dialog title", "title");
    QCommandLineOption appIdOpt("app-id", "Calling application ID", "appId");
    QCommandLineOption parentWinOpt("parent-window", "Parent window handle", "parent");
    QCommandLineOption cursorModeOpt("cursor-mode", "Cursor mode requested by the caller", "mode");
    QCommandLineOption persistModeOpt("persist-mode", "Persist mode requested by the caller", "mode");
    QCommandLineOption urlOpt("url", "Target URL / URI", "url");
    QCommandLineOption nameOpt("name", "Name for shortcut / launcher", "name");
    QCommandLineOption execOpt("exec", "Exec command for launcher", "exec");
    QCommandLineOption iconOpt("icon", "Icon name / path", "icon");
    QCommandLineOption subtitleOpt("subtitle", "Dialog subtitle", "subtitle");
    QCommandLineOption bodyOpt("body", "Dialog body explanation", "body");
    QCommandLineOption mimeOpt(QStringList{ "mime", "mime-type" }, "Target MIME type for app chooser", "mime");
    QCommandLineOption choicesOpt("choices", "Comma-separated list of application choices", "choices");
    QCommandLineOption interactiveOpt("interactive", "Interactive screenshot capture mode");
    QCommandLineOption delayOpt("delay", "Screenshot delay in seconds", "delay");
    QCommandLineOption multipleOpt("multiple", "Allow selecting multiple sources or files");
    QCommandLineOption directoryOpt("directory", "Select folder mode");
    QCommandLineOption saveOpt("save", "Save file mode");

    parser.addOption(daemonOpt);
    parser.addOption(screencastOpt);
    parser.addOption(screenshotOpt);
    parser.addOption(fileChooserOpt);
    parser.addOption(pickColorOpt);
    parser.addOption(appChooserOpt);
    parser.addOption(accessOpt);
    parser.addOption(accountOpt);
    parser.addOption(dynamicLauncherOpt);
    parser.addOption(wallpaperOpt);

    parser.addOption(typesOpt);
    parser.addOption(titleOpt);
    parser.addOption(appIdOpt);
    parser.addOption(parentWinOpt);
    parser.addOption(cursorModeOpt);
    parser.addOption(persistModeOpt);
    parser.addOption(urlOpt);
    parser.addOption(nameOpt);
    parser.addOption(execOpt);
    parser.addOption(iconOpt);
    parser.addOption(subtitleOpt);
    parser.addOption(bodyOpt);
    parser.addOption(mimeOpt);
    parser.addOption(choicesOpt);
    parser.addOption(interactiveOpt);
    parser.addOption(delayOpt);
    parser.addOption(multipleOpt);
    parser.addOption(directoryOpt);
    parser.addOption(saveOpt);

    parser.process(app);

    auto* controller = wormhole::core::AppController::instance();

    // Handle File Chooser directly via embedded picker
    if (parser.isSet(fileChooserOpt)) {
        controller->setDialogMode(wormhole::core::AppController::DialogMode::FileChooser);
        if (parser.isSet(titleOpt)) controller->setTitle(parser.value(titleOpt));
        if (parser.isSet(directoryOpt)) controller->setInitialDirectory(parser.value(directoryOpt));
        if (parser.isSet(multipleOpt)) controller->setSaveMode(false);
        if (parser.isSet(saveOpt)) controller->setSaveMode(true);
        if (parser.isSet(urlOpt)) controller->setInitialDirectory(parser.value(urlOpt));
    } else {
        if (parser.isSet(typesOpt)) controller->setAvailableSourceTypes(parser.value(typesOpt).toUInt());
        if (parser.isSet(multipleOpt)) controller->setMultipleSources(true);
        if (parser.isSet(titleOpt)) controller->setTitle(parser.value(titleOpt));
        if (parser.isSet(appIdOpt)) controller->setAppId(parser.value(appIdOpt));
        if (parser.isSet(parentWinOpt)) controller->setParentWindow(parser.value(parentWinOpt));
        if (parser.isSet(cursorModeOpt)) controller->setCursorMode(parser.value(cursorModeOpt).toUInt());
        if (parser.isSet(persistModeOpt)) controller->setAllowToken(parser.value(persistModeOpt).toUInt() != 0);
        if (parser.isSet(urlOpt)) {
            controller->setWallpaperUri(parser.value(urlOpt));
            controller->setLauncherUrl(parser.value(urlOpt));
            controller->setAppChooserUrl(parser.value(urlOpt));
        }
        if (parser.isSet(nameOpt)) controller->setLauncherName(parser.value(nameOpt));
        if (parser.isSet(execOpt)) controller->setLauncherExec(parser.value(execOpt));
        if (parser.isSet(iconOpt)) {
            controller->setAccessIcon(parser.value(iconOpt));
            controller->setLauncherIcon(parser.value(iconOpt));
        }
        if (parser.isSet(subtitleOpt)) controller->setAccessSubtitle(parser.value(subtitleOpt));
        if (parser.isSet(bodyOpt)) controller->setAccessBody(parser.value(bodyOpt));
        if (parser.isSet(mimeOpt)) controller->setAppChooserMime(parser.value(mimeOpt));
        if (parser.isSet(choicesOpt)) controller->setAppChooserChoices(parser.value(choicesOpt).split(QLatin1Char(','), Qt::SkipEmptyParts));
        if (parser.isSet(interactiveOpt)) controller->setIsScreenshotInteractive(true);
        if (parser.isSet(delayOpt)) controller->setScreenshotDelay(parser.value(delayOpt).toInt());

        if (parser.isSet(screencastOpt)) {
            controller->setDialogMode(wormhole::core::AppController::DialogMode::ScreenCast);
        } else if (parser.isSet(screenshotOpt)) {
            controller->setDialogMode(wormhole::core::AppController::DialogMode::Screenshot);
            controller->setIsPickColorMode(false);
        } else if (parser.isSet(pickColorOpt)) {
            controller->setDialogMode(wormhole::core::AppController::DialogMode::Screenshot);
            controller->setIsPickColorMode(true);
        } else if (parser.isSet(appChooserOpt)) {
            controller->setDialogMode(wormhole::core::AppController::DialogMode::AppChooser);
        } else if (parser.isSet(accessOpt)) {
            controller->setDialogMode(wormhole::core::AppController::DialogMode::Access);
        } else if (parser.isSet(accountOpt)) {
            controller->setDialogMode(wormhole::core::AppController::DialogMode::Account);
        } else if (parser.isSet(dynamicLauncherOpt)) {
            controller->setDialogMode(wormhole::core::AppController::DialogMode::DynamicLauncher);
        } else if (parser.isSet(wallpaperOpt)) {
            controller->setDialogMode(wormhole::core::AppController::DialogMode::Wallpaper);
        } else {
            // Default to ScreenCast picker
            controller->setDialogMode(wormhole::core::AppController::DialogMode::ScreenCast);
        }
    }

    // Connect stdout results
    QObject::connect(controller, &wormhole::core::AppController::accepted, [controller](const QVariantMap& results) {
        if (controller->dialogMode() == wormhole::core::AppController::DialogMode::Wallpaper) {
            QString uri = controller->wallpaperUri();
            QString localPath;
            if (uri.startsWith(QLatin1String("file://"))) {
                localPath = QUrl(uri).toLocalFile();
            } else {
                localPath = uri;
            }
            if (!localPath.isEmpty() && QFile::exists(localPath)) {
                QProcess::execute(QStringLiteral("caelestia"), QStringList{QStringLiteral("wallpaper"), QStringLiteral("-f"), localPath});
            }
        }

        if (controller->dialogMode() == wormhole::core::AppController::DialogMode::FileChooser) {
            // Print file URIs to stdout for CLI usage
            if (results.contains(QStringLiteral("uris"))) {
                const QStringList uris = results.value(QStringLiteral("uris")).toStringList();
                for (const QString& uri : uris) {
                    std::cout << uri.toStdString() << std::endl;
                }
            }
            QGuiApplication::exit(0);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromVariant(results);
        std::cout << doc.toJson(QJsonDocument::Compact).toStdString() << std::endl;
        QGuiApplication::exit(0);
    });

    QObject::connect(controller, &wormhole::core::AppController::rejected, []() {
        QGuiApplication::exit(1);
    });

    auto* colours = new wormhole::config::ColoursSingleton(&app);
    auto* tokens = wormhole::config::TokensSingleton::instance();

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icon"), new wormhole::core::IconImageProvider());
    engine.addImageProvider(QStringLiteral("monitor"), new wormhole::screencast::ScreenPreviewProvider());
    engine.addImageProvider(QStringLiteral("thumb"), new wormhole::core::ThumbnailImageProvider());

    auto* fileUtils = new wormhole::core::FileUtils(&app);
    auto* driveManager = new wormhole::core::DriveManager(&app);

    engine.rootContext()->setContextProperty(QStringLiteral("Colours"), colours);
    engine.rootContext()->setContextProperty(QStringLiteral("Tokens"), tokens);
    engine.rootContext()->setContextProperty(QStringLiteral("AppController"), controller);
    engine.rootContext()->setContextProperty(QStringLiteral("FileUtils"), fileUtils);
    engine.rootContext()->setContextProperty(QStringLiteral("DriveManager"), driveManager);

    const QUrl url(QStringLiteral("qrc:/qt/qml/wormhole/qml/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl)
                QGuiApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}