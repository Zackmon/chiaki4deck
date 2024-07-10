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


Q_LOGGING_CATEGORY(chiakiGuiOpenGL, "chiaki.gui.opengl", QtInfoMsg);
QmlOpenGLMainWindow::QmlOpenGLMainWindow(Settings *settings) : QmlMainWindow(settings) {
    init(settings);
}


QmlOpenGLMainWindow::QmlOpenGLMainWindow(const StreamSessionConnectInfo &connect_info) : QmlMainWindow(connect_info) {
    init(connect_info.settings);
    backend->createSession(connect_info);
    if (connect_info.zoom)
        setVideoMode(VideoMode::Zoom);
    else if (connect_info.stretch)
        setVideoMode(VideoMode::Stretch);

    if (connect_info.fullscreen || connect_info.zoom || connect_info.stretch)
        showFullScreen();

    connect(session, &StreamSession::ConnectedChanged, this, [this]() {
        if (session->IsConnected())
            connect(session, &StreamSession::SessionQuit, qGuiApp, &QGuiApplication::quit);
    });
}

QmlOpenGLMainWindow::~QmlOpenGLMainWindow() {
    quick_render->invalidate();
    delete quick_item;
    delete quick_window;
    delete quick_render;
    delete qml_engine;

    av_buffer_unref(&hw_dev_ctx);
    pl_unmap_avframe(placebo_opengl->gpu,&current_frame);
    pl_unmap_avframe(placebo_opengl->gpu, &previous_frame);

    pl_tex_destroy(placebo_opengl->gpu, &quick_tex);


    for (auto tex : placebo_tex)
        pl_tex_destroy(placebo_opengl->gpu, &tex);

    FILE *file = fopen(qPrintable(shader_cache_path()), "wb");
    if (file) {
        pl_cache_save_file(placebo_cache, file);
        fclose(file);
    }
    pl_cache_destroy(&placebo_cache);
    pl_renderer_destroy(&placebo_renderer);
    pl_opengl_destroy(&placebo_opengl);
    pl_log_destroy(&placebo_log);

}




void QmlOpenGLMainWindow::init(Settings *settings) {
    qWarning("Called Init");
    setSurfaceType(QWindow::OpenGLSurface);
    QSurfaceFormat surfaceFormat = requestedFormat();
    surfaceFormat.setRenderableType(QSurfaceFormat::RenderableType::OpenGLES);
    surfaceFormat.setSwapBehavior(QSurfaceFormat::SwapBehavior::SingleBuffer);
    setFormat(surfaceFormat);
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
        QSurfaceFormat test = requestedFormat();
        auto rendeType = test.renderableType();
        auto context_profile = test.profile();

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
            .get_proc_addr_ex = reinterpret_cast<pl_voidfunc_t (*)(void *, const char *)>(get_proc_addr_context),
            .proc_ctx = this,
            /*.get_proc_addr = get_proc_addr,*/
            .debug = true,
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
    //qt_opengl_context->makeCurrent(quick_window);
    quick_window->setSurfaceType(QSurface::OpenGLSurface);
    quick_window->setFormat(requestedFormat());

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
    if(!quick_render->initialize()){
        qFatal("Quick Render Failed to initialize");
    }


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

    qt_opengl_context->makeCurrent(quick_window);

}


void QmlOpenGLMainWindow::createSwapchain() {
    if (placebo_swapchain)
        return;

    QSurface *test = qt_opengl_context->surface();

    /*qOpenGlBuffer = QOpenGLBuffer();
    qOpenGlBuffer.create();
    if(!qOpenGlBuffer.isCreated()){
        qFatal("Failed to create OpenGL buffer");
    }
    qOpenGlBuffer.allocate(16384);
    qOpenGlBuffer.bind();*/


    qt_opengl_context->makeCurrent(this);
    struct pl_opengl_framebuffer opengl_buffer = {
            .id = static_cast<int>(qt_opengl_context->defaultFramebufferObject()),
            .flipped = true
    };



    struct pl_opengl_swapchain_params swapchain_params = {
            .swap_buffers = (void (*)(void *)) swapBuffers_context,
            .framebuffer = opengl_buffer,
            .max_swapchain_depth = 1,
            .priv = this

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
    uint renderbufferId;
    unsigned int textureId =  pl_opengl_unwrap(placebo_opengl->gpu,quick_tex,&out_target, &out_iformat, &renderbufferId);
    quick_window->setRenderTarget(QQuickRenderTarget::fromOpenGLTexture(textureId,out_iformat,swapchain_size));
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



/*pl_voidfunc_t QmlOpenGLMainWindow::get_proc_addr(const char *procname) {
    return qt_opengl_context->getProcAddress(procname);
}
void QmlOpenGLMainWindow::swapBuffers(QSurface *surface) {
    qt_opengl_context->swapBuffers(surface);
}*/

pl_voidfunc_t QmlOpenGLMainWindow::get_proc_addr_context(QmlOpenGLMainWindow* qmlOpenGlMainWindow, const char *procname) {
    return qmlOpenGlMainWindow->qt_opengl_context->getProcAddress(procname);
}

void QmlOpenGLMainWindow::swapBuffers_context(QmlOpenGLMainWindow* qmlOpenGlMainWindow) {
    qmlOpenGlMainWindow->swapBuffer_local();
}

void QmlOpenGLMainWindow::swapBuffer_local() {
    QSurface* qSurface = qt_opengl_context->surface();
    qt_opengl_context->swapBuffers(qSurface);
}

AVBufferRef *QmlOpenGLMainWindow::hwDeviceCtx() {
    if(hw_dev_ctx){
        return hw_dev_ctx;
    }

    hw_dev_ctx = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_MEDIACODEC);
    //AVHWDeviceContext *hwctx = reinterpret_cast<AVHWDeviceContext*>(hw_dev_ctx->data);
    //AVMediaCodecDeviceContext *ctx = reinterpret_cast<AVMediaCodecDeviceContext*>(hwctx->hwctx);

    if (av_hwdevice_ctx_init(hw_dev_ctx)<0){
        qCWarning(chiakiGui) << "Failed to create Vulkan decode context";
        av_buffer_unref(&hw_dev_ctx);
    }
    return hw_dev_ctx;
}


pl_gpu QmlOpenGLMainWindow::get_pl_gpu() {
    return placebo_opengl->gpu;
}
