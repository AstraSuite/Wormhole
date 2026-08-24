#include "waylandcapture.hpp"

#include <QDateTime>
#include <QDebug>
#include <QSocketNotifier>
#include <QTimer>
#include <QTransform>

#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "ext-foreign-toplevel-list-v1-client-protocol.h"
#include "ext-image-capture-source-v1-client-protocol.h"
#include "ext-image-copy-capture-v1-client-protocol.h"

namespace wormhole::screencast {

namespace {

constexpr int kSetupTimeoutMs = 2000;

QImage::Format imageFormatFor(uint32_t shmFormat) {
    switch (shmFormat) {
    case WL_SHM_FORMAT_XRGB8888:
        return QImage::Format_RGB32;
    case WL_SHM_FORMAT_ARGB8888:
        return QImage::Format_ARGB32;
    case WL_SHM_FORMAT_XBGR8888:
        return QImage::Format_RGBX8888;
    case WL_SHM_FORMAT_ABGR8888:
        return QImage::Format_RGBA8888;
    default:
        return QImage::Format_Invalid;
    }
}

int formatPriority(uint32_t shmFormat) {
    switch (shmFormat) {
    case WL_SHM_FORMAT_XRGB8888:
        return 4;
    case WL_SHM_FORMAT_ARGB8888:
        return 3;
    case WL_SHM_FORMAT_XBGR8888:
        return 2;
    case WL_SHM_FORMAT_ABGR8888:
        return 1;
    default:
        return 0;
    }
}

QTransform inverseOf(uint32_t transform) {
    QTransform t;
    switch (transform) {
    case WL_OUTPUT_TRANSFORM_90:
        t.rotate(-90);
        break;
    case WL_OUTPUT_TRANSFORM_180:
        t.rotate(-180);
        break;
    case WL_OUTPUT_TRANSFORM_270:
        t.rotate(-270);
        break;
    case WL_OUTPUT_TRANSFORM_FLIPPED:
        t.scale(-1, 1);
        break;
    case WL_OUTPUT_TRANSFORM_FLIPPED_90:
        t.rotate(-90);
        t.scale(-1, 1);
        break;
    case WL_OUTPUT_TRANSFORM_FLIPPED_180:
        t.rotate(-180);
        t.scale(-1, 1);
        break;
    case WL_OUTPUT_TRANSFORM_FLIPPED_270:
        t.rotate(-270);
        t.scale(-1, 1);
        break;
    default:
        break;
    }
    return t;
}

bool swapsAxes(uint32_t transform) {
    return transform == WL_OUTPUT_TRANSFORM_90 || transform == WL_OUTPUT_TRANSFORM_270 ||
           transform == WL_OUTPUT_TRANSFORM_FLIPPED_90 || transform == WL_OUTPUT_TRANSFORM_FLIPPED_270;
}

int createAnonymousFile(size_t size) {
    int fd = memfd_create("wormhole-capture", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) {
        return -1;
    }
    if (ftruncate(fd, static_cast<off_t>(size)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

void noopOutputGeometry(void*, wl_output*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t) {}
void noopOutputMode(void*, wl_output*, uint32_t, int32_t, int32_t, int32_t) {}
void noopOutputDone(void*, wl_output*) {}
void noopOutputScale(void*, wl_output*, int32_t) {}
void noopOutputDescription(void*, wl_output*, const char*) {}

void noopToplevelDone(void*, ext_foreign_toplevel_handle_v1*) {}
void noopToplevelIdentifier(void*, ext_foreign_toplevel_handle_v1*, const char*) {}
void noopToplevelListFinished(void*, ext_foreign_toplevel_list_v1*) {}

void noopDmabufDevice(void*, ext_image_copy_capture_session_v1*, wl_array*) {}
void noopDmabufFormat(void*, ext_image_copy_capture_session_v1*, uint32_t, wl_array*) {}

void noopFrameDamage(void*, ext_image_copy_capture_frame_v1*, int32_t, int32_t, int32_t, int32_t) {}
void noopFramePresentationTime(void*, ext_image_copy_capture_frame_v1*, uint32_t, uint32_t, uint32_t) {}

} // namespace

WaylandCapture::WaylandCapture(QObject* parent)
    : QObject(parent) {
}

WaylandCapture::~WaylandCapture() {
    stop();
    disconnectDisplay();
}

void WaylandCapture::handleGlobal(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    auto* self = static_cast<WaylandCapture*>(data);
    const QByteArray iface(interface);

    if (iface == wl_shm_interface.name) {
        self->m_shm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (iface == ext_output_image_capture_source_manager_v1_interface.name) {
        self->m_outputSources = static_cast<ext_output_image_capture_source_manager_v1*>(
            wl_registry_bind(registry, name, &ext_output_image_capture_source_manager_v1_interface, 1));
    } else if (iface == ext_foreign_toplevel_image_capture_source_manager_v1_interface.name) {
        self->m_toplevelSources = static_cast<ext_foreign_toplevel_image_capture_source_manager_v1*>(
            wl_registry_bind(registry, name, &ext_foreign_toplevel_image_capture_source_manager_v1_interface, 1));
    } else if (iface == ext_image_copy_capture_manager_v1_interface.name) {
        self->m_captureManager = static_cast<ext_image_copy_capture_manager_v1*>(
            wl_registry_bind(registry, name, &ext_image_copy_capture_manager_v1_interface, 1));
    } else if (iface == ext_foreign_toplevel_list_v1_interface.name) {
        static const ext_foreign_toplevel_list_v1_listener listener = {
            .toplevel = handleToplevel,
            .finished = noopToplevelListFinished,
        };
        self->m_toplevelList = static_cast<ext_foreign_toplevel_list_v1*>(
            wl_registry_bind(registry, name, &ext_foreign_toplevel_list_v1_interface, 1));
        ext_foreign_toplevel_list_v1_add_listener(self->m_toplevelList, &listener, self);
    } else if (iface == wl_output_interface.name) {
        static const wl_output_listener listener = {
            .geometry = noopOutputGeometry,
            .mode = noopOutputMode,
            .done = noopOutputDone,
            .scale = noopOutputScale,
            .name = handleOutputName,
            .description = noopOutputDescription,
        };
        OutputEntry entry;
        entry.output = static_cast<wl_output*>(
            wl_registry_bind(registry, name, &wl_output_interface, qMin(version, 4u)));
        self->m_outputs.append(entry);
        if (version >= 4) {
            wl_output_add_listener(entry.output, &listener, self);
        }
    }
}

void WaylandCapture::handleGlobalRemove(void*, wl_registry*, uint32_t) {
}

void WaylandCapture::handleOutputName(void* data, wl_output* output, const char* name) {
    auto* self = static_cast<WaylandCapture*>(data);
    for (auto& entry : self->m_outputs) {
        if (entry.output == output) {
            entry.name = QString::fromUtf8(name);
            return;
        }
    }
}

void WaylandCapture::handleToplevel(void* data, ext_foreign_toplevel_list_v1*, ext_foreign_toplevel_handle_v1* handle) {
    auto* self = static_cast<WaylandCapture*>(data);
    static const ext_foreign_toplevel_handle_v1_listener listener = {
        .closed = handleToplevelClosed,
        .done = noopToplevelDone,
        .title = handleToplevelTitle,
        .app_id = handleToplevelAppId,
        .identifier = noopToplevelIdentifier,
    };

    ToplevelEntry entry;
    entry.handle = handle;
    self->m_toplevels.append(entry);
    ext_foreign_toplevel_handle_v1_add_listener(handle, &listener, self);
}

void WaylandCapture::handleToplevelAppId(void* data, ext_foreign_toplevel_handle_v1* handle, const char* appId) {
    auto* self = static_cast<WaylandCapture*>(data);
    for (auto& entry : self->m_toplevels) {
        if (entry.handle == handle) {
            entry.appId = QString::fromUtf8(appId);
            return;
        }
    }
}

void WaylandCapture::handleToplevelTitle(void* data, ext_foreign_toplevel_handle_v1* handle, const char* title) {
    auto* self = static_cast<WaylandCapture*>(data);
    for (auto& entry : self->m_toplevels) {
        if (entry.handle == handle) {
            entry.title = QString::fromUtf8(title);
            return;
        }
    }
}

void WaylandCapture::handleToplevelClosed(void* data, ext_foreign_toplevel_handle_v1* handle) {
    auto* self = static_cast<WaylandCapture*>(data);
    for (auto& entry : self->m_toplevels) {
        if (entry.handle == handle) {
            entry.closed = true;
            return;
        }
    }
}

void WaylandCapture::handleBufferSize(void* data, ext_image_copy_capture_session_v1*, uint32_t width, uint32_t height) {
    auto* self = static_cast<WaylandCapture*>(data);
    const QSize size(static_cast<int>(width), static_cast<int>(height));
    if (self->m_bufferSize != size) {
        self->m_bufferSize = size;
        self->m_bufferValid = false;
    }
}

void WaylandCapture::handleShmFormat(void* data, ext_image_copy_capture_session_v1*, uint32_t format) {
    auto* self = static_cast<WaylandCapture*>(data);
    if (imageFormatFor(format) == QImage::Format_Invalid) {
        return;
    }
    if (!self->m_haveFormat || formatPriority(format) > formatPriority(self->m_shmFormat)) {
        if (self->m_haveFormat && self->m_shmFormat != format) {
            self->m_bufferValid = false;
        }
        self->m_shmFormat = format;
        self->m_haveFormat = true;
    }
}

void WaylandCapture::handleConstraintsDone(void* data, ext_image_copy_capture_session_v1*) {
    static_cast<WaylandCapture*>(data)->onConstraintsDone();
}

void WaylandCapture::handleSessionStopped(void* data, ext_image_copy_capture_session_v1*) {
    static_cast<WaylandCapture*>(data)->onSessionStopped();
}

void WaylandCapture::handleFrameTransform(void* data, ext_image_copy_capture_frame_v1*, uint32_t transform) {
    static_cast<WaylandCapture*>(data)->m_transform = transform;
}

void WaylandCapture::handleFrameReady(void* data, ext_image_copy_capture_frame_v1*) {
    static_cast<WaylandCapture*>(data)->onFrameReady();
}

void WaylandCapture::handleFrameFailed(void* data, ext_image_copy_capture_frame_v1*, uint32_t reason) {
    static_cast<WaylandCapture*>(data)->onFrameFailed(reason);
}

bool WaylandCapture::connectDisplay() {
    if (m_display) {
        return true;
    }

    m_display = wl_display_connect(nullptr);
    if (!m_display) {
        qWarning() << "Wormhole: no Wayland display available for screen capture";
        return false;
    }

    static const wl_registry_listener listener = {
        .global = handleGlobal,
        .global_remove = handleGlobalRemove,
    };

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &listener, this);
    wl_display_roundtrip(m_display);
    wl_display_roundtrip(m_display);

    if (!m_captureManager) {
        qWarning() << "Wormhole: compositor does not implement ext-image-copy-capture-v1";
    }

    return true;
}

void WaylandCapture::releaseNotifier() {
    if (m_notifier) {
        m_notifier->setEnabled(false);
        m_notifier->deleteLater();
        m_notifier = nullptr;
    }
}

void WaylandCapture::disconnectDisplay() {
    releaseNotifier();

    for (auto& entry : m_toplevels) {
        if (entry.handle) {
            ext_foreign_toplevel_handle_v1_destroy(entry.handle);
        }
    }
    m_toplevels.clear();

    for (auto& entry : m_outputs) {
        if (entry.output) {
            wl_output_destroy(entry.output);
        }
    }
    m_outputs.clear();

    if (m_toplevelList) {
        ext_foreign_toplevel_list_v1_destroy(m_toplevelList);
        m_toplevelList = nullptr;
    }
    if (m_captureManager) {
        ext_image_copy_capture_manager_v1_destroy(m_captureManager);
        m_captureManager = nullptr;
    }
    if (m_toplevelSources) {
        ext_foreign_toplevel_image_capture_source_manager_v1_destroy(m_toplevelSources);
        m_toplevelSources = nullptr;
    }
    if (m_outputSources) {
        ext_output_image_capture_source_manager_v1_destroy(m_outputSources);
        m_outputSources = nullptr;
    }
    if (m_shm) {
        wl_shm_destroy(m_shm);
        m_shm = nullptr;
    }
    if (m_registry) {
        wl_registry_destroy(m_registry);
        m_registry = nullptr;
    }
    if (m_display) {
        wl_display_disconnect(m_display);
        m_display = nullptr;
    }
}

bool WaylandCapture::captureOutput(const QString& outputName, bool paintCursor, int fps) {
    if (!connectDisplay() || !m_captureManager || !m_outputSources || !m_shm) {
        return false;
    }

    wl_output* target = nullptr;
    for (const auto& entry : m_outputs) {
        if (entry.name == outputName) {
            target = entry.output;
            break;
        }
    }
    if (!target && m_outputs.size() == 1) {
        target = m_outputs.first().output;
    }
    if (!target) {
        qWarning() << "Wormhole: output not found for capture:" << outputName;
        return false;
    }

    auto* source = ext_output_image_capture_source_manager_v1_create_source(m_outputSources, target);
    return beginSession(source, paintCursor, fps);
}

bool WaylandCapture::captureToplevel(const QString& appId, const QString& title, bool paintCursor, int fps) {
    if (!connectDisplay() || !m_captureManager || !m_toplevelSources || !m_shm || !m_toplevelList) {
        return false;
    }

    wl_display_roundtrip(m_display);
    wl_display_roundtrip(m_display);

    ext_foreign_toplevel_handle_v1* target = nullptr;
    for (const auto& entry : m_toplevels) {
        if (!entry.closed && entry.appId == appId && entry.title == title) {
            target = entry.handle;
            break;
        }
    }
    if (!target) {
        for (const auto& entry : m_toplevels) {
            if (!entry.closed && entry.appId == appId) {
                target = entry.handle;
                break;
            }
        }
    }
    if (!target) {
        qWarning() << "Wormhole: toplevel not found for capture:" << appId << title;
        return false;
    }

    auto* source = ext_foreign_toplevel_image_capture_source_manager_v1_create_source(m_toplevelSources, target);
    return beginSession(source, paintCursor, fps);
}

bool WaylandCapture::beginSession(ext_image_capture_source_v1* source, bool paintCursor, int fps) {
    if (!source) {
        return false;
    }

    static const ext_image_copy_capture_session_v1_listener listener = {
        .buffer_size = handleBufferSize,
        .shm_format = handleShmFormat,
        .dmabuf_device = noopDmabufDevice,
        .dmabuf_format = noopDmabufFormat,
        .done = handleConstraintsDone,
        .stopped = handleSessionStopped,
    };

    m_frameIntervalMs = fps > 0 ? qBound(1, 1000 / fps, 1000) : 16;
    m_source = source;
    m_constraintsDone = false;
    m_bufferValid = false;
    m_haveFormat = false;

    const uint32_t options = paintCursor ? EXT_IMAGE_COPY_CAPTURE_MANAGER_V1_OPTIONS_PAINT_CURSORS : 0;
    m_session = ext_image_copy_capture_manager_v1_create_session(m_captureManager, m_source, options);
    ext_image_copy_capture_session_v1_add_listener(m_session, &listener, this);

    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + kSetupTimeoutMs;
    while (!m_constraintsDone && QDateTime::currentMSecsSinceEpoch() < deadline) {
        if (wl_display_roundtrip(m_display) < 0) {
            break;
        }
    }

    if (!m_constraintsDone || !m_bufferValid) {
        endSession();
        return false;
    }

    m_notifier = new QSocketNotifier(wl_display_get_fd(m_display), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &WaylandCapture::onDisplayReadable);

    requestFrame();
    return true;
}

void WaylandCapture::onConstraintsDone() {
    m_constraintsDone = true;

    if (!m_bufferValid) {
        if (!allocateBuffer()) {
            return;
        }
    }

    const QSize logical = swapsAxes(m_transform)
        ? QSize(m_bufferSize.height(), m_bufferSize.width())
        : m_bufferSize;
    if (m_frameSize != logical) {
        m_frameSize = logical;
        emit frameSizeChanged(m_frameSize);
    }

    if (m_notifier && !m_frame && !m_capturePending) {
        scheduleFrame();
    }
}

bool WaylandCapture::allocateBuffer() {
    releaseBuffer();

    if (!m_haveFormat || m_bufferSize.isEmpty()) {
        return false;
    }

    const int stride = m_bufferSize.width() * 4;
    const size_t size = static_cast<size_t>(stride) * static_cast<size_t>(m_bufferSize.height());

    m_poolFd = createAnonymousFile(size);
    if (m_poolFd < 0) {
        qWarning() << "Wormhole: failed to allocate capture buffer";
        return false;
    }

    m_data = static_cast<uint8_t*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_poolFd, 0));
    if (m_data == MAP_FAILED) {
        m_data = nullptr;
        close(m_poolFd);
        m_poolFd = -1;
        qWarning() << "Wormhole: failed to map capture buffer";
        return false;
    }
    m_dataSize = size;

    m_pool = wl_shm_create_pool(m_shm, m_poolFd, static_cast<int32_t>(size));
    m_buffer = wl_shm_pool_create_buffer(m_pool, 0, m_bufferSize.width(), m_bufferSize.height(), stride, m_shmFormat);
    m_bufferValid = true;
    return true;
}

void WaylandCapture::releaseBuffer() {
    if (m_buffer) {
        wl_buffer_destroy(m_buffer);
        m_buffer = nullptr;
    }
    if (m_pool) {
        wl_shm_pool_destroy(m_pool);
        m_pool = nullptr;
    }
    if (m_data) {
        munmap(m_data, m_dataSize);
        m_data = nullptr;
        m_dataSize = 0;
    }
    if (m_poolFd >= 0) {
        close(m_poolFd);
        m_poolFd = -1;
    }
    m_bufferValid = false;
}

void WaylandCapture::scheduleFrame() {
    if (m_capturePending || m_frame || !m_session) {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 wait = m_lastCaptureMs + m_frameIntervalMs - now;
    m_capturePending = true;

    if (wait <= 0) {
        m_capturePending = false;
        requestFrame();
        return;
    }

    QTimer::singleShot(static_cast<int>(wait), this, [this]() {
        m_capturePending = false;
        requestFrame();
    });
}

void WaylandCapture::requestFrame() {
    if (!m_session || m_frame || !m_bufferValid) {
        return;
    }

    static const ext_image_copy_capture_frame_v1_listener listener = {
        .transform = handleFrameTransform,
        .damage = noopFrameDamage,
        .presentation_time = noopFramePresentationTime,
        .ready = handleFrameReady,
        .failed = handleFrameFailed,
    };

    m_lastCaptureMs = QDateTime::currentMSecsSinceEpoch();
    m_frame = ext_image_copy_capture_session_v1_create_frame(m_session);
    ext_image_copy_capture_frame_v1_add_listener(m_frame, &listener, this);
    ext_image_copy_capture_frame_v1_attach_buffer(m_frame, m_buffer);
    ext_image_copy_capture_frame_v1_damage_buffer(m_frame, 0, 0, m_bufferSize.width(), m_bufferSize.height());
    ext_image_copy_capture_frame_v1_capture(m_frame);
    wl_display_flush(m_display);
}

void WaylandCapture::destroyFrame() {
    if (m_frame) {
        ext_image_copy_capture_frame_v1_destroy(m_frame);
        m_frame = nullptr;
    }
}

QImage WaylandCapture::currentImage() const {
    if (!m_data || m_bufferSize.isEmpty()) {
        return {};
    }

    const QImage::Format format = imageFormatFor(m_shmFormat);
    if (format == QImage::Format_Invalid) {
        return {};
    }

    const QImage view(m_data, m_bufferSize.width(), m_bufferSize.height(), m_bufferSize.width() * 4, format);
    if (m_transform == WL_OUTPUT_TRANSFORM_NORMAL) {
        return view;
    }
    return view.transformed(inverseOf(m_transform));
}

void WaylandCapture::onFrameReady() {
    destroyFrame();

    const QImage image = currentImage();
    if (!image.isNull()) {
        const QSize logical = image.size();
        if (m_frameSize != logical) {
            m_frameSize = logical;
            emit frameSizeChanged(m_frameSize);
        }
        emit frameReady(image);
    }

    scheduleFrame();
}

void WaylandCapture::onFrameFailed(uint32_t reason) {
    destroyFrame();

    if (reason == EXT_IMAGE_COPY_CAPTURE_FRAME_V1_FAILURE_REASON_STOPPED) {
        onSessionStopped();
        return;
    }

    if (reason == EXT_IMAGE_COPY_CAPTURE_FRAME_V1_FAILURE_REASON_BUFFER_CONSTRAINTS) {
        m_bufferValid = false;
        return;
    }

    scheduleFrame();
}

void WaylandCapture::onSessionStopped() {
    endSession();
    emit stopped();
}

void WaylandCapture::onDisplayReadable() {
    if (!m_display) {
        return;
    }

    while (wl_display_prepare_read(m_display) != 0) {
        if (wl_display_dispatch_pending(m_display) < 0) {
            onSessionStopped();
            return;
        }
    }

    wl_display_flush(m_display);

    if (wl_display_read_events(m_display) < 0) {
        onSessionStopped();
        return;
    }

    if (wl_display_dispatch_pending(m_display) < 0) {
        onSessionStopped();
        return;
    }

    wl_display_flush(m_display);
}

void WaylandCapture::endSession() {
    releaseNotifier();

    destroyFrame();
    releaseBuffer();

    if (m_session) {
        ext_image_copy_capture_session_v1_destroy(m_session);
        m_session = nullptr;
    }
    if (m_source) {
        ext_image_capture_source_v1_destroy(m_source);
        m_source = nullptr;
    }

    m_constraintsDone = false;
    m_capturePending = false;

    if (m_display) {
        wl_display_flush(m_display);
    }
}

void WaylandCapture::stop() {
    endSession();
}

} // namespace wormhole::screencast
