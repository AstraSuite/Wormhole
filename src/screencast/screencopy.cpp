#include "screencopy.hpp"
#include <QBuffer>
#include <QDebug>
#include <QGuiApplication>
#include <QProcess>
#include <QRegularExpression>
#include <algorithm>

namespace wormhole::screencast {

ScreenCaptureWorker::ScreenCaptureWorker(uint32_t nodeId, const QString& sourceName, bool isWindow, const QString& windowAddress, QObject* parent)
    : QObject(parent), m_nodeId(nodeId), m_sourceName(sourceName), m_isWindow(isWindow), m_windowAddress(windowAddress) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ScreenCaptureWorker::captureFrame);
}

ScreenCaptureWorker::~ScreenCaptureWorker() {
    stop();
}

void ScreenCaptureWorker::start(int fps) {
    m_fps = std::clamp(fps > 0 ? fps : 30, 15, 60);
    int intervalMs = std::max(33, 1000 / m_fps);
    m_timer->start(intervalMs);
}

void ScreenCaptureWorker::stop() {
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
    m_busy.store(false);
}

void ScreenCaptureWorker::captureFrame() {
    if (m_nodeId == 0) return;

    if (m_busy.exchange(true)) {
        return; // Guard against overlapping capture runs
    }

    QProcess grimProc;
    QStringList args;
    if (!m_isWindow && !m_sourceName.isEmpty()) {
        args << QStringLiteral("-o") << m_sourceName;
    }
    args << QStringLiteral("-t") << QStringLiteral("ppm") << QStringLiteral("-");

    grimProc.start(QStringLiteral("grim"), args);
    if (grimProc.waitForFinished(120)) {
        QByteArray data = grimProc.readAllStandardOutput();
        if (data.size() > 16 && data.startsWith("P6")) {
            // Fast direct PPM parser
            int pos = 0;
            int newlines = 0;
            while (newlines < 3 && pos < data.size()) {
                if (data[pos++] == '\n') newlines++;
            }

            QString header = QString::fromLatin1(data.constData(), pos);
            QStringList tokens = header.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            if (tokens.size() >= 3) {
                int w = tokens[1].toInt();
                int h = tokens[2].toInt();
                int expectedBytes = w * h * 3;
                if (w > 0 && h > 0 && (data.size() - pos) >= expectedBytes) {
                    const uchar* pixels = reinterpret_cast<const uchar*>(data.constData() + pos);
                    QImage img(pixels, w, h, w * 3, QImage::Format_RGB888);
                    PipeWireStreamManager::instance()->pushFrame(m_nodeId, img);
                }
            }
        }
    } else {
        grimProc.kill();
        grimProc.waitForFinished(20);
    }

    m_busy.store(false);
}

} // namespace wormhole::screencast
