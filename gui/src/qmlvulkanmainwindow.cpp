//
// Created by zackmon on 7/10/24.
//

#include "qmlvulkanmainwindow.h"
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
#include <QQuickRenderControl>
#include <QQuickGraphicsDevice>

#if defined(Q_OS_MACOS)
#include <objc/message.h>
#endif

Q_LOGGING_CATEGORY(chiakiGuiVulkan, "chiaki.gui.vulkan", QtInfoMsg);
QmlVulkanMainWindow::QmlVulkanMainWindow(Settings *settings) : QmlMainWindow(settings) {
    init(settings);
}

QmlVulkanMainWindow::QmlVulkanMainWindow(const StreamSessionConnectInfo &connect_info) : QmlMainWindow(connect_info) {
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

QmlVulkanMainWindow::~QmlVulkanMainWindow() {
    Q_ASSERT(!placebo_swapchain);

    QMetaObject::invokeMethod(quick_render, &QQuickRenderControl::invalidate);
    render_thread->quit();
    render_thread->wait();
    delete render_thread->parent();

    delete quick_item;
    delete quick_window;
    delete quick_render;
    delete qml_engine;
    delete qt_vk_inst;

    av_buffer_unref(&vulkan_hw_dev_ctx);

    pl_unmap_avframe(placebo_vulkan->gpu, &current_frame);
    pl_unmap_avframe(placebo_vulkan->gpu, &previous_frame);

    pl_tex_destroy(placebo_vulkan->gpu, &quick_tex);
    pl_vulkan_sem_destroy(placebo_vulkan->gpu, &quick_sem);

    for (auto tex : placebo_tex)
        pl_tex_destroy(placebo_vulkan->gpu, &tex);

    FILE *file = fopen(qPrintable(shader_cache_path()), "wb");
    if (file) {
        pl_cache_save_file(placebo_cache, file);
        fclose(file);
    }
    pl_cache_destroy(&placebo_cache);
    pl_renderer_destroy(&placebo_renderer);
    pl_vulkan_destroy(&placebo_vulkan);
    pl_vk_inst_destroy(&placebo_vk_inst);
    pl_log_destroy(&placebo_log);
}

AVBufferRef *QmlVulkanMainWindow::hwDeviceCtx() {
    if (vulkan_hw_dev_ctx || vk_decode_queue_index < 0)
        return vulkan_hw_dev_ctx;

    vulkan_hw_dev_ctx = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VULKAN);
    AVHWDeviceContext *hwctx = reinterpret_cast<AVHWDeviceContext *>(vulkan_hw_dev_ctx->data);
    hwctx->user_opaque = const_cast<void *>(reinterpret_cast<const void *>(placebo_vulkan));
    AVVulkanDeviceContext *vkctx = reinterpret_cast<AVVulkanDeviceContext *>(hwctx->hwctx);
    vkctx->get_proc_addr = placebo_vulkan->get_proc_addr;
    vkctx->inst = placebo_vulkan->instance;
    vkctx->phys_dev = placebo_vulkan->phys_device;
    vkctx->act_dev = placebo_vulkan->device;
    vkctx->device_features = *placebo_vulkan->features;
    vkctx->enabled_inst_extensions = placebo_vk_inst->extensions;
    vkctx->nb_enabled_inst_extensions = placebo_vk_inst->num_extensions;
    vkctx->enabled_dev_extensions = placebo_vulkan->extensions;
    vkctx->nb_enabled_dev_extensions = placebo_vulkan->num_extensions;
    vkctx->queue_family_index = placebo_vulkan->queue_graphics.index;
    vkctx->nb_graphics_queues = placebo_vulkan->queue_graphics.count;
    vkctx->queue_family_tx_index = placebo_vulkan->queue_transfer.index;
    vkctx->nb_tx_queues = placebo_vulkan->queue_transfer.count;
    vkctx->queue_family_comp_index = placebo_vulkan->queue_compute.index;
    vkctx->nb_comp_queues = placebo_vulkan->queue_compute.count;
    vkctx->queue_family_decode_index = vk_decode_queue_index;
    vkctx->nb_decode_queues = 1;
    vkctx->lock_queue = [](struct AVHWDeviceContext *dev_ctx, uint32_t queue_family, uint32_t index) {
        auto vk = reinterpret_cast<pl_vulkan>(dev_ctx->user_opaque);
        vk->lock_queue(vk, queue_family, index);
    };
    vkctx->unlock_queue = [](struct AVHWDeviceContext *dev_ctx, uint32_t queue_family, uint32_t index) {
        auto vk = reinterpret_cast<pl_vulkan>(dev_ctx->user_opaque);
        vk->unlock_queue(vk, queue_family, index);
    };
    if (av_hwdevice_ctx_init(vulkan_hw_dev_ctx) < 0) {
        qCWarning(chiakiGui) << "Failed to create Vulkan decode context";
        av_buffer_unref(&vulkan_hw_dev_ctx);
    }

    return vulkan_hw_dev_ctx;
}

void QmlVulkanMainWindow::init(Settings *settings) {
    isVulkan = true;
    setSurfaceType(QWindow::VulkanSurface);

    const char *vk_exts[] = {
            nullptr,
            VK_KHR_SURFACE_EXTENSION_NAME,
    };

#if defined(Q_OS_LINUX)
    if (QGuiApplication::platformName().startsWith("wayland"))
        vk_exts[0] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    else if (QGuiApplication::platformName().startsWith("xcb"))
        vk_exts[0] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
    else
        Q_UNREACHABLE();
#elif defined(Q_OS_MACOS)
    vk_exts[0] = VK_EXT_METAL_SURFACE_EXTENSION_NAME;
#elif defined(Q_OS_WIN32)
    vk_exts[0] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#endif

    const char *opt_extensions[] = {
            VK_EXT_HDR_METADATA_EXTENSION_NAME,
    };

    struct pl_log_params log_params = {
            .log_cb = placebo_log_cb,
            .log_level = PL_LOG_DEBUG,
    };
    placebo_log = pl_log_create(PL_API_VER, &log_params);

    struct pl_vk_inst_params vk_inst_params = {
            .extensions = vk_exts,
            .num_extensions = 2,
            .opt_extensions = opt_extensions,
            .num_opt_extensions = 1,
    };
    placebo_vk_inst = pl_vk_inst_create(placebo_log, &vk_inst_params);

#define GET_PROC(name_) { \
    vk_funcs.name_ = reinterpret_cast<decltype(vk_funcs.name_)>( \
        placebo_vk_inst->get_proc_addr(placebo_vk_inst->instance, #name_)); \
    if (!vk_funcs.name_) { \
        qCCritical(chiakiGui) << "Failed to resolve" << #name_; \
        return; \
    } }
    GET_PROC(vkGetDeviceProcAddr)
#if defined(Q_OS_LINUX)
    if (QGuiApplication::platformName().startsWith("wayland")) GET_PROC(vkCreateWaylandSurfaceKHR)
    else if (QGuiApplication::platformName().startsWith("xcb")) GET_PROC(vkCreateXcbSurfaceKHR)
#elif defined(Q_OS_MACOS)
        GET_PROC(vkCreateMetalSurfaceEXT)
#elif defined(Q_OS_WIN32)
    GET_PROC(vkCreateWin32SurfaceKHR)
#endif
    GET_PROC(vkDestroySurfaceKHR)
    GET_PROC(vkGetPhysicalDeviceQueueFamilyProperties)
#undef GET_PROC

    const char *opt_dev_extensions[] = {
            VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
            VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME,
            VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME,
            VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME,
    };

    struct pl_vulkan_params vulkan_params = {
            .instance = placebo_vk_inst->instance,
            .get_proc_addr = placebo_vk_inst->get_proc_addr,
            .allow_software = true,
            PL_VULKAN_DEFAULTS
            .extra_queues = VK_QUEUE_VIDEO_DECODE_BIT_KHR,
            .opt_extensions = opt_dev_extensions,
            .num_opt_extensions = 4,
    };
    placebo_vulkan = pl_vulkan_create(placebo_log, &vulkan_params);

#define GET_PROC(name_) { \
    vk_funcs.name_ = reinterpret_cast<decltype(vk_funcs.name_)>( \
        vk_funcs.vkGetDeviceProcAddr(placebo_vulkan->device, #name_)); \
    if (!vk_funcs.name_) { \
        qCCritical(chiakiGui) << "Failed to resolve" << #name_; \
        return; \
    } }
    GET_PROC(vkWaitSemaphores)
#undef GET_PROC

    uint32_t queueFamilyCount = 0;
    vk_funcs.vkGetPhysicalDeviceQueueFamilyProperties(placebo_vulkan->phys_device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vk_funcs.vkGetPhysicalDeviceQueueFamilyProperties(placebo_vulkan->phys_device, &queueFamilyCount,
                                                      queueFamilyProperties.data());
    auto queue_it = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                                 [](VkQueueFamilyProperties prop) {
                                     return prop.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR;
                                 });
    if (queue_it != queueFamilyProperties.end())
        vk_decode_queue_index = std::distance(queueFamilyProperties.begin(), queue_it);

    struct pl_cache_params cache_params = {
            .log = placebo_log,
            .max_total_size = 10 << 20, // 10 MB
    };
    placebo_cache = pl_cache_create(&cache_params);
    pl_gpu_set_cache(placebo_vulkan->gpu, placebo_cache);
    FILE *file = fopen(qPrintable(shader_cache_path()), "rb");
    if (file) {
        pl_cache_load_file(placebo_cache, file);
        fclose(file);
    }

    placebo_renderer = pl_renderer_create(
            placebo_log,
            placebo_vulkan->gpu
    );

    struct pl_vulkan_sem_params sem_params = {
            .type = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    quick_sem = pl_vulkan_sem_create(placebo_vulkan->gpu, &sem_params);

    qt_vk_inst = new QVulkanInstance;
    qt_vk_inst->setVkInstance(placebo_vk_inst->instance);
    if (!qt_vk_inst->create())
        qFatal("Failed to create QVulkanInstance");

    quick_render = new RenderControl(this);

    QQuickWindow::setDefaultAlphaBuffer(true);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
    quick_window = new QQuickWindow(quick_render);
    quick_window->setVulkanInstance(qt_vk_inst);
    quick_window->setGraphicsDevice(
            QQuickGraphicsDevice::fromDeviceObjects(placebo_vulkan->phys_device, placebo_vulkan->device,
                                                    placebo_vulkan->queue_graphics.index));
    quick_window->setColor(QColor(0, 0, 0, 0));
    connect(quick_window, &QQuickWindow::focusObjectChanged, this, &QmlVulkanMainWindow::focusObjectChanged);

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
    connect(backend, &QmlBackend::windowTypeUpdated, this, &QmlMainWindow::updateWindowType);

    render_thread = new QThread;
    render_thread->setObjectName("render");
    render_thread->start();

    quick_render->prepareThread(render_thread);
    quick_render->moveToThread(render_thread);

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
    connect(update_timer, &QTimer::timeout, this, &QmlVulkanMainWindow::update);

    QMetaObject::invokeMethod(quick_render, &QQuickRenderControl::initialize);

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
    setZoomFactor(settings->GetZoomFactor());
}

void QmlVulkanMainWindow::createSwapchain() {

    Q_ASSERT(QThread::currentThread() == render_thread);

    if (placebo_swapchain)
        return;

    VkResult err = VK_ERROR_UNKNOWN;
#if defined(Q_OS_LINUX)
    if (QGuiApplication::platformName().startsWith("wayland")) {
        VkWaylandSurfaceCreateInfoKHR surfaceInfo = {};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.display = reinterpret_cast<struct wl_display *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow(
                "display", this));
        surfaceInfo.surface = reinterpret_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow(
                "surface", this));
        err = vk_funcs.vkCreateWaylandSurfaceKHR(placebo_vk_inst->instance, &surfaceInfo, nullptr, &surface);
    } else if (QGuiApplication::platformName().startsWith("xcb")) {
        VkXcbSurfaceCreateInfoKHR surfaceInfo = {};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.connection = reinterpret_cast<xcb_connection_t *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow(
                "connection", this));
        surfaceInfo.window = static_cast<xcb_window_t>(winId());
        err = vk_funcs.vkCreateXcbSurfaceKHR(placebo_vk_inst->instance, &surfaceInfo, nullptr, &surface);
    } else {
        Q_UNREACHABLE();
    }
#elif defined(Q_OS_MACOS)
    VkMetalSurfaceCreateInfoEXT surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    surfaceInfo.pLayer = static_cast<const CAMetalLayer*>(reinterpret_cast<void*(*)(id, SEL)>(objc_msgSend)(reinterpret_cast<id>(winId()), sel_registerName("layer")));
    err = vk_funcs.vkCreateMetalSurfaceEXT(placebo_vk_inst->instance, &surfaceInfo, nullptr, &surface);
#elif defined(Q_OS_WIN32)
    VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hinstance = GetModuleHandle(nullptr);
    surfaceInfo.hwnd = reinterpret_cast<HWND>(winId());
    err = vk_funcs.vkCreateWin32SurfaceKHR(placebo_vk_inst->instance, &surfaceInfo, nullptr, &surface);
#endif

    if (err != VK_SUCCESS)
        qFatal("Failed to create VkSurfaceKHR");

    struct pl_vulkan_swapchain_params swapchain_params = {
            .surface = surface,
            .present_mode = VK_PRESENT_MODE_FIFO_KHR,
            .swapchain_depth = 1,
    };
    placebo_swapchain = pl_vulkan_create_swapchain(placebo_vulkan, &swapchain_params);
}

void QmlVulkanMainWindow::destroySwapchain() {
    Q_ASSERT(QThread::currentThread() == render_thread);

    if (!placebo_swapchain)
        return;

    pl_swapchain_destroy(&placebo_swapchain);
    vk_funcs.vkDestroySurfaceKHR(placebo_vk_inst->instance, surface, nullptr);
}

void QmlVulkanMainWindow::resizeSwapchain() {
    Q_ASSERT(QThread::currentThread() == render_thread);

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
            .format = pl_find_fmt(placebo_vulkan->gpu, PL_FMT_UNORM, 4, 0, 0, PL_FMT_CAP_RENDERABLE),
            .sampleable = true,
            .renderable = true,
    };
    if (!pl_tex_recreate(placebo_vulkan->gpu, &quick_tex, &tex_params))
        qCCritical(chiakiGui) << "Failed to create placebo texture";

    VkFormat vk_format;
    VkImage vk_image = pl_vulkan_unwrap(placebo_vulkan->gpu, quick_tex, &vk_format, nullptr);
    quick_window->setRenderTarget(
            QQuickRenderTarget::fromVulkanImage(vk_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk_format,
                                                swapchain_size));
}

void QmlVulkanMainWindow::beginFrame() {
    if (quick_frame)
        return;

    struct pl_vulkan_hold_params hold_params = {
            .tex = quick_tex,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .qf = VK_QUEUE_FAMILY_IGNORED,
            .semaphore = {
                    .sem = quick_sem,
                    .value = ++quick_sem_value,
            }
    };
    pl_vulkan_hold_ex(placebo_vulkan->gpu, &hold_params);

    VkSemaphoreWaitInfo wait_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores = &quick_sem,
            .pValues = &quick_sem_value,
    };
    vk_funcs.vkWaitSemaphores(placebo_vulkan->device, &wait_info, UINT64_MAX);

    quick_frame = true;
    quick_render->beginFrame();
}

void QmlVulkanMainWindow::endFrame() {
    if (!quick_frame)
        return;

    quick_frame = false;
    quick_render->endFrame();

    struct pl_vulkan_release_params release_params = {
            .tex = quick_tex,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .qf = VK_QUEUE_FAMILY_IGNORED,
    };
    pl_vulkan_release_ex(placebo_vulkan->gpu, &release_params);
}

pl_gpu QmlVulkanMainWindow::get_pl_gpu() {
    return placebo_vulkan->gpu;
}




