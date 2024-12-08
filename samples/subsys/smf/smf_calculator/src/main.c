/*
 * Copyright (c) 2024 Glenn Andrews
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main.h"
#include <zephyr/kernel.h>
#include "smf_calculator_thread.h"
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <lvgl_input_device.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(main_app);

K_MSGQ_DEFINE(output_msgq, CALCULATOR_STRING_LENGTH, 2, 1);

#define CALCULATOR_BUTTON_LABEL_LENGTH 4
struct calculator_button {
	char label[CALCULATOR_BUTTON_LABEL_LENGTH];
	struct calculator_event event;
};

/**
 * Important: this enum MUST be in the order of the buttons on the screen
 * otherwise the mapping from button index to function will break
 */
enum calculator_ui_buttons {
	BUTTON_CANCEL_ENTRY = 0,
	BUTTON_CANCEL,
	BUTTON_7,
	BUTTON_8,
	BUTTON_9,
	BUTTON_DIVIDE,
	BUTTON_4,
	BUTTON_5,
	BUTTON_6,
	BUTTON_MULTIPLY,
	BUTTON_1,
	BUTTON_2,
	BUTTON_3,
	BUTTON_SUBTRACT,
	BUTTON_0,
	BUTTON_DECIMAL_POINT,
	BUTTON_EQUALS,
	BUTTON_ADD,
};

struct calculator_button buttons[] = {
	[BUTTON_CANCEL_ENTRY] = {.label = "CE",
				 .event = {.event_id = CANCEL_ENTRY, .operand = 'E'}},
	[BUTTON_CANCEL] = {.label = "C", .event = {.event_id = CANCEL_BUTTON, .operand = 'C'}},
	[BUTTON_ADD] = {.label = "+", .event = {.event_id = OPERATOR, .operand = '+'}},
	[BUTTON_SUBTRACT] = {.label = "-", .event = {.event_id = OPERATOR, .operand = '-'}},
	[BUTTON_MULTIPLY] = {.label = "*", .event = {.event_id = OPERATOR, .operand = '*'}},
	[BUTTON_DIVIDE] = {.label = "/", .event = {.event_id = OPERATOR, .operand = '/'}},
	[BUTTON_DECIMAL_POINT] = {.label = ".",
				  .event = {.event_id = DECIMAL_POINT, .operand = '.'}},
	[BUTTON_EQUALS] = {.label = "=", .event = {.event_id = EQUALS, .operand = '='}},
	[BUTTON_0] = {.label = "0", .event = {.event_id = DIGIT_0, .operand = '0'}},
	[BUTTON_1] = {.label = "1", .event = {.event_id = DIGIT_1_9, .operand = '1'}},
	[BUTTON_2] = {.label = "2", .event = {.event_id = DIGIT_1_9, .operand = '2'}},
	[BUTTON_3] = {.label = "3", .event = {.event_id = DIGIT_1_9, .operand = '3'}},
	[BUTTON_4] = {.label = "4", .event = {.event_id = DIGIT_1_9, .operand = '4'}},
	[BUTTON_5] = {.label = "5", .event = {.event_id = DIGIT_1_9, .operand = '5'}},
	[BUTTON_6] = {.label = "6", .event = {.event_id = DIGIT_1_9, .operand = '6'}},
	[BUTTON_7] = {.label = "7", .event = {.event_id = DIGIT_1_9, .operand = '7'}},
	[BUTTON_8] = {.label = "8", .event = {.event_id = DIGIT_1_9, .operand = '8'}},
	[BUTTON_9] = {.label = "9", .event = {.event_id = DIGIT_1_9, .operand = '9'}},
};

/* Where the result is printed */
static lv_obj_t *result_label;

/**
 * LVGL v8.4 is not thread safe, so use a msgq to pass updates back
 * to the thread that calls lv_task_handler()
 */
void update_display(const char *output)
{
	while (k_msgq_put(&output_msgq, output, K_NO_WAIT) != 0) {
		k_msgq_purge(&output_msgq);
	}
}

static void lv_btn_matrix_click_callback(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = lv_event_get_target(e);

	if (code == LV_EVENT_PRESSED) {
		uint32_t id;
		int rc;

		id = lv_btnmatrix_get_selected_btn(obj);
		if (id >= ARRAY_SIZE(buttons)) {
			LOG_ERR("Invalid button: %d", id);
			return;
		}

		rc = post_calculator_event(&buttons[id].event, K_FOREVER);
		if (rc != 0) {
			LOG_ERR("could not post to msgq: %d", rc);
		}
	}
}

static int setup_display(void)
{
	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting setup");
		return -ENODEV;
	}

	static const char *const btnm_map[] = {buttons[BUTTON_CANCEL_ENTRY].label,
					       buttons[BUTTON_CANCEL].label,
					       "\n",
					       buttons[BUTTON_7].label,
					       buttons[BUTTON_8].label,
					       buttons[BUTTON_9].label,
					       buttons[BUTTON_DIVIDE].label,
					       "\n",
					       buttons[BUTTON_4].label,
					       buttons[BUTTON_5].label,
					       buttons[BUTTON_6].label,
					       buttons[BUTTON_MULTIPLY].label,
					       "\n",
					       buttons[BUTTON_1].label,
					       buttons[BUTTON_2].label,
					       buttons[BUTTON_3].label,
					       buttons[BUTTON_SUBTRACT].label,
					       "\n",
					       buttons[BUTTON_0].label,
					       buttons[BUTTON_DECIMAL_POINT].label,
					       buttons[BUTTON_EQUALS].label,
					       buttons[BUTTON_ADD].label,
					       "\n",
					       ""};

	lv_obj_t *btn_matrix = lv_btnmatrix_create(lv_scr_act());

	lv_btnmatrix_set_map(btn_matrix, (const char **)btnm_map);
	lv_obj_align(btn_matrix, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_set_size(btn_matrix, lv_pct(CALC_BTN_WIDTH_PCT), lv_pct(CALC_BTN_HEIGHT_PCT));
	lv_obj_add_event_cb(btn_matrix, lv_btn_matrix_click_callback, LV_EVENT_ALL, NULL);

	result_label = lv_label_create(lv_scr_act());
	lv_obj_set_width(result_label, lv_pct(CALC_RESULT_WIDTH_PCT));
	lv_obj_set_style_text_align(result_label, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_align(result_label, LV_ALIGN_TOP_MID, 0, lv_pct(CALC_RESULT_OFFSET_PCT));

	static lv_style_t style_shadow;

	lv_style_init(&style_shadow);
	lv_style_set_shadow_width(&style_shadow, 5);
	lv_style_set_shadow_spread(&style_shadow, 2);
	lv_style_set_shadow_color(&style_shadow, lv_palette_main(LV_PALETTE_GREY));
	lv_obj_add_style(result_label, &style_shadow, 0);
	update_display("0");

	lv_task_handler();
	display_blanking_off(display_dev);

	return 0;
}

int main(void)
{
	printk("SMF Desk Calculator Demo\n");

	int rc = setup_display();

	if (rc != 0) {
		return rc;
	}

	while (1) {
		char output[CALCULATOR_STRING_LENGTH];

		if (k_msgq_get(&output_msgq, output, K_MSEC(50)) == 0) {
			lv_label_set_text(result_label, output);
		}

		lv_task_handler();
	}
	return 0;
}
