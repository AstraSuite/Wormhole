#pragma once

#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMimeDatabase>
#include <QPixmap>
#include <QQuickImageProvider>
#include <QUrl>

namespace wormhole::core {

class IconImageProvider : public QQuickImageProvider {
public:
    IconImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override {
        const int width = requestedSize.width() > 0 ? requestedSize.width() : 48;
        const int height = requestedSize.height() > 0 ? requestedSize.height() : 48;

        if (size) {
            *size = QSize(width, height);
        }

        QString cleanId = id;

        // 1. Strip query strings (?t=...) and URL schemes if passed through
        const int queryIndex = cleanId.indexOf('?');
        if (queryIndex != -1) {
            cleanId = cleanId.left(queryIndex);
        }
        if (cleanId.startsWith(QLatin1String("image://icon/"))) {
            cleanId.remove(0, 13);
        } else if (cleanId.startsWith(QLatin1String("image:/icon/"))) {
            cleanId.remove(0, 12);
        } else if (cleanId.startsWith(QLatin1String("file://"))) {
            cleanId = QUrl(cleanId).toLocalFile();
        }

        // 2. Direct absolute image / icon file path
        if (cleanId.startsWith(QLatin1String("/")) && QFile::exists(cleanId)) {
            QPixmap pix(cleanId);
            if (!pix.isNull()) {
                return pix.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }

        // 3. Build candidate list for fallback resolution
        QStringList candidates;
        candidates.append(cleanId);

        // Normalize hyphen/slash variations
        QString withHyphen = cleanId;
        withHyphen.replace('/', '-');
        if (!candidates.contains(withHyphen)) candidates.append(withHyphen);

        QString withSlash = cleanId;
        withSlash.replace('-', '/');
        if (!candidates.contains(withSlash)) candidates.append(withSlash);

        // Specific fallbacks for folders
        if (cleanId.contains(QLatin1String("download"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("folder-download") << QStringLiteral("folder-downloads") << QStringLiteral("folder-download-symbolic") << QStringLiteral("download");
        } else if (cleanId.contains(QLatin1String("document"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("folder-documents") << QStringLiteral("folder-document") << QStringLiteral("folder-text");
        } else if (cleanId.contains(QLatin1String("picture"), Qt::CaseInsensitive) || cleanId.contains(QLatin1String("image"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("folder-pictures") << QStringLiteral("folder-picture") << QStringLiteral("folder-image") << QStringLiteral("folder-images");
        } else if (cleanId.contains(QLatin1String("music"), Qt::CaseInsensitive) || cleanId.contains(QLatin1String("sound"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("folder-music") << QStringLiteral("folder-sound") << QStringLiteral("folder-audio");
        } else if (cleanId.contains(QLatin1String("video"), Qt::CaseInsensitive) || cleanId.contains(QLatin1String("movie"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("folder-videos") << QStringLiteral("folder-video") << QStringLiteral("folder-movies");
        } else if (cleanId.contains(QLatin1String("desktop"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("folder-desktop") << QStringLiteral("user-desktop");
        } else if (cleanId.contains(QLatin1String("config"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("folder-config") << QStringLiteral("folder-settings");
        } else if (cleanId.contains(QLatin1String("game"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("folder-games") << QStringLiteral("folder-steam");
        } else if (cleanId.contains(QLatin1String("project"), Qt::CaseInsensitive) || cleanId.contains(QLatin1String("dev"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("folder-development") << QStringLiteral("folder-code") << QStringLiteral("folder-git") << QStringLiteral("folder-projects");
        }

        // Specific script / source code fallbacks (e.g. .lua, .py, .sh)
        if (cleanId.contains(QLatin1String("lua"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("text-x-lua") << QStringLiteral("application-x-lua") << QStringLiteral("text-x-script") << QStringLiteral("text-x-generic");
        } else if (cleanId.contains(QLatin1String("python"), Qt::CaseInsensitive) || cleanId.contains(QLatin1String("py"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("text-x-python") << QStringLiteral("text-x-python3") << QStringLiteral("text-x-script") << QStringLiteral("text-x-generic");
        } else if (cleanId.contains(QLatin1String("shell"), Qt::CaseInsensitive) || cleanId.contains(QLatin1String("sh"), Qt::CaseInsensitive)) {
            candidates << QStringLiteral("application-x-shellscript") << QStringLiteral("text-x-script") << QStringLiteral("utilities-terminal") << QStringLiteral("text-x-generic");
        }

        // Generic fallbacks
        if (cleanId.startsWith(QLatin1String("folder")) || cleanId.startsWith(QLatin1String("inode-directory"))) {
            candidates << QStringLiteral("folder") << QStringLiteral("inode-directory");
        } else {
            candidates << QStringLiteral("text-x-generic") << QStringLiteral("application-x-zerosize") << QStringLiteral("unknown");
        }

        // 4. Resolve the first candidate with an actual rendered pixmap
        for (const QString& candidate : candidates) {
            if (candidate.isEmpty()) continue;

            QIcon icon = QIcon::fromTheme(candidate);
            QPixmap pix = icon.pixmap(width, height);
            if (!pix.isNull()) {
                return pix;
            }
        }

        // 5. Final fallback if theme rendering returned null
        QIcon fallbackIcon = QIcon::fromTheme(QStringLiteral("folder"));
        QPixmap pix = fallbackIcon.pixmap(width, height);
        if (!pix.isNull()) {
            return pix;
        }

        QPixmap transparent(width, height);
        transparent.fill(Qt::transparent);
        return transparent;
    }
};

} // namespace wormhole::core