/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef _VK_UTIL_H_
#define _VK_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <vulkan/vulkan.h>

#define VK_LOGI(fmt, ...) fprintf(stderr, "%s(%d): " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define VK_LOGE(fmt, ...) fprintf(stderr, "%s(%d): " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)

#define VK_PRINT(...) fprintf(stderr, __VA_ARGS__)

#define VK_CHECK(f)                                                             \
{                                                                               \
    VkResult vkret = (f);                                                       \
    if (vkret != VK_SUCCESS)                                                    \
    {                                                                           \
        VK_LOGE("ERR: vkret = %x", vkret);                                      \
    }                                                                           \
}

#define VK_CALLOC(type, count) ((type *)calloc(sizeof(type), count))

#define VK_GET_PROC_ADDR(inst, name)                                            \
{                                                                               \
    _##name = (void *)vkGetInstanceProcAddr(inst, #name);                       \
    if (!_##name)                                                               \
    {                                                                           \
        VK_LOGE("ERR: can't get proc addr: %s", #name "()");                    \
    }                                                                           \
}

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))


#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif


typedef struct _vk_t
{
    VkInstance                          instance;
    VkPhysicalDevice                    phydev;
    VkPhysicalDeviceMemoryProperties    phymem_props;

    uint32_t                            qidx_graphics;
    uint32_t                            qidx_compute;
    uint32_t                            qidx_transfer;

    VkDevice                            dev;
    VkQueue                             devq;

    VkCommandPool                       cmd_pool;

    VkSurfaceKHR                        surface;
    VkSurfaceFormatKHR                  sfc_fmt;
    VkSurfaceCapabilitiesKHR            sfc_caps;

    /* Swap Chain */
    VkSwapchainKHR                      swapchain;
    VkExtent2D                          swapchain_extent;
    VkImage                             *swapchain_imgs;
    VkImageView                         *swapchain_views;

    /* Depth Buffer */
    VkImage                             dbuf;
    VkDeviceMemory                      dbuf_mem;
    VkImageView                         dbuf_view;

    VkRenderPass                        render_pass;
    VkFramebuffer                       *framebuffers;

    VkCommandBuffer                     *cmd_bufs;
    VkFence                             *fences;
    VkSemaphore                         sem_render_complete;
    VkSemaphore                         sem_present_complete;

    /* for Render */
    uint32_t                            image_index;
    void                                *user_data;
} vk_t;


typedef struct _vt_buffer_t
{
    VkBuffer        buf;
    VkDeviceMemory  mem;
} vk_buffer_t;


typedef struct _vt_texture_t
{
    VkImage         img;
    VkDeviceMemory  mem;
    VkImageView     view;
    VkSampler       sampler;
} vk_texture_t;

    
#ifdef __cplusplus
}
#endif
#endif /* _VK_UTIL_H_ */
