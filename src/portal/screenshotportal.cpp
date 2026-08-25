#include "screenshotportal.hpp"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace wormhole::portal {

ScreenshotPortal::ScreenshotPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void ScreenshotPortal::Screenshot(const QDBusObjectPath& handle,
                                  const QString& app_id,
                                  const QString& parent_window,
                                  const QVariantMap& options,
                                  const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString wormholeBin = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QStringLiteral("--screenshot");
    args << QStringLiteral("--app-id") << app_id;
    if (!parent_window.isEmpty()) {
        args << QStringLiteral("--parent-window") << parent_window;
    }
    if (options.value(QStringLiteral("interactive"), true).toBool()) {
        args << QStringLiteral("--interactive");
    }

    auto* process = new QProcess(this);
    auto* reqObj = new PortalRequest(handle.path(), this);

    PendingRequest req;
    req.message = message;
    req.process = process;
    req.requestObject = reqObj;
    req.isPickColor = false;

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
                if (root.contains(QStringLiteral("uri"))) {
                    results.insert(QStringLiteral("uri"), root.value(QStringLiteral("uri")).toString());
                    reply << static_cast<uint>(0) << results;
                    QDBusConnection::sessionBus().send(reply);
                    return;
                }
            }
        }

        // Fallback or cancel
        reply << static_cast<uint>(1) << results;
        QDBusConnection::sessionBus().send(reply);
    });

    process->start(wormholeBin, args);
}

void ScreenshotPortal::PickColor(const QDBusObjectPath& handle,
                                 const QString& app_id,
                                 const QString& parent_window,
                                 const QVariantMap& /*options*/,
                                 const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString wormholeBin = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QStringLiteral("--pick-color");
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
    req.isPickColor = true;

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
                if (root.contains(QStringLiteral("color"))) {
                    QJsonArray colorArr = root.value(QStringLiteral("color")).toArray();
                    if (colorArr.size() >= 3) {
                        double r = colorArr.at(0).toDouble();
                        double g = colorArr.at(1).toDouble();
                        double b = colorArr.at(2).toDouble();

                        QDBusArgument colorArg;
                        colorArg.beginStructure();
                        colorArg << r << g << b;
                        colorArg.endStructure();

                        results.insert(QStringLiteral("color"), QVariant::fromValue(colorArg));
                        reply << static_cast<uint>(0) << results;
                        QDBusConnection::sessionBus().send(reply);
                        return;
                    }
                }
            }
        }

        // Cancelled
        reply << static_cast<uint>(1) << results;
        QDBusConnection::sessionBus().send(reply);
    });

    process->start(wormholeBin, args);
}

} // namespace wormhole::portal
