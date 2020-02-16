/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2019 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef _TEAPOT_H_
#define _TEAPOT_H_
 
int     init_teapot (vk_t *vk, float aspect);
void    draw_teapot (vk_t *vk, int count, float col[3]);
int     delete_teapot ();
 
 #endif /* _TEAPOT_H_ */
 