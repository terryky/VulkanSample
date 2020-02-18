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

int     vk_create_buffer (vk_t *vk, uint32_t size, 
                          VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, 
                          void *psrc, vk_buffer_t *vk_buf);
int     vk_create_texture (vk_t *vk, uint32_t width, uint32_t height, VkFormat format, vk_texture_t *vk_tex);
int     vk_load_shader_module (vk_t *vk, const char* fname, VkShaderModule *sm);
int     vk_devmemcpy (vk_t *vk, VkDeviceMemory mem, void *psrc, uint32_t size);


/* utility functions for vkCreateGraphicsPipelines() */
int     vk_get_default_input_assembly_state (vk_t *vk, VkPipelineInputAssemblyStateCreateInfo *state,
                                             VkPrimitiveTopology topology);
int     vk_get_default_viewport_state (vk_t *vk, VkPipelineViewportStateCreateInfo *state);
int     vk_get_default_rasterizer_state (vk_t *vk, VkPipelineRasterizationStateCreateInfo *state);
int     vk_get_default_multisample_state (vk_t *vk, VkPipelineMultisampleStateCreateInfo *state);
int     vk_get_default_depth_stencil_state (vk_t *vk, VkPipelineDepthStencilStateCreateInfo *state,
                                           int depth_en, int stencil_en);
int     vk_get_default_blend_state (vk_t *vk, VkPipelineColorBlendStateCreateInfo *state,
                                    int blend_en);

int     vk_destroy_default_input_assembly_state (vk_t *vk, VkPipelineInputAssemblyStateCreateInfo *state);
int     vk_destroy_default_viewport_state (vk_t *vk, VkPipelineViewportStateCreateInfo *state);
int     vk_destroy_default_rasterizer_state (vk_t *vk, VkPipelineRasterizationStateCreateInfo *state);
int     vk_destroy_default_multisample_state (vk_t *vk, VkPipelineMultisampleStateCreateInfo *state);
int     vk_destroy_default_depth_stencil_state (vk_t *vk, VkPipelineDepthStencilStateCreateInfo *state);
int     vk_destroy_default_blend_state (vk_t *vk, VkPipelineColorBlendStateCreateInfo *state);


#ifdef __cplusplus
}
#endif
#endif /* _VK_INIT_H_ */
