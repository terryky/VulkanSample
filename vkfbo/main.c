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
#include "vk_render_target.h"
#include "teapot.h"
#include "util_debugstr.h"
#include "util_pmeter.h"

#define UNUSED(x) (void)(x)

typedef struct _render_param_t
{
    int     count;
    float   col[4];
    vk_rtarget_t rtarget;
} render_param_t;



extern int  init_texcube (vk_t *vk, int win_w, int win_h, vk_texture_t *tex);
extern void draw_texcube (vk_t *vk);

void
cb_make_command (VkCommandBuffer command, void *usr_data)
{
    vk_t *vk = (vk_t *)usr_data;
    render_param_t *rparam = (render_param_t *)vk->user_data;
    vk_rtarget_t rtarget = rparam->rtarget;
    
    static double ttime0 = 0, ttime1 = 0, interval;
    static int count = 0;
    char strbuf[512];

    PMETER_RESET_LAP ();
    PMETER_SET_LAP ();

    ttime1 = pmeter_get_time_ms ();
    interval = (count > 0) ? ttime1 - ttime0 : 0;
    ttime0 = ttime1;


    /* --------------------- *
     * render to FBO
     * --------------------- */
    vk_begin_render_target (vk, &rtarget);

    draw_teapot (vk, rparam->count, rparam->col);

    vk_end_render_target (vk, &rtarget);


    /* --------------------- *
     * Barrier
     * --------------------- */
    vk_barrier_render_target (vk, &rtarget);


    /* --------------------- *
     * render to Default FB
     * --------------------- */
    vk_begin_render_target (vk, NULL);

    draw_texcube (vk);
    draw_pmeter (vk, 0, 40);

    begin_dbgstr (vk);
    sprintf (strbuf, "%.1f [ms]\n", interval);
    draw_dbgstr (vk, strbuf, 10, 10);
    end_dbgstr (vk);

    vk_end_render_target (vk, NULL);


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
    vk_rtarget_t   rtarget;
    render_param_t rparam;

    vk_t *vk = vk_init (960, 540);

    /* create FBO */
    vk_create_render_target (vk, &rtarget);

    rparam.rtarget = rtarget;
    vk->user_data = &rparam;

    init_teapot (vk, (float)win_w / (float)win_h);
    init_pmeter (vk, win_w, win_h, 500);
    init_dbgstr (vk, win_w, win_h);
    init_texcube(vk, win_w, win_h, &rtarget.color_tgt);

    srand (time (NULL));
    rparam.col[0] = (rand () % 255) / 255.0f;
    rparam.col[1] = (rand () % 255) / 255.0f;
    rparam.col[2] = (rand () % 255) / 255.0f;
    rparam.col[3] = 1.0f;
    for (count = 0; ; count ++)
    {
        rparam.count = count;

        vk_render (vk, 1, cb_make_command, vk);
    }

    return 0;
}



