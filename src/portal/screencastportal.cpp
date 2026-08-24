#include "screencastportal.hpp"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace wormhole::portal {

ScreenCastPortal::ScreenCastPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
    qDBusRegisterMetaType<ScreenCastPosition>();
    qDBusRegisterMetaType<ScreenCastSize>();
    qDBusRegisterMetaType<ScreenCastStream>();
    qDBusRegisterMetaType<ScreenCastStreamList>();
}

uint ScreenCastPortal::CreateSession(const QDBusObjectPath& /*handle*/,
                                     const QDBusObjectPath& session_handle,
                                     const QString& app_id,
                                     const QVariantMap& /*options*/,
                                     QVariantMap& /*results*/) {
    SessionData data;
    data.appId = app_id;
    m_sessions.insert(session_handle.path(), data);

    auto* sessionObj = new PortalSession(session_handle.path(), app_id, this);
    connect(sessionObj, &PortalSession::closed, this, [this, session_handle]() {
        if (m_sessions.contains(session_handle.path())) {
            auto data = m_sessions.take(session_handle.path());
            if (data.worker) {
                data.worker->stop();
                delete data.worker;
            }
            if (data.pipewireNodeId != 0) {
                screencast::PipeWireStreamManager::instance()->stopStream(data.pipewireNodeId);
            }
        }
    });

    return 0;
}

uint ScreenCastPortal::SelectSources(const QDBusObjectPath& /*handle*/,
                                     const QDBusObjectPath& session_handle,
                                     const QString& /*app_id*/,
                                     const QVariantMap& options,
                                     QVariantMap& /*results*/) {
    if (!m_sessions.contains(session_handle.path())) {
        return 2;
    }

    auto& data = m_sessions[session_handle.path()];
    data.types = options.value(QStringLiteral("types"), Monitor | Window).toUInt();
    data.cursorMode = options.value(QStringLiteral("cursor_mode"), Hidden).toUInt();
    data.persistMode = options.value(QStringLiteral("persist_mode"), NoPersist).toUInt();
    if (options.contains(QStringLiteral("restore_token"))) {
        data.restoreToken = options.value(QStringLiteral("restore_token")).toString();
    }

    return 0;
}

void ScreenCastPortal::Start(const QDBusObjectPath& handle,
                            const QDBusObjectPath& session_handle,
                            const QString& app_id,
                            const QString& parent_window,
                            const QVariantMap& /*options*/,
                            const QDBusMessage& message) {
    message.setDelayedReply(true);

    if (!m_sessions.contains(session_handle.path())) {
        QDBusMessage reply = message.createReply();
        reply << static_cast<uint>(2) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
        return;
    }

    launchScreenChooser(handle, session_handle, app_id, parent_window, message);
}

void ScreenCastPortal::launchScreenChooser(const QDBusObjectPath& handle,
                                           const QDBusObjectPath& sessionHandle,
                                           const QString& appId,
                                           const QString& parentWindow,
                                           const QDBusMessage& message) {
    QString wormholeBin = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QStringLiteral("--screencast");
    args << QStringLiteral("--app-id") << appId;
    if (!parentWindow.isEmpty()) {
        args << QStringLiteral("--parent-window") << parentWindow;
    }

    auto* process = new QProcess(this);
    auto* reqObj = new PortalRequest(handle.path(), this);

    PendingStartRequest req;
    req.message = message;
    req.sessionHandle = sessionHandle;
    req.appId = appId;
    req.process = process;
    req.requestObject = reqObj;

    m_startRequests.insert(handle.path(), req);

    connect(reqObj, &PortalRequest::closeRequested, this, [this, handle]() {
        if (m_startRequests.contains(handle.path())) {
            auto r = m_startRequests.take(handle.path());
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
        if (!m_startRequests.contains(handle.path())) {
            return;
        }
        auto req = m_startRequests.take(handle.path());
        QByteArray stdoutData = req.process->readAllStandardOutput();

        if (req.requestObject) req.requestObject->deleteLater();
        if (req.process) req.process->deleteLater();

        QDBusMessage reply = req.message.createReply();
        QVariantMap results;

        if (exitCode == 0 && !stdoutData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(stdoutData);
            if (!doc.isNull()) {
                QJsonObject root = doc.object();
                QString type = root.value(QStringLiteral("type")).toString(QStringLiteral("screen"));
                QString name = root.value(QStringLiteral("name")).toString();
                QString addr = root.value(QStringLiteral("address")).toString();
                int width = root.value(QStringLiteral("width")).toInt(1920);
                int height = root.value(QStringLiteral("height")).toInt(1080);
                int fps = root.value(QStringLiteral("fps")).toInt(60);
                bool isWindow = (type == QLatin1String("window"));

                uint32_t nodeId = screencast::PipeWireStreamManager::instance()->createStream(
                    isWindow ? root.value(QStringLiteral("title")).toString(QStringLiteral("Window")) : name,
                    width, height, fps
                );
                qDebug() << "[WORMHOLE] Created PipeWire stream nodeId:" << nodeId << "for" << (isWindow ? addr : name);

                if (nodeId != 0) {
                    auto* worker = new screencast::ScreenCaptureWorker(nodeId, name, isWindow, addr, this);
                    worker->start(fps);

                    if (m_sessions.contains(req.sessionHandle.path())) {
                        auto& sess = m_sessions[req.sessionHandle.path()];
                        sess.pipewireNodeId = nodeId;
                        sess.worker = worker;
                    }

                    // Build D-Bus streams array a(ua{sv}) matching xdp-hyprland format exactly
                    ScreenCastStream stream;
                    stream.nodeId = nodeId;
                    stream.properties.insert(QStringLiteral("position"), QVariant::fromValue(ScreenCastPosition{ 0, 0 }));
                    stream.properties.insert(QStringLiteral("size"), QVariant::fromValue(ScreenCastSize{ width, height }));
                    stream.properties.insert(QStringLiteral("source_type"), static_cast<uint>(isWindow ? 2 : 1));
                    stream.properties.insert(QStringLiteral("mapping_id"), isWindow ? addr : name);

                    ScreenCastStreamList streams;
                    streams.append(stream);

                    results.insert(QStringLiteral("streams"), QVariant::fromValue(streams));
                    results.insert(QStringLiteral("source_type"), static_cast<uint>(isWindow ? 2 : 1));

                    if (root.value(QStringLiteral("persist")).toBool()) {
                        QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
                        results.insert(QStringLiteral("restore_token"), token);
                    }

                    qDebug() << "[WORMHOLE] Sending Start success reply with nodeId:" << nodeId;
                    reply << static_cast<uint>(0) << results;
                    QDBusConnection::sessionBus().send(reply);
                    return;
                }
            }
        }

        // Cancelled or failure
        reply << static_cast<uint>(1) << results;
        QDBusConnection::sessionBus().send(reply);
    });

    process->start(wormholeBin, args);
}

} // namespace wormhole::portal
