/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2019 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vk_tools.h"
#include "vk_init.h"
#include "vk_render.h"
#include "teapot.h"
#include "util_debugstr.h"
#include "util_pmeter.h"

#define UNUSED(x) (void)(x)

typedef struct _render_param_t
{
    int     count;
    float   col[4];
} render_param_t;


void
cb_make_command (VkCommandBuffer command, void *usr_data)
{
    vk_t *vk = (vk_t *)usr_data;
    render_param_t *rparam = (render_param_t *)vk->user_data;
    static double ttime0 = 0, ttime1 = 0, interval;
    static int count = 0;
    char strbuf[512];

    PMETER_RESET_LAP ();
    PMETER_SET_LAP ();

    ttime1 = pmeter_get_time_ms ();
    interval = (count > 0) ? ttime1 - ttime0 : 0;
    ttime0 = ttime1;

    draw_teapot (vk, rparam->count, rparam->col);
    draw_pmeter (vk, 0, 40);

    begin_dbgstr (vk);
    sprintf (strbuf, "%.1f [ms]\n", interval);
    draw_dbgstr (vk, strbuf, 10, 10);
    end_dbgstr (vk);

    count ++;
}

/*--------------------------------------------------------------------------- *
 *      M A I N    F U N C T I O N
 *--------------------------------------------------------------------------- */
int
main(int argc, char *argv[])
{
    int count;
    int win_w = 960;
    int win_h = 540;
    render_param_t rparam;

    vk_t *vk = vk_init (960, 540);
    vk->user_data = &rparam;

    init_teapot (vk, (float)win_w / (float)win_h);
    init_pmeter (vk, win_w, win_h, 500);
    init_dbgstr (vk, win_w, win_h);

    srand (time (NULL));
    rparam.col[0] = (rand () % 255) / 255.0f;
    rparam.col[1] = (rand () % 255) / 255.0f;
    rparam.col[2] = (rand () % 255) / 255.0f;
    rparam.col[3] = 1.0f;
    for (count = 0; ; count ++)
    {
        rparam.count = count;

        vk_render (vk, cb_make_command, vk);
    }

    return 0;
}



