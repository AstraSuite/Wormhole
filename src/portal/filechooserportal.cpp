#include "filechooserportal.hpp"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <QUrl>

namespace wormhole::portal {

FileChooserPortal::FileChooserPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

QString FileChooserPortal::findAtlasBinary() {
    // Check if atlas is in same dir as wormhole
    QString appDir = QCoreApplication::applicationDirPath();
    if (QFile::exists(appDir + QStringLiteral("/atlas"))) {
        return appDir + QStringLiteral("/atlas");
    }
    // Check PATH
    QString inPath = QStandardPaths::findExecutable(QStringLiteral("atlas"));
    if (!inPath.isEmpty()) {
        return inPath;
    }
    // Check common prefixes
    if (QFile::exists(QStringLiteral("/usr/bin/atlas"))) {
        return QStringLiteral("/usr/bin/atlas");
    }
    if (QFile::exists(QStringLiteral("/usr/local/bin/atlas"))) {
        return QStringLiteral("/usr/local/bin/atlas");
    }
    return QStringLiteral("atlas");
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

    QString dialogTitle = title.isEmpty() ? QStringLiteral("Open File") : title;
    QString initialDir = parseInitialDirectory(options);
    bool directoryOnly = options.value(QStringLiteral("directory"), false).toBool();
    bool multiple = options.value(QStringLiteral("multiple"), false).toBool();

    QStringList filters;
    QString filterLabel;
    parseFilters(options, filters, filterLabel);

    launchAtlasPicker(dialogTitle, initialDir, directoryOnly, filters, filterLabel, handle, message, false, {}, false, {}, multiple);
}

void FileChooserPortal::SaveFile(const QDBusObjectPath& handle,
                                 const QString& /*app_id*/,
                                 const QString& /*parent_window*/,
                                 const QString& title,
                                 const QVariantMap& options,
                                 const QDBusMessage& message) {
    message.setDelayedReply(true);

    QString dialogTitle = title.isEmpty() ? QStringLiteral("Save File") : title;
    QString initialDir = parseInitialDirectory(options);

    QString suggestedName;
    if (options.contains(QStringLiteral("current_name"))) {
        suggestedName = options.value(QStringLiteral("current_name")).toString();
    }

    QStringList filters;
    QString filterLabel;
    parseFilters(options, filters, filterLabel);

    launchAtlasPicker(dialogTitle, initialDir, false, filters, filterLabel, handle, message, false, {}, true, suggestedName);
}

void FileChooserPortal::SaveFiles(const QDBusObjectPath& handle,
                                  const QString& /*app_id*/,
                                  const QString& /*parent_window*/,
                                  const QString& title,
                                  const QVariantMap& options,
                                  const QDBusMessage& message) {
    message.setDelayedReply(true);

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

    launchAtlasPicker(dialogTitle, initialDir, true, { QStringLiteral("*") }, QStringLiteral("Folders"), handle, message, true, files);
}

void FileChooserPortal::launchAtlasPicker(const QString& title,
                                         const QString& initialDir,
                                         bool directoryOnly,
                                         const QStringList& filters,
                                         const QString& filterLabel,
                                         const QDBusObjectPath& handle,
                                         const QDBusMessage& message,
                                         bool isSaveFiles,
                                         const QStringList& fileList,
                                         bool saveMode,
                                         const QString& suggestedName,
                                         bool multiple) {
    QString atlasBin = findAtlasBinary();
    QStringList args;
    args << QStringLiteral("--picker");
    args << QStringLiteral("--title") << title;

    if (!initialDir.isEmpty()) {
        args << QStringLiteral("--directory") << initialDir;
    }
    if (directoryOnly) {
        args << QStringLiteral("--directory-only");
    }
    if (multiple) {
        args << QStringLiteral("--multiple");
    }
    if (saveMode) {
        args << QStringLiteral("--save");
        if (!suggestedName.isEmpty()) {
            args << QStringLiteral("--name") << suggestedName;
        }
    }
    if (!filters.isEmpty() && !filters.contains(QStringLiteral("*"))) {
        args << QStringLiteral("--filter") << filters.join(QLatin1Char(','));
    }
    if (!filterLabel.isEmpty()) {
        args << QStringLiteral("--filter-label") << filterLabel;
    }

    auto* process = new QProcess(this);
    auto* reqObj = new PortalRequest(handle.path(), this);

    PendingRequest req;
    req.message = message;
    req.process = process;
    req.requestObject = reqObj;
    req.isSaveFiles = isSaveFiles;
    req.fileListToSave = fileList;

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
            QString output = QString::fromUtf8(stdoutData).trimmed();
            QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            QStringList uris;

            for (const QString& line : lines) {
                QString trimmed = line.trimmed();
                if (trimmed.startsWith(QLatin1String("file://"))) {
                    uris << trimmed;
                } else if (!trimmed.isEmpty() && QDir::isAbsolutePath(trimmed)) {
                    uris << QUrl::fromLocalFile(trimmed).toString();
                }
            }

            if (!uris.isEmpty()) {
                results.insert(QStringLiteral("uris"), uris);
                reply << static_cast<uint>(0) << results;
            } else {
                reply << static_cast<uint>(1) << results;
            }
        } else {
            // Cancelled or error
            reply << static_cast<uint>(1) << results;
        }

        QDBusConnection::sessionBus().send(reply);
    });

    process->start(atlasBin, args);
}

} // namespace wormhole::portal
