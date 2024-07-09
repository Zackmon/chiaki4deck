#ifndef QMLOPENGLMAINWINDOW_H
#define QMLOPENGLMAINWINDOW_H

#include "streamsession.h"

#include <QMutex>
#include <QWindow>
#include <QQuickWindow>
#include <QLoggingCategory>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext_vulkan.h>
#include "libavutil/hwcontext_mediacodec.h"
#include <libplacebo/options.h>
#include <libplacebo/renderer.h>
#include <libplacebo/log.h>
#include <libplacebo/cache.h>
#include <libplacebo/opengl.h>
}

Q_DECLARE_LOGGING_CATEGORY(chiakiGui);

class Settings;
class StreamSession;
class QmlBackend;

//static QOpenGLContext *qt_opengl_context = {};
class QmlOpenGLMainWindow : public QWindow
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

    QmlOpenGLMainWindow(Settings *settings);
    QmlOpenGLMainWindow(const StreamSessionConnectInfo &connect_info);
    ~QmlOpenGLMainWindow();

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

    /*static pl_voidfunc_t get_proc_addr (const char *procname);
    static void swapBuffers(QSurface *surface);*/

    static pl_voidfunc_t get_proc_addr_context (QmlOpenGLMainWindow* qmlOpenGlMainWindow,const char *procname);
    static void swapBuffers_context(QmlOpenGLMainWindow* qmlOpenGlMainWindow);
    void swapBuffer_local();
    AVBufferRef *hwDeviceCtx();


signals:
    void hasVideoChanged();
    void droppedFramesChanged();
    void keepVideoChanged();
    void videoModeChanged();
    void zoomFactorChanged();
    void videoPresetChanged();
    void menuRequested();



private:
    void init(Settings *settings);
    void update();
    void scheduleUpdate();
    void createSwapchain();
    void destroySwapchain();
    void resizeSwapchain();
    void updateSwapchain();
    void sync();
    void beginFrame();
    void endFrame();
    void render();
    bool handleShortcut(QKeyEvent *event);
    bool event(QEvent *event) override;
    QObject *focusObject() const override;

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
    pl_opengl placebo_opengl = {};

    pl_swapchain placebo_swapchain = {};
    pl_renderer placebo_renderer = {};
    std::array<pl_tex, 8> placebo_tex{};


    QSize swapchain_size;
    QMutex frame_mutex;
    //QThread *render_thread = {};
    AVFrame *av_frame = {};
    pl_frame current_frame = {};
    pl_frame previous_frame = {};
    std::atomic<bool> render_scheduled = {false};



    QQmlEngine *qml_engine = {};
    QQuickWindow *quick_window = {};
    QQuickRenderControl *quick_render = {};
    QQuickItem *quick_item = {};
    pl_tex quick_tex = {};

    uint64_t quick_sem_value = 0;
    QTimer *update_timer = {};
    bool quick_frame = false;
    bool quick_need_sync = false;
    std::atomic<bool> quick_need_render = {false};
    //QOpenGLBuffer qOpenGlBuffer = {};
    QOpenGLContext *qt_opengl_context = {};
    AVBufferRef *hw_dev_ctx = nullptr;

    friend class QmlBackend;


};

#endif // QMLOPENGLMAINWINDOW_H
