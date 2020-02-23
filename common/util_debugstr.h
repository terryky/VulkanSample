/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2019 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef _UTIL_DEBUGSTR_H_
#define _UTIL_DEBUGSTR_H_



int  init_dbgstr (vk_t *vk, int win_w, int win_h);
int  draw_dbgstr    (vk_t *vk, char *str, int x, int y);
int  draw_dbgstr_ex (vk_t *vk, char *str, int x, int y, float scale, float *col_fg, float *col_bg);

int  begin_dbgstr (vk_t *vk);
int  end_dbgstr (vk_t *vk);

#endif /* _UTIL_DEBUGSTR_H_ */
