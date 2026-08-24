#pragma once

#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QQuickImageProvider>

namespace wormhole::core {

class IconImageProvider : public QQuickImageProvider {
public:
    IconImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override {
        int width = requestedSize.width() > 0 ? requestedSize.width() : 48;
        int height = requestedSize.height() > 0 ? requestedSize.height() : 48;

        if (size) {
            *size = QSize(width, height);
        }

        // Check if id is a file path
        if (id.startsWith(QLatin1String("/")) && QFile::exists(id)) {
            QPixmap pix(id);
            if (!pix.isNull()) {
                return pix.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }

        QIcon icon = QIcon::fromTheme(id);
        if (icon.isNull()) {
            icon = QIcon::fromTheme(id.toLower());
        }

        if (icon.isNull()) {
            icon = QIcon::fromTheme(QStringLiteral("application-x-executable"));
        }

        QPixmap pix = icon.pixmap(width, height);
        if (pix.isNull()) {
            QPixmap fallback(width, height);
            fallback.fill(Qt::transparent);
            return fallback;
        }
        return pix;
    }
};

} // namespace wormhole::core
