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
        closeSession(session_handle.path());
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
    args << QStringLiteral("--cursor-mode")
         << QString::number(m_sessions.value(sessionHandle.path()).cursorMode);
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
                const QJsonObject root = doc.object();
                const bool isWindow = root.value(QStringLiteral("type")).toString(QStringLiteral("screen")) == QLatin1String("window");
                const QString name = root.value(QStringLiteral("name")).toString();
                const QString address = root.value(QStringLiteral("address")).toString();
                const QString title = root.value(QStringLiteral("title")).toString();
                const QString className = root.value(QStringLiteral("className")).toString();
                const int fps = root.value(QStringLiteral("fps")).toInt(60);
                const uint requestedCursor = m_sessions.value(req.sessionHandle.path()).cursorMode;
                const bool paintCursor = requestedCursor == Embedded
                    && root.value(QStringLiteral("cursorMode")).toInt(Hidden) == Embedded;
                const QString label = isWindow ? (title.isEmpty() ? className : title) : name;

                auto* capture = new screencast::WaylandCapture(this);
                const bool started = isWindow
                    ? capture->captureToplevel(className, title, paintCursor, fps)
                    : capture->captureOutput(name, paintCursor, fps);

                if (started) {
                    const QSize size = capture->frameSize();
                    const uint32_t nodeId = screencast::PipeWireStreamManager::instance()->createStream(
                        label, size.width(), size.height(), fps);

                    if (nodeId != 0) {
                        connect(capture, &screencast::WaylandCapture::frameReady, this, [nodeId](const QImage& frame) {
                            screencast::PipeWireStreamManager::instance()->pushFrame(nodeId, frame);
                        });

                        const QString sessionPath = req.sessionHandle.path();
                        connect(capture, &screencast::WaylandCapture::stopped, this, [this, sessionPath]() {
                            closeSession(sessionPath);
                        });

                        if (m_sessions.contains(sessionPath)) {
                            auto& sess = m_sessions[sessionPath];
                            sess.pipewireNodeId = nodeId;
                            sess.capture = capture;
                        }

                        ScreenCastStream stream;
                        stream.nodeId = nodeId;
                        stream.properties.insert(QStringLiteral("position"), QVariant::fromValue(ScreenCastPosition{
                            root.value(QStringLiteral("x")).toInt(0),
                            root.value(QStringLiteral("y")).toInt(0)
                        }));
                        stream.properties.insert(QStringLiteral("size"), QVariant::fromValue(ScreenCastSize{ size.width(), size.height() }));
                        stream.properties.insert(QStringLiteral("source_type"), static_cast<uint>(isWindow ? Window : Monitor));
                        stream.properties.insert(QStringLiteral("mapping_id"), isWindow ? address : name);

                        ScreenCastStreamList streams;
                        streams.append(stream);

                        results.insert(QStringLiteral("streams"), QVariant::fromValue(streams));
                        results.insert(QStringLiteral("source_type"), static_cast<uint>(isWindow ? Window : Monitor));

                        if (root.value(QStringLiteral("persist")).toBool()) {
                            results.insert(QStringLiteral("restore_token"), QUuid::createUuid().toString(QUuid::WithoutBraces));
                        }

                        reply << static_cast<uint>(0) << results;
                        QDBusConnection::sessionBus().send(reply);
                        return;
                    }
                }

                capture->deleteLater();
            }
        }

        reply << static_cast<uint>(1) << results;
        QDBusConnection::sessionBus().send(reply);
    });

    process->start(wormholeBin, args);
}

void ScreenCastPortal::closeSession(const QString& sessionPath) {
    if (!m_sessions.contains(sessionPath)) {
        return;
    }

    auto data = m_sessions.take(sessionPath);
    if (data.capture) {
        data.capture->stop();
        data.capture->deleteLater();
    }
    if (data.pipewireNodeId != 0) {
        screencast::PipeWireStreamManager::instance()->stopStream(data.pipewireNodeId);
    }
}

} // namespace wormhole::portal
