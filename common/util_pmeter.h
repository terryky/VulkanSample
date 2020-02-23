/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2019 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef _PMETER_H_
#define _PMETER_H_

#define PMETER_MAX_LAP_NUM 128


#if 1

#define PMETER_RESET_LAP_EX(id) pmeter_reset_lap (id)
#define PMETER_SET_LAP_EX(id)   pmeter_set_lap (id)

#define PMETER_RESET_LAP() pmeter_reset_lap (0)
#define PMETER_SET_LAP()   pmeter_set_lap (0)

#else
#define PMETER_RESET_LAP_EX(id) ((void)0)
#define PMETER_SET_LAP_EX(id)   ((void)0)

#define PMETER_RESET_LAP() ((void)0)
#define PMETER_SET_LAP()   ((void)0)

#endif

double pmeter_get_time_ms ();
void   pmeter_reset_lap (int id);
void   pmeter_set_lap (int id);
int    init_pmeter (vk_t *vk, int win_w, int win_h, int data_num);
int    draw_pmeter_ex (vk_t *vk, int id, int x, int y, float scale);
int    draw_pmeter (vk_t *vk, int x, int y);


#endif
