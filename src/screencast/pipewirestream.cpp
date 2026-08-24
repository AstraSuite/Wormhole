#include "pipewirestream.hpp"
#include <QCoreApplication>
#include <QDebug>
#include <unistd.h>
#include <time.h>

#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/raw.h>
#include <spa/param/buffers.h>
#include <spa/param/props.h>
#include <spa/buffer/meta.h>
#include <spa/pod/builder.h>

namespace wormhole::screencast {

static void on_stream_state_changed(void* data, enum pw_stream_state old, enum pw_stream_state state, const char* error) {
    auto* ctx = static_cast<StreamContext*>(data);
    if (!ctx) return;
    if (ctx->stream) {
        uint32_t nid = pw_stream_get_node_id(ctx->stream);
        if (nid != 0 && nid != SPA_ID_INVALID) {
            ctx->nodeId = nid;
        }
    }
    if (state == PW_STREAM_STATE_STREAMING) {
        ctx->active = true;
    } else if (state == PW_STREAM_STATE_PAUSED) {
        ctx->active = true;
    } else if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED) {
        ctx->active = false;
    }
    if (ctx->managerLoop) {
        pw_thread_loop_signal(ctx->managerLoop, false);
    }
    Q_UNUSED(old);
    Q_UNUSED(error);
}

static void on_stream_param_changed(void* data, uint32_t id, const struct spa_pod* param) {
    auto* ctx = static_cast<StreamContext*>(data);
    if (!ctx || !param || id != SPA_PARAM_Format) return;

    struct spa_video_info_raw info;
    if (spa_format_video_raw_parse(param, &info) < 0) return;

    ctx->width = info.size.width;
    ctx->height = info.size.height;
    if (info.framerate.denom > 0) {
        ctx->fps = info.framerate.num / info.framerate.denom;
    }

    uint8_t buffer[4][1024];
    struct spa_pod_builder b[4];
    for (int i = 0; i < 4; ++i) {
        b[i] = SPA_POD_BUILDER_INIT(buffer[i], sizeof(buffer[i]));
    }

    const struct spa_pod* params[4];
    int stride = ctx->width * 4;
    int size = stride * ctx->height;

    spa_pod_frame f;
    spa_pod_builder_push_object(&b[0], &f, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers);
    spa_pod_builder_add(&b[0], SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(8, 2, 32), 0);
    spa_pod_builder_add(&b[0], SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1), 0);
    spa_pod_builder_add(&b[0], SPA_PARAM_BUFFERS_size, SPA_POD_Int(size), 0);
    spa_pod_builder_add(&b[0], SPA_PARAM_BUFFERS_stride, SPA_POD_Int(stride), 0);
    spa_pod_builder_add(&b[0], SPA_PARAM_BUFFERS_align, SPA_POD_Int(16), 0);
    spa_pod_builder_add(&b[0], SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemFd) | (1 << SPA_DATA_MemPtr)), 0);
    params[0] = static_cast<const struct spa_pod*>(spa_pod_builder_pop(&b[0], &f));

    params[1] = static_cast<const struct spa_pod*>(spa_pod_builder_add_object(&b[1],
        SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
        SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Header),
        SPA_PARAM_META_size, SPA_POD_Int(sizeof(struct spa_meta_header))
    ));

    params[2] = static_cast<const struct spa_pod*>(spa_pod_builder_add_object(&b[2],
        SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
        SPA_PARAM_META_type, SPA_POD_Id(SPA_META_VideoTransform),
        SPA_PARAM_META_size, SPA_POD_Int(sizeof(struct spa_meta_videotransform))
    ));

    params[3] = static_cast<const struct spa_pod*>(spa_pod_builder_add_object(&b[3],
        SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
        SPA_PARAM_META_type, SPA_POD_Id(SPA_META_VideoDamage),
        SPA_PARAM_META_size, SPA_POD_CHOICE_RANGE_Int(sizeof(struct spa_meta_region) * 4, sizeof(struct spa_meta_region) * 1, sizeof(struct spa_meta_region) * 4)
    ));

    pw_stream_update_params(ctx->stream, params, 4);
}

static const struct pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = on_stream_state_changed,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = on_stream_param_changed,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = nullptr,
    .drained = nullptr,
    .command = nullptr,
    .trigger_done = nullptr
};

PipeWireStreamManager* PipeWireStreamManager::instance() {
    static auto* inst = new PipeWireStreamManager(qApp);
    return inst;
}

PipeWireStreamManager::PipeWireStreamManager(QObject* parent)
    : QObject(parent) {
    initialize();
}

PipeWireStreamManager::~PipeWireStreamManager() {
    cleanup();
}

bool PipeWireStreamManager::initialize() {
    if (m_initialized) return true;

    pw_init(nullptr, nullptr);

    m_loop = pw_thread_loop_new("wormhole-pipewire", nullptr);
    if (!m_loop) {
        qWarning() << "Failed to create PipeWire thread loop";
        return false;
    }

    m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
    if (!m_context) {
        qWarning() << "Failed to create PipeWire context";
        pw_thread_loop_destroy(m_loop);
        m_loop = nullptr;
        return false;
    }

    if (pw_thread_loop_start(m_loop) < 0) {
        qWarning() << "Failed to start PipeWire thread loop";
        pw_context_destroy(m_context);
        pw_thread_loop_destroy(m_loop);
        m_context = nullptr;
        m_loop = nullptr;
        return false;
    }

    pw_thread_loop_lock(m_loop);
    m_core = pw_context_connect(m_context, nullptr, 0);
    pw_thread_loop_unlock(m_loop);

    if (!m_core) {
        qWarning() << "Failed to connect to PipeWire core";
        pw_thread_loop_stop(m_loop);
        pw_context_destroy(m_context);
        pw_thread_loop_destroy(m_loop);
        m_context = nullptr;
        m_loop = nullptr;
        return false;
    }

    m_initialized = true;
    return true;
}

void PipeWireStreamManager::cleanup() {
    if (!m_initialized) return;

    if (m_loop) {
        pw_thread_loop_lock(m_loop);
        if (m_core) {
            pw_core_disconnect(m_core);
            m_core = nullptr;
        }
        pw_thread_loop_unlock(m_loop);

        pw_thread_loop_stop(m_loop);

        if (m_context) {
            pw_context_destroy(m_context);
            m_context = nullptr;
        }
        pw_thread_loop_destroy(m_loop);
        m_loop = nullptr;
    }

    pw_deinit();
    m_initialized = false;
}

uint32_t PipeWireStreamManager::createStream(const QString& title, int width, int height, int fps) {
    if (!initialize()) {
        return 0;
    }

    pw_thread_loop_lock(m_loop);

    pw_properties* props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Screen",
        PW_KEY_MEDIA_CLASS, "Video/Source",
        PW_KEY_NODE_NAME, title.toUtf8().constData(),
        PW_KEY_NODE_DESCRIPTION, QStringLiteral("Wormhole Screen Share - %1").arg(title).toUtf8().constData(),
        nullptr
    );

    auto ctx = std::make_shared<StreamContext>();
    ctx->width = width > 0 ? width : 1920;
    ctx->height = height > 0 ? height : 1080;
    ctx->fps = fps > 0 ? fps : 60;
    ctx->streamListener = new spa_hook();
    ctx->managerLoop = m_loop;

    pw_stream* stream = pw_stream_new(m_core, title.toUtf8().constData(), props);
    if (!stream) {
        pw_thread_loop_unlock(m_loop);
        delete ctx->streamListener;
        return 0;
    }
    ctx->stream = stream;

    pw_stream_add_listener(stream, ctx->streamListener, &stream_events, ctx.get());

    uint8_t buffer[1024];
    spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    spa_pod_frame f;
    spa_pod_builder_push_object(&b, &f, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat);
    spa_pod_builder_add(&b, SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video), 0);
    spa_pod_builder_add(&b, SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), 0);
    spa_pod_builder_add(&b, SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(3,
        SPA_VIDEO_FORMAT_BGRx,
        SPA_VIDEO_FORMAT_BGRx,
        SPA_VIDEO_FORMAT_BGRA
    ), 0);
    struct spa_rectangle rect = SPA_RECTANGLE(static_cast<uint32_t>(ctx->width), static_cast<uint32_t>(ctx->height));
    struct spa_fraction framerate = SPA_FRACTION(0, 1);
    struct spa_fraction maxFramerate = SPA_FRACTION(static_cast<uint32_t>(ctx->fps), 1);
    struct spa_fraction minFramerate = SPA_FRACTION(1, 1);

    spa_pod_builder_add(&b, SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&rect), 0);
    spa_pod_builder_add(&b, SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&framerate), 0);
    spa_pod_builder_add(&b, SPA_FORMAT_VIDEO_maxFramerate, SPA_POD_CHOICE_RANGE_Fraction(
        &maxFramerate,
        &minFramerate,
        &maxFramerate
    ), 0);

    const spa_pod* params[1];
    params[0] = static_cast<const spa_pod*>(spa_pod_builder_pop(&b, &f));

    int res = pw_stream_connect(stream,
                                PW_DIRECTION_OUTPUT,
                                PW_ID_ANY,
                                static_cast<enum pw_stream_flags>(PW_STREAM_FLAG_DRIVER | PW_STREAM_FLAG_MAP_BUFFERS),
                                params, 1);

    if (res < 0) {
        qWarning() << "Failed to connect PipeWire stream:" << res;
        pw_stream_destroy(stream);
        pw_thread_loop_unlock(m_loop);
        delete ctx->streamListener;
        return 0;
    }

    int attempts = 0;
    while (attempts++ < 50) {
        uint32_t nid = pw_stream_get_node_id(stream);
        if (nid != 0 && nid != SPA_ID_INVALID) {
            ctx->nodeId = nid;
            break;
        }
        if (pw_thread_loop_timed_wait(m_loop, 1) != 0) {
            break;
        }
    }

    uint32_t nodeId = ctx->nodeId;
    if (nodeId == 0 || nodeId == SPA_ID_INVALID) {
        nodeId = pw_stream_get_node_id(stream);
        ctx->nodeId = nodeId;
    }

    pw_thread_loop_unlock(m_loop);

    {
        QMutexLocker locker(&m_mutex);
        m_streams.insert(nodeId, ctx);
    }

    emit streamStarted(nodeId);
    return nodeId;
}

void PipeWireStreamManager::stopStream(uint32_t nodeId) {
    std::shared_ptr<StreamContext> ctx;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_streams.contains(nodeId)) return;
        ctx = m_streams.take(nodeId);
    }

    if (ctx && ctx->stream && m_loop) {
        pw_thread_loop_lock(m_loop);
        pw_stream_destroy(ctx->stream);
        pw_thread_loop_unlock(m_loop);
        delete ctx->streamListener;
    }

    emit streamStopped(nodeId);
}

void PipeWireStreamManager::pushFrame(uint32_t nodeId, const QImage& frame) {
    if (frame.isNull()) return;

    std::shared_ptr<StreamContext> ctx;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_streams.contains(nodeId)) return;
        ctx = m_streams.value(nodeId);
    }

    if (!ctx || !ctx->stream || !m_loop) return;

    pw_thread_loop_lock(m_loop);

    struct pw_buffer* b = pw_stream_dequeue_buffer(ctx->stream);
    if (!b) {
        pw_thread_loop_unlock(m_loop);
        return;
    }

    struct spa_buffer* buf = b->buffer;
    if (!buf || buf->n_datas == 0 || !buf->datas || !buf->datas[0].chunk) {
        pw_stream_queue_buffer(ctx->stream, b);
        pw_thread_loop_unlock(m_loop);
        return;
    }

    const uint32_t requiredSize = static_cast<uint32_t>(ctx->width * 4 * ctx->height);
    uint8_t* dst = static_cast<uint8_t*>(buf->datas[0].data);
    if (!dst || buf->datas[0].maxsize < requiredSize) {
        buf->datas[0].chunk->size = 0;
        pw_stream_queue_buffer(ctx->stream, b);
        pw_thread_loop_unlock(m_loop);
        return;
    }

    QImage scaled;
    if (frame.width() != ctx->width || frame.height() != ctx->height) {
        scaled = frame.scaled(ctx->width, ctx->height, Qt::IgnoreAspectRatio, Qt::FastTransformation)
                      .convertToFormat(QImage::Format_RGB32);
    } else {
        scaled = frame.convertToFormat(QImage::Format_RGB32);
    }

    int srcStride = scaled.bytesPerLine();
    int dstStride = ctx->width * 4;
    int copyRows = std::min(scaled.height(), ctx->height);
    int rowBytes = std::min(srcStride, dstStride);

    const uint8_t* src = scaled.constBits();

    for (int r = 0; r < copyRows; ++r) {
        memcpy(dst + (r * dstStride), src + (r * srcStride), rowBytes);
    }

    buf->datas[0].chunk->flags = SPA_CHUNK_FLAG_NONE;
    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->size = dstStride * ctx->height;
    buf->datas[0].chunk->stride = dstStride;

    struct spa_meta_header* h = static_cast<struct spa_meta_header*>(
        spa_buffer_find_meta_data(buf, SPA_META_Header, sizeof(*h))
    );
    if (h) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        h->pts = SPA_TIMESPEC_TO_NSEC(&ts);
        h->flags = 0;
        h->seq = ctx->seq++;
        h->dts_offset = 0;
    }

    struct spa_meta_videotransform* vt = static_cast<struct spa_meta_videotransform*>(
        spa_buffer_find_meta_data(buf, SPA_META_VideoTransform, sizeof(*vt))
    );
    if (vt) {
        vt->transform = SPA_META_TRANSFORMATION_None;
    }

    struct spa_meta* damage = spa_buffer_find_meta(buf, SPA_META_VideoDamage);
    if (damage) {
        struct spa_region* damageRegion = static_cast<struct spa_region*>(spa_meta_first(damage));
        if (damageRegion) {
            *damageRegion = SPA_REGION(0, 0, static_cast<uint32_t>(ctx->width), static_cast<uint32_t>(ctx->height));
        }
    }

    pw_stream_queue_buffer(ctx->stream, b);
    if (pw_stream_is_driving(ctx->stream)) {
        pw_stream_trigger_process(ctx->stream);
    }
    pw_thread_loop_unlock(m_loop);
}

int PipeWireStreamManager::getRemoteFd() {
    if (!initialize()) return -1;

    pw_thread_loop_lock(m_loop);
    pw_core* core = pw_context_connect(m_context, nullptr, 0);
    int fd = core ? pw_core_steal_fd(core) : -1;
    if (core) {
        pw_core_disconnect(core);
    }
    pw_thread_loop_unlock(m_loop);

    return fd;
}

} // namespace wormhole::screencast
