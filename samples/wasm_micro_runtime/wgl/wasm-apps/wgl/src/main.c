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

#include <stdlib.h>
#include <unistd.h>
#include "wasm_app.h"

static void btn_event_cb(wgl_obj_t btn, wgl_event_t event);

uint32_t count = 0;
char count_str[11] = { 0 };
wgl_obj_t hello_world_label;
wgl_obj_t count_label;
wgl_obj_t btn1;
wgl_obj_t label_count1;
int label_count1_value = 0;
char label_count1_str[11] = { 0 };

void timer1_update(user_timer_t timer1)
{
    if ((count % 100) == 0) {
        sprintf(count_str, "%d", count / 100);
        wgl_label_set_text(count_label, count_str);
    }
    ++count;
}

void on_init()
{
    hello_world_label = wgl_label_create((wgl_obj_t)NULL, (wgl_obj_t)NULL);
    wgl_label_set_text(hello_world_label, "Hello world!");
    wgl_obj_align(hello_world_label, (wgl_obj_t)NULL, WGL_ALIGN_IN_TOP_LEFT, 0, 0);

    count_label = wgl_label_create((wgl_obj_t)NULL, (wgl_obj_t)NULL);
    wgl_obj_align(count_label, (wgl_obj_t)NULL, WGL_ALIGN_IN_TOP_MID, 0, 0);

    btn1 = wgl_btn_create((wgl_obj_t)NULL, (wgl_obj_t)NULL); /*Create a button on the currently loaded screen*/
    wgl_obj_set_event_cb(btn1, btn_event_cb); /*Set function to be called when the button is released*/
    wgl_obj_align(btn1, (wgl_obj_t)NULL, WGL_ALIGN_CENTER, 0, 0); /*Align below the label*/

    /*Create a label on the button*/
    wgl_obj_t btn_label = wgl_label_create(btn1, (wgl_obj_t)NULL);
    wgl_label_set_text(btn_label, "Click ++");

    label_count1 = wgl_label_create((wgl_obj_t)NULL, (wgl_obj_t)NULL);
    wgl_label_set_text(label_count1, "0");
    wgl_obj_align(label_count1, (wgl_obj_t)NULL, WGL_ALIGN_IN_BOTTOM_MID, 0, 0);

    /* set up a timer */
    user_timer_t timer;
    timer = api_timer_create(10, true, false, timer1_update);
    api_timer_restart(timer, 10);
}

static void btn_event_cb(wgl_obj_t btn, wgl_event_t event)
{
    if(event == WGL_EVENT_RELEASED) {
        label_count1_value++;
        sprintf(label_count1_str, "%d", label_count1_value);
        wgl_label_set_text(label_count1, label_count1_str);

        //wgl_cont_set_fit4(btn, WGL_FIT_FLOOD, WGL_FIT_FLOOD, WGL_FIT_FLOOD, WGL_FIT_FLOOD);
        //wgl_obj_clean(btn);
    }
}
