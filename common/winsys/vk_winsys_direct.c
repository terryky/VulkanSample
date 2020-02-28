/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include "vk_tools.h"
#include "vk_winsys.h"


static int s_win_w;
static int s_win_h;

int
vkin_winsys_init (vk_t *vk, int win_w, int win_h)
{
    s_win_w = win_w;
    s_win_h = win_h;

    return 0;
}


VkSurfaceKHR
vkin_winsys_create_surface (vk_t *vk)
{
    int width  = s_win_w;
    int height = s_win_h;

    /* -------------------------- *
     *  select display
     * -------------------------- */
    uint32_t dpyPropCount;
    vkGetPhysicalDeviceDisplayPropertiesKHR (vk->phydev, &dpyPropCount, NULL);
    VkDisplayPropertiesKHR *pDisplayProperties = VK_CALLOC (VkDisplayPropertiesKHR, dpyPropCount);
    vkGetPhysicalDeviceDisplayPropertiesKHR (vk->phydev, &dpyPropCount, pDisplayProperties);

    for (uint32_t idpy = 0; idpy < dpyPropCount; idpy++)
    {
        VkDisplayPropertiesKHR *dprop = &pDisplayProperties[idpy];

        VK_PRINT ("---------- Display[%d/%d] --------\n", idpy, dpyPropCount);
        VK_PRINT ("display      : %p\n",             dprop->display);
        VK_PRINT ("displayName  : %s\n",             dprop->displayName);
        VK_PRINT ("dimensions   : (%4dx%4d)[mm] \n", dprop->physicalDimensions.width, dprop->physicalDimensions.height);
        VK_PRINT ("resolution   : (%4dx%4d)[pix]\n", dprop->physicalResolution.width, dprop->physicalResolution.height);
        VK_PRINT ("plane_reorder: %d\n",             dprop->planeReorderPossible);
        VK_PRINT ("persistent   : %d\n",             dprop->persistentContent);
    }

    /* choose the first display */
    VkDisplayKHR display = pDisplayProperties[0].display; 
    VK_FREE (pDisplayProperties);

    /* -------------------------- *
     *  select display mode
     * -------------------------- */
    VkDisplayModeKHR displayMode = 0;
    VkDisplayModePropertiesKHR *pModeProperties;
    bool foundMode = false;

    uint32_t modeCount;
    vkGetDisplayModePropertiesKHR (vk->phydev, display, &modeCount, NULL);
    pModeProperties = VK_CALLOC (VkDisplayModePropertiesKHR, modeCount);
    vkGetDisplayModePropertiesKHR (vk->phydev, display, &modeCount, pModeProperties);

    for (uint32_t imode = 0; imode < modeCount; imode++)
    {
        const VkDisplayModePropertiesKHR *mode = &pModeProperties[imode];

        VK_PRINT ("MODE[%2d/%2d] %4dx%4d @%.1f ", imode, modeCount, 
                        mode->parameters.visibleRegion.width,
                        mode->parameters.visibleRegion.height,
                        mode->parameters.refreshRate / 1000.0f);

        /* select the first mode that supports the specified resolution. */
        if (foundMode == false &&
            mode->parameters.visibleRegion.width  == width && 
            mode->parameters.visibleRegion.height == height)
        {
            displayMode = mode->displayMode;
            foundMode = true;
            VK_PRINT ("<<<selected>>>");
        }
        VK_PRINT ("\n");
    }

    VK_FREE (pModeProperties);

    if (!foundMode)
    {
        VK_LOGE ("Can't find a display and a display mode! (%d)\n", 0);
        return 0;
    }


    /* -------------------------- *
     *  select display plane
     * -------------------------- */
    uint32_t bestPlaneIndex = UINT32_MAX;
    uint32_t bestPlaneStackIndex = UINT32_MAX;
    bool foundPlaneIndex = false;

    uint32_t planePropCount;
    vkGetPhysicalDeviceDisplayPlanePropertiesKHR (vk->phydev, &planePropCount, NULL);
    VkDisplayPlanePropertiesKHR *pPlaneProperties = VK_CALLOC (VkDisplayPlanePropertiesKHR, planePropCount);
    vkGetPhysicalDeviceDisplayPlanePropertiesKHR (vk->phydev, &planePropCount, pPlaneProperties);

    for (uint32_t i = 0; i < planePropCount; i++)
    {
        VK_PRINT ("------- DisplayPlane[%d/%d] ------\n", i, planePropCount);
        VK_PRINT ("currentDisplay       : %p\n", pPlaneProperties[i].currentDisplay);
        VK_PRINT ("currentStackIndex    : %d\n", pPlaneProperties[i].currentStackIndex);

        uint32_t supDpyCount;
        vkGetDisplayPlaneSupportedDisplaysKHR (vk->phydev, i, &supDpyCount, NULL);
        VkDisplayKHR *pSupDisplays = VK_CALLOC (VkDisplayKHR, supDpyCount);
        vkGetDisplayPlaneSupportedDisplaysKHR (vk->phydev, i, &supDpyCount, pSupDisplays);

        /* select the first plane that is supported by the current display */
        for (uint32_t j = 0; j < supDpyCount; j++)
        {
            VK_PRINT ("support display[%d/%d] : %p ", j, supDpyCount, pSupDisplays[j]);
            if ((foundPlaneIndex == false) && (display == pSupDisplays[j]))
            {
                bestPlaneIndex = i;
                bestPlaneStackIndex = pPlaneProperties[i].currentStackIndex;
                foundPlaneIndex = true;
                VK_PRINT ("<<<selected>>>");
            }
            VK_PRINT ("\n");
        }

        VK_FREE (pSupDisplays);
    }

    VK_FREE (pPlaneProperties);

    if (!foundPlaneIndex)
    {
        VK_LOGE ("Can't find a plane for displaying! (%d)\n", 0);
        return 0;
    }


    /* -------------------------- *
     *  select plane alpha mode
     * -------------------------- */
    VkDisplayPlaneCapabilitiesKHR  planeCap;
    VkDisplayPlaneAlphaFlagBitsKHR alphaMode = (VkDisplayPlaneAlphaFlagBitsKHR)0;
    vkGetDisplayPlaneCapabilitiesKHR (vk->phydev, displayMode, bestPlaneIndex, &planeCap);

    if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR)
    {
        alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR;
        VK_PRINT ("plane alpha mode : VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR\n");
    }
    else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR)
    {
        alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR;
        VK_PRINT ("plane alpha mode : VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR\n");
    }
    else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR)
    {
        alphaMode = VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR;
        VK_PRINT ("plane alpha mode : VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR\n");
    }
    else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR)
    {
        alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
        VK_PRINT ("plane alpha mode : VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR\n");
    }


    /* -------------------------- *
     *  create surface
     * -------------------------- */
    VkDisplaySurfaceCreateInfoKHR surfaceInfo = {0};
    surfaceInfo.sType              = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.pNext              = NULL;
    surfaceInfo.flags              = 0;
    surfaceInfo.displayMode        = displayMode;
    surfaceInfo.planeIndex         = bestPlaneIndex;
    surfaceInfo.planeStackIndex    = bestPlaneStackIndex;
    surfaceInfo.transform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    surfaceInfo.globalAlpha        = 1.0;
    surfaceInfo.alphaMode          = alphaMode;
    surfaceInfo.imageExtent.width  = width;
    surfaceInfo.imageExtent.height = height;

    VkSurfaceKHR surface;
    VK_CHECK (vkCreateDisplayPlaneSurfaceKHR (vk->instance, &surfaceInfo, NULL, &surface));

    return surface;
}
