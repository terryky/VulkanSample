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

typedef struct _vk_t
{
    VkInstance                          instance;
    VkPhysicalDevice                    phydev;
    VkPhysicalDeviceMemoryProperties    phymem_props;

} vk_t;

static vk_t vk = {0};



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

int
vk_init (int win_w, int win_h)
{
    vkin_create_instance ("VulkanApp");

    vkin_debug_init (vk.instance);

    vkin_select_physical_device ();

    return 0;
}
