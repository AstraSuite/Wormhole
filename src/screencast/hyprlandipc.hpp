#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

namespace wormhole::screencast {

struct HyprlandMonitor {
    int id = 0;
    QString name;
    QString description;
    QString make;
    QString model;
    int width = 1920;
    int height = 1080;
    double refreshRate = 60.0;
    int x = 0;
    int y = 0;
    double scale = 1.0;
    int transform = 0; // 0: 0 deg, 1: 90 deg, 2: 180 deg, 3: 270 deg
    bool focused = false;
    int activeWorkspaceId = 1;
    QString activeWorkspaceName = QStringLiteral("1");

    int effectiveWidth() const {
        double s = (scale > 0.0) ? scale : 1.0;
        return (transform % 2 == 1) ? static_cast<int>(height / s) : static_cast<int>(width / s);
    }

    int effectiveHeight() const {
        double s = (scale > 0.0) ? scale : 1.0;
        return (transform % 2 == 1) ? static_cast<int>(width / s) : static_cast<int>(height / s);
    }

    int rotationDegrees() const {
        switch (transform) {
            case 1: return 90;
            case 2: return 180;
            case 3: return 270;
            default: return 0;
        }
    }
};

struct HyprlandWindow {
    QString address;
    QString title;
    QString className;
    QString initialTitle;
    QString initialClass;
    int pid = 0;
    int workspaceId = 1;
    QString workspaceName = QStringLiteral("1");
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int monitorId = 0;
    bool fullscreen = false;
    bool floating = false;
};

class HyprlandIPC : public QObject {
    Q_OBJECT

public:
    explicit HyprlandIPC(QObject* parent = nullptr);

    static bool isHyprlandRunning();
    static QByteArray requestSocket(const QString& command);

    QList<HyprlandMonitor> getMonitors();
    QList<HyprlandWindow> getClients();

    static void focusMonitor(const QString& name);
    static void focusWindow(const QString& address);

    static QString getAppIconName(const QString& className, const QString& title);
};

} // namespace wormhole::screencast
