#pragma once

#include <QImage>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QString>
#include <cstdint>
#include <memory>

struct pw_thread_loop;
struct pw_context;
struct pw_core;
struct pw_stream;
struct spa_hook;

namespace wormhole::screencast {

struct StreamContext {
    uint32_t nodeId = 0;
    pw_stream* stream = nullptr;
    spa_hook* streamListener = nullptr;
    pw_thread_loop* managerLoop = nullptr;
    int width = 1920;
    int height = 1080;
    int fps = 60;
    uint64_t seq = 0;
    bool active = false;
};

class PipeWireStreamManager : public QObject {
    Q_OBJECT

public:
    static PipeWireStreamManager* instance();

    bool initialize();
    void cleanup();

    uint32_t createStream(const QString& title, int width, int height, int fps = 60);
    void stopStream(uint32_t nodeId);
    void pushFrame(uint32_t nodeId, const QImage& frame);

    int getRemoteFd();
    pw_context* context() const { return m_context; }
    pw_thread_loop* loop() const { return m_loop; }

signals:
    void streamStarted(uint32_t nodeId);
    void streamStopped(uint32_t nodeId);

private:
    explicit PipeWireStreamManager(QObject* parent = nullptr);
    ~PipeWireStreamManager() override;

    pw_thread_loop* m_loop = nullptr;
    pw_context* m_context = nullptr;
    pw_core* m_core = nullptr;
    spa_hook* m_coreListener = nullptr;

    QMap<uint32_t, std::shared_ptr<StreamContext>> m_streams;
    QMutex m_mutex;
    bool m_initialized = false;
};

} // namespace wormhole::screencast
