/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
/*
 * At first, you need to set up environment as below.
 *  > sudo apt install libglfw3-dev
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include "vk_tools.h"
#include "vk_winsys.h"

static int s_win_w;
static int s_win_h;



static void
key_cb (GLFWwindow *window, int key, int scancode, int action, int mods)
{
    VK_LOGI ("glfw: key(%d), action(%d), modes(%d)", key, action, mods);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose (window, GLFW_TRUE);
    }
}

static void
mouse_button_cb (GLFWwindow *window, int button, int action, int mods)
{
    VK_LOGI ("glfw: mouse button(%d), action(%d), modes(%d)", button, action, mods);
}

static void
mouse_pos_cb (GLFWwindow *window, double x, double y)
{
    VK_LOGI ("glfw: mouse pos(%f, %f)", x, y);
}

static void
mouse_scroll_cb (GLFWwindow *window, double x, double y) 
{
    VK_LOGI ("glfw: mouse scroll(%f, %f)", x, y);
}

int
vkin_winsys_init (vk_t *vk, int win_w, int win_h)
{
    int ret;
    const char **extension_name;
    uint32_t     extension_count;
    
    ret = glfwInit ();
    if (ret != GLFW_TRUE)
    {
        VK_LOGE ("glfwInit(): ret=%d", ret);
        return -1;
    }

    extension_name = glfwGetRequiredInstanceExtensions (&extension_count);
    for (uint32_t i = 0; i < extension_count; i ++)
    {
        VK_LOGI ("glfw:ext[%d/%d]: %s", i, extension_count, extension_name[i]);
    }

    s_win_w = win_w;
    s_win_h = win_h;

    return 0;
}


VkSurfaceKHR
vkin_winsys_create_surface (vk_t *vk)
{
    int is_fullscreen = 0;

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

    GLFWwindow *window;

    window = glfwCreateWindow (s_win_w, s_win_h, "GLFWSurface", 
                    is_fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (window == NULL)
    {
        VK_LOGE ("glfw:glfwCreateWindow (%d)", 0);
        glfwTerminate ();
        return NULL;
    }

    glfwSetMouseButtonCallback (window, mouse_button_cb);
    glfwSetCursorPosCallback (window, mouse_pos_cb);
    glfwSetScrollCallback (window, mouse_scroll_cb);
    glfwSetKeyCallback (window, key_cb);

    VkSurfaceKHR surface;
    VK_CHECK (glfwCreateWindowSurface (vk->instance, window, NULL, &surface));

    return surface;
}


int
vkin_winsys_swap (vk_t *vk)
{
    glfwPollEvents ();
    return 0;
}

