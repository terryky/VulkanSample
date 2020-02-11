/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef _VK_INIT_H_
#define _VK_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>


vk_t    *vk_init (int win_w, int win_h);
int     vk_create_buffer (vk_t *vk, uint32_t size, VkBufferUsageFlags usage, void *psrc, vk_buffer_t *vk_buf);
int     vk_create_texture (vk_t *vk, uint32_t width, uint32_t height, VkFormat format, vk_texture_t *vk_tex);
int     vk_load_shader_module (vk_t *vk, const char* fname, VkShaderModule *sm);




#ifdef __cplusplus
}
#endif
#endif /* _VK_INIT_H_ */
