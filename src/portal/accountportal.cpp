#include "accountportal.hpp"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <pwd.h>
#include <unistd.h>

namespace wormhole::portal {

AccountPortal::AccountPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void AccountPortal::GetUserInformation(const QDBusObjectPath& handle,
                                      const QString& app_id,
                                      const QString& parent_window,
                                      const QVariantMap& /*options*/,
                                      const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString wormholeBin = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QStringLiteral("--account");
    args << QStringLiteral("--app-id") << app_id;
    if (!parent_window.isEmpty()) {
        args << QStringLiteral("--parent-window") << parent_window;
    }

    auto* process = new QProcess(this);
    auto* reqObj = new PortalRequest(handle.path(), this);

    PendingRequest req;
    req.message = message;
    req.process = process;
    req.requestObject = reqObj;
    req.appId = app_id;

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
            this, [this, handle](int exitCode, QProcess::ExitStatus /*status*/) {
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
            QString username = qEnvironmentVariable("USER", QStringLiteral("user"));
            QString realName = username;

            struct passwd* pw = getpwuid(getuid());
            if (pw && pw->pw_gecos) {
                QString gecos = QString::fromUtf8(pw->pw_gecos).split(QLatin1Char(',')).first().trimmed();
                if (!gecos.isEmpty()) {
                    realName = gecos;
                }
            }

            QString facePath = QDir::homePath() + QStringLiteral("/.face");
            if (!QFile::exists(facePath)) {
                facePath = QDir::homePath() + QStringLiteral("/.face.icon");
            }

            results.insert(QStringLiteral("id"), username);
            results.insert(QStringLiteral("name"), realName);
            if (QFile::exists(facePath)) {
                results.insert(QStringLiteral("image"), QStringLiteral("file://") + facePath);
            }

            reply << static_cast<uint>(0) << results;
        } else {
            reply << static_cast<uint>(1) << results;
        }

        QDBusConnection::sessionBus().send(reply);
    });

    process->start(wormholeBin, args);
}

} // namespace wormhole::portal
