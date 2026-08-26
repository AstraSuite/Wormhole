#include "fileutils.hpp"

#include <QDateTime>
#include <QLocale>
#include <atomic>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDirIterator>
#include <QStorageInfo>
#include <QGuiApplication>
#include <QReadWriteLock>
#include <QStandardPaths>
#include <QIcon>
#include <QRegularExpression>

namespace wormhole::core {

FileUtils::FileUtils(QObject* parent) : QObject(parent) {}

QString FileUtils::home() const {
    return QDir::homePath();
}

QString FileUtils::pictures() const {
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

QString FileUtils::videos() const {
    return QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
}

QString FileUtils::documents() const {
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString FileUtils::downloads() const {
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString FileUtils::music() const {
    return QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
}

QString FileUtils::desktop() const {
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

namespace {
std::atomic<int> g_dateFormat{FileUtils::Iso};
std::atomic<bool> g_thumbsEnabled{true};
std::atomic<qint64> g_thumbMaxBytes{0};
std::atomic<int> g_folderCountMode{FileUtils::FolderCountNever};
QReadWriteLock g_customDateLock;
QString g_customDateFormat = QStringLiteral("yyyy-MM-dd hh:mm");
}

void FileUtils::setDateFormat(int format) {
    g_dateFormat.store(format, std::memory_order_relaxed);
}

int FileUtils::dateFormat() {
    return g_dateFormat.load(std::memory_order_relaxed);
}

void FileUtils::setCustomDateFormat(const QString& pattern) {
    QWriteLocker locker(&g_customDateLock);
    g_customDateFormat = pattern.trimmed().isEmpty() ? QStringLiteral("yyyy-MM-dd hh:mm") : pattern;
}

QString FileUtils::formatDateTime(const QDateTime& dt, int format) {
    if (!dt.isValid())
        return {};

    if (format < 0)
        format = g_dateFormat.load(std::memory_order_relaxed);

    switch (format) {
    case SystemLocale:
        return QLocale::system().toString(dt, QLocale::ShortFormat);
    case LongLocale:
        return QLocale::system().toString(dt, QLocale::LongFormat);
    case Custom: {
        QReadLocker locker(&g_customDateLock);
        const QString pattern = g_customDateFormat;
        locker.unlock();
        const QString rendered = QLocale::system().toString(dt, pattern);
        return rendered.isEmpty() ? dt.toString(QStringLiteral("yyyy-MM-dd hh:mm")) : rendered;
    }
    default:
        return dt.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    }
}

void FileUtils::setThumbnailsEnabled(bool enabled) {
    g_thumbsEnabled.store(enabled, std::memory_order_relaxed);
}

void FileUtils::setThumbnailMaxBytes(qint64 bytes) {
    g_thumbMaxBytes.store(bytes, std::memory_order_relaxed);
}

bool FileUtils::shouldThumbnail(bool isImage, bool isVideo, qint64 size) {
    if (!isImage && !isVideo)
        return false;
    if (!g_thumbsEnabled.load(std::memory_order_relaxed))
        return false;

    const qint64 limit = g_thumbMaxBytes.load(std::memory_order_relaxed);
    return limit <= 0 || size <= limit;
}

void FileUtils::setFolderCountMode(int mode) {
    g_folderCountMode.store(mode, std::memory_order_relaxed);
}

int FileUtils::folderCountMode() {
    return g_folderCountMode.load(std::memory_order_relaxed);
}

QString FileUtils::countFolderItems(const QString& path) {
    const int mode = g_folderCountMode.load(std::memory_order_relaxed);
    if (mode == FolderCountNever)
        return {};

    if (mode == FolderCountLocalOnly) {
        const QStorageInfo storage(path);
        const QByteArray device = storage.device();
        if (!device.startsWith('/'))
            return {};
    }

    QDir dir(path);
    if (!dir.isReadable())
        return {};

    int count = 0;
    QDirIterator it(path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    while (it.hasNext()) {
        it.next();
        ++count;
    }

    return count == 1 ? QObject::tr("1 item") : QObject::tr("%1 items").arg(count);
}

QString FileUtils::formatSize(qint64 bytes) {
    if (bytes < 1024)
        return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return QString("%1 KB").arg(QString::number(bytes / 1024.0, 'f', 1));
    if (bytes < 1024 * 1024 * 1024)
        return QString("%1 MB").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 1));
    return QString("%1 GB").arg(QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1));
}

QString FileUtils::shortenHome(const QString& path) {
    QString h = QDir::homePath();
    if (path.startsWith(h)) {
        return QString("~") + path.mid(h.length());
    }
    return path;
}

QString FileUtils::toLocalFile(const QUrl& url) {
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

QString FileUtils::baseName(const QString& path) {
    if (path.isEmpty()) return QString();
    return QFileInfo(path).fileName();
}

QVariantList FileUtils::describePaths(const QStringList& paths) {
    QVariantList entries;
    entries.reserve(paths.size());

    for (const QString& path : paths) {
        const QFileInfo fi(path);
        if (!fi.exists())
            continue;

        QVariantMap entry;
        entry.insert(QStringLiteral("path"), fi.absoluteFilePath());
        entry.insert(QStringLiteral("name"), fi.fileName());
        entry.insert(QStringLiteral("isDir"), fi.isDir());
        entry.insert(QStringLiteral("modified"), fi.lastModified().toMSecsSinceEpoch());
        entry.insert(QStringLiteral("created"), fi.birthTime().isValid() ? fi.birthTime().toMSecsSinceEpoch() : fi.lastModified().toMSecsSinceEpoch());
        entry.insert(QStringLiteral("accessed"), fi.lastRead().isValid() ? fi.lastRead().toMSecsSinceEpoch() : fi.lastModified().toMSecsSinceEpoch());
        entries.append(entry);
    }

    return entries;
}

QString FileUtils::freeSpaceFor(const QString& path) {
    if (path.isEmpty() || path.contains(QLatin1String("://")) || path.startsWith(QLatin1String("recent:")))
        return {};

    const QStorageInfo storage(path);
    if (!storage.isValid() || !storage.isReady())
        return {};

    return formatSize(storage.bytesAvailable());
}

QVariantList FileUtils::templates() {
    QVariantList entries;

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TemplatesLocation);
    if (dir.isEmpty() || !QDir(dir).exists())
        return entries;

    const QFileInfoList files = QDir(dir).entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& fi : files) {
        if (entries.size() >= 50)
            break;

        QVariantMap entry;
        entry.insert(QStringLiteral("name"), fi.fileName());
        entry.insert(QStringLiteral("path"), fi.absoluteFilePath());
        const QString mime = mimeTypeForFile(fi.absoluteFilePath());
        QString glyph = QStringLiteral("description");
        if (mime.startsWith(QLatin1String("image/")))
            glyph = QStringLiteral("image");
        else if (mime.startsWith(QLatin1String("audio/")))
            glyph = QStringLiteral("music_note");
        else if (mime.startsWith(QLatin1String("video/")))
            glyph = QStringLiteral("movie");
        else if (mime.contains(QLatin1String("shellscript")) || mime.contains(QLatin1String("executable")))
            glyph = QStringLiteral("terminal");
        else if (mime.contains(QLatin1String("spreadsheet")) || mime.contains(QLatin1String("csv")))
            glyph = QStringLiteral("table");
        else if (mime.contains(QLatin1String("presentation")))
            glyph = QStringLiteral("slideshow");
        else if (mime.contains(QLatin1String("pdf")))
            glyph = QStringLiteral("picture_as_pdf");

        entry.insert(QStringLiteral("icon"), glyph);
        entries.append(entry);
    }

    return entries;
}

bool FileUtils::isImage(const QString& path) {
    if (path.isEmpty()) return false;
    static const QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(path);
    return mime.name().startsWith(QLatin1String("image/"));
}

bool FileUtils::isVideo(const QString& path) {
    if (path.isEmpty()) return false;
    static const QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(path);
    return mime.name().startsWith(QLatin1String("video/"));
}

bool FileUtils::isAudio(const QString& path) {
    if (path.isEmpty()) return false;
    static const QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(path);
    return mime.name().startsWith(QLatin1String("audio/"));
}

QString FileUtils::iconForName(const QString& name, const QString& fallback) {
    if (QIcon::hasThemeIcon(name)) {
        return QString("image://icon/") + name;
    }
    if (!fallback.isEmpty() && QIcon::hasThemeIcon(fallback)) {
        return QString("image://icon/") + fallback;
    }
    return QString("image://icon/") + (fallback.isEmpty() ? name : fallback);
}

QString FileUtils::iconForFile(const QString& name, bool isDir, const QString& mimeType, const QString& path) {
    if (isDir) {
        QString fullPath = path;
        if (fullPath.isEmpty() && !name.isEmpty()) {
            fullPath = expandPath(name);
        }

        const QString lower = name.toLower();

        // Check path & name against known special folder types
        if (lower == QLatin1String("downloads") ||
            (!fullPath.isEmpty() && fullPath == QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))) {
            return QStringLiteral("image://icon/folder-download");
        }
        if (lower == QLatin1String("documents") ||
            (!fullPath.isEmpty() && fullPath == QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))) {
            return QStringLiteral("image://icon/folder-documents");
        }
        if (lower == QLatin1String("pictures") ||
            (!fullPath.isEmpty() && fullPath == QStandardPaths::writableLocation(QStandardPaths::PicturesLocation))) {
            return QStringLiteral("image://icon/folder-pictures");
        }
        if (lower == QLatin1String("music") ||
            (!fullPath.isEmpty() && fullPath == QStandardPaths::writableLocation(QStandardPaths::MusicLocation))) {
            return QStringLiteral("image://icon/folder-music");
        }
        if (lower == QLatin1String("videos") || lower == QLatin1String("movies") ||
            (!fullPath.isEmpty() && fullPath == QStandardPaths::writableLocation(QStandardPaths::MoviesLocation))) {
            return QStringLiteral("image://icon/folder-videos");
        }
        if (lower == QLatin1String("desktop") ||
            (!fullPath.isEmpty() && fullPath == QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))) {
            return QStringLiteral("image://icon/folder-desktop");
        }
        if (lower == QLatin1String("templates") ||
            (!fullPath.isEmpty() && fullPath == QStandardPaths::writableLocation(QStandardPaths::TemplatesLocation))) {
            return QStringLiteral("image://icon/folder-templates");
        }
        if (lower == QLatin1String("public") ||
            (!fullPath.isEmpty() && fullPath == QStandardPaths::writableLocation(QStandardPaths::PublicShareLocation))) {
            return QStringLiteral("image://icon/folder-publicshare");
        }
        if (lower == QLatin1String("projects") || lower == QLatin1String("code") || lower == QLatin1String("dev")) {
            return QStringLiteral("image://icon/folder-development");
        }
        if (lower == QLatin1String("games")) {
            return QStringLiteral("image://icon/folder-games");
        }
        if (lower == QLatin1String("applications")) {
            return QStringLiteral("image://icon/folder-applications");
        }
        if (!fullPath.isEmpty() && fullPath == QDir::homePath()) {
            return QStringLiteral("image://icon/user-home");
        }

        return QStringLiteral("image://icon/inode-directory");
    }

    if (name.endsWith(".bak", Qt::CaseInsensitive) || name.endsWith("~") ||
        name.endsWith(".backup", Qt::CaseInsensitive) || name.endsWith(".old", Qt::CaseInsensitive)) {
        return QStringLiteral("image://icon/application-x-backup");
    }

    QString mimeIcon = mimeType;
    mimeIcon.replace('/', '-');
    return QStringLiteral("image://icon/") + mimeIcon;
}

QString FileUtils::mimeTypeForFile(const QString& path) {
    if (path.isEmpty()) return QString();
    static const QMimeDatabase db;
    return db.mimeTypeForFile(path).name();
}

QString FileUtils::expandPath(const QString& input, const QString& currentDir) {
    QString p = input.trimmed();
    if (p.isEmpty()) return p;

    QString home = QDir::homePath();

    if (p == QLatin1String("~") || p == QLatin1String("~/")) {
        p = home;
    } else if (p.startsWith(QLatin1String("~/"))) {
        p = home + "/" + p.mid(2);
    } else if (p.startsWith(QLatin1String("~"))) {
        int slashIdx = p.indexOf('/');
        QString firstPart = (slashIdx == -1) ? p.mid(1) : p.mid(1, slashIdx - 1);
        QString rest = (slashIdx == -1) ? QString() : p.mid(slashIdx);

        QString otherUserHome = "/home/" + firstPart;
        if (QDir(otherUserHome).exists() && !firstPart.isEmpty()) {
            p = otherUserHome + rest;
        } else {
            p = home + "/" + firstPart + rest;
        }
    }

    static const QRegularExpression envRegex(QStringLiteral(R"(\$(?:([A-Za-z_][A-Za-z0-9_]*)|(?:\{([A-Za-z_][A-Za-z0-9_]*)\})))"));
    auto match = envRegex.match(p);
    while (match.hasMatch()) {
        QString varName = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
        QString val = qEnvironmentVariable(varName.toLocal8Bit().constData());
        if (val.isEmpty() && (varName == "HOME" || varName == "home")) {
            val = home;
        }
        p.replace(match.capturedStart(0), match.capturedLength(0), val);
        match = envRegex.match(p);
    }

    if (!p.startsWith('/') && !currentDir.isEmpty()) {
        p = currentDir + "/" + p;
    }

    bool trailingSlash = p.endsWith('/');
    p = QDir::cleanPath(p);
    if (trailingSlash && !p.endsWith('/') && p != "/") {
        p += "/";
    }

    return p;
}

QVariantList FileUtils::getPathSuggestions(const QString& input, const QString& currentDir) {
    QVariantList results;
    QString trimmed = input.trimmed();
    QString home = QDir::homePath();

    QString displayPrefix;
    QString searchDirStr;
    QString filterPrefix;

    if (trimmed.isEmpty()) {
        displayPrefix = "";
        searchDirStr = currentDir.isEmpty() ? home : currentDir;
        filterPrefix = "";
    } else if (trimmed == "~" || trimmed == "~/") {
        displayPrefix = "~/";
        filterPrefix = "";
        searchDirStr = home;
    } else if (trimmed.startsWith("~/")) {
        int lastSlash = trimmed.lastIndexOf('/');
        displayPrefix = trimmed.left(lastSlash + 1);
        filterPrefix = trimmed.mid(lastSlash + 1);
        searchDirStr = expandPath(displayPrefix, currentDir);
    } else if (trimmed.startsWith("~")) {
        displayPrefix = "~/";
        filterPrefix = trimmed.mid(1);
        searchDirStr = home;
    } else {
        int lastSlash = trimmed.lastIndexOf('/');
        if (lastSlash != -1) {
            displayPrefix = trimmed.left(lastSlash + 1);
            filterPrefix = trimmed.mid(lastSlash + 1);
            searchDirStr = expandPath(displayPrefix, currentDir);
        } else {
            displayPrefix = "";
            filterPrefix = trimmed;
            searchDirStr = currentDir.isEmpty() ? home : currentDir;
        }
    }

    QDir searchDir(searchDirStr);
    if (!searchDir.exists()) {
        return results;
    }

    const auto entries = searchDir.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase
    );

    int count = 0;
    for (const auto& entry : entries) {
        if (count >= 30) break;

        QString name = entry.fileName();
        if (!filterPrefix.isEmpty() && !name.startsWith(filterPrefix, Qt::CaseInsensitive)) {
            continue;
        }

        bool isDir = entry.isDir();
        QString disp = displayPrefix + name + (isDir ? "/" : "");
        QString full = entry.absoluteFilePath() + (isDir ? "/" : "");
        QString icon = isDir ? "folder" : "description";
        if (!isDir) {
            QMimeDatabase db;
            QString mime = db.mimeTypeForFile(entry.absoluteFilePath()).name();
            if (mime.startsWith("image/")) icon = "image";
            else if (mime.startsWith("video/")) icon = "movie";
            else if (mime.startsWith("audio/")) icon = "audiotrack";
            else if (mime.contains("compressed") || mime.contains("zip") || mime.contains("tar") || mime.contains("7z")) icon = "archive";
            else if (mime.contains("pdf")) icon = "picture_as_pdf";
            else if (mime.startsWith("text/")) icon = "description";
        }

        QVariantMap m;
        m["name"] = name + (isDir ? "/" : "");
        m["displayPath"] = disp;
        m["fullPath"] = full;
        m["isDir"] = isDir;
        m["icon"] = icon;
        results.append(m);
        count++;
    }

    return results;
}

QString FileUtils::getCompletedPath(const QString& input, const QString& currentDir) {
    QVariantList suggestions = getPathSuggestions(input, currentDir);
    if (suggestions.isEmpty()) {
        return input;
    }

    if (suggestions.size() == 1) {
        return suggestions.first().toMap()["displayPath"].toString();
    }

    QString first = suggestions.first().toMap()["displayPath"].toString();
    int commonLen = first.length();

    for (int i = 1; i < suggestions.size(); ++i) {
        QString s = suggestions[i].toMap()["displayPath"].toString();
        int j = 0;
        while (j < commonLen && j < s.length() && first[j].toLower() == s[j].toLower()) {
            j++;
        }
        commonLen = j;
    }

    if (commonLen > input.length()) {
        return first.left(commonLen);
    }

    return suggestions.first().toMap()["displayPath"].toString();
}

int FileUtils::queryKeyboardModifiers() {
    return static_cast<int>(QGuiApplication::queryKeyboardModifiers());
}

} // namespace wormhole::core