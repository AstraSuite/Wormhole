#include "dynamiclauncherportal.hpp"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
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
    if (options.contains(QStringLiteral("url"))) {
        args << QStringLiteral("--url") << options.value(QStringLiteral("url")).toString();
    }
    if (options.contains(QStringLiteral("exec_name"))) {
        args << QStringLiteral("--exec") << options.value(QStringLiteral("exec_name")).toString();
    }

    auto* process = new QProcess(this);
    auto* reqObj = new PortalRequest(handle.path(), this);

    PendingRequest req;
    req.message = message;
    req.process = process;
    req.requestObject = reqObj;

    m_requests.insert(handle.path(), req);

    connect(reqObj, &PortalRequest::closeRequested, this, [this, handle]() {
        if (m_requests.contains(handle.path())) {
            auto r = m_requests.take(handle.path());
            if (r.process && r.process->state() != QProcess::NotRunning) {
                r.process->terminate();
            }
            delete r.requestObject;
            delete r.process;

            QDBusMessage reply = r.message.createReply();
            reply << static_cast<uint>(1) << QVariantMap();
            QDBusConnection::sessionBus().send(reply);
        }
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, handle, name](int exitCode, QProcess::ExitStatus /*status*/) {
        if (!m_requests.contains(handle.path())) {
            return;
        }
        auto req = m_requests.take(handle.path());
        QByteArray stdoutData = req.process->readAllStandardOutput();

        delete req.requestObject;
        req.process->deleteLater();

        QDBusMessage reply = req.message.createReply();
        QVariantMap results;

        if (exitCode == 0) {
            QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
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
                                   const QString& /*token*/,
                                   const QString& /*desktop_file_id*/,
                                   const QVariantMap& /*options*/) {
    return 0;
}

uint DynamicLauncherPortal::Uninstall(const QString& /*app_id*/,
                                     const QString& desktop_file_id,
                                     const QVariantMap& /*options*/) {
    QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString path = appsDir + QLatin1Char('/') + desktop_file_id;
    if (QFile::exists(path)) {
        QFile::remove(path);
    }
    return 0;
}

uint DynamicLauncherPortal::Launch(const QString& /*app_id*/,
                                  const QString& desktop_file_id,
                                  const QVariantMap& /*options*/) {
    QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString path = appsDir + QLatin1Char('/') + desktop_file_id;
    if (!QFile::exists(path)) {
        path = QStringLiteral("/usr/share/applications/") + desktop_file_id;
    }
    if (QFile::exists(path)) {
        QProcess::startDetached(QStringLiteral("gio"), { QStringLiteral("launch"), path });
        return 0;
    }
    return 1;
}

} // namespace wormhole::portal
