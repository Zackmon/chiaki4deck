#pragma once

#include "streamsession.h"

#include <QMutex>
#include <QWindow>
#include <QQuickWindow>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QQuickRenderControl>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext_vulkan.h>
#include <libplacebo/options.h>

#include <libplacebo/renderer.h>
#include <libplacebo/log.h>
#include <libplacebo/cache.h>
#include <libplacebo/gpu.h>
}


Q_DECLARE_LOGGING_CATEGORY(chiakiGui);



static void placebo_log_cb(void *user, pl_log_level level, const char *msg)
{
    ChiakiLogLevel chiaki_level;
    switch (level) {
        case PL_LOG_NONE:
        case PL_LOG_TRACE:
        case PL_LOG_DEBUG:
            qCDebug(chiakiGui).noquote() << "[libplacebo]" << msg;
            break;
        case PL_LOG_INFO:
            qCInfo(chiakiGui).noquote() << "[libplacebo]" << msg;
            break;
        case PL_LOG_WARN:
            qCWarning(chiakiGui).noquote() << "[libplacebo]" << msg;
            break;
        case PL_LOG_ERR:
        case PL_LOG_FATAL:
            qCCritical(chiakiGui).noquote() << "[libplacebo]" << msg;
            break;
    }
}

static QString shader_cache_path()
{
    static QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/pl_shader.cache";
    return path;
}



class RenderControl : public QQuickRenderControl
{
public:
    explicit RenderControl(QWindow *window)
            : window(window)
    {
    }

    QWindow *renderWindow(QPoint *offset) override
    {
        Q_UNUSED(offset);
        return window;
    }

private:
    QWindow *window = {};
};

class Settings;
class StreamSession;
class QmlBackend;

class QmlMainWindow : public QWindow
{
    Q_OBJECT
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(int droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
    Q_PROPERTY(bool keepVideo READ keepVideo WRITE setKeepVideo NOTIFY keepVideoChanged)
    Q_PROPERTY(VideoMode videoMode READ videoMode WRITE setVideoMode NOTIFY videoModeChanged)
    Q_PROPERTY(float ZoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(VideoPreset videoPreset READ videoPreset WRITE setVideoPreset NOTIFY videoPresetChanged)

public:
    enum class VideoMode {
        Normal,
        Stretch,
        Zoom
    };
    Q_ENUM(VideoMode);

    enum class VideoPreset {
        Fast,
        Default,
        HighQuality
    };
    Q_ENUM(VideoPreset);

    QmlMainWindow(Settings *settings);
    QmlMainWindow(const StreamSessionConnectInfo &connect_info);
    ~QmlMainWindow();
    void updateWindowType(WindowType type);

    bool hasVideo() const;
    int droppedFrames() const;

    bool keepVideo() const;
    void setKeepVideo(bool keep);

    VideoMode videoMode() const;
    void setVideoMode(VideoMode mode);

    float zoomFactor() const;
    void setZoomFactor(float factor);

    VideoPreset videoPreset() const;
    void setVideoPreset(VideoPreset mode);

    Q_INVOKABLE void grabInput();
    Q_INVOKABLE void releaseInput();

    void show();
    void presentFrame(AVFrame *frame, int32_t frames_lost);

   virtual AVBufferRef *hwDeviceCtx() = 0;

signals:
    void hasVideoChanged();
    void droppedFramesChanged();
    void keepVideoChanged();
    void videoModeChanged();
    void zoomFactorChanged();
    void videoPresetChanged();
    void menuRequested();

protected:
    //virtual void init(Settings *settings);
    void update();
    void scheduleUpdate();
    virtual void createSwapchain() = 0;
    virtual void destroySwapchain() = 0;
    virtual void resizeSwapchain() = 0;
    void updateSwapchain();
    void sync();
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    void render();
    bool handleShortcut(QKeyEvent *event);
    bool event(QEvent *event) override;
    QObject *focusObject() const override;
    virtual pl_gpu get_pl_gpu() = 0;

    bool has_video = false;
    bool keep_video = false;
    int grab_input = 0;
    int dropped_frames = 0;
    int dropped_frames_current = 0;
    VideoMode video_mode = VideoMode::Normal;
    float zoom_factor = 0;
    VideoPreset video_preset = VideoPreset::HighQuality;

    QmlBackend *backend = {};
    StreamSession *session = {};

    pl_cache placebo_cache = {};
    pl_log placebo_log = {};

    pl_swapchain placebo_swapchain = {};
    pl_renderer placebo_renderer = {};
    std::array<pl_tex, 8> placebo_tex{};

    QSize swapchain_size;
    QMutex frame_mutex;
    QThread *render_thread = {};
    AVFrame *av_frame = {};
    pl_frame current_frame = {};
    pl_frame previous_frame = {};
    std::atomic<bool> render_scheduled = {false};


    QQmlEngine *qml_engine = {};
    QQuickWindow *quick_window = {};
    QQuickRenderControl *quick_render = {};
    QQuickItem *quick_item = {};
    pl_tex quick_tex = {};

    QTimer *update_timer = {};
    bool quick_frame = false;
    bool quick_need_sync = false;
    std::atomic<bool> quick_need_render = {false};

    bool isVulkan = false;

    friend class QmlBackend;


};
