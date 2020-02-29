/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include "vk_tools.h"
#include "vk_debug.h"


/* -------------------------------------------------------------------- *
 *  for "VK_EXT_debug_report" extension.
 *  this extension is Deprecated by "VK_EXT_debug_utils".
 * -------------------------------------------------------------------- */
static PFN_vkCreateDebugReportCallbackEXT   _vkCreateDebugReportCallbackEXT;
static VkDebugReportCallbackEXT             s_debugReportCallback;


VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallbackFunc(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type,
    uint64_t object, size_t location, int32_t messageCode,
    const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    VK_PRINT ("[Vulkan DebugCall] %s\n", pMessage);

    return VK_FALSE;
}


static int
vkin_debug_report_init (VkInstance instance)
{
    VK_GET_PROC_ADDR (instance, vkCreateDebugReportCallbackEXT );
    if (_vkCreateDebugReportCallbackEXT == NULL)
    {
        return -1;
    }

    VkDebugReportCallbackCreateInfoEXT callbackInfo = {0};

    callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    callbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT               |
                         VK_DEBUG_REPORT_WARNING_BIT_EXT             |
                         VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                         VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    callbackInfo.pfnCallback = &debugCallbackFunc;

    VK_CHECK (_vkCreateDebugReportCallbackEXT (instance, &callbackInfo, NULL, &s_debugReportCallback));

    return 0;
}


/* -------------------------------------------------------------------- *
 *  for "VK_EXT_debug_utils" extension.
 * -------------------------------------------------------------------- */
static PFN_vkCreateDebugUtilsMessengerEXT  _vkCreateDebugUtilsMessengerEXT;
static PFN_vkDestroyDebugUtilsMessengerEXT _vkDestroyDebugUtilsMessengerEXT;

static VkDebugUtilsMessengerEXT debugUtilsMessenger;


VKAPI_ATTR VkBool32 VKAPI_CALL 
debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT  *pCallbackData,
    void* pUserData)
{
    const char *prefix;

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        prefix = "VERBOSE: ";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        prefix = "INFO: ";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        prefix = "WARNING: ";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        prefix = "ERROR: ";
    else
        prefix = "-----: ";

    VK_PRINT ("%s[%d] %s\n", prefix,
                             pCallbackData->messageIdNumber,
                             pCallbackData->pMessage);
    return VK_FALSE;
}


int
vkin_debug_init (VkInstance instance)
{
    /* if "VK_EXT_debug_utils" is enabled, use it. */
    VK_GET_PROC_ADDR (instance, vkCreateDebugUtilsMessengerEXT );
    VK_GET_PROC_ADDR (instance, vkDestroyDebugUtilsMessengerEXT );

    if (_vkCreateDebugUtilsMessengerEXT == NULL)
    {
        return vkin_debug_report_init (instance);
    }

    VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {0};
    dbgInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    dbgInfo.pfnUserCallback = debugUtilsMessengerCallback;

    VK_CHECK (_vkCreateDebugUtilsMessengerEXT (instance, &dbgInfo, NULL, &debugUtilsMessenger));

    return 0;
}

