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
    static int count = 0;

    draw_teapot (vk, rparam->count, rparam->col);

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



