#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vulkan/vulkan.h>

#define APP_NAME "000_query_vk_devices"



VKAPI_ATTR VkBool32 VKAPI_CALL
dbgMessengerCb(
    VkDebugUtilsMessageSeverityFlagBitsEXT     messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT            messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                       *pUserData)
{
    uint32_t i;

    fprintf(stderr, "===========================================\n");
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) fprintf(stderr, "[VERBOSE]");
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    fprintf(stderr, "[INFO]");
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) fprintf(stderr, "[WARNING]");
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   fprintf(stderr, "[ERROR]");

    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)         fprintf(stderr, "(GENERAL)");
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)      fprintf(stderr, "(VALIDATION)");
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)     fprintf(stderr, "(PERFORMANCE)");

    fprintf(stderr, "\n");
    fprintf(stderr, "  MessageID: %d\n", pCallbackData->messageIdNumber);
    fprintf(stderr, "  MessageID: %s\n", pCallbackData->pMessageIdName);
    fprintf(stderr, "  Message  : %s\n", pCallbackData->pMessage);

    for (i = 0; i < pCallbackData->objectCount; i++)
    {
        fprintf(stderr, "  ---Object [%d/%d] ", i, pCallbackData->objectCount);
        fprintf(stderr, "objectType: %d, Handle:%p, Name:%s\n",
            pCallbackData->pObjects[i].objectType,
            (void*)pCallbackData->pObjects[i].objectHandle,
            pCallbackData->pObjects[i].pObjectName);
    }

    for (i = 0; i < pCallbackData->cmdBufLabelCount; i++)
    {
        fprintf(stderr, "  ---Command Buffer Labes [%d/%d] ", i, pCallbackData->cmdBufLabelCount);
        fprintf(stderr, "%s {%f, %f, %f, %f}\n",
            pCallbackData->pCmdBufLabels[i].pLabelName,
            pCallbackData->pCmdBufLabels[i].color[0],
            pCallbackData->pCmdBufLabels[i].color[1],
            pCallbackData->pCmdBufLabels[i].color[2],
            pCallbackData->pCmdBufLabels[i].color[3]);
}

    return VK_FALSE;
}


VkResult
print_instance_extension (char *layer_name)
{
    VkExtensionProperties *vk_exts = NULL;
    uint32_t inst_ext_cnt;
    VkResult err;

    do {
        err = vkEnumerateInstanceExtensionProperties(layer_name, &inst_ext_cnt, NULL);
        assert (err == VK_SUCCESS);

        vk_exts = (VkExtensionProperties *)realloc(vk_exts, inst_ext_cnt * sizeof(VkExtensionProperties));

        err = vkEnumerateInstanceExtensionProperties(layer_name, &inst_ext_cnt, vk_exts);
    } while (err == VK_INCOMPLETE);  /* need to retry ? */


    fprintf(stderr, "\n");
    fprintf(stderr, "-----------------------------------------------------\n");
    if (layer_name)
        fprintf(stderr, " Instance Extension (LAYER:%s) count : %d\n", layer_name, inst_ext_cnt);
    else
        fprintf(stderr, " Instance Extension (GLOBAL) count : %d\n", inst_ext_cnt);
    fprintf(stderr, "-----------------------------------------------------\n");

    for (uint32_t i = 0; i < inst_ext_cnt; i++) 
    {
        fprintf(stderr, "  %-40s ver:%2d\n", vk_exts[i].extensionName, vk_exts[i].specVersion);
    }
    free (vk_exts);

    return err;
}


VkResult
print_global_layer_properties() 
{
    VkResult err;
    uint32_t instance_layer_count;
    VkLayerProperties *vk_props = NULL;

    /* get global instance extension */
    print_instance_extension (NULL);

    /* Gets a list of layer and instance extensions */
    do {
        err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        assert(err == VK_SUCCESS);

        vk_props = (VkLayerProperties *)realloc(vk_props, instance_layer_count * sizeof(VkLayerProperties));

        err = vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props);
    } while (err == VK_INCOMPLETE);  /* need to retry ? */

    fprintf(stderr, "\n");
    fprintf(stderr, "-----------------------------------------------------\n");
    fprintf(stderr, " Instance Layer count : %d\n", instance_layer_count);
    fprintf(stderr, "-----------------------------------------------------\n");

    for (uint32_t i = 0; i < instance_layer_count; i++) 
    {
        fprintf(stderr, "----- VkLayerProperties[%d/%d] ----- \n", i, instance_layer_count);
        fprintf(stderr, " LayerName    : %s\n", vk_props[i].layerName);
        fprintf(stderr, " Spec Version : %d\n", vk_props[i].specVersion);
        fprintf(stderr, " Impl Version : %d\n", vk_props[i].implementationVersion);
        fprintf(stderr, " Description  : %s\n", vk_props[i].description);

        print_instance_extension(vk_props[i].layerName);
    }
    free (vk_props);

    return err;
}

uint32_t
print_instance_version ()
{
    VkResult err;
    uint32_t instance_version;
    PFN_vkEnumerateInstanceVersion _vkEnumerateInstanceVersion =
            (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");

    /* Vulkan 1.0 doesn't have vkEnumerateInstanceVersion(). */
    if (_vkEnumerateInstanceVersion == NULL)
    {
        instance_version = VK_API_VERSION_1_0;
    }
    else 
    {
        err = _vkEnumerateInstanceVersion(&instance_version);
        assert(err == VK_SUCCESS);
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "Vulkan Instance Version = %d.%d.%d\n",
             VK_VERSION_MAJOR(instance_version),
             VK_VERSION_MINOR(instance_version),
             VK_VERSION_PATCH(instance_version));

    return instance_version;
}

VkInstance
init_vulkan_instance(void)
{
    VkInstance instance;
    VkResult err;
    uint32_t enabled_extension_count = 0;
    uint32_t enabled_layer_count = 0;

    const VkApplicationInfo app_info = {
        .sType                 = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext                 = NULL,      // for extension
        .pApplicationName      = APP_NAME,
        .applicationVersion    = 0,
        .pEngineName           = APP_NAME,
        .engineVersion         = 0,
        .apiVersion            = VK_API_VERSION_1_0
    };

    VkDebugUtilsMessengerCreateInfoEXT dbg_msg_info = {
        .sType                 = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext                 = NULL,
        .flags                 = 0,
        .messageSeverity       = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT  |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType           = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback       = dbgMessengerCb,
        .pUserData             = NULL,
    };

    VkInstanceCreateInfo inst_info = {
        .sType                 = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                 = &dbg_msg_info,
        .flags                 = 0,
        .pApplicationInfo      = &app_info,
        .enabledLayerCount     = enabled_layer_count,
        .ppEnabledLayerNames   = NULL,
        .enabledExtensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = NULL,
    };

    err = vkCreateInstance(&inst_info, NULL, &instance);
    if (err == VK_SUCCESS)
    {
        return instance;
    }

    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) 
    {
        fprintf(stderr, "vkCreateInstance Failure: [VK_ERROR_INCOMPATIBLE_DRIVER].\n");
    }
    else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) 
    {
        fprintf(stderr, "vkCreateInstance Failure: [VK_ERROR_EXTENSION_NOT_PRESENT].\n");
    } 
    else
    {
        fprintf(stderr, "vkCreateInstance Failure: [%x].\n", err);
    }

    return 0;
}

char *get_vkdevtype(VkPhysicalDeviceType devtype)
{
    switch (devtype)
    {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:          return "OTHER";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "INTEGRATED_GPU ";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return "DISCRETE_GPU";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return "VIRTUAL_GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:            return "TYPE_CPU ";
    default: return "unknown";
    }
}

#define PRINT_FOMAT_FEATURE(flags, feature) \
do { \
    if (flags & feature) fprintf(stderr, "        "#feature"\n"); \
}while (0)

static void
print_FormatFeatureFlags (VkFormatFeatureFlags flags)
{
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_BLIT_SRC_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_BLIT_DST_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_TRANSFER_SRC_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_TRANSFER_DST_BIT);
    PRINT_FOMAT_FEATURE(flags, VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT);
}

static void
print_format_properties(VkPhysicalDevice phydev, VkFormat format, char *format_name)
{
    fprintf(stderr, " [%s]\n", format_name);

    VkFormatProperties fmt_props;
    vkGetPhysicalDeviceFormatProperties(phydev, format, &fmt_props);

    VkFormatFeatureFlags lflags = fmt_props.linearTilingFeatures;
    VkFormatFeatureFlags oflags = fmt_props.optimalTilingFeatures;
    VkFormatFeatureFlags bflags = fmt_props.bufferFeatures;
    fprintf(stderr, "   linearTilingFeatures : 0x%08x\n", lflags);
    print_FormatFeatureFlags(lflags);
    fprintf(stderr, "   optimalTilingFeatures: 0x%08x\n", oflags);
    print_FormatFeatureFlags(oflags);
    fprintf(stderr, "   bufferFeatures       : 0x%08x\n", bflags);
    print_FormatFeatureFlags(bflags);
}

VkPhysicalDevice
query_vulkan_device(VkInstance vk_instance)
{
    VkResult err;
    uint32_t gpu_count, i, j;
    VkPhysicalDevice phydev, *phydev_array;

    err = vkEnumeratePhysicalDevices(vk_instance, &gpu_count, NULL);
    assert(err == VK_SUCCESS);

    fprintf(stderr, "\n");
    fprintf(stderr, "-----------------------------------------------------\n");
    fprintf(stderr, " Physical Devices count : %d\n", gpu_count);
    fprintf(stderr, "-----------------------------------------------------\n");

    if (gpu_count == 0)
    {
        fprintf(stderr, "vkEnumeratePhysicalDevices() returned no devices.\n");
        return NULL;
    }

    phydev_array = malloc(sizeof(VkPhysicalDevice) * gpu_count);

    err = vkEnumeratePhysicalDevices(vk_instance, &gpu_count, phydev_array);
    assert(err == VK_SUCCESS);

    for (i = 0; i < gpu_count; i ++)
    {
        fprintf(stderr, " \n");
        fprintf(stderr, " ##### Device[%d/%d] #####\n", i, gpu_count);

        /* ------------------------------------ *
         * PhysicalDeviceProperties
         * ------------------------------------ */
        phydev = phydev_array[i];
        VkPhysicalDeviceProperties  dev_props;

        vkGetPhysicalDeviceProperties(phydev, &dev_props);

        fprintf(stderr, " \n");
        fprintf(stderr, " ------ PhysicalDeviceProperties\n");
        fprintf(stderr, " deviceName   : %s\n",   dev_props.deviceName);
        fprintf(stderr, " deviceType   : %s\n", get_vkdevtype(dev_props.deviceType));
        fprintf(stderr, " apiVersion   : %d.%d.%d\n",
                VK_VERSION_MAJOR(dev_props.apiVersion),
                VK_VERSION_MINOR(dev_props.apiVersion),
                VK_VERSION_PATCH(dev_props.apiVersion));
        fprintf(stderr, " driverVersion: 0x%x\n", dev_props.driverVersion);
        fprintf(stderr, " vendorID     : 0x%x\n", dev_props.vendorID);
        fprintf(stderr, " deviceID     : 0x%x\n", dev_props.deviceID);

        fprintf(stderr, " limits.maxImageDimension1D:   %d\n", dev_props.limits.maxImageDimension1D);
        fprintf(stderr, " limits.maxImageDimension2D:   %d\n", dev_props.limits.maxImageDimension2D);
        fprintf(stderr, " limits.maxImageDimension3D:   %d\n", dev_props.limits.maxImageDimension3D);
        fprintf(stderr, " limits.maxImageDimensionCube: %d\n", dev_props.limits.maxImageDimensionCube);
        fprintf(stderr, " (snipped)\n");

        fprintf(stderr, " sparseProp.residencyStandard2DBlockShape           : %d\n", dev_props.sparseProperties.residencyStandard2DBlockShape);
        fprintf(stderr, " sparseProp.residencyStandard2DMultisampleBlockShape: %d\n", dev_props.sparseProperties.residencyStandard2DMultisampleBlockShape);
        fprintf(stderr, " sparseProp.residencyStandard3DBlockShape           : %d\n", dev_props.sparseProperties.residencyStandard3DBlockShape);
        fprintf(stderr, " sparseProp.residencyAlignedMipSize                 : %d\n", dev_props.sparseProperties.residencyAlignedMipSize);
        fprintf(stderr, " sparseProp.residencyNonResidentStrict              : %d\n", dev_props.sparseProperties.residencyNonResidentStrict);


        /* ------------------------------------ *
         * ExtensionProperties
         * ------------------------------------ */
        uint32_t ext_count = 0;
        VkExtensionProperties *ext_prop = NULL;

        err = vkEnumerateDeviceExtensionProperties(phydev, NULL, &ext_count, NULL);
        assert(err == VK_SUCCESS);

        fprintf(stderr, " \n");
        fprintf(stderr, " ------ ExtensionProperties count = %d\n", ext_count);
        ext_prop = malloc(ext_count * sizeof(VkExtensionProperties));
        err = vkEnumerateDeviceExtensionProperties(phydev, NULL, &ext_count, ext_prop);
        assert(err == VK_SUCCESS);

        for (j = 0; j < ext_count; j++)
        {
            fprintf(stderr, " %s (ver:%d)\n", 
                            ext_prop[j].extensionName, ext_prop[j].specVersion);
        }


        /* ----------------------------------------------------- *
         * QueueFamilyProperties
         * ----------------------------------------------------- */
        uint32_t qfamily_count;
        VkQueueFamilyProperties *queue_props;

        vkGetPhysicalDeviceQueueFamilyProperties(phydev, &qfamily_count, NULL);
        assert(qfamily_count >= 1);

        fprintf(stderr, " \n");
        fprintf(stderr, " ------ QueueFamilyProperties count = %d\n", qfamily_count);

        queue_props = (VkQueueFamilyProperties *)malloc(qfamily_count * sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(phydev, &qfamily_count, queue_props);

        for (j = 0; j < qfamily_count; j++)
        {
            fprintf(stderr, " QueueFamily[%d/%d]\n", j, qfamily_count);

            fprintf(stderr, "   queueFlags            : ");
            if (queue_props[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)       fprintf(stderr, "GRAPHICS, ");
            if (queue_props[j].queueFlags & VK_QUEUE_COMPUTE_BIT )       fprintf(stderr, "COMPUTE, ");
            if (queue_props[j].queueFlags & VK_QUEUE_TRANSFER_BIT)       fprintf(stderr, "TRANSFER, ");
            if (queue_props[j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) fprintf(stderr, "SPARSE, ");
            if (queue_props[j].queueFlags & VK_QUEUE_PROTECTED_BIT)      fprintf(stderr, "PROTECTED, ");
            fprintf(stderr, "\n");

            fprintf(stderr, "   queueCount            : %d\n", queue_props[j].queueCount);
            fprintf(stderr, "   timestampValidBits    : %d\n", queue_props[j].timestampValidBits);
            fprintf(stderr, "   minImgTransGranularity: (%d, %d, %d)\n", 
                queue_props[j].minImageTransferGranularity.width,
                queue_props[j].minImageTransferGranularity.height,
                queue_props[j].minImageTransferGranularity.depth);
        }


        /* ------------------------------------ *
         * PhysicalDeviceFeatures
         * ------------------------------------ */
        VkPhysicalDeviceFeatures dev_feat;
        vkGetPhysicalDeviceFeatures(phydev, &dev_feat);

        fprintf(stderr, " \n");
        fprintf(stderr, " ------ PhysicalDeviceFeatures\n");
        fprintf(stderr, " robustBufferAccess    : %d\n", dev_feat.robustBufferAccess);
        fprintf(stderr, " fullDrawIndexUint32   : %d\n", dev_feat.fullDrawIndexUint32);
        fprintf(stderr, " imageCubeArray        : %d\n", dev_feat.imageCubeArray);
        fprintf(stderr, " independentBlend      : %d\n", dev_feat.independentBlend);
        fprintf(stderr, " geometryShader        : %d\n", dev_feat.geometryShader);
        fprintf(stderr, " tessellationShader    : %d\n", dev_feat.tessellationShader);
        fprintf(stderr, " sampleRateShading     : %d\n", dev_feat.sampleRateShading);
        fprintf(stderr, " shaderFloat64         : %d\n", dev_feat.shaderFloat64);
        fprintf(stderr, " shaderInt64           : %d\n", dev_feat.shaderInt64);
        fprintf(stderr, " shaderInt16           : %d\n", dev_feat.shaderInt16);
        fprintf(stderr, " (snipped)\n");


        /* ------------------------------------ *
         * PhysicalDeviceMemoryProperties
         * ------------------------------------ */
        VkPhysicalDeviceMemoryProperties memory_props;
        vkGetPhysicalDeviceMemoryProperties(phydev, &memory_props);

        fprintf(stderr, " \n");
        fprintf(stderr, " ------ PhysicalDeviceMemoryProperties\n");
        fprintf(stderr, " memoryTypeCount = %d\n",   memory_props.memoryTypeCount);
        for (j = 0; j < memory_props.memoryTypeCount; j++)
        {
            fprintf(stderr, " memoryType[%d/%d]\n", j, memory_props.memoryTypeCount);
            fprintf(stderr, "   heapIndex    : %d\n", memory_props.memoryTypes[j].heapIndex);

            VkMemoryPropertyFlags flags = memory_props.memoryTypes[j].propertyFlags;
            fprintf(stderr, "   propertyFlags: 0x%08x\n", flags);
            if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)     fprintf(stderr, "        DEVICE_LOCAL_BIT\n");
            if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)     fprintf(stderr, "        HOST_VISIBLE_BIT\n");
            if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)    fprintf(stderr, "        HOST_COHERENT_BIT\n");
            if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)      fprintf(stderr, "        HOST_CACHED_BIT\n");
            if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) fprintf(stderr, "        LAZILY_ALLOCATED_BIT\n");
            if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT )       fprintf(stderr, "        PROTECTED_BIT\n");
        }


        fprintf(stderr, " memoryHeapCount = %d\n",   memory_props.memoryHeapCount);
        for (j = 0; j < memory_props.memoryHeapCount; j++)
        {
            fprintf(stderr, " memoryHeap[%d/%d]\n", j, memory_props.memoryHeapCount);
            fprintf(stderr, "   size         : %ld [MB]\n", memory_props.memoryHeaps[j].size/1024/1024);

            VkMemoryHeapFlags flags = memory_props.memoryHeaps[j].flags;
            fprintf(stderr, "   flags        : 0x%08x\n", flags);
            if (flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)       fprintf(stderr, "        DEVICE_LOCAL_BIT\n");
            if (flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)     fprintf(stderr, "        MULTI_INSTANCE_BIT\n");
            if (flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR) fprintf(stderr, "        MULTI_INSTANCE_BIT_KHR\n");
        }


        /* ------------------------------------ *
         * PhysicalDeviceFormatProperties
         * ------------------------------------ */
        fprintf(stderr, " \n");
        fprintf(stderr, " ------ PhysicalDeviceFormatProperties\n");
        print_format_properties(phydev, VK_FORMAT_R8G8B8A8_UINT,  "VK_FORMAT_R8G8B8A8_UINT");
        print_format_properties(phydev, VK_FORMAT_R8G8B8A8_UNORM, "VK_FORMAT_R8G8B8A8_UNORM");
        fprintf(stderr, "  (snipped)\n");
    }

    phydev = phydev_array[0];

    free (phydev_array);

    return phydev;
}




int
main(int argc, char *argv[])
{
    VkInstance vk_instance;

    print_instance_version();
    print_global_layer_properties();

    vk_instance = init_vulkan_instance();
    query_vulkan_device(vk_instance);

    return 0;
}

