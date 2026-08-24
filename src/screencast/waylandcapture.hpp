#pragma once

#include <QImage>
#include <QList>
#include <QObject>
#include <QSize>
#include <QString>
#include <cstdint>
#include <functional>

struct wl_buffer;
struct wl_display;
struct wl_output;
struct wl_registry;
struct wl_shm;
struct wl_shm_pool;
struct ext_foreign_toplevel_handle_v1;
struct ext_foreign_toplevel_image_capture_source_manager_v1;
struct ext_foreign_toplevel_list_v1;
struct ext_image_capture_source_v1;
struct ext_output_image_capture_source_manager_v1;
struct ext_image_copy_capture_frame_v1;
struct ext_image_copy_capture_manager_v1;
struct ext_image_copy_capture_session_v1;

class QSocketNotifier;

namespace wormhole::screencast {

class WaylandCapture : public QObject {
    Q_OBJECT

public:
    explicit WaylandCapture(QObject* parent = nullptr);
    ~WaylandCapture() override;

    bool captureOutput(const QString& outputName, bool paintCursor, int fps);
    bool captureToplevel(const QString& appId, const QString& title, bool paintCursor, int fps);
    void stop();

    QImage grabOutput(const QString& outputName, bool paintCursor, int timeoutMs = 2000);
    QImage grabToplevel(const QString& appId, const QString& title, bool paintCursor, int timeoutMs = 2000);

    QSize frameSize() const { return m_frameSize; }

signals:
    void frameReady(const QImage& frame);
    void frameSizeChanged(const QSize& size);
    void stopped();

private:
    struct OutputEntry {
        wl_output* output = nullptr;
        QString name;
    };

    struct ToplevelEntry {
        ext_foreign_toplevel_handle_v1* handle = nullptr;
        QString appId;
        QString title;
        bool closed = false;
    };

    bool connectDisplay();
    void disconnectDisplay();
    void releaseNotifier();
    bool beginSession(ext_image_capture_source_v1* source, bool paintCursor, int fps, bool live);
    void endSession();

    bool waitFor(const std::function<bool()>& predicate, int timeoutMs);
    QImage grabOnce(int timeoutMs);

    wl_output* findOutput(const QString& name) const;
    ext_foreign_toplevel_handle_v1* findToplevel(const QString& appId, const QString& title);

    bool allocateBuffer();
    void releaseBuffer();

    void scheduleFrame();
    void requestFrame();
    void destroyFrame();

    void onDisplayReadable();
    void onConstraintsDone();
    void onFrameReady();
    void onFrameFailed(uint32_t reason);
    void onSessionStopped();

    QImage currentImage() const;

    static void handleGlobal(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    static void handleGlobalRemove(void* data, wl_registry* registry, uint32_t name);
    static void handleOutputName(void* data, wl_output* output, const char* name);
    static void handleToplevel(void* data, ext_foreign_toplevel_list_v1* list, ext_foreign_toplevel_handle_v1* handle);
    static void handleToplevelAppId(void* data, ext_foreign_toplevel_handle_v1* handle, const char* appId);
    static void handleToplevelTitle(void* data, ext_foreign_toplevel_handle_v1* handle, const char* title);
    static void handleToplevelClosed(void* data, ext_foreign_toplevel_handle_v1* handle);
    static void handleBufferSize(void* data, ext_image_copy_capture_session_v1* session, uint32_t width, uint32_t height);
    static void handleShmFormat(void* data, ext_image_copy_capture_session_v1* session, uint32_t format);
    static void handleConstraintsDone(void* data, ext_image_copy_capture_session_v1* session);
    static void handleSessionStopped(void* data, ext_image_copy_capture_session_v1* session);
    static void handleFrameTransform(void* data, ext_image_copy_capture_frame_v1* frame, uint32_t transform);
    static void handleFrameReady(void* data, ext_image_copy_capture_frame_v1* frame);
    static void handleFrameFailed(void* data, ext_image_copy_capture_frame_v1* frame, uint32_t reason);

    wl_display* m_display = nullptr;
    wl_registry* m_registry = nullptr;
    wl_shm* m_shm = nullptr;
    ext_output_image_capture_source_manager_v1* m_outputSources = nullptr;
    ext_foreign_toplevel_image_capture_source_manager_v1* m_toplevelSources = nullptr;
    ext_image_copy_capture_manager_v1* m_captureManager = nullptr;
    ext_foreign_toplevel_list_v1* m_toplevelList = nullptr;
    QSocketNotifier* m_notifier = nullptr;

    QList<OutputEntry> m_outputs;
    QList<ToplevelEntry> m_toplevels;

    ext_image_capture_source_v1* m_source = nullptr;
    ext_image_copy_capture_session_v1* m_session = nullptr;
    ext_image_copy_capture_frame_v1* m_frame = nullptr;

    wl_shm_pool* m_pool = nullptr;
    wl_buffer* m_buffer = nullptr;
    uint8_t* m_data = nullptr;
    size_t m_dataSize = 0;
    int m_poolFd = -1;

    QImage m_grabbed;
    QSize m_bufferSize;
    QSize m_frameSize;
    uint32_t m_shmFormat = 0;
    uint32_t m_transform = 0;
    bool m_haveFormat = false;
    bool m_constraintsDone = false;
    bool m_bufferValid = false;
    bool m_capturePending = false;
    bool m_live = false;
    bool m_grabDone = false;
    int m_frameIntervalMs = 16;
    qint64 m_lastCaptureMs = 0;
};

} // namespace wormhole::screencast
