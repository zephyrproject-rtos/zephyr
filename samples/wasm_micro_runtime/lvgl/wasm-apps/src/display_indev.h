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

#ifndef DISPLAY_INDEV_H_
#define DISPLAY_INDEV_H_
#include <stdio.h>
#include <inttypes.h>

#include "lvgl/lv_misc/lv_color.h"
#include "lvgl/lv_hal/lv_hal_indev.h"
extern void display_init(void);
extern void display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        const lv_color_t * color_p);
extern bool display_input_read(lv_indev_data_t * data);
extern void display_deinit(void);
extern void display_vdb_write(void *buf, lv_coord_t buf_w, lv_coord_t x,
        lv_coord_t y, lv_color_t *color, lv_opa_t opa);
extern uint32_t time_get_ms(void);

#endif
