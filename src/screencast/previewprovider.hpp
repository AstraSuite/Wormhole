#pragma once

#include <QCache>
#include <QColor>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QQuickImageProvider>

#include "waylandcapture.hpp"

namespace wormhole::screencast {

class ScreenPreviewProvider : public QQuickImageProvider {
public:
    static constexpr int kMaxPreviewWidth = 1280;

    ScreenPreviewProvider()
        : QQuickImageProvider(QQuickImageProvider::Image) {
        m_cache.setMaxCost(10);
    }

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override {
        const QString outputName = id.section(QLatin1Char('?'), 0, 0);

        {
            QMutexLocker locker(&m_mutex);
            if (const QImage* cached = m_cache.object(outputName)) {
                if (!cached->isNull()) {
                    if (size) *size = cached->size();
                    return *cached;
                }
            }
        }

        QImage frame;
        {
            QMutexLocker locker(&m_mutex);
            WaylandCapture capture;
            frame = capture.grabOutput(outputName, false);
        }

        if (!frame.isNull()) {
            if (requestedSize.width() > 0 && requestedSize.height() > 0) {
                frame = frame.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            } else if (frame.width() > kMaxPreviewWidth) {
                frame = frame.scaledToWidth(kMaxPreviewWidth, Qt::SmoothTransformation);
            }
            if (size) *size = frame.size();

            QMutexLocker locker(&m_mutex);
            m_cache.insert(outputName, new QImage(frame));
            return frame;
        }

        const int fallbackWidth = requestedSize.width() > 0 ? requestedSize.width() : 1280;
        const int fallbackHeight = requestedSize.height() > 0 ? requestedSize.height() : 720;
        QImage fallback(fallbackWidth, fallbackHeight, QImage::Format_ARGB32_Premultiplied);
        fallback.fill(QColor(19, 27, 26));
        if (size) *size = fallback.size();
        return fallback;
    }

private:
    QCache<QString, QImage> m_cache;
    QMutex m_mutex;
};

} // namespace wormhole::screencast
