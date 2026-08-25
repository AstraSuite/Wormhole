#include "dynamiclauncherportal.hpp"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

namespace wormhole::portal {

DynamicLauncherPortal::DynamicLauncherPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void DynamicLauncherPortal::PrepareInstall(const QDBusObjectPath& handle,
                                          const QString& app_id,
                                          const QString& parent_window,
                                          const QString& name,
                                          const QDBusVariant& /*icon*/,
                                          const QVariantMap& options,
                                          const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString wormholeBin = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QStringLiteral("--dynamic-launcher");
    args << QStringLiteral("--app-id") << app_id;
    args << QStringLiteral("--name") << name;
    if (!parent_window.isEmpty()) {
        args << QStringLiteral("--parent-window") << parent_window;
    }
    QString url;
    if (options.contains(QStringLiteral("url"))) {
        url = options.value(QStringLiteral("url")).toString();
        args << QStringLiteral("--url") << url;
    }
    QString execName;
    if (options.contains(QStringLiteral("exec_name"))) {
        execName = options.value(QStringLiteral("exec_name")).toString();
        args << QStringLiteral("--exec") << execName;
    }
    QString iconName;
    if (options.contains(QStringLiteral("icon_name"))) {
        iconName = options.value(QStringLiteral("icon_name")).toString();
        args << QStringLiteral("--icon") << iconName;
    }

    auto* process = new QProcess(this);
    auto* reqObj = new PortalRequest(handle.path(), this);

    PendingRequest req;
    req.message = message;
    req.process = process;
    req.requestObject = reqObj;
    req.name = name;
    req.iconName = iconName;
    req.execName = execName;
    req.url = url;

    m_requests.insert(handle.path(), req);

    connect(reqObj, &PortalRequest::closeRequested, this, [this, handle]() {
        if (m_requests.contains(handle.path())) {
            auto r = m_requests.take(handle.path());
            if (r.process && r.process->state() != QProcess::NotRunning) {
                r.process->terminate();
            }
            if (r.requestObject) r.requestObject->deleteLater();
            if (r.process) r.process->deleteLater();

            QDBusMessage reply = r.message.createReply();
            reply << static_cast<uint>(1) << QVariantMap();
            QDBusConnection::sessionBus().send(reply);
        }
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, handle, name, app_id](int exitCode, QProcess::ExitStatus /*status*/) {
        if (!m_requests.contains(handle.path())) {
            return;
        }
        auto req = m_requests.take(handle.path());
        QByteArray stdoutData = req.process->readAllStandardOutput();

        if (req.requestObject) req.requestObject->deleteLater();
        if (req.process) req.process->deleteLater();

        QDBusMessage reply = req.message.createReply();
        QVariantMap results;

        if (exitCode == 0) {
            QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
            PreparedLauncher prep;
            prep.name = req.name;
            prep.iconName = req.iconName;
            prep.execName = req.execName;
            prep.url = req.url;
            prep.appId = app_id;
            m_prepared.insert(token, prep);

            results.insert(QStringLiteral("token"), token);
            results.insert(QStringLiteral("name"), name);
            reply << static_cast<uint>(0) << results;
        } else {
            reply << static_cast<uint>(1) << results;
        }

        QDBusConnection::sessionBus().send(reply);
    });

    process->start(wormholeBin, args);
}

uint DynamicLauncherPortal::Install(const QString& /*app_id*/,
                                   const QString& token,
                                   const QString& desktop_file_id,
                                   const QVariantMap& options) {
    QString fileName = desktop_file_id;
    if (!fileName.endsWith(QLatin1String(".desktop"))) {
        fileName += QLatin1String(".desktop");
    }

    QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(appsDir);
    QString path = appsDir + QLatin1Char('/') + fileName;

    PreparedLauncher prep;
    if (m_prepared.contains(token)) {
        prep = m_prepared.take(token);
    }

    QString name = options.value(QStringLiteral("name"), prep.name).toString();
    if (name.isEmpty()) name = fileName.chopped(8);

    QString icon = options.value(QStringLiteral("icon"), prep.iconName).toString();
    if (icon.isEmpty()) icon = QStringLiteral("application-x-executable");

    QString exec = options.value(QStringLiteral("exec"), prep.execName).toString();
    if (exec.isEmpty() && !prep.url.isEmpty()) {
        exec = QStringLiteral("xdg-open %1").arg(prep.url);
    } else if (exec.isEmpty()) {
        exec = fileName.chopped(8);
    }

    QString content = QStringLiteral(
        "[Desktop Entry]\n"
        "Version=1.0\n"
        "Type=Application\n"
        "Name=%1\n"
        "Exec=%2\n"
        "Icon=%3\n"
        "Terminal=false\n"
        "Categories=Network;Application;\n"
    ).arg(name, exec, icon);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return 1;
    }
    file.write(content.toUtf8());
    if (!file.commit()) {
        return 1;
    }

    QProcess::startDetached(QStringLiteral("update-desktop-database"), { appsDir });
    return 0;
}

uint DynamicLauncherPortal::Uninstall(const QString& /*app_id*/,
                                     const QString& desktop_file_id,
                                     const QVariantMap& /*options*/) {
    QString fileName = desktop_file_id;
    if (!fileName.endsWith(QLatin1String(".desktop"))) {
        fileName += QLatin1String(".desktop");
    }

    QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString path = appsDir + QLatin1Char('/') + fileName;
    if (QFile::exists(path)) {
        QFile::remove(path);
        QProcess::startDetached(QStringLiteral("update-desktop-database"), { appsDir });
    }
    return 0;
}

uint DynamicLauncherPortal::Launch(const QString& /*app_id*/,
                                  const QString& desktop_file_id,
                                  const QVariantMap& /*options*/) {
    QString fileName = desktop_file_id;
    if (!fileName.endsWith(QLatin1String(".desktop"))) {
        fileName += QLatin1String(".desktop");
    }

    QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString path = appsDir + QLatin1Char('/') + fileName;
    if (!QFile::exists(path)) {
        path = QStringLiteral("/usr/share/applications/") + fileName;
    }
    if (QFile::exists(path)) {
        QProcess::startDetached(QStringLiteral("gio"), { QStringLiteral("launch"), path });
        return 0;
    }
    return 1;
}

} // namespace wormhole::portal
