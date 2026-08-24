#include "hyprlandipc.hpp"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QProcess>
#include <QStandardPaths>

namespace wormhole::screencast {

HyprlandIPC::HyprlandIPC(QObject* parent)
    : QObject(parent) {
}

bool HyprlandIPC::isHyprlandRunning() {
    return qEnvironmentVariableIsSet("HYPRLAND_INSTANCE_SIGNATURE");
}

QByteArray HyprlandIPC::requestSocket(const QString& command) {
    const QString his = qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE");
    if (his.isEmpty()) {
        // Fallback to hyprctl command line
        QProcess proc;
        proc.start(QStringLiteral("hyprctl"), { QStringLiteral("-j"), command.startsWith(QLatin1String("j/")) ? command.mid(2) : command });
        if (proc.waitForFinished(1000)) {
            return proc.readAllStandardOutput();
        }
        return {};
    }

    QString socketPath;
    const QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR", QStringLiteral("/tmp"));
    QString p1 = QStringLiteral("%1/hypr/%2/.socket.sock").arg(runtimeDir, his);
    QString p2 = QStringLiteral("/tmp/hypr/%1/.socket.sock").arg(his);

    if (QFile::exists(p1)) {
        socketPath = p1;
    } else if (QFile::exists(p2)) {
        socketPath = p2;
    } else {
        socketPath = p1;
    }

    QLocalSocket socket;
    socket.connectToServer(socketPath);
    if (!socket.waitForConnected(500)) {
        // Fallback to hyprctl command line
        QProcess proc;
        proc.start(QStringLiteral("hyprctl"), { QStringLiteral("-j"), command.startsWith(QLatin1String("j/")) ? command.mid(2) : command });
        if (proc.waitForFinished(1000)) {
            return proc.readAllStandardOutput();
        }
        return {};
    }

    socket.write(command.toUtf8());
    socket.flush();

    QByteArray response;
    while (socket.waitForReadyRead(500)) {
        response.append(socket.readAll());
    }
    return response;
}

QList<HyprlandMonitor> HyprlandIPC::getMonitors() {
    QList<HyprlandMonitor> list;
    QByteArray data = requestSocket(QStringLiteral("j/monitors"));
    if (data.isEmpty()) {
        return list;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return list;
    }

    const QJsonArray arr = doc.array();
    for (const QJsonValue& val : arr) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        HyprlandMonitor m;
        m.id = obj.value(QStringLiteral("id")).toInt();
        m.name = obj.value(QStringLiteral("name")).toString();
        m.description = obj.value(QStringLiteral("description")).toString();
        m.make = obj.value(QStringLiteral("make")).toString();
        m.model = obj.value(QStringLiteral("model")).toString();
        m.width = obj.value(QStringLiteral("width")).toInt();
        m.height = obj.value(QStringLiteral("height")).toInt();
        m.refreshRate = obj.value(QStringLiteral("refreshRate")).toDouble(60.0);
        m.x = obj.value(QStringLiteral("x")).toInt();
        m.y = obj.value(QStringLiteral("y")).toInt();
        m.scale = obj.value(QStringLiteral("scale")).toDouble(1.0);
        m.transform = obj.value(QStringLiteral("transform")).toInt(0);
        m.focused = obj.value(QStringLiteral("focused")).toBool();

        QJsonObject ws = obj.value(QStringLiteral("activeWorkspace")).toObject();
        m.activeWorkspaceId = ws.value(QStringLiteral("id")).toInt(1);
        m.activeWorkspaceName = ws.value(QStringLiteral("name")).toString(QStringLiteral("1"));

        list.append(m);
    }
    return list;
}

QList<HyprlandWindow> HyprlandIPC::getClients() {
    QList<HyprlandWindow> list;
    QByteArray data = requestSocket(QStringLiteral("j/clients"));
    if (data.isEmpty()) {
        return list;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return list;
    }

    const QJsonArray arr = doc.array();
    for (const QJsonValue& val : arr) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        HyprlandWindow w;
        w.address = obj.value(QStringLiteral("address")).toString();
        w.title = obj.value(QStringLiteral("title")).toString();
        w.className = obj.value(QStringLiteral("class")).toString();
        w.initialTitle = obj.value(QStringLiteral("initialTitle")).toString();
        w.initialClass = obj.value(QStringLiteral("initialClass")).toString();
        w.pid = obj.value(QStringLiteral("pid")).toInt();
        w.monitorId = obj.value(QStringLiteral("monitor")).toInt();
        w.fullscreen = obj.value(QStringLiteral("fullscreen")).toInt() != 0;
        w.floating = obj.value(QStringLiteral("floating")).toBool();

        QJsonArray atArr = obj.value(QStringLiteral("at")).toArray();
        if (atArr.size() >= 2) {
            w.x = atArr.at(0).toInt();
            w.y = atArr.at(1).toInt();
        }
        QJsonArray sizeArr = obj.value(QStringLiteral("size")).toArray();
        if (sizeArr.size() >= 2) {
            w.width = sizeArr.at(0).toInt();
            w.height = sizeArr.at(1).toInt();
        }

        QJsonObject ws = obj.value(QStringLiteral("workspace")).toObject();
        w.workspaceId = ws.value(QStringLiteral("id")).toInt(1);
        w.workspaceName = ws.value(QStringLiteral("name")).toString(QStringLiteral("1"));

        // Ignore empty windows or helper popups
        if (w.title.isEmpty() && w.className.isEmpty()) {
            continue;
        }

        list.append(w);
    }
    return list;
}

void HyprlandIPC::focusMonitor(const QString& name) {
    if (name.isEmpty()) return;
    requestSocket(QStringLiteral("/dispatch focusmonitor ") + name);
}

void HyprlandIPC::focusWindow(const QString& address) {
    if (address.isEmpty()) return;
    requestSocket(QStringLiteral("/dispatch focuswindow address:") + address);
}

QString HyprlandIPC::getAppIconName(const QString& className, const QString& /*title*/) {
    QString c = className.toLower();
    if (c.contains(QLatin1String("discord")) || c.contains(QLatin1String("equibop")) || c.contains(QLatin1String("vesktop")) || c.contains(QLatin1String("webcord"))) {
        return QStringLiteral("discord");
    }
    if (c.contains(QLatin1String("spotify"))) {
        return QStringLiteral("spotify");
    }
    if (c.contains(QLatin1String("chrome")) || c.contains(QLatin1String("chromium"))) {
        return QStringLiteral("google-chrome");
    }
    if (c.contains(QLatin1String("firefox"))) {
        return QStringLiteral("firefox");
    }
    if (c.contains(QLatin1String("zen"))) {
        return QStringLiteral("zen-browser");
    }
    if (c.contains(QLatin1String("kitty"))) {
        return QStringLiteral("kitty");
    }
    if (c.contains(QLatin1String("foot"))) {
        return QStringLiteral("foot");
    }
    if (c.contains(QLatin1String("alacritty"))) {
        return QStringLiteral("Alacritty");
    }
    if (c.contains(QLatin1String("code")) || c.contains(QLatin1String("vscode"))) {
        return QStringLiteral("visual-studio-code");
    }
    if (c.contains(QLatin1String("obs"))) {
        return QStringLiteral("com.obsproject.Studio");
    }
    if (c.contains(QLatin1String("steam"))) {
        return QStringLiteral("steam");
    }
    if (c.contains(QLatin1String("telegram"))) {
        return QStringLiteral("telegram");
    }
    if (c.contains(QLatin1String("antigravity"))) {
        return QStringLiteral("antigravity");
    }
    if (c.contains(QLatin1String("atlas")) || c.contains(QLatin1String("prism"))) {
        return QStringLiteral("atlas");
    }
    return className;
}

} // namespace wormhole::screencast
