#include "filechooserportal.hpp"
#include <QDBusArgument>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>

namespace wormhole::portal {

FileChooserPortal::FileChooserPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

QString FileChooserPortal::parseInitialDirectory(const QVariantMap& options) {
    QString raw;
    if (options.contains(QStringLiteral("current_folder"))) {
        QVariant var = options.value(QStringLiteral("current_folder"));
        if (var.userType() == QMetaType::QByteArray) {
            QByteArray bytes = var.toByteArray();
            if (bytes.endsWith('\0')) {
                bytes.chop(1);
            }
            raw = QString::fromUtf8(bytes);
        } else {
            raw = var.toString();
        }
    }
    if (raw.isEmpty() && options.contains(QStringLiteral("current_file"))) {
        QVariant var = options.value(QStringLiteral("current_file"));
        if (var.userType() == QMetaType::QByteArray) {
            QByteArray bytes = var.toByteArray();
            if (bytes.endsWith('\0')) {
                bytes.chop(1);
            }
            raw = QString::fromUtf8(bytes);
        } else {
            raw = var.toString();
        }
    }

    if (raw.isEmpty()) {
        return {};
    }

    if (raw.startsWith(QLatin1String("file://"))) {
        raw = QUrl(raw).toLocalFile();
    }

    QFileInfo fi(raw);
    if (fi.exists()) {
        return fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    }

    if (!fi.absolutePath().isEmpty() && QDir(fi.absolutePath()).exists()) {
        return fi.absolutePath();
    }

    return raw;
}

void FileChooserPortal::parseFilters(const QVariantMap& options, QStringList& outFilters, QString& outLabel) {
    outFilters.clear();
    outLabel = QStringLiteral("All files");

    if (!options.contains(QStringLiteral("filters"))) {
        outFilters << QStringLiteral("*");
        return;
    }

    const QDBusArgument arg = options.value(QStringLiteral("filters")).value<QDBusArgument>();
    arg.beginArray();
    QMimeDatabase mimeDb;

    while (!arg.atEnd()) {
        arg.beginStructure();
        QString userVisibleName;
        arg >> userVisibleName;

        if (outLabel == QLatin1String("All files") && !userVisibleName.isEmpty()) {
            outLabel = userVisibleName;
        }

        arg.beginArray();
        while (!arg.atEnd()) {
            arg.beginStructure();
            uint type = 0;
            QString pattern;
            arg >> type >> pattern;

            if (type == 0) { // Glob pattern
                if (pattern.startsWith(QLatin1String("*."))) {
                    pattern = pattern.mid(2);
                } else if (pattern.startsWith(QLatin1Char('.'))) {
                    pattern = pattern.mid(1);
                }
                if (!pattern.isEmpty() && !outFilters.contains(pattern)) {
                    outFilters << pattern;
                }
            } else if (type == 1) { // MIME type
                QMimeType mime = mimeDb.mimeTypeForName(pattern);
                if (mime.isValid()) {
                    for (const QString& suf : mime.suffixes()) {
                        if (!outFilters.contains(suf)) {
                            outFilters << suf;
                        }
                    }
                }
            }
            arg.endStructure();
        }
        arg.endArray();
        arg.endStructure();
    }
    arg.endArray();

    if (outFilters.isEmpty()) {
        outFilters << QStringLiteral("*");
    }
}

void FileChooserPortal::OpenFile(const QDBusObjectPath& handle,
                                 const QString& /*app_id*/,
                                 const QString& /*parent_window*/,
                                 const QString& title,
                                 const QVariantMap& options,
                                 const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString handleStr = handle.path();
    auto* reqObj = new PortalRequest(handleStr, this);

    PendingRequest req;
    req.message = message;
    req.requestObject = reqObj;
    qDebug() << "OpenFile: handle.path()=" << handleStr << "title=" << title;
    m_requests.insert(handleStr, req);

    connect(reqObj, &PortalRequest::closeRequested, this, [this, handleStr]() {
        cleanupRequest(handleStr);
    });

    QString dialogTitle = title.isEmpty() ? QStringLiteral("Open File") : title;
    QString initialDir = parseInitialDirectory(options);
    bool directoryOnly = options.value(QStringLiteral("directory"), false).toBool();
    bool multiple = options.value(QStringLiteral("multiple"), false).toBool();

    QStringList filters;
    QString filterLabel;
    parseFilters(options, filters, filterLabel);

    emit openFileRequested(handle.path(), dialogTitle, filters, filterLabel, directoryOnly, multiple, initialDir);
}

void FileChooserPortal::SaveFile(const QDBusObjectPath& handle,
                                 const QString& /*app_id*/,
                                 const QString& /*parent_window*/,
                                 const QString& title,
                                 const QVariantMap& options,
                                 const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString handleStr = handle.path();
    auto* reqObj = new PortalRequest(handleStr, this);

    PendingRequest req;
    req.message = message;
    req.requestObject = reqObj;
    m_requests.insert(handleStr, req);

    connect(reqObj, &PortalRequest::closeRequested, this, [this, handleStr]() {
        cleanupRequest(handleStr);
    });

    QString dialogTitle = title.isEmpty() ? QStringLiteral("Save File") : title;
    QString initialDir = parseInitialDirectory(options);

    QString suggestedName;
    if (options.contains(QStringLiteral("current_name"))) {
        suggestedName = options.value(QStringLiteral("current_name")).toString();
    }

    QStringList filters;
    QString filterLabel;
    parseFilters(options, filters, filterLabel);

    emit saveFileRequested(handle.path(), dialogTitle, filters, filterLabel, suggestedName, initialDir);
}

void FileChooserPortal::SaveFiles(const QDBusObjectPath& handle,
                                   const QString& /*app_id*/,
                                   const QString& /*parent_window*/,
                                   const QString& title,
                                   const QVariantMap& options,
                                   const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString handleStr = handle.path();
    auto* reqObj = new PortalRequest(handleStr, this);

    PendingRequest req;
    req.message = message;
    req.requestObject = reqObj;
    m_requests.insert(handleStr, req);

    connect(reqObj, &PortalRequest::closeRequested, this, [this, handleStr]() {
        cleanupRequest(handleStr);
    });

    QString dialogTitle = title.isEmpty() ? QStringLiteral("Save Files") : title;
    QString initialDir = parseInitialDirectory(options);

    QStringList files;
    if (options.contains(QStringLiteral("files"))) {
        const QDBusArgument arg = options.value(QStringLiteral("files")).value<QDBusArgument>();
        arg.beginArray();
        while (!arg.atEnd()) {
            QByteArray rawFile;
            arg >> rawFile;
            if (rawFile.endsWith('\0')) {
                rawFile.chop(1);
            }
            files << QString::fromUtf8(rawFile);
        }
        arg.endArray();
    }

    emit saveFilesRequested(handle.path(), dialogTitle, files, initialDir);
}

void FileChooserPortal::finishRequest(const QString& handlePath, quint32 response, const QVariantMap& results) {
    qDebug() << "finishRequest: handlePath=" << handlePath << "response=" << response;
    if (!m_requests.contains(handlePath)) {
        qWarning() << "finishRequest: no pending request for" << handlePath;
        return;
    }

    // Sanitize results: ensure all URIs are properly formatted file:// strings
    QVariantMap sanitizedResults = results;
    if (sanitizedResults.contains(QStringLiteral("uris"))) {
        QStringList formattedUris;
        const QStringList rawUris = sanitizedResults.value(QStringLiteral("uris")).toStringList();
        for (const QString& item : rawUris) {
            if (item.startsWith(QLatin1String("file://"))) {
                formattedUris.append(item);
            } else {
                formattedUris.append(QUrl::fromLocalFile(item).toString());
            }
        }
        sanitizedResults.insert(QStringLiteral("uris"), formattedUris);
    }

    sendResponse(handlePath, response, sanitizedResults);
}

void FileChooserPortal::sendResponse(const QString& handlePath, quint32 response, const QVariantMap& results) {
    if (!m_requests.contains(handlePath)) {
        qWarning() << "sendResponse: no pending request for" << handlePath;
        return;
    }
    PendingRequest req = m_requests.take(handlePath);

    // 1. Emit the Response signal expected by xdg-desktop-portal
    QDBusMessage responseSignal = QDBusMessage::createSignal(
        handlePath,
        QStringLiteral("org.freedesktop.impl.portal.Request"),
        QStringLiteral("Response")
    );
    responseSignal << static_cast<uint>(response) << results;
    QDBusConnection::sessionBus().send(responseSignal);

    // 2. Reply to the initial D-Bus method call
    if (req.message.isDelayedReply()) {
        QDBusMessage reply = req.message.createReply();
        reply << static_cast<uint>(response) << results;
        QDBusConnection::sessionBus().send(reply);
    }

    qDebug() << "sendResponse: Response signal and method reply sent for" << handlePath << "response=" << response;

    // 3. Clean up the request object asynchronously
    if (req.requestObject) {
        req.requestObject->setParent(nullptr);
        QMetaObject::invokeMethod(req.requestObject, &QObject::deleteLater, Qt::QueuedConnection);
    }
}

void FileChooserPortal::cleanupRequest(const QString& handlePath) {
    if (m_requests.contains(handlePath)) {
        sendResponse(handlePath, 1, QVariantMap());
    }
}

} // namespace wormhole::portal