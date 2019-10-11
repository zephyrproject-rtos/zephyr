/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
//#include <unistd.h>
#include <inttypes.h>
#include "lvgl/lvgl.h"
#include "display_indev.h"
#include "wasm_app.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init(void);
//static int tick_thread(void * data);
//static void memory_monitor(void * param);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
uint32_t count = 0;
char count_str[11] = { 0 };
lv_obj_t *hello_world_label;
lv_obj_t *count_label;
lv_obj_t * btn1;

lv_obj_t * label_count1;
int label_count1_value = 0;
char label_count1_str[11] = { 0 };

void timer1_update(user_timer_t timer1)
{
    if ((count % 100) == 0) {
        sprintf(count_str, "%d", count / 100);
        lv_label_set_text(count_label, count_str);
    }
    lv_task_handler();
    ++count;
}


static lv_res_t btn_rel_action(lv_obj_t * btn)
{
    label_count1_value++;
    sprintf(label_count1_str, "%d", label_count1_value);
    lv_label_set_text(label_count1, label_count1_str);
    return LV_RES_OK;
}


void on_init()
{
    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LittlevGL*/
    hal_init();

    hello_world_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(hello_world_label, "Hello world!");
    lv_obj_align(hello_world_label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    count_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(count_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

    btn1 = lv_btn_create(lv_scr_act(), NULL); /*Create a button on the currently loaded screen*/
    lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, btn_rel_action); /*Set function to be called when the button is released*/
    lv_obj_align(btn1, NULL, LV_ALIGN_CENTER, 0, 20); /*Align below the label*/

    /*Create a label on the button*/
    lv_obj_t * btn_label = lv_label_create(btn1, NULL);
    lv_label_set_text(btn_label, "Click ++");

    label_count1 = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(label_count1, "0");
    lv_obj_align(label_count1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

    /* set up a timer */
    user_timer_t timer;
    timer = api_timer_create(10, true, false, timer1_update);
    api_timer_restart(timer, 10);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
 */
void display_flush_wrapper(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        const lv_color_t * color_p)
{
    display_flush(x1, y1, x2, y2, color_p);
    lv_flush_ready();
}
void display_vdb_write_wrapper(uint8_t *buf, lv_coord_t buf_w, lv_coord_t x,
        lv_coord_t y, lv_color_t color, lv_opa_t opa)
{
    display_vdb_write(buf, buf_w, x, y, &color, opa);
}
extern void display_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        lv_color_t color_p);
extern void display_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        const lv_color_t * color_p);
static void hal_init(void)
{
    /* Add a display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv); /*Basic initialization*/
    disp_drv.disp_flush = display_flush_wrapper; /*Used when `LV_VDB_SIZE != 0` in lv_conf.h (buffered drawing)*/
    disp_drv.disp_fill = display_fill; /*Used when `LV_VDB_SIZE == 0` in lv_conf.h (unbuffered drawing)*/
    disp_drv.disp_map = display_map; /*Used when `LV_VDB_SIZE == 0` in lv_conf.h (unbuffered drawing)*/
#if LV_VDB_SIZE != 0
    disp_drv.vdb_wr = display_vdb_write_wrapper;
#endif
    lv_disp_drv_register(&disp_drv);

    /* Add the mouse as input device
     * Use the 'mouse' driver which reads the PC's mouse*/
//    mouse_init();
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv); /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read = display_input_read; /*This function will be called periodically (by the library) to get the mouse position and state*/
    lv_indev_t * mouse_indev = lv_indev_drv_register(&indev_drv);

}

