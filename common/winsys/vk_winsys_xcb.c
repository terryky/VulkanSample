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
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#include "vk_tools.h"
#include "vk_winsys.h"


static xcb_connection_t *connection;
static xcb_screen_t *screen;
static xcb_window_t window;
static xcb_intern_atom_reply_t *atom_wm_delete_window;



static inline xcb_intern_atom_reply_t *
intern_atom_helper (xcb_connection_t *conn, bool only_if_exists, const char *str)
{
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom (conn, only_if_exists, strlen(str), str);
    return xcb_intern_atom_reply (conn, cookie, NULL);
}


int
vkin_winsys_init (vk_t *vk, int win_w, int win_h)
{
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    connection = xcb_connect (NULL, &scr);
    if (connection == NULL)
    {
        VK_LOGE ("xcb_connect failed: %d", 0);
        return -1;
    }

    setup = xcb_get_setup (connection);
    iter = xcb_setup_roots_iterator (setup);
    while (scr-- > 0)
        xcb_screen_next (&iter);

    screen = iter.data;
    window = xcb_generate_id (connection);

    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t value_list[32] =
    {
        screen->black_pixel,
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_POINTER_MOTION |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE
    };

#if 0
    if (fullscreen)
    {
        win_w = screen->width_in_pixels;
        win_h = screen->height_in_pixels;
    }
#endif

    xcb_create_window (connection, XCB_COPY_FROM_PARENT, window, screen->root,
                       0, 0, win_w, win_h, 0, 
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                       value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_reply_t *reply = intern_atom_helper (connection, true, "WM_PROTOCOLS");
    atom_wm_delete_window = intern_atom_helper (connection, false, "WM_DELETE_WINDOW");

    xcb_change_property (connection, XCB_PROP_MODE_REPLACE,
                         window, (*reply).atom, 4, 32, 1, &(*atom_wm_delete_window).atom);

    char windowTitle[] = "VulkanTest";
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
                        window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                        strlen(windowTitle), windowTitle);

    free (reply);

#if 0    
    if (settings.fullscreen)
    {
        xcb_intern_atom_reply_t *atom_wm_state      = intern_atom_helper (connection, false, "_NET_WM_STATE");
        xcb_intern_atom_reply_t *atom_wm_fullscreen = intern_atom_helper (connection, false, "_NET_WM_STATE_FULLSCREEN");
        xcb_change_property (connection, XCB_PROP_MODE_REPLACE,
                             window, atom_wm_state->atom, XCB_ATOM_ATOM, 32, 1,
                             &(atom_wm_fullscreen->atom));
        free (atom_wm_fullscreen);
        free (atom_wm_state);
    }
#endif

    xcb_map_window (connection, window);

#if 0
    xcb_flush(connection);
    while (1)
        ;
#endif

    return 0;
}


VkSurfaceKHR
vkin_winsys_create_surface (vk_t *vk)
{
    VkSurfaceKHR surface;

    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {0};
    surfaceCreateInfo.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.connection = connection;
    surfaceCreateInfo.window     = window;

    VK_CHECK (vkCreateXcbSurfaceKHR (vk->instance, &surfaceCreateInfo, NULL, &surface));

    return surface;
}

int
vkin_winsys_swap (vk_t *vk)
{
    return 0;
}

