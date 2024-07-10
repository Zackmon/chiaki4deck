//
// Created by zackmon on 7/10/24.
//

#ifndef CHIAKI_QMLVULKANMAINWINDOW_H
#define CHIAKI_QMLVULKANMAINWINDOW_H

#include "qmlmainwindow.h"

extern "C"{
#include <libplacebo/vulkan.h>
}

#include <vulkan/vulkan.h>

#if defined(Q_OS_LINUX)
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#include <vulkan/vulkan_wayland.h>
#elif defined(Q_OS_MACOS)
#include <vulkan/vulkan_metal.h>
#elif defined(Q_OS_WIN)
#include <vulkan/vulkan_win32.h>
#endif
Q_DECLARE_LOGGING_CATEGORY(chiakiGuiVulkan);

class QmlVulkanMainWindow : public QmlMainWindow{

public:
    QmlVulkanMainWindow(Settings *settings);
    QmlVulkanMainWindow(const StreamSessionConnectInfo &connect_info);
    ~QmlVulkanMainWindow();

    AVBufferRef *hwDeviceCtx() override;


protected:
    void init(Settings *settings);
    void createSwapchain() override;
    void destroySwapchain() override;
    void resizeSwapchain() override;
    void beginFrame() override;
    void endFrame() override;
    pl_gpu get_pl_gpu() override;

private:
    pl_vk_inst placebo_vk_inst = {};
    pl_vulkan placebo_vulkan = {};
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    AVBufferRef *vulkan_hw_dev_ctx = nullptr;
    int vk_decode_queue_index = -1;



    QVulkanInstance *qt_vk_inst = {};
    VkSemaphore quick_sem = VK_NULL_HANDLE;
    uint64_t quick_sem_value = 0;

    struct {
        PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
#if defined(Q_OS_LINUX)
        PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
        PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
#elif defined(Q_OS_MACOS)
        PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT;
#elif defined(Q_OS_WIN32)
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#endif
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
        PFN_vkWaitSemaphores vkWaitSemaphores;
        PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    } vk_funcs;

};


#endif //CHIAKI_QMLVULKANMAINWINDOW_H
