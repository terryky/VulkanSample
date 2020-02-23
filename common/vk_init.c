/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include "vk_tools.h"
#include "vk_init.h"
#include "vk_debug.h"
#include "vk_winsys.h"


static int
vkin_create_instance (vk_t *vk, const char* app_name)
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
        //VK_PRINT ("inst_ext[%2d/%2d] %s\n", i, inst_ext_num, inst_ext[i]);
    }

    /* Layer properties */
    uint32_t lyr_num;
    VK_CHECK (vkEnumerateInstanceLayerProperties (&lyr_num, NULL));
    VkLayerProperties *lyr_props = VK_CALLOC (VkLayerProperties, lyr_num);
    VK_CHECK (vkEnumerateInstanceLayerProperties (&lyr_num, lyr_props));

    inst_lyr = VK_CALLOC (const char *, lyr_num);
    for (uint32_t i = 0; i < lyr_num; i ++)
    {
        char *lyr_name = lyr_props[i].layerName;
        VK_PRINT ("inst_lyr[%2d/%2d] %s ", i, inst_lyr_num, lyr_name);

        if (!strcmp(lyr_name, "VK_LAYER_KHRONOS_validation"))
        {
            VK_PRINT ("[ENABLED]");
            inst_lyr[inst_lyr_num] = lyr_name;
            inst_lyr_num ++;
        }
        VK_PRINT ("\n");
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

    VK_CHECK (vkCreateInstance (&ci, NULL, &vk->instance));

    return 0;
}

static int
vkin_select_physical_device (vk_t *vk)
{
    uint32_t gpu_count = 0;
    VK_CHECK (vkEnumeratePhysicalDevices (vk->instance, &gpu_count, NULL));
    VK_PRINT ("GPU count = %d\n", gpu_count);

    VkPhysicalDevice *phydevs = VK_CALLOC (VkPhysicalDevice, gpu_count);
    VK_CHECK (vkEnumeratePhysicalDevices (vk->instance, &gpu_count, phydevs));

    /* select the first one */
    vk->phydev = phydevs[0];

    /* general properties of physical devices */
    const char dtype_name[8][32] = {"OTHER", "INTEGRATED_GPU", "DISCRETE_GPU", "VIRTUAL_GPU", "CPU"};
    VkPhysicalDeviceProperties dprops;
    vkGetPhysicalDeviceProperties (vk->phydev, &dprops);
    VK_PRINT ("------- VkPhysicalDeviceProperties -------\n");
    VK_PRINT (" apiVersion   : 0x%08x (%d.%d.%d)\n", dprops.apiVersion,
                                VK_VERSION_MAJOR (dprops.apiVersion),
                                VK_VERSION_MINOR (dprops.apiVersion),
                                VK_VERSION_PATCH (dprops.apiVersion));
    VK_PRINT (" driverVersion: 0x%08x (%d.%d.%d)\n", dprops.driverVersion,
                                VK_VERSION_MAJOR (dprops.driverVersion),
                                VK_VERSION_MINOR (dprops.driverVersion),
                                VK_VERSION_PATCH (dprops.driverVersion));
    VK_PRINT (" vendorID     : 0x%08x\n", dprops.vendorID);
    VK_PRINT (" deviceID     : 0x%08x\n", dprops.deviceID);
    VK_PRINT (" deviceType   : 0x%08x (%s)\n", dprops.deviceType,
                                          dtype_name[dprops.deviceType & 0xF]);
    VK_PRINT (" deviceName   : %s\n",     dprops.deviceName);
    VK_PRINT ("------------------------------------------\n");
    vk->phydev_props = dprops;

    /* Memory properties of physical devices */
    vkGetPhysicalDeviceMemoryProperties (vk->phydev, &vk->phymem_props);

    VK_PRINT ("phymem_props: memoryHeapCount = %d\n", vk->phymem_props.memoryHeapCount);
    VK_PRINT ("phymem_props: memoryTypeCount = %d\n", vk->phymem_props.memoryTypeCount);


    if (dprops.apiVersion >= VK_MAKE_VERSION (1, 1, 0))
    {
        vk_struct_chain_info_t chain_info[1] = {0};
        chain_info[0].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES_KHR;
        chain_info[0].size  = sizeof (VkPhysicalDeviceDriverPropertiesKHR);

        VkPhysicalDeviceProperties2 dprops2 = {0};
        dprops2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        vk_build_struct_chain ((VkBaseOutStructure *)&dprops2, chain_info, ARRAY_LENGTH (chain_info));

        vkGetPhysicalDeviceProperties2 (vk->phydev, &dprops2);
        
        VkPhysicalDeviceDriverPropertiesKHR *ddprop = dprops2.pNext;
        VK_PRINT ("-- VkPhysicalDeviceDriverPropertiesKHR ---\n");
        VK_PRINT (" driverID     : %d\n",     ddprop->driverID);
        VK_PRINT (" driverName   : %s\n",     ddprop->driverName);
        VK_PRINT (" driverInfo   : %s\n",     ddprop->driverInfo);
        VK_PRINT (" conformance  : (%d.%d.%d.%d)\n", 
                            ddprop->conformanceVersion.major,
                            ddprop->conformanceVersion.minor,
                            ddprop->conformanceVersion.subminor,
                            ddprop->conformanceVersion.patch);
        VK_PRINT ("------------------------------------------\n");
    }
    
    return 0;
}

static int
vkin_select_devqueue_index (vk_t *vk)
{
    uint32_t propcnt;
    vkGetPhysicalDeviceQueueFamilyProperties (vk->phydev, &propcnt, NULL);
    VkQueueFamilyProperties *props = VK_CALLOC (VkQueueFamilyProperties, propcnt);
    vkGetPhysicalDeviceQueueFamilyProperties (vk->phydev, &propcnt, props);

    uint32_t qidx_graphics = ~0;
    for (uint32_t i = 0; i < propcnt; i++)
    {
        VK_PRINT ("VkQueueFamilyProperties[%d/%d] ", i, propcnt);
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            VK_PRINT ("GRAPHICS | ");
            if (qidx_graphics == ~0)
                qidx_graphics = i;
        }
        if (props[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            VK_PRINT ("COMPUTE | ");
        }
        if (props[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            VK_PRINT ("TRANSFER | ");
        }
        if (props[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT )
        {
            VK_PRINT ("SPARSE | ");
        }
        if (props[i].queueFlags & VK_QUEUE_PROTECTED_BIT )
        {
            VK_PRINT ("PROTECTED | ");
        }
        VK_PRINT ("\n");
    }

    if (qidx_graphics == ~0)
    {
        return -1;
    }

    vk->qidx_graphics = qidx_graphics;
    VK_PRINT ("qidx_graphics = %d/%d\n", qidx_graphics, propcnt);

    return 0;
}


static int
vkin_create_device (vk_t *vk)
{
    const char **dev_ext = NULL;
    uint32_t dev_ext_num = 0;

    /* Enable all device extensions */
    VK_CHECK (vkEnumerateDeviceExtensionProperties (vk->phydev, NULL, &dev_ext_num, NULL));
    VkExtensionProperties *props = VK_CALLOC (VkExtensionProperties, dev_ext_num);
    VK_CHECK (vkEnumerateDeviceExtensionProperties (vk->phydev, NULL, &dev_ext_num, props));

    dev_ext = VK_CALLOC (const char *, dev_ext_num);
    for (uint32_t i = 0; i < dev_ext_num; i ++)
    {
        dev_ext[i] = props[i].extensionName;
        //VK_PRINT ("dev_ext[%2d/%2d] %s\n", i, dev_ext_num, dev_ext[i]);
    }


    const float defaultQueuePriority = 0.0f;

    VkDeviceQueueCreateInfo devQueueCI = {0};
    devQueueCI.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    devQueueCI.queueFamilyIndex = vk->qidx_graphics;
    devQueueCI.queueCount       = 1;
    devQueueCI.pQueuePriorities = &defaultQueuePriority;

    VkDeviceCreateInfo ci = {0};
    ci.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.pQueueCreateInfos        = &devQueueCI;
    ci.queueCreateInfoCount     = 1;
    ci.ppEnabledExtensionNames  = dev_ext;
    ci.enabledExtensionCount    = dev_ext_num;

    VK_CHECK (vkCreateDevice (vk->phydev, &ci, NULL, &vk->dev));

    /* device queue */
    vkGetDeviceQueue (vk->dev, vk->qidx_graphics, 0, &vk->devq);

    return 0;
}


static int
vkin_create_command_pool (vk_t *vk)
{
    VkCommandPoolCreateInfo ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.queueFamilyIndex = vk->qidx_graphics;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_CHECK (vkCreateCommandPool (vk->dev, &ci, NULL, &vk->cmd_pool));

    return 0;
}


static int
vkin_create_surface (vk_t *vk, VkFormat format)
{
    vk->surface = vkin_winsys_create_surface (vk->instance);

    /* available format */
    uint32_t count = 0;
    VK_CHECK (vkGetPhysicalDeviceSurfaceFormatsKHR (vk->phydev, vk->surface, &count, NULL));
    VkSurfaceFormatKHR *formats = VK_CALLOC (VkSurfaceFormatKHR, count);
    VK_CHECK (vkGetPhysicalDeviceSurfaceFormatsKHR (vk->phydev, vk->surface, &count, formats));

    for (uint32_t i = 0; i < count; i ++)
    {
        if (formats[i].format == format)
        {
            vk->sfc_fmt = formats[i];
            VK_PRINT ("surface_fmt[%2d/%2d] \n", i, count);
        }
    }

    /* capabilities */
    VK_CHECK (vkGetPhysicalDeviceSurfaceCapabilitiesKHR (vk->phydev, vk->surface, &vk->sfc_caps));

    /* This queue family supports presentation to the surface ? */
    VkBool32 is_support;
    VK_CHECK (vkGetPhysicalDeviceSurfaceSupportKHR (vk->phydev, vk->qidx_graphics, vk->surface, &is_support));
    if (is_support == false)
    {
        VK_LOGE ("vkGetPhysicalDeviceSurfaceSupportKHR(): qid=%d", vk->qidx_graphics);
        return -1;
    }

    return 0;
}


static int
vkin_create_swap_chain (vk_t *vk, int win_w, int win_h, VkPresentModeKHR present_mode)
{
    VkExtent2D extent = vk->sfc_caps.currentExtent;
    if (extent.width == ~0u)
    {
        extent.width  = win_w;
        extent.height = win_h;
    }

    uint32_t imgCount = vk->sfc_caps.minImageCount;

    /* available present mode */
    uint32_t count = 0;
    VK_CHECK (vkGetPhysicalDeviceSurfacePresentModesKHR (vk->phydev, vk->surface, &count, NULL));
    VkPresentModeKHR *present_modes = VK_CALLOC (VkPresentModeKHR, count);
    VK_CHECK (vkGetPhysicalDeviceSurfacePresentModesKHR (vk->phydev, vk->surface, &count, present_modes));

    int found = 0;
    for (uint32_t i = 0; i < count; i ++)
    {
        if (present_modes[i] == present_mode )
        {
            found = 1;
            break;
        }
    }
    if (found == 0)
    {
        VK_LOGE ("This surface doesn't support the indicated present mode.(%d)", present_mode);
        return -1;
    }

#if 0 /* force disable VSYNC */
    present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
#endif

    VkSwapchainCreateInfoKHR ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = vk->surface;
    ci.minImageCount    = imgCount;
    ci.imageFormat      = vk->sfc_fmt.format;
    ci.imageColorSpace  = vk->sfc_fmt.colorSpace;
    ci.imageExtent      = extent;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.preTransform     = vk->sfc_caps.currentTransform;
    ci.imageArrayLayers = 1;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.queueFamilyIndexCount = 0;
    ci.presentMode      = present_mode;
    ci.oldSwapchain     = VK_NULL_HANDLE;
    ci.clipped          = VK_TRUE;
    ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    VK_CHECK (vkCreateSwapchainKHR (vk->dev, &ci, NULL, &vk->swapchain));

    vk->swapchain_extent = extent;
    return 0;
}


static int
vkin_create_swap_chain_views (vk_t *vk)
{
    /* Get "Image" for Color Buffer */
    uint32_t imageCount;
    VK_CHECK (vkGetSwapchainImagesKHR (vk->dev, vk->swapchain, &imageCount, NULL));
    vk->swapchain_imgs = VK_CALLOC (VkImage, imageCount);
    VK_CHECK (vkGetSwapchainImagesKHR (vk->dev, vk->swapchain, &imageCount, vk->swapchain_imgs));

    vk->swapchain_views = VK_CALLOC (VkImageView, imageCount);

    /* Create "ImageView" for Color Buffer */
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        VkImageViewCreateInfo ci = {0};
        ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        ci.format           = vk->sfc_fmt.format;
        ci.components.r     = VK_COMPONENT_SWIZZLE_R;
        ci.components.g     = VK_COMPONENT_SWIZZLE_G;
        ci.components.b     = VK_COMPONENT_SWIZZLE_B;
        ci.components.a     = VK_COMPONENT_SWIZZLE_A;
        ci.image            = vk->swapchain_imgs[i];
        ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ci.subresourceRange.baseMipLevel   = 0;
        ci.subresourceRange.levelCount     = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount     = 1;

        VK_CHECK (vkCreateImageView (vk->dev, &ci, NULL, &vk->swapchain_views[i]));
    }

    return 0;
}


uint32_t
vk_get_memory_type_index (vk_t *vk, uint32_t requestBits, VkMemoryPropertyFlags requestProps)
{
    uint32_t result = ~0u;
    for (uint32_t i = 0; i < vk->phymem_props.memoryTypeCount; i++)
    {
        if (requestBits & 1)
        {
            VkMemoryType memtype = vk->phymem_props.memoryTypes[i];
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


/*
 *
 *  +---------------+------------------------------------+---------------------+
 *  |   U S A G E   | Image Usage Flag                   |ImageView Aspect Mask|
 *  |               | (VK_IMAGE_USAGE_XXX_BIT)           |(VK_IMAGE_ASPECT_XXX)|
 *  +-+-------------+------------------------------------+---------------------+
 *  | |Color Present| COLOR_ATTACHMENT                   |                     |
 *  |0+-------------+------------------------------------+        COLOR        |
 *  | |Color FBO    | COLOR_ATTACHMENT         | SAMPLED |                     |
 *  +-+-------------+------------------------------------+---------------------+
 *  | |Depth Present| DEPTH_STENCIL_ATTACHMENT           |                     |
 *  |1+-------------+------------------------------------+    DEPTH | STENCIL  |
 *  | |Depth FBO    | DEPTH_STENCIL_ATTACHMENT | SAMPLED |                     |
 *  +-+-------------+------------------------------------+---------------------+
 *  |2|Texture      | TRANSFER_DST             | SAMPLED |        COLOR        |
 *  +-+-------------+------------------------------------+---------------------+
 */
int
vk_create_render_buffer (vk_t *vk, vk_render_buffer_t *depth_buf,
                         uint32_t width, uint32_t height, VkFormat format,
                         uint32_t usage, VkImageUsageFlags ext_img_usage)
{
    VkImage         img;
    VkDeviceMemory  mem;
    VkImageView     view;
    VkImageUsageFlags     img_usage;
    VkImageAspectFlagBits img_aspect;

    if (width == 0 || height == 0)
    {
        width  = vk->swapchain_extent.width;
        height = vk->swapchain_extent.height;
    }
    
    switch (usage)
    {
    /* for Color */
    case 0:
        img_usage  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        img_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    /* for Depth */
    case 1:
        img_usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ;
        img_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
    /* for Texture */
    case 2:
        img_usage  = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        img_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    default:
        VK_LOGE ("vk_create_render_buffer(): usage=%d", usage);
        return -1;
    }
    img_usage |= ext_img_usage;


    /* Create "Image" for Color/Depth/Texture Buffer */
    VkImageCreateInfo ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType        = VK_IMAGE_TYPE_2D;
    ci.format           = format;
    ci.extent.width     = width;
    ci.extent.height    = height;
    ci.extent.depth     = 1;
    ci.mipLevels        = 1;
    ci.arrayLayers      = 1;
    ci.samples          = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling           = VK_IMAGE_TILING_OPTIMAL;
    ci.usage            = img_usage;
    ci.sharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK (vkCreateImage (vk->dev, &ci, NULL, &img));


    /* Allocate memory for Color/Depth/Texture Buffer */
    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements (vk->dev, img, &reqs);

    VkMemoryAllocateInfo ai = {0};
    ai.sType            = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize   = reqs.size;
    ai.memoryTypeIndex  = vk_get_memory_type_index (vk, reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK (vkAllocateMemory  (vk->dev, &ai, NULL, &mem));
    VK_CHECK (vkBindImageMemory (vk->dev, img, mem, 0));


    /* Create "ImageView" for Color/Depth/Texture */
    VkImageViewCreateInfo vci = {0};
    vci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    vci.image            = img;
    vci.format           = format;
    vci.components.r     = VK_COMPONENT_SWIZZLE_R;
    vci.components.g     = VK_COMPONENT_SWIZZLE_G;
    vci.components.b     = VK_COMPONENT_SWIZZLE_B;
    vci.components.a     = VK_COMPONENT_SWIZZLE_A;
    vci.subresourceRange.aspectMask     = img_aspect;
    vci.subresourceRange.baseMipLevel   = 0;
    vci.subresourceRange.levelCount     = 1;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount     = 1;

    VK_CHECK (vkCreateImageView (vk->dev, &vci, NULL, &view));

    depth_buf->img  = img;
    depth_buf->mem  = mem;
    depth_buf->view = view;

    return 0;
}


static int
vkin_create_render_pass (vk_t *vk)
{
    VkAttachmentDescription attachments[2] = {0};

    /* color */
    attachments[0].format         = vk->sfc_fmt.format;
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

    VK_CHECK (vkCreateRenderPass (vk->dev, &ci, NULL, &vk->render_pass));

    return 0;
}


static int
vkin_create_frame_buffer (vk_t *vk)
{
    uint32_t imageCount;
    VK_CHECK (vkGetSwapchainImagesKHR (vk->dev, vk->swapchain, &imageCount, NULL));
    vk->framebuffers = VK_CALLOC (VkFramebuffer, imageCount);


    VkFramebufferCreateInfo ci = {0};
    ci.sType        = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass   = vk->render_pass;
    ci.width        = vk->swapchain_extent.width;
    ci.height       = vk->swapchain_extent.height;
    ci.layers       = 1;

    for (uint32_t i = 0; i < imageCount; i ++)
    {
        VkImageView attachments[2];
        attachments[0] = vk->swapchain_views[i];
        attachments[1] = vk->depth_buf.view;

        ci.attachmentCount = 2;
        ci.pAttachments    = attachments;

        VK_CHECK (vkCreateFramebuffer (vk->dev, &ci, NULL, &vk->framebuffers[i]));
    }

    return 0;
}

static int
vkin_create_command_buffer (vk_t *vk)
{
    uint32_t imageCount;
    VK_CHECK (vkGetSwapchainImagesKHR (vk->dev, vk->swapchain, &imageCount, NULL));

    VkCommandBufferAllocateInfo ai = {0};
    ai.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool          = vk->cmd_pool;
    ai.commandBufferCount   = imageCount;
    ai.level                = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vk->cmd_bufs = VK_CALLOC (VkCommandBuffer, imageCount);

    VK_CHECK (vkAllocateCommandBuffers (vk->dev, &ai, vk->cmd_bufs));

    /* allocate fences */
    vk->fences = VK_CALLOC (VkFence, imageCount);

    VkFenceCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < imageCount; i ++)
    {
        VK_CHECK (vkCreateFence (vk->dev, &ci, NULL, &vk->fences[i]));
    }

    return 0;
}

static int
vkin_create_semaphore (vk_t *vk)
{
    VkSemaphoreCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CHECK (vkCreateSemaphore (vk->dev, &ci, NULL, &vk->sem_render_complete));
    VK_CHECK (vkCreateSemaphore (vk->dev, &ci, NULL, &vk->sem_present_complete));

    return 0;
}

int
vk_create_buffer (vk_t *vk, uint32_t size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags mflags, 
                  void *psrc, vk_buffer_t *vk_buf)
{
    VkBuffer        buf;
    VkDeviceMemory  mem;

    /* create VkBuffer */
    VkBufferCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.usage = usage;
    ci.size  = size;
    VK_CHECK (vkCreateBuffer (vk->dev, &ci, NULL, &buf));

    /* Allocate Memory for VkBuffer */
    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements (vk->dev, buf, &reqs);

    VkMemoryAllocateInfo ai = {0};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = reqs.size;
    ai.memoryTypeIndex = vk_get_memory_type_index (vk, reqs.memoryTypeBits, mflags);

    VK_CHECK (vkAllocateMemory (vk->dev, &ai, NULL, &mem));
    VK_CHECK (vkBindBufferMemory (vk->dev, buf, mem, 0));

    if (psrc)
    {
        vk_devmemcpy (vk, mem, psrc, size);
    }


    vk_buf->buf = buf;
    vk_buf->mem = mem;
    return 0;
}


uint32_t
vk_get_bpp_of_format (VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8G8B8A8_UNORM:
        return 4;
        break;
    case VK_FORMAT_R8_UNORM:
        return 1;
        break;
    default:
        VK_LOGE ("vk_get_bpp_of_format(): format=%08x", format);
        return 0;
    }
    return 0;
}


int
vk_upload_texture (vk_t *vk, uint32_t width, uint32_t height, VkFormat format,
                   void *psrc, vk_texture_t *vk_tex)
{
    /* create staging buffer */
    vk_buffer_t staging_buf;
    uint32_t imageSize = width * height * vk_get_bpp_of_format (format);
    VkBufferUsageFlags    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags mflag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_create_buffer (vk, imageSize, usage, mflag, psrc, &staging_buf);


    /* create Command buffer */
    VkCommandBuffer command;
    VkCommandBufferAllocateInfo ai = {0};
    ai.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandBufferCount   = 1;
    ai.commandPool          = vk->cmd_pool;
    ai.level                = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_CHECK (vkAllocateCommandBuffers (vk->dev, &ai, &command));


    /* Copy from (staging buffer) ==> (texture buffer) */
    VkCommandBufferBeginInfo commandBI = {0};
    commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK (vkBeginCommandBuffer (command, &commandBI));

    vk_put_image_barrier (vk, command, vk_tex->img, 
                          0, VK_ACCESS_TRANSFER_WRITE_BIT);

    VkBufferImageCopy copyRegion = {0};
    copyRegion.imageExtent.width               = width;
    copyRegion.imageExtent.height              = height;
    copyRegion.imageExtent.depth               = 1;
    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel       = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount     = 1;

    vkCmdCopyBufferToImage (command, staging_buf.buf, vk_tex->img, 
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    vk_put_image_barrier (vk, command, vk_tex->img, 
                          VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    VK_CHECK (vkEndCommandBuffer (command));


    /* Submit command */
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &command;
    VK_CHECK (vkQueueSubmit (vk->devq, 1, &submitInfo, VK_NULL_HANDLE));

    /* wait copy done */
    VK_CHECK (vkDeviceWaitIdle (vk->dev));

    vkFreeCommandBuffers (vk->dev, vk->cmd_pool, 1, &command);

    /* clean up staging buffer */
    vkFreeMemory (vk->dev, staging_buf.mem, NULL);
    vkDestroyBuffer (vk->dev, staging_buf.buf, NULL);
    
    return 0;
}

int
vk_create_texture (vk_t *vk, uint32_t width, uint32_t height, VkFormat format, 
                   void *psrc, vk_texture_t *vk_tex)
{
    vk_render_buffer_t texture_buf;
    vk_create_render_buffer (vk, &texture_buf, width, height, format, 2, 0);

    VkSampler sampler;
    vk_create_sampler (vk, &sampler);

    vk_tex->img     = texture_buf.img;
    vk_tex->mem     = texture_buf.mem;
    vk_tex->view    = texture_buf.view;
    vk_tex->sampler = sampler;

    if (psrc)
    {
        vk_upload_texture (vk, width, height, format, psrc, vk_tex);
    }

    return 0;
}

int
vk_create_sampler (vk_t *vk, VkSampler *sampler)
{
    /* create VkSampler */
    VkSamplerCreateInfo sci = {0};
    sci.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.minFilter     = VK_FILTER_LINEAR;
    sci.magFilter     = VK_FILTER_LINEAR;
    sci.addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.maxAnisotropy = 1.0f;
    sci.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    VK_CHECK (vkCreateSampler (vk->dev, &sci, NULL, sampler));

    return 0;
}


int
vk_load_shader_module (vk_t *vk, const char* fname, VkShaderModule *sm)
{
    FILE *fp;
    char *lpbuf;
    int  file_size;
    int  read_size;

    fp = fopen (fname, "rb");
    if (fp == NULL)
    {
        VK_LOGE ("can't open %s\n", fname);
        return -1;
    }

    fseek (fp, 0, SEEK_END);
    file_size = ftell (fp);
    fseek (fp, 0, SEEK_SET);

    lpbuf = VK_CALLOC (char, file_size);

    read_size = fread (lpbuf, 1, file_size, fp);
    if (file_size != read_size)
    {
        VK_LOGE ("can't read %s\n", fname);
        return -1;
    }

    /* ceate a new shader module */
    VkShaderModuleCreateInfo ci = {0};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.pCode    = (uint32_t *)lpbuf;
    ci.codeSize = file_size;

    VkShaderModule shaderModule;
    VK_CHECK (vkCreateShaderModule (vk->dev, &ci, NULL, &shaderModule));

    *sm = shaderModule;
    return 0;
}

int
vk_devmemcpy (vk_t *vk, VkDeviceMemory mem, void *psrc, uint32_t size)
{
    void *p;

    VK_CHECK (vkMapMemory (vk->dev, mem, 0, size, 0, &p));
    memcpy (p, psrc, size);
    vkUnmapMemory (vk->dev, mem);

    return 0;
}

/* ----------------------------------------------------------------------- *
 *    Utility functions for Pipeline Barrier
 * ----------------------------------------------------------------------- */

/*
 * [Before Texture upload]
 * +----------------------+--------------------------+--------------------------+
 * |                      |           old      ===>  |  ===>     new            |
 * +----------------------+--------------------------+--------------------------+
 * |Pipeline stage barrier| ALL_COMMANDS             | TRANSFER                 |
 * +--------+-------------+--------------------------+--------------------------+
 * | Image  | Access      | 0                        | TRANSFER_WRITE           |
 * | Barrier| Layout      | UNDEFINED                | TRANSFER_DST_OPTIMAL     |
 * +--------+-------------+--------------------------+--------------------------+
 *
 * [After Texture upload, Before Texture read]
 * +----------------------+--------------------------+--------------------------+
 * |                      |           old      ===>  |  ===>     new            |
 * +----------------------+--------------------------+--------------------------+
 * |Pipeline stage barrier| TRANSFER                 | FRAGMENT_SHADER          |
 * +--------+-------------+--------------------------+--------------------------+
 * | Image  | Access      | TRANSFER_WRITE           | SHADER_READ              |
 * | Barrier| Layout      | TRANSFER_DST_OPTIMAL     | SHADER_READ_ONLY_OPTIMAL |
 * +--------+-------------+--------------------------+--------------------------+
 *
 * [After FBO rendering, Before Texture read]
 * +----------------------+--------------------------+--------------------------+
 * |                      |           old      ===>  |  ===>     new            |
 * +----------------------+--------------------------+--------------------------+
 * |Pipeline stage barrier| COLOR_ATTACHMENT_OUTPUT  | FRAGMENT_SHADER          |
 * +--------+-------------+--------------------------+--------------------------+
 * | Image  | Access      | COLOR_ATTACHMENT_WRITE   | SHADER_READ              |
 * | Barrier| Layout      | COLOR_ATTACHMENT_OPTIMAL | SHADER_READ_ONLY_OPTIMAL |
 * +--------+-------------+--------------------------+--------------------------+
 */

int
vk_put_image_barrier (vk_t *vk, VkCommandBuffer command, VkImage image,
                      VkAccessFlags src_access, VkAccessFlags dst_access)
{
    VkPipelineStageFlags src_stage = 0;
    VkPipelineStageFlags dst_stage = 0;
    VkImageLayout        src_layout= 0;
    VkImageLayout        dst_layout= 0;

    switch (src_access)
    {
    /* finished [nothing] */
    case 0:
        src_stage  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        src_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        break;
    /* finished [Transfer Write] */
    case VK_ACCESS_TRANSFER_WRITE_BIT:
        src_stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
        src_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        break;
    /* finished [Color Write] */
    case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:
        src_stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        src_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        break;
    default:
        VK_LOGE ("vk_put_image_barrier(): src_access=%08x", src_access);
        return -1;
    }

    switch (dst_access)
    {
    /* staring [Transfer Write] */
    case VK_ACCESS_TRANSFER_WRITE_BIT:
        dst_stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        break;
    /* staring [Shader Read] */
    case VK_ACCESS_SHADER_READ_BIT:
        dst_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dst_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        break;
    default:
        VK_LOGE ("vk_put_image_barrier(): dst_access=%08x", dst_access);
        return -1;
    }

    VkImageMemoryBarrier img_barrier = {0};
    img_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    img_barrier.srcAccessMask       = src_access;
    img_barrier.dstAccessMask       = dst_access;
    img_barrier.oldLayout           = src_layout;
    img_barrier.newLayout           = dst_layout;
    img_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.image               = image;
    img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_barrier.subresourceRange.baseMipLevel   = 0;
    img_barrier.subresourceRange.levelCount     = 1;
    img_barrier.subresourceRange.baseArrayLayer = 0;
    img_barrier.subresourceRange.layerCount     = 1;

    vkCmdPipelineBarrier (command, src_stage, dst_stage,
        VK_DEPENDENCY_BY_REGION_BIT,    /* dependency */
        0, NULL,                        /* Memory barrier */
        0, NULL,                        /* Bufer memory barrier */
        1, &img_barrier);               /* Image memory barrier */

    return 0;
}


/* ----------------------------------------------------------------------- *
 *    Utility functions for Graphics Pipeline Createion
 * ----------------------------------------------------------------------- */

/* Primitive Type */
int
vk_get_default_input_assembly_state (vk_t *vk, VkPipelineInputAssemblyStateCreateInfo *state, 
                                     VkPrimitiveTopology topology)
{
    state->sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    state->topology = topology;

    return 0;
}

int
vk_destroy_default_input_assembly_state (vk_t *vk, VkPipelineInputAssemblyStateCreateInfo *state)
{
    return 0;
}


/* Viewport, Scissor */
int
vk_get_default_viewport_state (vk_t *vk, VkPipelineViewportStateCreateInfo *state)
{
    VkViewport *viewport = VK_CALLOC (VkViewport, 1);
    VkRect2D   *scissor  = VK_CALLOC (VkRect2D, 1);

    /*
     * Differences regarding viewport coordinates:
     *   [Vulkan] Top    Left is the origin.
     *   [OpenGL] Bottom Left is the origin.
     *
     * we set a negative height to resolve these difference.
     * (this requires "VK_KHR_maintenance1" extension)
     */
    viewport->x        = 0.0f;
    viewport->y        = vk->swapchain_extent.height;
    viewport->width    = vk->swapchain_extent.width;
    viewport->height   = -1.0f * vk->swapchain_extent.height;
    viewport->minDepth = 0.0f;
    viewport->maxDepth = 1.0f;

    scissor->offset.x  = 0;
    scissor->offset.y  = 0;
    scissor->extent    = vk->swapchain_extent;

    state->sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    state->viewportCount = 1;
    state->pViewports    = viewport;
    state->scissorCount  = 1;
    state->pScissors     = scissor;

    return 0;
}

int
vk_destroy_default_viewport_state (vk_t *vk, VkPipelineViewportStateCreateInfo *state)
{
    VK_FREE (state->pViewports);
    VK_FREE (state->pScissors);

    return 0;
}


/* Rasterizer */
int
vk_get_default_rasterizer_state (vk_t *vk, VkPipelineRasterizationStateCreateInfo *state)
{
    state->sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    state->polygonMode = VK_POLYGON_MODE_FILL;
    state->cullMode    = VK_CULL_MODE_NONE;
    state->frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    state->lineWidth   = 1.0f;

    return 0;
}

int
vk_destroy_default_rasterizer_state (vk_t *vk, VkPipelineRasterizationStateCreateInfo *state)
{
    return 0;
}


/* Multi Sample */
int
vk_get_default_multisample_state (vk_t *vk, VkPipelineMultisampleStateCreateInfo *state)
{
    state->sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    state->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    return 0;
}

int
vk_destroy_default_multisample_state (vk_t *vk, VkPipelineMultisampleStateCreateInfo *state)
{
    return 0;
}

/* Depth, Stencil */
int
vk_get_default_depth_stencil_state (vk_t *vk, VkPipelineDepthStencilStateCreateInfo *state, 
                                    int depth_en, int stencil_en)
{
    state->sType             = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    state->depthTestEnable   = VK_FALSE;
    state->depthCompareOp    = VK_COMPARE_OP_ALWAYS;
    state->depthWriteEnable  = VK_FALSE;
    state->stencilTestEnable = VK_FALSE;

    if (depth_en)
    {
        state->depthTestEnable  = VK_TRUE;
        state->depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;
        state->depthWriteEnable = VK_TRUE;
    }

    if (stencil_en)
    {
        state->stencilTestEnable = VK_TRUE;
    }

    return 0;
}

int
vk_destroy_default_depth_stencil_state (vk_t *vk, VkPipelineDepthStencilStateCreateInfo *state)
{
    return 0;
}


/* Blend */
int
vk_get_default_blend_state (vk_t *vk, VkPipelineColorBlendStateCreateInfo *state, int blend_en)
{
    VkPipelineColorBlendAttachmentState *ba = VK_CALLOC (VkPipelineColorBlendAttachmentState, 1);

    ba->blendEnable         = VK_TRUE;
    ba->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    ba->dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    ba->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    ba->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    ba->colorBlendOp        = VK_BLEND_OP_ADD;
    ba->alphaBlendOp        = VK_BLEND_OP_ADD;
    ba->colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    if (blend_en)
    {
        ba->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        ba->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ba->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ba->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    }

    state->sType            = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    state->attachmentCount  = 1;
    state->pAttachments     = ba;

    return 0;
}

int
vk_destroy_default_blend_state (vk_t *vk, VkPipelineColorBlendStateCreateInfo *state)
{
    VK_FREE (state->pAttachments);
    return 0;
}


/* ----------------------------------------------------------------------- *
 *    Create "struct-chain" for extentions.
 *
 *      [head]
 *      +-----+   +-----+   +-----+
 *      |sType|   |sType|   |sType|
 *      |pNext|-->|pNext|-->|pNext|
 *      +-----+   +-----+   +-----+
 * ----------------------------------------------------------------------- */
int
vk_build_struct_chain (VkBaseOutStructure *head, vk_struct_chain_info_t *chain_info, uint32_t chain_len)
{
    VkBaseOutStructure *phead = head;

    for (uint32_t i = 0; i < chain_len; i++)
    {
        VkBaseOutStructure *phead_new = calloc (1, chain_info[i].size);
        phead_new->sType = chain_info[i].sType;

        phead->pNext = phead_new;
        phead = phead->pNext;
    }
    return 0;
}




vk_t *
vk_init (int win_w, int win_h)
{
    vk_t *vk = VK_CALLOC (vk_t, 1);

    vkin_create_instance (vk, "VulkanApp");

    vkin_winsys_init (win_w, win_h);

    vkin_debug_init (vk->instance);

    vkin_select_physical_device (vk);

    vkin_select_devqueue_index (vk);

    vkin_create_device (vk);

    vkin_create_command_pool (vk);

    vkin_create_surface (vk, VK_FORMAT_B8G8R8A8_UNORM);

    vkin_create_swap_chain (vk, win_w, win_h, VK_PRESENT_MODE_FIFO_KHR);

    vkin_create_swap_chain_views (vk);

    /* depth buffer */
    vk_create_render_buffer (vk, &vk->depth_buf, 0, 0, VK_FORMAT_D32_SFLOAT, 1, 0);

    vkin_create_render_pass (vk);

    vkin_create_frame_buffer (vk);

    vkin_create_command_buffer (vk);

    vkin_create_semaphore (vk);

    return vk;
}
