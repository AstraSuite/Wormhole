#include "livescreenitem.hpp"
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

namespace wormhole::screencast {

LiveScreenItem::LiveScreenItem(QQuickItem* parent)
    : QQuickPaintedItem(parent) {
    setOpaquePainting(false);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setPerformanceHint(QQuickPaintedItem::FastFBOResizing, true);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &LiveScreenItem::grabFrame);

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus /*status*/) {
        m_busy = false;
        if (exitCode == 0 && m_process) {
            QByteArray data = m_process->readAllStandardOutput();
            if (!data.isEmpty()) {
                QBuffer buf(&data);
                buf.open(QIODevice::ReadOnly);
                QImageReader reader(&buf, "ppm");
                QImage img = reader.read();
                if (!img.isNull()) {
                    m_currentFrame = img;
                    update();
                }
            }
        }
    });

    updateTimerState();
}

LiveScreenItem::~LiveScreenItem() {
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
}

void LiveScreenItem::setTarget(const QString& t) {
    if (m_target != t) {
        m_target = t;
        emit targetChanged();
        grabFrame();
    }
}

void LiveScreenItem::setLive(bool l) {
    if (m_live != l) {
        m_live = l;
        emit liveChanged();
        updateTimerState();
    }
}

void LiveScreenItem::setFps(int f) {
    if (m_fps != f) {
        m_fps = qBound(1, f, 60);
        emit fpsChanged();
        updateTimerState();
    }
}

void LiveScreenItem::setRadius(qreal r) {
    if (!qFuzzyCompare(m_radius, r)) {
        m_radius = r;
        emit radiusChanged();
        update();
    }
}

void LiveScreenItem::setIsWindow(bool w) {
    if (m_isWindow != w) {
        m_isWindow = w;
        emit isWindowChanged();
    }
}

void LiveScreenItem::setWindowGeometry(const QString& g) {
    if (m_windowGeometry != g) {
        m_windowGeometry = g;
        emit windowGeometryChanged();
    }
}

void LiveScreenItem::updateTimerState() {
    if (m_live && isVisible() && !m_target.isEmpty()) {
        int interval = 1000 / m_fps;
        if (m_timer->interval() != interval || !m_timer->isActive()) {
            m_timer->start(interval);
        }
    } else {
        m_timer->stop();
    }
}

void LiveScreenItem::itemChange(ItemChange change, const ItemChangeData& value) {
    QQuickPaintedItem::itemChange(change, value);
    if (change == ItemVisibleHasChanged) {
        updateTimerState();
        if (isVisible()) {
            grabFrame();
        }
    }
}

void LiveScreenItem::grabFrame() {
    if (!m_live || !isVisible() || m_target.isEmpty() || m_busy) {
        return;
    }

    if (m_process->state() != QProcess::NotRunning) {
        return;
    }

    m_busy = true;

    QStringList args;
    args << QStringLiteral("-t") << QStringLiteral("ppm")
         << QStringLiteral("-s") << QStringLiteral("0.75");

    if (m_isWindow && !m_windowGeometry.isEmpty()) {
        args << QStringLiteral("-g") << m_windowGeometry;
    } else {
        args << QStringLiteral("-o") << m_target;
    }
    args << QStringLiteral("-");

    m_process->start(QStringLiteral("grim"), args);
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
