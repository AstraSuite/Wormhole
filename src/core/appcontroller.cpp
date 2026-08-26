#include "appcontroller.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include "screencast/waylandcapture.hpp"

namespace wormhole::core {

AppController* AppController::instance() {
    static auto* inst = new AppController(qApp);
    return inst;
}

AppController::AppController(QObject* parent)
    : QObject(parent) {
}

void AppController::setDialogMode(DialogMode mode) {
    if (m_dialogMode != mode) {
        m_dialogMode = mode;
        emit dialogModeChanged();
    }
}

void AppController::setTitle(const QString& title) {
    if (m_title != title) {
        m_title = title;
        emit titleChanged();
    }
}

void AppController::setAppId(const QString& appId) {
    if (m_appId != appId) {
        m_appId = appId;
        emit appIdChanged();
    }
}

void AppController::setParentWindow(const QString& win) {
    if (m_parentWindow != win) {
        m_parentWindow = win;
        emit parentWindowChanged();
    }
}

void AppController::setAvailableSourceTypes(uint types) {
    if (m_availableSourceTypes != types) {
        m_availableSourceTypes = types;
        emit availableSourceTypesChanged();
    }
}

void AppController::setAvailableCursorModes(uint modes) {
    if (m_availableCursorModes != modes) {
        m_availableCursorModes = modes;
        emit availableCursorModesChanged();
    }
}

void AppController::setCursorMode(uint mode) {
    if (m_cursorMode != mode) {
        m_cursorMode = mode;
        emit cursorModeChanged();
    }
}

void AppController::setAllowToken(bool allow) {
    if (m_allowToken != allow) {
        m_allowToken = allow;
        emit allowTokenChanged();
    }
}

void AppController::setMultipleSources(bool multiple) {
    if (m_multipleSources != multiple) {
        m_multipleSources = multiple;
        emit multipleSourcesChanged();
    }
}

void AppController::setIsScreenshotInteractive(bool interactive) {
    if (m_isScreenshotInteractive != interactive) {
        m_isScreenshotInteractive = interactive;
        emit isScreenshotInteractiveChanged();
    }
}

void AppController::setIsPickColorMode(bool pickColor) {
    if (m_isPickColorMode != pickColor) {
        m_isPickColorMode = pickColor;
        emit isPickColorModeChanged();
    }
}

void AppController::setScreenshotDelay(int delay) {
    if (m_screenshotDelay != delay) {
        m_screenshotDelay = delay;
        emit screenshotDelayChanged();
    }
}

void AppController::setAppChooserMime(const QString& mime) {
    if (m_appChooserMime != mime) {
        m_appChooserMime = mime;
        emit appChooserMimeChanged();
    }
}

void AppController::setAppChooserUrl(const QString& url) {
    if (m_appChooserUrl != url) {
        m_appChooserUrl = url;
        emit appChooserUrlChanged();
    }
}

void AppController::setAppChooserChoices(const QStringList& choices) {
    if (m_appChooserChoices != choices) {
        m_appChooserChoices = choices;
        emit appChooserChoicesChanged();
    }
}

void AppController::setAccessSubtitle(const QString& sub) {
    if (m_accessSubtitle != sub) {
        m_accessSubtitle = sub;
        emit accessSubtitleChanged();
    }
}

void AppController::setAccessBody(const QString& body) {
    if (m_accessBody != body) {
        m_accessBody = body;
        emit accessBodyChanged();
    }
}

void AppController::setAccessIcon(const QString& icon) {
    if (m_accessIcon != icon) {
        m_accessIcon = icon;
        emit accessIconChanged();
    }
}

void AppController::setLauncherName(const QString& name) {
    if (m_launcherName != name) {
        m_launcherName = name;
        emit launcherNameChanged();
    }
}

void AppController::setLauncherIcon(const QString& icon) {
    if (m_launcherIcon != icon) {
        m_launcherIcon = icon;
        emit launcherIconChanged();
    }
}

void AppController::setLauncherExec(const QString& exec) {
    if (m_launcherExec != exec) {
        m_launcherExec = exec;
        emit launcherExecChanged();
    }
}

void AppController::setLauncherUrl(const QString& url) {
    if (m_launcherUrl != url) {
        m_launcherUrl = url;
        emit launcherUrlChanged();
    }
}

void AppController::setWallpaperUri(const QString& uri) {
    if (m_wallpaperUri != uri) {
        m_wallpaperUri = uri;
        emit wallpaperUriChanged();
    }
}

void AppController::setWallpaperSetOn(const QString& on) {
    if (m_wallpaperSetOn != on) {
        m_wallpaperSetOn = on;
        emit wallpaperSetOnChanged();
    }
}

void AppController::setInitialDirectory(const QString& dir) {
    if (m_initialDirectory != dir) {
        m_initialDirectory = dir;
        emit initialDirectoryChanged();
    }
}

void AppController::setFilterLabel(const QString& label) {
    if (m_filterLabel != label) {
        m_filterLabel = label;
        emit filterLabelChanged();
    }
}

void AppController::setFilters(const QStringList& filters) {
    if (m_filters != filters) {
        m_filters = filters;
        emit filtersChanged();
    }
}

void AppController::setDirectoryOnly(bool dirOnly) {
    if (m_directoryOnly != dirOnly) {
        m_directoryOnly = dirOnly;
        emit directoryOnlyChanged();
    }
}

void AppController::setSaveMode(bool save) {
    if (m_saveMode != save) {
        m_saveMode = save;
        emit saveModeChanged();
    }
}

void AppController::setSuggestedName(const QString& name) {
    if (m_suggestedName != name) {
        m_suggestedName = name;
        emit suggestedNameChanged();
    }
}

void AppController::setShowHidden(bool show) {
    if (m_showHidden != show) {
        m_showHidden = show;
        emit showHiddenChanged();
    }
}

void AppController::setPlacesIconSize(int size) {
    if (m_placesIconSize != size && size > 0) {
        m_placesIconSize = size;
        emit placesIconSizeChanged();
    }
}

void AppController::setSingleClick(bool single) {
    if (m_singleClick != single) {
        m_singleClick = single;
        emit singleClickChanged();
    }
}

void AppController::setCaseSensitiveSort(bool sensitive) {
    if (m_caseSensitiveSort != sensitive) {
        m_caseSensitiveSort = sensitive;
        emit caseSensitiveSortChanged();
    }
}

void AppController::setShowDirsFirst(bool dirsFirst) {
    if (m_showDirsFirst != dirsFirst) {
        m_showDirsFirst = dirsFirst;
        emit showDirsFirstChanged();
    }
}

bool AppController::fileExists(const QString& path) {
    return QFileInfo::exists(path);
}

void AppController::accept(const QVariantMap& results) {
    emit accepted(results);
}

void AppController::reject() {
    emit rejected();
}

void AppController::quit() {
    QCoreApplication::quit();
}

void AppController::captureScreen() {
    screencast::WaylandCapture capture;
    m_screenCapture = capture.grabOutput(QString(), false, 2000);

    if (m_screenCapture.isNull()) {
        // Fallback: capture via grim to a temporary buffer
        QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/wormhole_tmp_capture.png");
        QDir().mkpath(QFileInfo(tmpPath).absolutePath());
        if (QProcess::execute(QStringLiteral("grim"), { tmpPath }) == 0) {
            m_screenCapture.load(tmpPath);
            QFile::remove(tmpPath);
        }
    }
}

QVariantList AppController::pickColorAt(int x, int y) {
    if (m_screenCapture.isNull()) {
        captureScreen();
    }

    if (m_screenCapture.isNull() || x < 0 || y < 0 || x >= m_screenCapture.width() || y >= m_screenCapture.height()) {
        // Fallback default
        return { 0.815, 0.745, 0.957 };
    }

    QColor color = m_screenCapture.pixelColor(x, y);
    return { color.redF(), color.greenF(), color.blueF() };
}

QString AppController::saveScreenshotRegion(int x, int y, int width, int height) {
    QString picsDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QStringLiteral("/Screenshots");
    QDir().mkpath(picsDir);
    QString filePath = picsDir + QStringLiteral("/Screenshot_%1.png").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));

    if (m_screenCapture.isNull()) {
        captureScreen();
    }

    if (!m_screenCapture.isNull()) {
        QRect cropRect = QRect(x, y, width, height).intersected(m_screenCapture.rect());
        if (cropRect.isEmpty() || width <= 0 || height <= 0) {
            cropRect = m_screenCapture.rect();
        }
        QImage cropped = m_screenCapture.copy(cropRect);
        if (cropped.save(filePath, "PNG")) {
            return QUrl::fromLocalFile(filePath).toString();
        }
    }

    // Try grim with geometry
    QString geom = QStringLiteral("%1,%2 %3x%4").arg(x).arg(y).arg(width).arg(height);
    if (QProcess::execute(QStringLiteral("grim"), { QStringLiteral("-g"), geom, filePath }) == 0 && QFile::exists(filePath)) {
        return QUrl::fromLocalFile(filePath).toString();
    }

    // Fallback grim fullscreen
    if (QProcess::execute(QStringLiteral("grim"), { filePath }) == 0 && QFile::exists(filePath)) {
        return QUrl::fromLocalFile(filePath).toString();
    }

    // Last resort dummy image
    QImage dummy(qMax(100, width), qMax(100, height), QImage::Format_ARGB32_Premultiplied);
    dummy.fill(QColor(30, 30, 30));
    dummy.save(filePath, "PNG");
    return QUrl::fromLocalFile(filePath).toString();
}

QString AppController::saveFullscreen() {
    QString picsDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QStringLiteral("/Screenshots");
    QDir().mkpath(picsDir);
    QString filePath = picsDir + QStringLiteral("/Screenshot_%1.png").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));

    if (m_screenCapture.isNull()) {
        captureScreen();
    }

    if (!m_screenCapture.isNull()) {
        if (m_screenCapture.save(filePath, "PNG")) {
            return QUrl::fromLocalFile(filePath).toString();
        }
    }

    // Try grim directly
    if (QProcess::execute(QStringLiteral("grim"), { filePath }) == 0 && QFile::exists(filePath)) {
        return QUrl::fromLocalFile(filePath).toString();
    }

    // Last resort
    QImage dummy(1920, 1080, QImage::Format_ARGB32_Premultiplied);
    dummy.fill(QColor(30, 30, 30));
    dummy.save(filePath, "PNG");
    return QUrl::fromLocalFile(filePath).toString();
}

} // namespace wormhole::core
