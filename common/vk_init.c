/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include "vk_tools.h"
#include "vk_debug.h"
#include "vk_winsys.h"


vk_t vk = {0};



static int
vkin_create_instance (const char* app_name)
{
    const char **inst_ext = NULL;
    const char **inst_lyr = NULL;
    uint32_t inst_ext_num = 0;
    uint32_t inst_lyr_num = 0;

    /* Enable all instance extensions */
    VK_CHECK (vkEnumerateInstanceExtensionProperties (NULL, &inst_ext_num, NULL));
    VkExtensionProperties *props = VK_CALLOC (VkExtensionProperties, inst_ext_num);
    VK_CHECK (vkEnumerateInstanceExtensionProperties (NULL, &inst_ext_num, props));

    inst_ext = VK_CALLOC (const char *, inst_ext_num);
    for (uint32_t i = 0; i < inst_ext_num; i ++)
    {
        inst_ext[i] = props[i].extensionName;
        VK_PRINT ("inst_ext[%2d/%2d] %s\n", i, inst_ext_num, inst_ext[i]);
    }

    VkApplicationInfo appInfo = {0};
    appInfo.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = app_name;
    appInfo.pEngineName      = app_name;
    appInfo.apiVersion       = VK_API_VERSION_1_0;
    appInfo.engineVersion    = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo ci = {0};
    ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pNext                   = NULL;
    ci.pApplicationInfo        = &appInfo;
    ci.enabledExtensionCount   = inst_ext_num;
    ci.ppEnabledExtensionNames = inst_ext;
    ci.enabledLayerCount       = inst_lyr_num;
    ci.ppEnabledLayerNames     = inst_lyr;

    VK_CHECK (vkCreateInstance (&ci, NULL, &vk.instance));

    return 0;
}

static int
vkin_select_physical_device ()
{
    uint32_t gpu_count = 0;
    VK_CHECK (vkEnumeratePhysicalDevices (vk.instance, &gpu_count, NULL));
    VK_PRINT ("GPU count = %d\n", gpu_count);

    VkPhysicalDevice *phydevs = VK_CALLOC (VkPhysicalDevice, gpu_count);
    VK_CHECK (vkEnumeratePhysicalDevices (vk.instance, &gpu_count, phydevs));

    /* select the first one */
    vk.phydev = phydevs[0];

    vkGetPhysicalDeviceMemoryProperties (vk.phydev, &vk.phymem_props);

    VK_PRINT ("phymem_props: memoryHeapCount = %d\n", vk.phymem_props.memoryHeapCount);
    VK_PRINT ("phymem_props: memoryTypeCount = %d\n", vk.phymem_props.memoryTypeCount);
    
    return 0;
}

static int
vkin_select_devqueue_index ()
{
    uint32_t propcnt;
    vkGetPhysicalDeviceQueueFamilyProperties (vk.phydev, &propcnt, NULL);
    VkQueueFamilyProperties *props = VK_CALLOC (VkQueueFamilyProperties, propcnt);
    vkGetPhysicalDeviceQueueFamilyProperties (vk.phydev, &propcnt, props);

    uint32_t qidx_graphics = ~0;
    for (uint32_t i = 0; i < propcnt; i++)
    {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            qidx_graphics = i;
            break;
        }
    }

    if (qidx_graphics == ~0)
    {
        return -1;
    }

    vk.qidx_graphics = qidx_graphics;
    VK_PRINT ("qidx_graphics = %d/%d\n", qidx_graphics, propcnt);

    return 0;
}


static int
vkin_create_device ()
{
    const char **dev_ext = NULL;
    uint32_t dev_ext_num = 0;

    /* Enable all device extensions */
    VK_CHECK (vkEnumerateDeviceExtensionProperties (vk.phydev, NULL, &dev_ext_num, NULL));
    VkExtensionProperties *props = VK_CALLOC (VkExtensionProperties, dev_ext_num);
    VK_CHECK (vkEnumerateDeviceExtensionProperties (vk.phydev, NULL, &dev_ext_num, props));

    dev_ext = VK_CALLOC (const char *, dev_ext_num);
    for (uint32_t i = 0; i < dev_ext_num; i ++)
    {
        dev_ext[i] = props[i].extensionName;
        VK_PRINT ("dev_ext[%2d/%2d] %s\n", i, dev_ext_num, dev_ext[i]);
    }


    const float defaultQueuePriority = 0.0f;

    VkDeviceQueueCreateInfo devQueueCI = {0};
    devQueueCI.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    devQueueCI.queueFamilyIndex = vk.qidx_graphics;
    devQueueCI.queueCount       = 1;
    devQueueCI.pQueuePriorities = &defaultQueuePriority;

    VkDeviceCreateInfo ci = {0};
    ci.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.pQueueCreateInfos        = &devQueueCI;
    ci.queueCreateInfoCount     = 1;
    ci.ppEnabledExtensionNames  = dev_ext;
    ci.enabledExtensionCount    = dev_ext_num;

    VK_CHECK (vkCreateDevice (vk.phydev, &ci, NULL, &vk.dev));

    /* device queue */
    vkGetDeviceQueue (vk.dev, vk.qidx_graphics, 0, &vk.devq);

    return 0;
}


static int
vkin_create_command_pool ()
{
    VkCommandPoolCreateInfo ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.queueFamilyIndex = vk.qidx_graphics;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_CHECK (vkCreateCommandPool (vk.dev, &ci, NULL, &vk.cmd_pool));

    return 0;
}


static int
vkin_create_surface (VkFormat format)
{
    vk.surface = vkin_winsys_create_surface (vk.instance);

    uint32_t count = 0;
    VK_CHECK (vkGetPhysicalDeviceSurfaceFormatsKHR (vk.phydev, vk.surface, &count, NULL));
    VkSurfaceFormatKHR *formats = VK_CALLOC (VkSurfaceFormatKHR, count);
    VK_CHECK (vkGetPhysicalDeviceSurfaceFormatsKHR (vk.phydev, vk.surface, &count, formats));

    for (uint32_t i = 0; i < count; i ++)
    {
        if (formats[i].format == format)
        {
            vk.sfc_fmt = formats[i];
            VK_PRINT ("surface_fmt[%2d/%2d] \n", i, count);
        }
    }

    VK_CHECK (vkGetPhysicalDeviceSurfaceCapabilitiesKHR (vk.phydev, vk.surface, &vk.sfc_caps));

    VkBool32 is_support;
    VK_CHECK (vkGetPhysicalDeviceSurfaceSupportKHR (vk.phydev, vk.qidx_graphics, vk.surface, &is_support));
    if (is_support == false)
    {
        VK_LOGE ("vkGetPhysicalDeviceSurfaceSupportKHR(): qid=%d", vk.qidx_graphics);
        return -1;
    }

    return 0;
}


static int
vkin_create_swap_chain (int win_w, int win_h)
{
    VkExtent2D extent = vk.sfc_caps.currentExtent;
    if (extent.width == ~0u)
    {
        extent.width  = win_w;
        extent.height = win_h;
    }

    uint32_t imgCount = vk.sfc_caps.minImageCount;

    VkSwapchainCreateInfoKHR ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = vk.surface;
    ci.minImageCount    = imgCount;
    ci.imageFormat      = vk.sfc_fmt.format;
    ci.imageColorSpace  = vk.sfc_fmt.colorSpace;
    ci.imageExtent      = extent;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.preTransform     = vk.sfc_caps.currentTransform;
    ci.imageArrayLayers = 1;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.queueFamilyIndexCount = 0;
    ci.presentMode      = VK_PRESENT_MODE_FIFO_KHR;
    ci.oldSwapchain     = VK_NULL_HANDLE;
    ci.clipped          = VK_TRUE;
    ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    VK_CHECK (vkCreateSwapchainKHR (vk.dev, &ci, NULL, &vk.swapchain));

    vk.swapchain_extent = extent;
    return 0;
}


static uint32_t
getMemoryTypeIndex (uint32_t requestBits, VkMemoryPropertyFlags requestProps)
{
    uint32_t result = ~0u;
    for (uint32_t i = 0; i < vk.phymem_props.memoryTypeCount; i++)
    {
        if (requestBits & 1)
        {
            VkMemoryType memtype = vk.phymem_props.memoryTypes[i];
            if ((memtype.propertyFlags & requestProps) == requestProps)
            {
                result = i;
                break;
            }
        }
        requestBits >>= 1;
    }
    return result;
}

static int
vkin_create_depth_buffer ()
{
    VkImageCreateInfo ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType        = VK_IMAGE_TYPE_2D;
    ci.format           = VK_FORMAT_D32_SFLOAT;
    ci.extent.width     = vk.swapchain_extent.width;
    ci.extent.height    = vk.swapchain_extent.height;
    ci.extent.depth     = 1;
    ci.mipLevels        = 1;
    ci.usage            = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ci.samples          = VK_SAMPLE_COUNT_1_BIT;
    ci.arrayLayers      = 1;

    VK_CHECK (vkCreateImage (vk.dev, &ci, NULL, &vk.dbuf));


    /* Allocate memory for Depth Buffer */
    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements (vk.dev, vk.dbuf, &reqs);

    VkMemoryAllocateInfo ai = {0};
    ai.sType            = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize   = reqs.size;
    ai.memoryTypeIndex  = getMemoryTypeIndex (reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK (vkAllocateMemory  (vk.dev, &ai, NULL, &vk.dbuf_mem));


    /* bind */
    VK_CHECK (vkBindImageMemory (vk.dev, vk.dbuf, vk.dbuf_mem, 0));

    return 0;
}


static int
vkin_create_views ()
{
    uint32_t imageCount;
    VK_CHECK (vkGetSwapchainImagesKHR (vk.dev, vk.swapchain, &imageCount, NULL));
    vk.swapchain_imgs = VK_CALLOC (VkImage, imageCount);
    VK_CHECK (vkGetSwapchainImagesKHR (vk.dev, vk.swapchain, &imageCount, vk.swapchain_imgs));

    vk.swapchain_views = VK_CALLOC (VkImageView, imageCount);

    // color buffer
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        VkImageViewCreateInfo ci = {0};
        ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        ci.format           = vk.sfc_fmt.format;
        ci.components.r     = VK_COMPONENT_SWIZZLE_R;
        ci.components.g     = VK_COMPONENT_SWIZZLE_G;
        ci.components.b     = VK_COMPONENT_SWIZZLE_B;
        ci.components.a     = VK_COMPONENT_SWIZZLE_A;
        ci.image            = vk.swapchain_imgs[i];
        ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ci.subresourceRange.baseMipLevel   = 0;
        ci.subresourceRange.levelCount     = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount     = 1;

        VK_CHECK (vkCreateImageView (vk.dev, &ci, NULL, &vk.swapchain_views[i]));
    }

    // depth buffer
    {
        VkImageViewCreateInfo ci = {0};
        ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        ci.format           = VK_FORMAT_D32_SFLOAT;
        ci.components.r     = VK_COMPONENT_SWIZZLE_R;
        ci.components.g     = VK_COMPONENT_SWIZZLE_G;
        ci.components.b     = VK_COMPONENT_SWIZZLE_B;
        ci.components.a     = VK_COMPONENT_SWIZZLE_A;
        ci.image            = vk.dbuf;
        ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        ci.subresourceRange.baseMipLevel   = 0;
        ci.subresourceRange.levelCount     = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount     = 1;

        VK_CHECK (vkCreateImageView (vk.dev, &ci, NULL, &vk.dbuf_view));
    }

    return 0;
}


static int
vkin_create_render_pass ()
{
    VkAttachmentDescription attachments[2] = {0};

    /* color */
    attachments[0].format         = vk.sfc_fmt.format;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    /* depth */
    attachments[1].format         = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {0};
    colorReference.attachment   = 0;
    colorReference.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {0};
    depthReference.attachment   = 1;
    depthReference.layout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc = {0};
    subpassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount    = 1;
    subpassDesc.pColorAttachments       = &colorReference;
    subpassDesc.pDepthStencilAttachment = &depthReference;


    VkRenderPassCreateInfo ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount  = 2;
    ci.pAttachments     = attachments;
    ci.subpassCount     = 1;
    ci.pSubpasses       = &subpassDesc;

    VK_CHECK (vkCreateRenderPass (vk.dev, &ci, NULL, &vk.render_pass));

    return 0;
}


static int
vkin_create_frame_buffer ()
{
    uint32_t imageCount;
    VK_CHECK (vkGetSwapchainImagesKHR (vk.dev, vk.swapchain, &imageCount, NULL));
    vk.framebuffers = VK_CALLOC (VkFramebuffer, imageCount);


    VkFramebufferCreateInfo ci = {0};
    ci.sType        = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass   = vk.render_pass;
    ci.width        = vk.swapchain_extent.width;
    ci.height       = vk.swapchain_extent.height;
    ci.layers       = 1;

    for (uint32_t i = 0; i < imageCount; i ++)
    {
        VkImageView attachments[2];
        attachments[0] = vk.swapchain_views[i];
        attachments[1] = vk.dbuf_view;

        ci.attachmentCount = 2;
        ci.pAttachments    = attachments;

        VK_CHECK (vkCreateFramebuffer (vk.dev, &ci, NULL, &vk.framebuffers[i]));
    }

    return 0;
}

static int
vkin_create_command_buffer ()
{
    uint32_t imageCount;
    VK_CHECK (vkGetSwapchainImagesKHR (vk.dev, vk.swapchain, &imageCount, NULL));

    VkCommandBufferAllocateInfo ai = {0};
    ai.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool          = vk.cmd_pool;
    ai.commandBufferCount   = imageCount;
    ai.level                = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vk.cmd_bufs = VK_CALLOC (VkCommandBuffer, imageCount);

    VK_CHECK (vkAllocateCommandBuffers (vk.dev, &ai, vk.cmd_bufs));

    /* allocate fences */
    vk.fences = VK_CALLOC (VkFence, imageCount);

    VkFenceCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < imageCount; i ++)
    {
        VK_CHECK (vkCreateFence (vk.dev, &ci, NULL, &vk.fences[i]));
    }

    return 0;
}

static int
vkin_create_semaphore ()
{
    VkSemaphoreCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CHECK (vkCreateSemaphore (vk.dev, &ci, NULL, &vk.sem_render_complete));
    VK_CHECK (vkCreateSemaphore (vk.dev, &ci, NULL, &vk.sem_present_complete));

    return 0;
}


int
vk_init (int win_w, int win_h)
{
    vkin_create_instance ("VulkanApp");

    vkin_winsys_init (win_w, win_h);

    vkin_debug_init (vk.instance);

    vkin_select_physical_device ();

    vkin_select_devqueue_index ();

    vkin_create_device ();

    vkin_create_command_pool ();

    vkin_create_surface (VK_FORMAT_B8G8R8A8_UNORM);

    vkin_create_swap_chain (win_w, win_h);

    vkin_create_depth_buffer ();

    vkin_create_views ();

    vkin_create_render_pass ();

    vkin_create_frame_buffer ();

    vkin_create_command_buffer ();

    vkin_create_semaphore ();

    return 0;
}
