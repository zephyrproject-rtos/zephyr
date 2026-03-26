/*
 * Copyright (c) 2025 rede97
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * LVGL on EFI GOP display sample (x86_64 UEFI).
 * Shows a status bar (title + resolution + uptime) and three progress
 * bars driven by worker threads via message queue.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#define STACK_SIZE      (1024 * 8)
#define LVGL_STACK_SIZE (1024 * 8)
#define PRIO            7
#define LVGL_PRIO       8
#define TICKS_MAX       100

#define THREAD_ID_A 0
#define THREAD_ID_B 1
#define THREAD_ID_C 2

struct progress_msg {
	uint8_t thread_id;
	uint32_t count;
};

#define PROGRESS_MSGQ_MAX 16
K_MSGQ_DEFINE(progress_msgq, sizeof(struct progress_msg), PROGRESS_MSGQ_MAX, 4);

static K_THREAD_STACK_DEFINE(thread_a_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(thread_b_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(thread_c_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(lvgl_thread_stack, LVGL_STACK_SIZE);
static struct k_thread thread_a_data;
static struct k_thread thread_b_data;
static struct k_thread thread_c_data;
static struct k_thread lvgl_thread_data;

#if defined(CONFIG_LVGL) && DT_HAS_CHOSEN(zephyr_display)

struct lvgl_ui {
	lv_obj_t *bar_a;
	lv_obj_t *bar_b;
	lv_obj_t *bar_c;
	lv_obj_t *time_label;
};

static bool lvgl_display_init(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	struct display_capabilities caps;

	if (!device_is_ready(display)) {
		return false;
	}
	display_get_capabilities(display, &caps);
	if (display_blanking_off(display) != 0) {
		return false;
	}
	printk("LVGL + GOP: %u x %u\n", caps.x_resolution, caps.y_resolution);
	return true;
}

static lv_obj_t *lvgl_create_bar_row(lv_obj_t *parent, const char *label_text)
{
	lv_obj_t *row = lv_obj_create(parent);
	lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_row(row, 4, 0);
	lv_obj_set_style_pad_column(row, 0, 0);
	lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *label = lv_label_create(row);
	lv_label_set_text(label, label_text);

	lv_obj_t *bar = lv_bar_create(row);
	lv_obj_set_width(bar, LV_PCT(100));
	lv_obj_set_height(bar, 20);
	lv_bar_set_range(bar, 0, TICKS_MAX);
	lv_bar_set_value(bar, 0, LV_ANIM_OFF);

	return bar;
}

static void lvgl_create_top_bar(struct lvgl_ui *ui, lv_obj_t *parent)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	struct display_capabilities caps;
	char title_buf[64];
	lv_obj_t *top_bar = lv_obj_create(parent);
	lv_obj_t *label_title;
	lv_obj_t *spacer;

	display_get_capabilities(display, &caps);

	lv_obj_set_size(top_bar, LV_PCT(100), 36);
	lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
	lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1e1e1e), 0);
	lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(top_bar, 0, 0);
	lv_obj_set_style_pad_left(top_bar, 24, 0);
	lv_obj_set_style_pad_right(top_bar, 24, 0);
	lv_obj_set_style_pad_top(top_bar, 8, 0);
	lv_obj_set_style_pad_bottom(top_bar, 8, 0);
	lv_obj_set_style_pad_column(top_bar, 16, 0);
	lv_obj_remove_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

	label_title = lv_label_create(top_bar);
	snprintf(title_buf, sizeof(title_buf), "Zephyr + LVGL on GOP (%ux%u)",
		 caps.x_resolution, caps.y_resolution);
	lv_label_set_text(label_title, title_buf);
	lv_obj_set_style_text_color(label_title, lv_color_hex(0xFFFFFF), 0);

	spacer = lv_obj_create(top_bar);
	lv_obj_set_size(spacer, 0, 0);
	lv_obj_set_flex_grow(spacer, 1);
	lv_obj_remove_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

	ui->time_label = lv_label_create(top_bar);
	lv_label_set_text(ui->time_label, "00:00:00");
	lv_obj_set_style_text_color(ui->time_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(ui->time_label, LV_TEXT_ALIGN_RIGHT, 0);
}

static void lvgl_create_content(struct lvgl_ui *ui, lv_obj_t *parent)
{
	lv_obj_t *content = lv_obj_create(parent);
	lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(content, 16, 0);
	lv_obj_set_style_pad_row(content, 12, 0);
	lv_obj_set_flex_grow(content, 1);
	lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

	ui->bar_a = lvgl_create_bar_row(content, "thread_a");
	lv_obj_set_flex_grow(lv_obj_get_parent(ui->bar_a), 1);
	ui->bar_b = lvgl_create_bar_row(content, "thread_b");
	lv_obj_set_flex_grow(lv_obj_get_parent(ui->bar_b), 1);
	ui->bar_c = lvgl_create_bar_row(content, "thread_c");
	lv_obj_set_flex_grow(lv_obj_get_parent(ui->bar_c), 1);
}

static void lvgl_ui_create(struct lvgl_ui *ui)
{
	lv_obj_t *screen = lv_screen_active();
	lv_obj_t *main_cont = lv_obj_create(screen);
	lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
	lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(main_cont, 0, 0);
	lv_obj_set_style_pad_row(main_cont, 0, 0);
	lv_obj_remove_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);

	lvgl_create_top_bar(ui, main_cont);
	lvgl_create_content(ui, main_cont);
}

static void lvgl_thread_entry(void *p1, void *p2, void *p3)
{
	struct lvgl_ui *ui = (struct lvgl_ui *)p1;
	lv_obj_t *bars[] = { ui->bar_a, ui->bar_b, ui->bar_c };
	uint32_t last_uptime_sec = 0;
	struct progress_msg msg;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		uint32_t uptime_sec;

		while (k_msgq_get(&progress_msgq, &msg, K_NO_WAIT) == 0) {
			if (msg.thread_id < 3 && bars[msg.thread_id] != NULL) {
				lv_bar_set_value(bars[msg.thread_id],
						 (int32_t)msg.count, LV_ANIM_OFF);
			}
		}
		uptime_sec = k_uptime_get_32() / 1000;
		if (ui->time_label != NULL && uptime_sec != last_uptime_sec) {
			uint32_t h = uptime_sec / 3600;
			uint32_t m = (uptime_sec % 3600) / 60;
			uint32_t s = uptime_sec % 60;
			char buf[16];

			last_uptime_sec = uptime_sec;
			snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
				 (unsigned)h, (unsigned)m, (unsigned)s);
			lv_label_set_text(ui->time_label, buf);
		}
		lv_timer_handler();
		k_msleep(10);
	}
}

#endif /* CONFIG_LVGL && DT_HAS_CHOSEN(zephyr_display) */

static void thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t sleep_ms = (uint32_t)(uintptr_t)p1;
	uint8_t thread_id = (uint8_t)(uintptr_t)p3;
	uint32_t count = 0;
	struct progress_msg msg;

	ARG_UNUSED(p2);

	while (count < TICKS_MAX) {
		msg.thread_id = thread_id;
		msg.count = count;
		k_msgq_put(&progress_msgq, &msg, K_NO_WAIT);
		count++;
		k_msleep(sleep_ms);
	}
	msg.thread_id = thread_id;
	msg.count = TICKS_MAX;
	k_msgq_put(&progress_msgq, &msg, K_FOREVER);
}

int main(void)
{
	printk("\n*** LVGL on EFI GOP sample ***\n");
	printk("Board: %s\n\n", CONFIG_BOARD);

#if defined(CONFIG_LVGL) && DT_HAS_CHOSEN(zephyr_display)
	if (lvgl_display_init()) {
		static struct lvgl_ui lvgl_ui_ctx;

		lvgl_ui_create(&lvgl_ui_ctx);
		k_thread_create(&lvgl_thread_data, lvgl_thread_stack,
				K_THREAD_STACK_SIZEOF(lvgl_thread_stack),
				lvgl_thread_entry, &lvgl_ui_ctx, NULL, NULL,
				LVGL_PRIO, 0, K_NO_WAIT);
		k_thread_name_set(&lvgl_thread_data, "lvgl");
	} else {
		printk("GOP display not ready, run threads only.\n");
	}
#endif

	k_thread_create(&thread_a_data, thread_a_stack,
			K_THREAD_STACK_SIZEOF(thread_a_stack),
			thread_entry, (void *)(uintptr_t)20, NULL, (void *)(uintptr_t)THREAD_ID_A,
			PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&thread_a_data, "thread_a");
	k_thread_create(&thread_b_data, thread_b_stack,
			K_THREAD_STACK_SIZEOF(thread_b_stack),
			thread_entry, (void *)(uintptr_t)30, NULL, (void *)(uintptr_t)THREAD_ID_B,
			PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&thread_b_data, "thread_b");
	k_thread_create(&thread_c_data, thread_c_stack,
			K_THREAD_STACK_SIZEOF(thread_c_stack),
			thread_entry, (void *)(uintptr_t)40, NULL, (void *)(uintptr_t)THREAD_ID_C,
			PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&thread_c_data, "thread_c");

	k_thread_join(&thread_a_data, K_FOREVER);
	k_thread_join(&thread_b_data, K_FOREVER);
	k_thread_join(&thread_c_data, K_FOREVER);

	printk("Threads finished.\n");
	return 0;
}
