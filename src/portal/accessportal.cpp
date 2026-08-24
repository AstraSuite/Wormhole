#include "accessportal.hpp"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace wormhole::portal {

AccessPortal::AccessPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void AccessPortal::AccessDialog(const QDBusObjectPath& handle,
                               const QString& app_id,
                               const QString& parent_window,
                               const QString& title,
                               const QString& subtitle,
                               const QString& body,
                               const QVariantMap& options,
                               const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString wormholeBin = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QStringLiteral("--access");
    args << QStringLiteral("--app-id") << app_id;
    if (!parent_window.isEmpty()) {
        args << QStringLiteral("--parent-window") << parent_window;
    }
    if (!title.isEmpty()) {
        args << QStringLiteral("--title") << title;
    }
    if (!subtitle.isEmpty()) {
        args << QStringLiteral("--subtitle") << subtitle;
    }
    if (!body.isEmpty()) {
        args << QStringLiteral("--body") << body;
    }
    if (options.contains(QStringLiteral("icon"))) {
        args << QStringLiteral("--icon") << options.value(QStringLiteral("icon")).toString();
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
            this, [this, handle](int exitCode, QProcess::ExitStatus /*status*/) {
        if (!m_requests.contains(handle.path())) {
            return;
        }
        auto req = m_requests.take(handle.path());
        QByteArray stdoutData = req.process->readAllStandardOutput();

        delete req.requestObject;
        req.process->deleteLater();

        QDBusMessage reply = req.message.createReply();
        QVariantMap results;

        if (exitCode == 0 && !stdoutData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(stdoutData);
            if (doc.isObject()) {
                QJsonObject root = doc.object();
                if (root.value(QStringLiteral("allow")).toBool(true)) {
                    reply << static_cast<uint>(0) << results;
                    QDBusConnection::sessionBus().send(reply);
                    return;
                }
            }
        }

        // Denied or cancelled
        reply << static_cast<uint>(1) << results;
        QDBusConnection::sessionBus().send(reply);
    });

    process->start(wormholeBin, args);
}

} // namespace wormhole::portal
