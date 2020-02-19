/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef _VK_RENDER_H_
#define _VK_RENDER_H_

#ifdef __cplusplus
extern "C" {
#endif

int vk_render (vk_t *vk, uint32_t flags, void (*cb_make_command)(VkCommandBuffer, void *), void *usr_data);

#ifdef __cplusplus
}
#endif
#endif /* _VK_RENDER_H_ */
