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

#ifdef __cplusplus
}
#endif
#endif /* _VK_UTIL_H_ */
