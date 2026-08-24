#include "livescreenitem.hpp"

#include <QColor>

namespace wormhole::screencast {

LiveScreenItem::LiveScreenItem(QQuickItem* parent)
    : QQuickPaintedItem(parent) {
    setOpaquePainting(false);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setPerformanceHint(QQuickPaintedItem::FastFBOResizing, true);
}

LiveScreenItem::~LiveScreenItem() {
    stopCapture();
}

void LiveScreenItem::setTarget(const QString& target) {
    if (m_target != target) {
        m_target = target;
        emit targetChanged();
        restart();
    }
}

void LiveScreenItem::setLive(bool live) {
    if (m_live != live) {
        m_live = live;
        emit liveChanged();
        restart();
    }
}

void LiveScreenItem::setFps(int fps) {
    const int bounded = qBound(1, fps, 60);
    if (m_fps != bounded) {
        m_fps = bounded;
        emit fpsChanged();
        restart();
    }
}

void LiveScreenItem::setRadius(qreal radius) {
    if (!qFuzzyCompare(m_radius, radius)) {
        m_radius = radius;
        emit radiusChanged();
        update();
    }
}

void LiveScreenItem::setIsWindow(bool isWindow) {
    if (m_isWindow != isWindow) {
        m_isWindow = isWindow;
        emit isWindowChanged();
        restart();
    }
}

void LiveScreenItem::setAppId(const QString& appId) {
    if (m_appId != appId) {
        m_appId = appId;
        emit appIdChanged();
        restart();
    }
}

void LiveScreenItem::setTitle(const QString& title) {
    if (m_title != title) {
        m_title = title;
        emit titleChanged();
        restart();
    }
}

void LiveScreenItem::setIncludeCursor(bool includeCursor) {
    if (m_includeCursor != includeCursor) {
        m_includeCursor = includeCursor;
        emit includeCursorChanged();
        restart();
    }
}

void LiveScreenItem::itemChange(ItemChange change, const ItemChangeData& value) {
    QQuickPaintedItem::itemChange(change, value);
    if (change == ItemVisibleHasChanged) {
        restart();
    }
}

void LiveScreenItem::stopCapture() {
    if (m_capture) {
        m_capture->stop();
        m_capture->deleteLater();
        m_capture = nullptr;
    }
}

void LiveScreenItem::restart() {
    stopCapture();

    const bool wanted = m_live && isVisible() && (m_isWindow ? !m_appId.isEmpty() : !m_target.isEmpty());
    if (!wanted) {
        return;
    }

    m_capture = new WaylandCapture(this);
    connect(m_capture, &WaylandCapture::frameReady, this, &LiveScreenItem::onFrame);
    connect(m_capture, &WaylandCapture::stopped, this, &LiveScreenItem::stopCapture);

    const bool started = m_isWindow
        ? m_capture->captureToplevel(m_appId, m_title, m_includeCursor, m_fps)
        : m_capture->captureOutput(m_target, m_includeCursor, m_fps);

    if (!started) {
        stopCapture();
    }
}

void LiveScreenItem::onFrame(const QImage& frame) {
    m_currentFrame = frame.copy();
    update();
}

void LiveScreenItem::paint(QPainter* painter) {
    const QRectF rect = boundingRect();
    if (rect.isEmpty()) return;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (m_radius > 0.0) {
        QPainterPath clipPath;
        clipPath.addRoundedRect(rect, m_radius, m_radius);
        painter->setClipPath(clipPath);
    }

    if (m_currentFrame.isNull()) {
        painter->fillRect(rect, QColor(19, 27, 26));
        return;
    }

    painter->drawImage(rect, m_currentFrame);
}

} // namespace wormhole::screencast
