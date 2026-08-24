#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QJSEngine>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <qqmlintegration.h>

namespace wormhole::core {

class AppController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // General Dialog Mode
    Q_PROPERTY(DialogMode dialogMode READ dialogMode WRITE setDialogMode NOTIFY dialogModeChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString appId READ appId WRITE setAppId NOTIFY appIdChanged)
    Q_PROPERTY(QString parentWindow READ parentWindow WRITE setParentWindow NOTIFY parentWindowChanged)

    // ScreenCast Specific
    Q_PROPERTY(uint availableSourceTypes READ availableSourceTypes WRITE setAvailableSourceTypes NOTIFY availableSourceTypesChanged)
    Q_PROPERTY(uint availableCursorModes READ availableCursorModes WRITE setAvailableCursorModes NOTIFY availableCursorModesChanged)
    Q_PROPERTY(uint cursorMode READ cursorMode WRITE setCursorMode NOTIFY cursorModeChanged)
    Q_PROPERTY(bool allowToken READ allowToken WRITE setAllowToken NOTIFY allowTokenChanged)
    Q_PROPERTY(bool multipleSources READ multipleSources WRITE setMultipleSources NOTIFY multipleSourcesChanged)

    // Screenshot Specific
    Q_PROPERTY(bool isScreenshotInteractive READ isScreenshotInteractive WRITE setIsScreenshotInteractive NOTIFY isScreenshotInteractiveChanged)
    Q_PROPERTY(bool isPickColorMode READ isPickColorMode WRITE setIsPickColorMode NOTIFY isPickColorModeChanged)
    Q_PROPERTY(int screenshotDelay READ screenshotDelay WRITE setScreenshotDelay NOTIFY screenshotDelayChanged)

    // AppChooser Specific
    Q_PROPERTY(QString appChooserMime READ appChooserMime WRITE setAppChooserMime NOTIFY appChooserMimeChanged)
    Q_PROPERTY(QString appChooserUrl READ appChooserUrl WRITE setAppChooserUrl NOTIFY appChooserUrlChanged)
    Q_PROPERTY(QStringList appChooserChoices READ appChooserChoices WRITE setAppChooserChoices NOTIFY appChooserChoicesChanged)

    // Access Specific
    Q_PROPERTY(QString accessSubtitle READ accessSubtitle WRITE setAccessSubtitle NOTIFY accessSubtitleChanged)
    Q_PROPERTY(QString accessBody READ accessBody WRITE setAccessBody NOTIFY accessBodyChanged)
    Q_PROPERTY(QString accessIcon READ accessIcon WRITE setAccessIcon NOTIFY accessIconChanged)

    // DynamicLauncher Specific
    Q_PROPERTY(QString launcherName READ launcherName WRITE setLauncherName NOTIFY launcherNameChanged)
    Q_PROPERTY(QString launcherIcon READ launcherIcon WRITE setLauncherIcon NOTIFY launcherIconChanged)
    Q_PROPERTY(QString launcherExec READ launcherExec WRITE setLauncherExec NOTIFY launcherExecChanged)
    Q_PROPERTY(QString launcherUrl READ launcherUrl WRITE setLauncherUrl NOTIFY launcherUrlChanged)

    // Wallpaper Specific
    Q_PROPERTY(QString wallpaperUri READ wallpaperUri WRITE setWallpaperUri NOTIFY wallpaperUriChanged)
    Q_PROPERTY(QString wallpaperSetOn READ wallpaperSetOn WRITE setWallpaperSetOn NOTIFY wallpaperSetOnChanged)

public:
    enum class DialogMode {
        None,
        ScreenCast,
        Screenshot,
        AppChooser,
        Access,
        Account,
        DynamicLauncher,
        Wallpaper
    };
    Q_ENUM(DialogMode)

    static AppController* instance();
    static AppController* create(QQmlEngine* = nullptr, QJSEngine* = nullptr) {
        return instance();
    }

    DialogMode dialogMode() const { return m_dialogMode; }
    void setDialogMode(DialogMode mode);

    QString title() const { return m_title; }
    void setTitle(const QString& title);

    QString appId() const { return m_appId; }
    void setAppId(const QString& appId);

    QString parentWindow() const { return m_parentWindow; }
    void setParentWindow(const QString& win);

    uint availableSourceTypes() const { return m_availableSourceTypes; }
    void setAvailableSourceTypes(uint types);

    uint availableCursorModes() const { return m_availableCursorModes; }
    void setAvailableCursorModes(uint modes);

    uint cursorMode() const { return m_cursorMode; }
    void setCursorMode(uint mode);

    bool allowToken() const { return m_allowToken; }
    void setAllowToken(bool allow);

    bool multipleSources() const { return m_multipleSources; }
    void setMultipleSources(bool multiple);

    bool isScreenshotInteractive() const { return m_isScreenshotInteractive; }
    void setIsScreenshotInteractive(bool interactive);

    bool isPickColorMode() const { return m_isPickColorMode; }
    void setIsPickColorMode(bool pickColor);

    int screenshotDelay() const { return m_screenshotDelay; }
    void setScreenshotDelay(int delay);

    QString appChooserMime() const { return m_appChooserMime; }
    void setAppChooserMime(const QString& mime);

    QString appChooserUrl() const { return m_appChooserUrl; }
    void setAppChooserUrl(const QString& url);

    QStringList appChooserChoices() const { return m_appChooserChoices; }
    void setAppChooserChoices(const QStringList& choices);

    QString accessSubtitle() const { return m_accessSubtitle; }
    void setAccessSubtitle(const QString& sub);

    QString accessBody() const { return m_accessBody; }
    void setAccessBody(const QString& body);

    QString accessIcon() const { return m_accessIcon; }
    void setAccessIcon(const QString& icon);

    QString launcherName() const { return m_launcherName; }
    void setLauncherName(const QString& name);

    QString launcherIcon() const { return m_launcherIcon; }
    void setLauncherIcon(const QString& icon);

    QString launcherExec() const { return m_launcherExec; }
    void setLauncherExec(const QString& exec);

    QString launcherUrl() const { return m_launcherUrl; }
    void setLauncherUrl(const QString& url);

    QString wallpaperUri() const { return m_wallpaperUri; }
    void setWallpaperUri(const QString& uri);

    QString wallpaperSetOn() const { return m_wallpaperSetOn; }
    void setWallpaperSetOn(const QString& on);

    Q_INVOKABLE void accept(const QVariantMap& results = {});
    Q_INVOKABLE void reject();
    Q_INVOKABLE void quit();

signals:
    void dialogModeChanged();
    void titleChanged();
    void appIdChanged();
    void parentWindowChanged();
    void availableSourceTypesChanged();
    void availableCursorModesChanged();
    void cursorModeChanged();
    void allowTokenChanged();
    void multipleSourcesChanged();
    void isScreenshotInteractiveChanged();
    void isPickColorModeChanged();
    void screenshotDelayChanged();
    void appChooserMimeChanged();
    void appChooserUrlChanged();
    void appChooserChoicesChanged();
    void accessSubtitleChanged();
    void accessBodyChanged();
    void accessIconChanged();
    void launcherNameChanged();
    void launcherIconChanged();
    void launcherExecChanged();
    void launcherUrlChanged();
    void wallpaperUriChanged();
    void wallpaperSetOnChanged();

    void accepted(const QVariantMap& results);
    void rejected();

private:
    explicit AppController(QObject* parent = nullptr);

    DialogMode m_dialogMode = DialogMode::None;
    QString m_title;
    QString m_appId;
    QString m_parentWindow;

    uint m_availableSourceTypes = 7; // Monitor | Window | Virtual
    uint m_availableCursorModes = 7; // Hidden | Embedded | Metadata
    uint m_cursorMode = 1;
    bool m_allowToken = true;
    bool m_multipleSources = false;

    bool m_isScreenshotInteractive = true;
    bool m_isPickColorMode = false;
    int m_screenshotDelay = 0;

    QString m_appChooserMime;
    QString m_appChooserUrl;
    QStringList m_appChooserChoices;

    QString m_accessSubtitle;
    QString m_accessBody;
    QString m_accessIcon;

    QString m_launcherName;
    QString m_launcherIcon;
    QString m_launcherExec;
    QString m_launcherUrl;

    QString m_wallpaperUri;
    QString m_wallpaperSetOn = QStringLiteral("both");
};

} // namespace wormhole::core
