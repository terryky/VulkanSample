/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef _VK_RENDER_TARGET_H_
#define _VK_RENDER_TARGET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vk_tools.h"

typedef struct _vk_rtarget_t
{
    VkFramebuffer       framebuffer;
    VkRenderPass        render_pass;
    vk_texture_t        color_tgt;
    vk_render_buffer_t  depth_tgt;
    uint32_t            width;
    uint32_t            height;
} vk_rtarget_t;


int vk_create_render_target  (vk_t *vk, vk_rtarget_t *rtarget);
int vk_begin_render_target   (vk_t *vk, vk_rtarget_t *rtarget);
int vk_end_render_target     (vk_t *vk, vk_rtarget_t *rtarget);
int vk_barrier_render_target (vk_t *vk, vk_rtarget_t *rtarget);


#ifdef __cplusplus
}
#endif
#endif /* _VK_RENDER_TARGET_H_ */
