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

    VK_PRINT ("%s[%d] %s\n", prefix,
                             pCallbackData->messageIdNumber,
                             pCallbackData->pMessage);
    return VK_FALSE;
}

int
vkin_debug_init (VkInstance instance)
{
    VK_GET_PROC_ADDR (instance, vkCreateDebugUtilsMessengerEXT );
    VK_GET_PROC_ADDR (instance, vkDestroyDebugUtilsMessengerEXT );

    VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {0};
    dbgInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    dbgInfo.pfnUserCallback = debugUtilsMessengerCallback;

    VkResult result = _vkCreateDebugUtilsMessengerEXT (instance, &dbgInfo, NULL, &debugUtilsMessenger);
    assert(result == VK_SUCCESS);

    return 0;
}

