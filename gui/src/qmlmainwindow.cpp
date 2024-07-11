#include "qmlmainwindow.h"
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
#include <QVulkanInstance>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickRenderTarget>

#include <QQuickGraphicsDevice>
#if defined(Q_OS_MACOS)
#include <objc/message.h>
#endif

Q_LOGGING_CATEGORY(chiakiGui, "chiaki.gui", QtInfoMsg);

QmlMainWindow::QmlMainWindow(Settings *settings)
    : QWindow()
{

}

QmlMainWindow::QmlMainWindow(const StreamSessionConnectInfo &connect_info)
    : QWindow()
{

}

QmlMainWindow::~QmlMainWindow()
{

}

void QmlMainWindow::updateWindowType(WindowType type)
{
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

bool QmlMainWindow::hasVideo() const
{
    return has_video;
}

int QmlMainWindow::droppedFrames() const
{
    return dropped_frames;
}

bool QmlMainWindow::keepVideo() const
{
    return keep_video;
}

void QmlMainWindow::setKeepVideo(bool keep)
{
    keep_video = keep;
    emit keepVideoChanged();
}

void QmlMainWindow::grabInput()
{
    setCursor(Qt::ArrowCursor);
    grab_input++;
    if (session)
        session->BlockInput(grab_input);
}

void QmlMainWindow::releaseInput()
{
    if (!grab_input)
        return;
    grab_input--;
    if (!grab_input && has_video)
        setCursor(Qt::BlankCursor);
    if (session)
        session->BlockInput(grab_input);
}

QmlMainWindow::VideoMode QmlMainWindow::videoMode() const
{
    return video_mode;
}

void QmlMainWindow::setVideoMode(VideoMode mode)
{
    video_mode = mode;
    emit videoModeChanged();
}

float QmlMainWindow::zoomFactor() const
{
    return zoom_factor;
}

void QmlMainWindow::setZoomFactor(float factor)
{
    zoom_factor = factor;
    emit zoomFactorChanged();
}

QmlMainWindow::VideoPreset QmlMainWindow::videoPreset() const
{
    return video_preset;
}

void QmlMainWindow::setVideoPreset(VideoPreset preset)
{
    video_preset = preset;
    emit videoPresetChanged();
}

void QmlMainWindow::show()
{
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
#ifdef __ANDROID__
    showFullScreen();

#else
    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP") == "gamescope")
        showFullScreen();
    else
        showNormal();
#endif

}

void QmlMainWindow::presentFrame(AVFrame *frame, int32_t frames_lost)
{
    if (isVulkan)
        frame_mutex.lock();
    if (av_frame) {
        qCDebug(chiakiGui) << "Dropping rendering frame";
        dropped_frames_current++;
        av_frame_free(&av_frame);
    }
    av_frame = frame;
    if(isVulkan)
        frame_mutex.unlock();

    dropped_frames_current += frames_lost;

    if (!has_video) {
        has_video = true;
        if (!grab_input)
            setCursor(Qt::BlankCursor);
        emit hasVideoChanged();
    }

    update();
}


void QmlMainWindow::update()
{
    if (isVulkan)
        Q_ASSERT(QThread::currentThread() == QGuiApplication::instance()->thread());

    if (render_scheduled)
        return;

    if (quick_need_sync) {
        quick_need_sync = false;
        quick_render->polishItems();
        if(isVulkan)
            QMetaObject::invokeMethod(quick_render, std::bind(&QmlMainWindow::sync, this), Qt::BlockingQueuedConnection);
        else
            sync();
    }

    update_timer->stop();
    render_scheduled = true;
    if (isVulkan)
        QMetaObject::invokeMethod(quick_render, std::bind(&QmlMainWindow::render, this));
    else
        render();
}

void QmlMainWindow::scheduleUpdate()
{
    if(isVulkan)
        Q_ASSERT(QThread::currentThread() == QGuiApplication::instance()->thread());

    if (!update_timer->isActive())
        update_timer->start(has_video ? 50 : 10);
}


void QmlMainWindow::updateSwapchain()
{
    if(isVulkan)
        Q_ASSERT(QThread::currentThread() == QGuiApplication::instance()->thread());

    quick_item->setSize(size());
    quick_window->resize(size());
    if (isVulkan){
        QMetaObject::invokeMethod(quick_render, std::bind(&QmlMainWindow::resizeSwapchain, this), Qt::BlockingQueuedConnection);
        quick_render->polishItems();
        QMetaObject::invokeMethod(quick_render, std::bind(&QmlMainWindow::sync, this), Qt::BlockingQueuedConnection);
    }else {
        resizeSwapchain();
        quick_render->polishItems();
        sync();
    }

    update();
}

void QmlMainWindow::sync()
{
    if(isVulkan)
        Q_ASSERT(QThread::currentThread() == render_thread);

    if (!quick_tex)
        return;

    beginFrame();
    quick_need_render = quick_render->sync();
}



void QmlMainWindow::render()
{
    if(isVulkan)
        Q_ASSERT(QThread::currentThread() == render_thread);

    render_scheduled = false;

    if (!placebo_swapchain)
        return;

    AVFrame *frame = nullptr;
    pl_tex *tex = &placebo_tex[0];
    if (isVulkan)
        frame_mutex.lock();
    if (av_frame || (!has_video && !keep_video)) {
        pl_unmap_avframe(get_pl_gpu(), &previous_frame);
        if (av_frame && av_frame->decode_error_flags) {
            std::swap(previous_frame, current_frame);
            if (previous_frame.planes[0].texture == *tex)
                tex = &placebo_tex[4];
        }
        pl_unmap_avframe(get_pl_gpu(), &current_frame);
        std::swap(frame, av_frame);
    }
    if (isVulkan)
        frame_mutex.unlock();

    if (frame) {
        struct pl_avframe_params avparams = {
            .frame = frame,
            .tex = tex,
        };
        if (!pl_map_avframe_ex(get_pl_gpu(), &current_frame, &avparams))
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

bool QmlMainWindow::handleShortcut(QKeyEvent *event)
{
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

bool QmlMainWindow::event(QEvent *event)
{
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
        if (isVulkan)
            QMetaObject::invokeMethod(quick_render, std::bind(&QmlMainWindow::destroySwapchain, this), Qt::BlockingQueuedConnection);
        else
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
        else {
            if (isVulkan)
                QMetaObject::invokeMethod(quick_render, std::bind(&QmlMainWindow::destroySwapchain, this), Qt::BlockingQueuedConnection);
            else
                destroySwapchain();
        }

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

QObject *QmlMainWindow::focusObject() const
{
    return quick_window->focusObject();
}
