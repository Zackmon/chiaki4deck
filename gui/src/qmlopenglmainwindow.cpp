#include "qmlopenglmainwindow.h"
#include "qmlbackend.h"
#include "qmlsvgprovider.h"
#include "chiaki/log.h"
#include "streamsession.h"

#include <qpa/qplatformnativeinterface.h>

#define PL_LIBAV_IMPLEMENTATION 0

#include <libplacebo/utils/libav.h>

#include <QDebug>
#include <QThread>
#include <QShortcut>
#include <QStandardPaths>
#include <QGuiApplication>

#include <QOpenGLContext>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickRenderTarget>
#include <QQuickRenderControl>
#include <QQuickGraphicsDevice>

#if defined(Q_OS_MACOS)
#include <objc/message.h>
#endif

Q_LOGGING_CATEGORY(chiakiGuiOpenGL, "chiaki.guiOpenGL", QtInfoMsg);
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
class RenderControl : public QQuickRenderControl {
public:
    explicit RenderControl(QWindow *window)
            : window(window) {
    }

    QWindow *renderWindow(QPoint *offset) override {
        Q_UNUSED(offset);
        return window;
    }


private:
    QWindow *window = {};
};


QmlOpenGLMainWindow::QmlOpenGLMainWindow(Settings *settings) : QWindow() {
    init(settings);
}


QmlOpenGLMainWindow::QmlOpenGLMainWindow(const StreamSessionConnectInfo &connect_info) : QWindow() {
    qFatal("not imlemented yet");
}

QmlOpenGLMainWindow::~QmlOpenGLMainWindow() {

}


void QmlOpenGLMainWindow::updateWindowType(WindowType type) {
    switch (type) {
        case WindowType::SelectedResolution:
            break;
        case WindowType::Fullscreen:
            showFullScreen();
            break;
        case WindowType::Zoom:
            showFullScreen();
            setVideoMode(VideoMode::Zoom);
            break;
        case WindowType::Stretch:
            showFullScreen();
            setVideoMode(VideoMode::Stretch);
            break;
        default:
            break;
    }
}

bool QmlOpenGLMainWindow::hasVideo() const {
    return has_video;
}

int QmlOpenGLMainWindow::droppedFrames() const {
    return dropped_frames;
}

bool QmlOpenGLMainWindow::keepVideo() const {
    return keep_video;
}

void QmlOpenGLMainWindow::setKeepVideo(bool keep) {
    keep_video = keep;
    emit keepVideoChanged();

}

void QmlOpenGLMainWindow::grabInput() {
    setCursor(Qt::ArrowCursor);
    grab_input++;
    if (session)
        session->BlockInput(grab_input);
}

void QmlOpenGLMainWindow::releaseInput() {
    if (!grab_input)
        return;
    grab_input--;
    if (!grab_input && has_video)
        setCursor(Qt::BlankCursor);
    if (session)
        session->BlockInput(grab_input);
}

QmlOpenGLMainWindow::VideoMode QmlOpenGLMainWindow::videoMode() const {
    return video_mode;
}

void QmlOpenGLMainWindow::setVideoMode(QmlOpenGLMainWindow::VideoMode mode) {
    video_mode = mode;
    emit videoModeChanged();
}

float QmlOpenGLMainWindow::zoomFactor() const {
    return zoom_factor;
}

void QmlOpenGLMainWindow::setZoomFactor(float factor) {
    zoom_factor = factor;
    emit zoomFactorChanged();
}

QmlOpenGLMainWindow::VideoPreset QmlOpenGLMainWindow::videoPreset() const {
    return video_preset;
}

void QmlOpenGLMainWindow::setVideoPreset(VideoPreset preset) {
    video_preset = preset;
    emit videoPresetChanged();
}

void QmlOpenGLMainWindow::show() {
    qWarning("Called Show");
    QQmlComponent component(qml_engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    if (!component.isReady()) {
        qCCritical(chiakiGui) << "Component not ready\n" << component.errors();
        QMetaObject::invokeMethod(QGuiApplication::instance(), &QGuiApplication::quit, Qt::QueuedConnection);
        return;
    }

    QVariantMap props;
    props[QStringLiteral("parent")] = QVariant::fromValue(quick_window->contentItem());
    quick_item = qobject_cast<QQuickItem*>(component.createWithInitialProperties(props));
    if (!quick_item) {
        qCCritical(chiakiGui) << "Failed to create root item\n" << component.errors();
        QMetaObject::invokeMethod(QGuiApplication::instance(), &QGuiApplication::quit, Qt::QueuedConnection);
        return;
    }

    resize(1280, 720);

    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP") == "gamescope")
        showFullScreen();
    else
        showFullScreen();

}

void QmlOpenGLMainWindow::presentFrame(AVFrame *frame, int32_t frames_lost)
{
    //frame_mutex.lock();
    if (av_frame) {
        qCDebug(chiakiGui) << "Dropping rendering frame";
        dropped_frames_current++;
        av_frame_free(&av_frame);
    }
    av_frame = frame;
    //frame_mutex.unlock();

    dropped_frames_current += frames_lost;

    if (!has_video) {
        has_video = true;
        if (!grab_input)
            setCursor(Qt::BlankCursor);
        emit hasVideoChanged();
    }

    update();
}

void QmlOpenGLMainWindow::init(Settings *settings) {
    qWarning("Called Init");
    setSurfaceType(QWindow::OpenGLSurface);
    create();



    struct pl_log_params log_params = {
            .log_cb = placebo_log_cb,
            .log_level = PL_LOG_DEBUG,
    };
    placebo_log = pl_log_create(PL_API_VER, &log_params);

    qt_opengl_context = QOpenGLContext::currentContext();
    if (!qt_opengl_context){
        qWarning("There is no currentContext for OpenGL , trying to make one ");
        qt_opengl_context = new QOpenGLContext();
        qt_opengl_context->setFormat(requestedFormat());
        if (!qt_opengl_context->create()){
            qFatal("Failed to create QOpenGLContext");
        }
        if (!qt_opengl_context->makeCurrent(this)){
            qFatal("Failed to makeCurrent");
        }
    }

    QOpenGLFunctions *qOpenGlFunctions = new QOpenGLFunctions(qt_opengl_context);
    qOpenGlFunctions->initializeOpenGLFunctions();



    struct pl_opengl_params opengl_params = {
            .get_proc_addr = get_proc_addr,
            .allow_software = true,
            //.make_current = make_current,
            //.release_current = make_release,
    };
    placebo_opengl = pl_opengl_create(placebo_log,&opengl_params);
    if (!placebo_opengl){
        qFatal("pl_opengl_create failed");
    }
    struct pl_cache_params cache_params = {
            .log = placebo_log,
            .max_total_size = 10 << 20, // 10 MB
    };
    placebo_cache = pl_cache_create(&cache_params);
    pl_gpu_set_cache(placebo_opengl->gpu, placebo_cache);
    FILE *file = fopen(qPrintable(shader_cache_path()), "rb");
    if (file) {
        pl_cache_load_file(placebo_cache, file);
        fclose(file);
    }

    placebo_renderer = pl_renderer_create(
            placebo_log,
            placebo_opengl->gpu
    );












    quick_render = new RenderControl(this);
    QQuickWindow::setDefaultAlphaBuffer(true);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    quick_window = new QQuickWindow(quick_render);
    quick_window->setGraphicsDevice(QQuickGraphicsDevice::fromOpenGLContext(qt_opengl_context));

    quick_window->setColor(QColor(0, 0, 0, 0));
    connect(quick_window, &QmlOpenGLMainWindow::focusObjectChanged, this, &QmlOpenGLMainWindow::focusObjectChanged);

    qml_engine = new QQmlEngine(this);
    qml_engine->addImageProvider(QStringLiteral("svg"), new QmlSvgProvider);

    if (!qml_engine->incubationController())
        qml_engine->setIncubationController(quick_window->incubationController());
    connect(qml_engine, &QQmlEngine::quit, this, &QWindow::close);
    backend = new QmlBackend(settings, this);
    connect(backend, &QmlBackend::sessionChanged, this, [this](StreamSession *s) {
        session = s;
        grab_input = 0;
        if (has_video) {
            has_video = false;
            setCursor(Qt::ArrowCursor);
            emit hasVideoChanged();
        }
    });
    connect(backend, &QmlBackend::windowTypeUpdated, this, &QmlOpenGLMainWindow::updateWindowType);

    /*render_thread = new QThread;
    render_thread->setObjectName("renderControlThread");
    render_thread->start();

    quick_render->prepareThread(render_thread);
    quick_render->moveToThread(render_thread);*/

    connect(quick_render, &QQuickRenderControl::sceneChanged, this, [this]() {
        quick_need_sync = true;
        scheduleUpdate();
    });
    connect(quick_render, &QQuickRenderControl::renderRequested, this, [this]() {
        quick_need_render = true;
        scheduleUpdate();
    });

    update_timer = new QTimer(this);
    update_timer->setSingleShot(true);
    connect(update_timer, &QTimer::timeout, this, &QmlOpenGLMainWindow::update);

    //QMetaObject::invokeMethod(quick_render, &QQuickRenderControl::initialize);
    quick_render->initialize();

    QTimer *dropped_frames_timer = new QTimer(this);
    dropped_frames_timer->setInterval(1000);
    dropped_frames_timer->start();
    connect(dropped_frames_timer, &QTimer::timeout, this, [this]() {
        if (dropped_frames != dropped_frames_current) {
            dropped_frames = dropped_frames_current;
            emit droppedFramesChanged();
        }
        dropped_frames_current = 0;
    });
    switch (settings->GetPlaceboPreset()) {
        case PlaceboPreset::Fast:
            setVideoPreset(VideoPreset::Fast);
            break;
        case PlaceboPreset::Default:
            setVideoPreset(VideoPreset::Default);
            break;
        case PlaceboPreset::HighQuality:
        default:
            setVideoPreset(VideoPreset::HighQuality);
            break;
    }
    setVideoPreset(VideoPreset::Default);
    setZoomFactor(settings->GetZoomFactor());

}

void QmlOpenGLMainWindow::update() {
    //Q_ASSERT(QThread::currentThread() == QGuiApplication::instance()->thread());

    if (render_scheduled)
        return;

    if (quick_need_sync) {
        quick_need_sync = false;
        quick_render->polishItems();
        //QMetaObject::invokeMethod(quick_render, std::bind(&QmlOpenGLMainWindow::sync, this), Qt::BlockingQueuedConnection);
        sync();
    }

    update_timer->stop();
    render_scheduled = true;
    //QMetaObject::invokeMethod(quick_render, std::bind(&QmlOpenGLMainWindow::render, this));
    render();
}

void QmlOpenGLMainWindow::scheduleUpdate() {
    //Q_ASSERT(QThread::currentThread() == QGuiApplication::instance()->thread());

    if (!update_timer->isActive())
        update_timer->start(has_video ? 50 : 10);
}

void QmlOpenGLMainWindow::createSwapchain() {
    if (placebo_swapchain)
        return;
    struct pl_opengl_swapchain_params swapchain_params = {
            .swap_buffers = (void (*)(void *)) swapBuffers,
            .max_swapchain_depth = 1,
            .priv = qt_opengl_context->surface()

    };
    placebo_swapchain = pl_opengl_create_swapchain(placebo_opengl,&swapchain_params);

}

void QmlOpenGLMainWindow::destroySwapchain() {
    if (!placebo_swapchain)
        return;
    pl_swapchain_destroy(&placebo_swapchain);
}

void QmlOpenGLMainWindow::resizeSwapchain() {
    if (!placebo_swapchain)
        createSwapchain();

    const QSize window_size(width() * devicePixelRatio(), height() * devicePixelRatio());
    if (window_size == swapchain_size)
        return;
    swapchain_size = window_size;
    pl_swapchain_resize(placebo_swapchain, &swapchain_size.rwidth(), &swapchain_size.rheight());
    struct pl_tex_params tex_params = {
            .w = swapchain_size.width(),
            .h = swapchain_size.height(),
            .format = pl_find_fmt(placebo_opengl->gpu, PL_FMT_UNORM, 4, 0, 0, PL_FMT_CAP_RENDERABLE),
            .sampleable = true,
            .renderable = true,
    };
    if (!pl_tex_recreate(placebo_opengl->gpu, &quick_tex, &tex_params))
        qCCritical(chiakiGui) << "Failed to create placebo texture";
    unsigned int out_target;
    int out_iformat;
    unsigned int textureId =  pl_opengl_unwrap(placebo_opengl->gpu,quick_tex,&out_target, &out_iformat, nullptr);
    quick_window->setRenderTarget(QQuickRenderTarget::fromOpenGLTexture(textureId,out_iformat,swapchain_size));
}

void QmlOpenGLMainWindow::updateSwapchain() {
    quick_item->setSize(size());
    quick_window->resize(size());

    //QMetaObject::invokeMethod(quick_render, std::bind(&QmlOpenGLMainWindow::resizeSwapchain, this), Qt::BlockingQueuedConnection);
    resizeSwapchain();
    quick_render->polishItems();
    //QMetaObject::invokeMethod(quick_render, std::bind(&QmlOpenGLMainWindow::sync, this), Qt::BlockingQueuedConnection);
    sync();
    update();
}

void QmlOpenGLMainWindow::sync() {
    if (!quick_tex)
        return;

    beginFrame();
    quick_need_render = quick_render->sync();
}

void QmlOpenGLMainWindow::beginFrame() {
    if (quick_frame)
        return;

    quick_frame = true;
    quick_render->beginFrame();
}

void QmlOpenGLMainWindow::endFrame() {
    if (!quick_frame)
        return;

    quick_frame = false;
    quick_render->endFrame();
}

void QmlOpenGLMainWindow::render() {
    render_scheduled = false;

    if (!placebo_swapchain)
        return;

    AVFrame *frame = nullptr;
    pl_tex *tex = &placebo_tex[0];

    //frame_mutex.lock();

    if (av_frame || (!has_video && !keep_video)) {

        pl_unmap_avframe(placebo_opengl->gpu, &previous_frame);
        if (av_frame && av_frame->decode_error_flags) {
            std::swap(previous_frame, current_frame);
            if (previous_frame.planes[0].texture == *tex)
                tex = &placebo_tex[4];
        }
        pl_unmap_avframe(placebo_opengl->gpu, &current_frame);

        std::swap(frame, av_frame);
    }

//    frame_mutex.unlock();

    if (frame) {
        struct pl_avframe_params avparams = {
                .frame = frame,
                .tex = tex,
        };
        if (!pl_map_avframe_ex(placebo_opengl->gpu, &current_frame, &avparams))
            qCWarning(chiakiGui) << "Failed to map AVFrame to Placebo frame!";
        av_frame_free(&frame);
    }

    struct pl_swapchain_frame sw_frame = {};
    if (!pl_swapchain_start_frame(placebo_swapchain, &sw_frame)) {
        qCWarning(chiakiGui) << "Failed to start Placebo frame!";
        return;
    }

    struct pl_frame target_frame = {};
    pl_frame_from_swapchain(&target_frame, &sw_frame);

    if (quick_need_render) {
        quick_need_render = false;
        beginFrame();
        quick_render->render();
    }
    endFrame();

    struct pl_overlay_part overlay_part = {
            .src = {0, 0, static_cast<float>(swapchain_size.width()), static_cast<float>(swapchain_size.height())},
            .dst = {0, 0, static_cast<float>(swapchain_size.width()), static_cast<float>(swapchain_size.height())},
    };
    struct pl_overlay overlay = {
            .tex = quick_tex,
            .repr = pl_color_repr_rgb,
            .color = pl_color_space_srgb,
            .parts = &overlay_part,
            .num_parts = 1,
    };
    target_frame.overlays = &overlay;
    target_frame.num_overlays = 1;

    const struct pl_render_params *render_params;
    switch (video_preset) {
        case VideoPreset::Fast:
            render_params = &pl_render_fast_params;
            break;
        case VideoPreset::Default:
            render_params = &pl_render_default_params;
            break;
        case VideoPreset::HighQuality:
            render_params = &pl_render_high_quality_params;
            break;
    }

    if (current_frame.num_planes) {
        pl_rect2df crop = current_frame.crop;
        switch (video_mode) {
            case VideoMode::Normal:
                pl_rect2df_aspect_copy(&target_frame.crop, &crop, 0.0);
                break;
            case VideoMode::Stretch:
                // Nothing to do, target.crop already covers the full image
                break;
            case VideoMode::Zoom:
                if(zoom_factor == -1)
                    pl_rect2df_aspect_copy(&target_frame.crop, &crop, 1.0);
                else
                {
                    const float z = powf(2.0f, zoom_factor);
                    const float sx = z * fabsf(pl_rect_w(crop)) / pl_rect_w(target_frame.crop);
                    const float sy = z * fabsf(pl_rect_h(crop)) / pl_rect_h(target_frame.crop);
                    pl_rect2df_stretch(&target_frame.crop, sx, sy);
                }
                break;
        }
        pl_swapchain_colorspace_hint(placebo_swapchain, &current_frame.color);
    }
    if (current_frame.num_planes && previous_frame.num_planes) {
        const struct pl_frame *frames[] = { &previous_frame, &current_frame, };
        const uint64_t sig = QDateTime::currentMSecsSinceEpoch();
        const uint64_t signatures[] = { sig, sig + 1 };
        const float timestamps[] = { -0.5, 0.5 };
        struct pl_frame_mix frame_mix = {
                .num_frames = 2,
                .frames = frames,
                .signatures = signatures,
                .timestamps = timestamps,
                .vsync_duration = 1.0,
        };
        struct pl_render_params params = *render_params;
        params.frame_mixer = pl_find_filter_config("linear", PL_FILTER_FRAME_MIXING);
        if (!pl_render_image_mix(placebo_renderer, &frame_mix, &target_frame, &params))
            qCWarning(chiakiGui) << "Failed to render Placebo frame!";
    } else {
        if (!pl_render_image(placebo_renderer, current_frame.num_planes ? &current_frame : nullptr, &target_frame, render_params))
            qCWarning(chiakiGui) << "Failed to render Placebo frame!";
    }

    if (!pl_swapchain_submit_frame(placebo_swapchain))
        qCWarning(chiakiGui) << "Failed to submit Placebo frame!";

    pl_swapchain_swap_buffers(placebo_swapchain);
}

bool QmlOpenGLMainWindow::handleShortcut(QKeyEvent *event) {
    if (event->modifiers() == Qt::NoModifier) {
        switch (event->key()) {
            case Qt::Key_F11:
                if (windowState() != Qt::WindowFullScreen)
                    showFullScreen();
                else
                    showNormal();
                return true;
            default:
                break;
        }
    }

    if (!event->modifiers().testFlag(Qt::ControlModifier))
        return false;

    switch (event->key()) {
        case Qt::Key_S:
            if (has_video)
                setVideoMode(videoMode() == VideoMode::Stretch ? VideoMode::Normal : VideoMode::Stretch);
            return true;
        case Qt::Key_Z:
            if (has_video)
                setVideoMode(videoMode() == VideoMode::Zoom ? VideoMode::Normal : VideoMode::Zoom);
            return true;
        case Qt::Key_M:
            if (session)
                session->ToggleMute();
            return true;
        case Qt::Key_O:
            emit menuRequested();
            return true;
        case Qt::Key_Q:
            close();
            return true;
        default:
            return false;
    }
}

bool QmlOpenGLMainWindow::event(QEvent *event) {

    switch (event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            if (static_cast<QMouseEvent*>(event)->source() != Qt::MouseEventNotSynthesized)
                return true;
            if (session && !grab_input) {
                if (event->type() == QEvent::MouseMove)
                    session->HandleMouseMoveEvent(static_cast<QMouseEvent*>(event), width(), height());
                else if (event->type() == QEvent::MouseButtonPress)
                    session->HandleMousePressEvent(static_cast<QMouseEvent*>(event));
                else if (event->type() == QEvent::MouseButtonRelease)
                    session->HandleMouseReleaseEvent(static_cast<QMouseEvent*>(event));
                return true;
            }
            QGuiApplication::sendEvent(quick_window, event);
            break;
        case QEvent::MouseButtonDblClick:
            if (session && !grab_input) {
                if (windowState() != Qt::WindowFullScreen)
                    showFullScreen();
                else
                    showNormal();
            }
            break;
        case QEvent::KeyPress:
            if (handleShortcut(static_cast<QKeyEvent*>(event)))
                return true;
        case QEvent::KeyRelease:
            if (session && !grab_input) {
                QKeyEvent *e = static_cast<QKeyEvent*>(event);
                if (e->timestamp()) // Don't send controller emulated key events
                    session->HandleKeyboardEvent(e);
                return true;
            }
            QGuiApplication::sendEvent(quick_window, event);
            break;
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
            if (session && !grab_input) {
                session->HandleTouchEvent(static_cast<QTouchEvent*>(event), width(), height());
                return true;
            }
            QGuiApplication::sendEvent(quick_window, event);
            break;
        case QEvent::Close:
            if (!backend->closeRequested()) {
                event->ignore();
                return true;
            }
            //QMetaObject::invokeMethod(quick_render, std::bind(&QmlOpenGLMainWindow::destroySwapchain, this), Qt::BlockingQueuedConnection);
            destroySwapchain();
            break;
        default:
            break;
    }

    bool ret = QWindow::event(event);

    switch (event->type()) {
        case QEvent::Expose:
            if (isExposed())
                updateSwapchain();
            else
                destroySwapchain();
                //QMetaObject::invokeMethod(quick_render, std::bind(&QmlOpenGLMainWindow::destroySwapchain, this), Qt::BlockingQueuedConnection);
            break;
        case QEvent::Resize:
            if (isExposed())
                updateSwapchain();
            break;
        default:
            break;
    }

    return ret;
}

QObject *QmlOpenGLMainWindow::focusObject() const {
    return quick_window->focusObject();
}

pl_voidfunc_t QmlOpenGLMainWindow::get_proc_addr(const char *procname) {
    return qt_opengl_context->getProcAddress(procname);
}
void QmlOpenGLMainWindow::swapBuffers(QSurface *surface) {
    qt_opengl_context->swapBuffers(surface);
}


















