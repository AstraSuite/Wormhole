#include "appchooserportal.hpp"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace wormhole::portal {

AppChooserPortal::AppChooserPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void AppChooserPortal::ChooseApplication(const QDBusObjectPath& handle,
                                        const QString& app_id,
                                        const QString& parent_window,
                                        const QStringList& choices,
                                        const QVariantMap& options,
                                        const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString wormholeBin = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QStringLiteral("--appchooser");
    args << QStringLiteral("--app-id") << app_id;
    if (!parent_window.isEmpty()) {
        args << QStringLiteral("--parent-window") << parent_window;
    }
    if (options.contains(QStringLiteral("mime_type"))) {
        args << QStringLiteral("--mime") << options.value(QStringLiteral("mime_type")).toString();
    }
    if (options.contains(QStringLiteral("uri"))) {
        args << QStringLiteral("--url") << options.value(QStringLiteral("uri")).toString();
    }
    if (!choices.isEmpty()) {
        args << QStringLiteral("--choices") << choices.join(QLatin1Char(','));
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

        if (exitCode == 0 && !stdoutData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(stdoutData);
            if (doc.isObject()) {
                QJsonObject root = doc.object();
                if (root.contains(QStringLiteral("choice"))) {
                    results.insert(QStringLiteral("choice"), root.value(QStringLiteral("choice")).toString());
                    reply << static_cast<uint>(0) << results;
                    QDBusConnection::sessionBus().send(reply);
                    return;
                }
            }
        }

        // Cancelled
        reply << static_cast<uint>(1) << results;
        QDBusConnection::sessionBus().send(reply);
    });

    process->start(wormholeBin, args);
}

void AppChooserPortal::UpdateChoices(const QDBusObjectPath& /*handle*/,
                                    const QStringList& /*choices*/) {
}

} // namespace wormhole::portal
