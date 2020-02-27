/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef _WINSYS_H_
#define _WINSYS_H_

#ifdef __cplusplus
extern "C" {
#endif

int vkin_winsys_init (vk_t *vk, int win_w, int win_h);
VkSurfaceKHR vkin_winsys_create_surface (vk_t *vk);

#ifdef __cplusplus
}
#endif
#endif /* _WINSYS_H_ */
