/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2019 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "vk_tools.h"
#include "vk_init.h"
#include "vk_render.h"

int 
main (int argc, char *argv[])
{
    vk_init (960, 540);
 
    while (1)
    {
        vk_render ();
    }
}
