#pragma once

#include <QImage>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <atomic>
#include "pipewirestream.hpp"

namespace wormhole::screencast {

class ScreenCaptureWorker : public QObject {
    Q_OBJECT

public:
    explicit ScreenCaptureWorker(uint32_t nodeId, const QString& sourceName, bool isWindow, const QString& windowAddress, QObject* parent = nullptr);
    ~ScreenCaptureWorker() override;

    void start(int fps = 60);
    void stop();

private slots:
    void captureFrame();

private:
    uint32_t m_nodeId = 0;
    QString m_sourceName;
    bool m_isWindow = false;
    QString m_windowAddress;
    QTimer* m_timer = nullptr;
    int m_fps = 60;
    std::atomic<bool> m_busy{false};
};

} // namespace wormhole::screencast
