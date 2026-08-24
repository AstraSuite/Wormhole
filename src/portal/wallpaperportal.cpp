#include "wallpaperportal.hpp"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QUrl>

namespace wormhole::portal {

WallpaperPortal::WallpaperPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void WallpaperPortal::SetWallpaperURI(const QDBusObjectPath& handle,
                                     const QString& app_id,
                                     const QString& parent_window,
                                     const QString& uri,
                                     const QVariantMap& options,
                                     const QDBusMessage& message) {
    bool showPreview = true;
    if (options.contains(QStringLiteral("show-preview"))) {
        showPreview = options.value(QStringLiteral("show-preview")).toBool();
    }

    if (!showPreview) {
        QString localPath;
        if (uri.startsWith(QLatin1String("file://"))) {
            localPath = QUrl(uri).toLocalFile();
        } else {
            localPath = uri;
        }
        int res = 1;
        if (!localPath.isEmpty() && QFile::exists(localPath)) {
            res = QProcess::execute(QStringLiteral("caelestia"), QStringList{QStringLiteral("wallpaper"), QStringLiteral("-f"), localPath});
        }
        QDBusMessage reply = message.createReply();
        reply << static_cast<uint>(res == 0 ? 0 : 1) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
        return;
    }

    message.setDelayedReply(true);

    QString wormholeBin = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QStringLiteral("--wallpaper");
    args << QStringLiteral("--app-id") << app_id;
    args << QStringLiteral("--url") << uri;
    if (!parent_window.isEmpty()) {
        args << QStringLiteral("--parent-window") << parent_window;
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
        delete req.requestObject;
        req.process->deleteLater();

        QDBusMessage reply = req.message.createReply();
        reply << static_cast<uint>(exitCode == 0 ? 0 : 1) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
    });

    process->start(wormholeBin, args);
}

void WallpaperPortal::SetWallpaperFile(const QDBusObjectPath& handle,
                                      const QString& app_id,
                                      const QString& parent_window,
                                      const QDBusUnixFileDescriptor& fd,
                                      const QVariantMap& options,
                                      const QDBusMessage& message) {
    QString path = QStringLiteral("/proc/self/fd/%1").arg(fd.fileDescriptor());
    SetWallpaperURI(handle, app_id, parent_window, QStringLiteral("file://") + path, options, message);
}

} // namespace wormhole::portal
