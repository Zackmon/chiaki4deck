#ifndef QMLOPENGLMAINWINDOW_H
#define QMLOPENGLMAINWINDOW_H

#include "streamsession.h"
#include "qmlmainwindow.h"
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

Q_DECLARE_LOGGING_CATEGORY(chiakiGuiOpenGL);

/*class Settings;
class StreamSession;
class QmlBackend;*/

//static QOpenGLContext *qt_opengl_context = {};
class QmlOpenGLMainWindow : public QmlMainWindow
{
   /* Q_OBJECT
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(int droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
    Q_PROPERTY(bool keepVideo READ keepVideo WRITE setKeepVideo NOTIFY keepVideoChanged)
    Q_PROPERTY(VideoMode videoMode READ videoMode WRITE setVideoMode NOTIFY videoModeChanged)
    Q_PROPERTY(float ZoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(VideoPreset videoPreset READ videoPreset WRITE setVideoPreset NOTIFY videoPresetChanged)*/

public:
   /* enum class VideoMode {
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
    Q_ENUM(VideoPreset);*/

    QmlOpenGLMainWindow(Settings *settings);
    QmlOpenGLMainWindow(const StreamSessionConnectInfo &connect_info);
    ~QmlOpenGLMainWindow();

    /*void updateWindowType(WindowType type);

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
    void presentFrame(AVFrame *frame, int32_t frames_lost);*/

    /*static pl_voidfunc_t get_proc_addr (const char *procname);
    static void swapBuffers(QSurface *surface);*/

    static pl_voidfunc_t get_proc_addr_context (QmlOpenGLMainWindow* qmlOpenGlMainWindow,const char *procname);
    static void swapBuffers_context(QmlOpenGLMainWindow* qmlOpenGlMainWindow);
    void swapBuffer_local();
    AVBufferRef *hwDeviceCtx() override;



private:
    void init(Settings *settings);


    void createSwapchain() override;
    void destroySwapchain() override;
    void resizeSwapchain() override;


    void beginFrame() override;
    void endFrame() override;

    pl_gpu get_pl_gpu() override;



    pl_opengl placebo_opengl = {};

    QOpenGLContext *qt_opengl_context = {};
    AVBufferRef *hw_dev_ctx = nullptr;



};

#endif // QMLOPENGLMAINWINDOW_H
