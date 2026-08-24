#pragma once

#include <QBuffer>
#include <QCache>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QImageReader>
#include <QProcess>
#include <QQuickImageProvider>

namespace wormhole::screencast {

class ScreenPreviewProvider : public QQuickImageProvider {
public:
    ScreenPreviewProvider()
        : QQuickImageProvider(QQuickImageProvider::Image) {
        m_cache.setMaxCost(10);
    }

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override {
        QString cleanId = id;
        int qIdx = cleanId.indexOf(QLatin1Char('?'));
        if (qIdx != -1) {
            cleanId = cleanId.left(qIdx);
        }

        if (m_cache.contains(cleanId)) {
            QImage* cached = m_cache.object(cleanId);
            if (cached && !cached->isNull()) {
                if (size) *size = cached->size();
                return *cached;
            }
        }

        int targetW = requestedSize.width() > 0 ? requestedSize.width() : 1280;
        int targetH = requestedSize.height() > 0 ? requestedSize.height() : 720;

        QProcess proc;
        QStringList args;
        // Fast, high-resolution uncompressed snapshot via ppm
        args << QStringLiteral("-t") << QStringLiteral("ppm")
             << QStringLiteral("-s") << QStringLiteral("0.75")
             << QStringLiteral("-o") << cleanId
             << QStringLiteral("-");

        proc.start(QStringLiteral("grim"), args);
        if (proc.waitForFinished(1000)) {
            QByteArray data = proc.readAllStandardOutput();
            if (!data.isEmpty()) {
                QBuffer buf(&data);
                buf.open(QIODevice::ReadOnly);
                QImageReader reader(&buf, "ppm");
                QImage img = reader.read();
                if (!img.isNull()) {
                    if (size) *size = img.size();
                    m_cache.insert(cleanId, new QImage(img));
                    return img;
                }
            }
        }

        // Fallback placeholder image
        QImage fallback(targetW, targetH, QImage::Format_ARGB32_Premultiplied);
        fallback.fill(QColor(19, 27, 26));
        if (size) *size = fallback.size();
        return fallback;
    }

private:
    QCache<QString, QImage> m_cache;
};

} // namespace wormhole::screencast
