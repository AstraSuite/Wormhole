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

    const QString sessionPath = session_handle.path();
    if (!m_sessions.contains(sessionPath)) {
        sendReply(message, 2, QVariantMap());
        return;
    }

    const SessionData data = m_sessions.value(sessionPath);

    RestoreEntry entry;
    if (data.persistMode != NoPersist && RestoreStore::instance()->take(data.restoreToken, app_id, entry)) {
        SelectedSource source;
        source.isWindow = entry.isWindow;
        source.outputName = entry.outputName;
        source.windowAppId = entry.windowAppId;
        source.windowTitle = entry.windowTitle;
        source.windowAddress = entry.windowAddress;
        source.x = entry.x;
        source.y = entry.y;
        source.fps = entry.fps;
        source.paintCursor = data.cursorMode == Embedded;

        QVariantMap results;
        if (startStream(sessionPath, source, results)) {
            results.insert(QStringLiteral("restore_token"), data.restoreToken);
            sendReply(message, 0, results);
            return;
        }

        RestoreStore::instance()->remove(data.restoreToken);
    }

    launchScreenChooser(handle, session_handle, app_id, parent_window, message);
}

void ScreenCastPortal::sendReply(const QDBusMessage& message, uint response, const QVariantMap& results) {
    QDBusMessage reply = message.createReply();
    reply << response << results;
    QDBusConnection::sessionBus().send(reply);
}

bool ScreenCastPortal::startStream(const QString& sessionPath, const SelectedSource& source, QVariantMap& results) {
    auto* capture = new screencast::WaylandCapture(this);
    const bool started = source.isWindow
        ? capture->captureToplevel(source.windowAppId, source.windowTitle, source.paintCursor, source.fps)
        : capture->captureOutput(source.outputName, source.paintCursor, source.fps);

    if (!started) {
        capture->deleteLater();
        return false;
    }

    const QSize size = capture->frameSize();
    const QString label = source.isWindow
        ? (source.windowTitle.isEmpty() ? source.windowAppId : source.windowTitle)
        : source.outputName;

    const uint32_t nodeId = screencast::PipeWireStreamManager::instance()->createStream(
        label, size.width(), size.height(), source.fps);

    if (nodeId == 0) {
        capture->deleteLater();
        return false;
    }

    connect(capture, &screencast::WaylandCapture::frameReady, this, [nodeId](const QImage& frame) {
        screencast::PipeWireStreamManager::instance()->pushFrame(nodeId, frame);
    });
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
    stream.properties.insert(QStringLiteral("position"),
                             QVariant::fromValue(ScreenCastPosition{ source.x, source.y }));
    stream.properties.insert(QStringLiteral("size"),
                             QVariant::fromValue(ScreenCastSize{ size.width(), size.height() }));
    stream.properties.insert(QStringLiteral("source_type"),
                             static_cast<uint>(source.isWindow ? Window : Monitor));
    stream.properties.insert(QStringLiteral("mapping_id"),
                             source.isWindow ? source.windowAddress : source.outputName);

    ScreenCastStreamList streams;
    streams.append(stream);

    results.insert(QStringLiteral("streams"), QVariant::fromValue(streams));
    results.insert(QStringLiteral("source_type"), static_cast<uint>(source.isWindow ? Window : Monitor));
    return true;
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
    const SessionData data = m_sessions.value(sessionHandle.path());
    args << QStringLiteral("--cursor-mode") << QString::number(data.cursorMode);
    args << QStringLiteral("--persist-mode") << QString::number(data.persistMode);
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

            sendReply(r.message, 1, QVariantMap());
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

        QVariantMap results;

        if (exitCode == 0 && !stdoutData.isEmpty()) {
            const QJsonDocument doc = QJsonDocument::fromJson(stdoutData);
            if (!doc.isNull()) {
                const QJsonObject root = doc.object();
                const QString sessionPath = req.sessionHandle.path();
                const SessionData data = m_sessions.value(sessionPath);

                SelectedSource source;
                source.isWindow = root.value(QStringLiteral("type")).toString(QStringLiteral("screen")) == QLatin1String("window");
                source.outputName = root.value(QStringLiteral("name")).toString();
                source.windowAppId = root.value(QStringLiteral("className")).toString();
                source.windowTitle = root.value(QStringLiteral("title")).toString();
                source.windowAddress = root.value(QStringLiteral("address")).toString();
                source.x = root.value(QStringLiteral("x")).toInt(0);
                source.y = root.value(QStringLiteral("y")).toInt(0);
                source.fps = root.value(QStringLiteral("fps")).toInt(60);
                source.paintCursor = data.cursorMode == Embedded
                    && root.value(QStringLiteral("cursorMode")).toInt(Hidden) == Embedded;

                if (startStream(sessionPath, source, results)) {
                    if (data.persistMode != NoPersist && root.value(QStringLiteral("persist")).toBool()) {
                        RestoreEntry entry;
                        entry.appId = req.appId;
                        entry.isWindow = source.isWindow;
                        entry.outputName = source.outputName;
                        entry.windowAppId = source.windowAppId;
                        entry.windowTitle = source.windowTitle;
                        entry.windowAddress = source.windowAddress;
                        entry.x = source.x;
                        entry.y = source.y;
                        entry.fps = source.fps;
                        entry.durable = data.persistMode == PersistUntilRevoked;

                        results.insert(QStringLiteral("restore_token"),
                                       RestoreStore::instance()->add(entry));
                    }

                    sendReply(req.message, 0, results);
                    return;
                }
            }
        }

        sendReply(req.message, 1, results);
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
