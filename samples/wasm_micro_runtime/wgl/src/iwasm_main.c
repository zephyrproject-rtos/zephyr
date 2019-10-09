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
#include "bh_platform.h"
#include "runtime_lib.h"
#include "native_interface.h"
#include "app_manager_export.h"
#include "board_config.h"
#include "bh_common.h"
#include "bh_queue.h"
#include "bh_thread.h"
#include "bh_memory.h"
#include "runtime_sensor.h"
#include "attr_container.h"
#include "module_wasm_app.h"
#include "wasm_export.h"
#include "display.h"
#include "lvgl.h"

extern void * thread_timer_check(void *);
extern void init_sensor_framework();
extern int aee_host_msg_callback(void *msg, uint16_t msg_len);
extern bool touchscreen_read(lv_indev_data_t * data);
extern int ili9340_init();
extern void xpt2046_init(void);
extern void wgl_init();

#include <zephyr.h>
#include <uart.h>
#include <device.h>

int uart_char_cnt = 0;

static void uart_irq_callback(struct device *dev)
{
    unsigned char ch;

    while (uart_poll_in(dev, &ch) == 0) {
        uart_char_cnt++;
        aee_host_msg_callback(&ch, 1);
    }
}

struct device *uart_dev = NULL;

static bool host_init()
{
    uart_dev = device_get_binding(HOST_DEVICE_COMM_UART_NAME);
    if (!uart_dev) {
        printf("UART: Device driver not found.\n");
        return false;
    }
    uart_irq_rx_enable(uart_dev);
    uart_irq_callback_set(uart_dev, uart_irq_callback);
    return true;
}

int host_send(void * ctx, const char *buf, int size)
{
    if (!uart_dev)
        return 0;

    for (int i = 0; i < size; i++)
        uart_poll_out(uart_dev, buf[i]);

    return size;
}

void host_destroy()
{
}

host_interface interface = {
    .init = host_init,
    .send = host_send,
    .destroy = host_destroy
};

timer_ctx_t timer_ctx;

static char global_heap_buf[270 * 1024] = { 0 };

static uint8_t color_copy[320 * 10 * 3];

static void display_flush(lv_disp_drv_t *disp_drv,
                          const lv_area_t *area,
                          lv_color_t *color)
{
    u16_t w = area->x2 - area->x1 + 1;
    u16_t h = area->y2 - area->y1 + 1;
    struct display_buffer_descriptor desc;
    int i;
    uint8_t *color_p = color_copy;

    desc.buf_size = 3 * w * h;
    desc.width = w;
    desc.pitch = w;
    desc.height = h;

    for (i = 0; i < w * h; i++, color++) {
        color_p[i * 3] = color->ch.red;
        color_p[i * 3 + 1] = color->ch.green;
        color_p[i * 3 + 2] = color->ch.blue;
    }

    display_write(NULL, area->x1, area->y1, &desc, (void *) color_p);

    lv_disp_flush_ready(disp_drv); /* in v5.3 is lv_flush_ready */
}

static bool display_input_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    return touchscreen_read(data);
}

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
 */
static void hal_init(void)
{
    xpt2046_init();
    ili9340_init();
    display_blanking_off(NULL);

    /*Create a display buffer*/
    static lv_disp_buf_t disp_buf1;
    static lv_color_t buf1_1[320*10];
    lv_disp_buf_init(&disp_buf1, buf1_1, NULL, 320*10);

    /*Create a display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.buffer = &disp_buf1;
    disp_drv.flush_cb = display_flush;
    //    disp_drv.hor_res = 200;
    //    disp_drv.ver_res = 100;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);          /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = display_input_read;
    lv_indev_drv_register(&indev_drv);
}

int iwasm_main()
{
    host_init();

    if (bh_memory_init_with_pool(global_heap_buf, sizeof(global_heap_buf))
            != 0) {
        printf("Init global heap failed.\n");
        return -1;
    }

    if (vm_thread_sys_init() != 0) {
        goto fail1;
    }

    wgl_init();
    hal_init();

    // timer manager
    init_wasm_timer();

    // TODO:
    app_manager_startup(&interface);

fail1:
    bh_memory_destroy();
    return -1;
}
