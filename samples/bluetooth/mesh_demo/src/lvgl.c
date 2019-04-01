/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <lvgl.h>
#include <stdio.h>
#include <misc/printk.h>

#include "board.h"

/* How often to run the lvgl task handler (ms) */
#define LVGL_PERIOD	50

/* How long to wait before shrinking out the popup box (ms) */
#define POPUP_DELAY	1000

/* Duration of the shrink out animation (ms) */
#define POPUP_TIME	200

static char popup_text[48];
static lv_obj_t *popup_box;
static lv_obj_t *popup_label;

K_MUTEX_DEFINE(lvgl_mutex);

static void lvgl_work_handler(struct k_work *work)
{
	k_mutex_lock(&lvgl_mutex, K_FOREVER);
	lv_task_handler();
	k_mutex_unlock(&lvgl_mutex);
}

K_WORK_DEFINE(lvgl_work, lvgl_work_handler);

static void lvgl_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&lvgl_work);
}

K_TIMER_DEFINE(lvgl_timer, lvgl_timer_handler, NULL);

void board_play_tune(const char *str)
{
	printk("%s\n", __func__);
}

void board_heartbeat(u8_t hops, u16_t feat)
{
	printk("%s\n", __func__);
}

void board_other_dev_pressed(u16_t addr)
{
	k_mutex_lock(&lvgl_mutex, K_FOREVER);

	printk("%s(0x%04x)\n", __func__, addr);
	sprintf(popup_text, "Message from node %d", addr);

	lv_label_set_text(popup_label, popup_text);
	lv_obj_set_hidden(popup_box, false);
	lv_cont_set_fit(popup_box, true, true);
	lv_obj_align(popup_box, NULL, LV_ALIGN_CENTER, 0, 0);

	lv_obj_animate(popup_box, LV_ANIM_GROW_H | LV_ANIM_OUT,
		       POPUP_TIME, POPUP_DELAY, NULL);

	lv_obj_animate(popup_box, LV_ANIM_GROW_V | LV_ANIM_OUT,
		       POPUP_TIME, POPUP_DELAY, NULL);

	lv_cont_set_fit(popup_box, false, false);

	k_mutex_unlock(&lvgl_mutex);
}

void board_attention(bool attention)
{
	printk("%s\n", __func__);
}

void board_init(u16_t *addr)
{
#if defined(NODE_ADDR)
	*addr = NODE_ADDR;
#else
	*addr = 0x0b0c;
#endif
	k_mutex_lock(&lvgl_mutex, K_FOREVER);

	popup_box = lv_cont_create(lv_scr_act(), NULL);
	lv_obj_set_hidden(popup_box, true);

	popup_label = lv_label_create(popup_box, NULL);

	k_mutex_unlock(&lvgl_mutex);

	k_timer_start(&lvgl_timer, K_MSEC(LVGL_PERIOD), K_MSEC(LVGL_PERIOD));
}
