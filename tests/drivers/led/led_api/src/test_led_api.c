/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/drivers/led.h>

#define BRIGHTNESS_MAX	100
#define TEST_MAX_COLORS	8
#define COLOR_FULL	0xff

#define _COLOR_MAPPING(led_node_id)				\
const uint8_t test_color_mapping_##led_node_id[] =		\
	DT_PROP(led_node_id, color_mapping)

#define COLOR_MAPPING(led_node_id)					\
	IF_ENABLED(DT_NODE_HAS_PROP(led_node_id, color_mapping),	\
		   (_COLOR_MAPPING(led_node_id);))

#define LED_INFO_COLOR(led_node_id)				\
{								\
	.label		= DT_LABEL(led_node_id),		\
	.index		= DT_PROP_OR(led_node_id, index, 0),	\
	.num_colors	=					\
		DT_PROP_LEN(led_node_id, color_mapping),	\
	.color_mapping	= test_color_mapping_##led_node_id,	\
},

#define LED_INFO_NO_COLOR(led_node_id)				\
{								\
	.label		= DT_LABEL(led_node_id),		\
	.index		= DT_PROP_OR(led_node_id, index, 0),	\
	.num_colors	= 0,					\
	.color_mapping	= NULL,					\
},

#define LED_INFO(led_node_id)	\
	COND_CODE_1(DT_NODE_HAS_PROP(led_node_id, color_mapping),	\
		    (LED_INFO_COLOR(led_node_id)),			\
		    (LED_INFO_NO_COLOR(led_node_id)))

#define LED_CONTROLLER_INFO(node_id)				\
								\
DT_FOREACH_CHILD(node_id, COLOR_MAPPING)			\
								\
const struct led_info test_led_info[] = {			\
	DT_FOREACH_CHILD(node_id, LED_INFO)			\
};								\
								\
static ZTEST_DMEM int num_leds = ARRAY_SIZE(test_led_info)

LED_CONTROLLER_INFO(DT_ALIAS(led_controller_0));

static ZTEST_BMEM const struct device *led_ctrl = DEVICE_DT_GET(DT_ALIAS(led_controller_0));

void test_led_setup(void)
{
	zassert_true(device_is_ready(led_ctrl), "LED controller is not ready");

	zassert_not_equal(num_leds, 0, "No LEDs subnodes found in DT for controller");

	k_object_access_grant(led_ctrl, k_current_get());
}

void test_led_get_info(void)
{
	uint8_t led;
	int ret;

	if (!led_ctrl || !num_leds) {
		ztest_test_skip();
	}

	for (led = 0; led < num_leds; led++) {
		const struct led_info *info;
		uint8_t col;

		ret = led_get_info(led_ctrl, led, &info);
		if (ret == -ENOTSUP) {
			TC_PRINT("led_get_info() syscall is not supported.\n");
			ztest_test_skip();
			return;
		}

		zassert_equal(ret, 0, "LED %d - led_get_info() error (ret=%d)",
			      led, ret);

		zassert_true(!strcmp(info->label, test_led_info[led].label),
			     "LED %d - label: %s instead of %s",
			     led, info->label, test_led_info[led].label);

		zassert_equal(info->index, test_led_info[led].index,
			      "LED %d - index: %d instead of %d", led,
			      info->index, test_led_info[led].index);

		zassert_equal(info->num_colors, test_led_info[led].num_colors,
			      "LED %d - num_colors: %d instead of %d", led,
			      info->num_colors, test_led_info[led].num_colors);

		TC_PRINT("LED %d - label: %s, index: %d, num_colors: %d",
			 led, info->label, info->index, info->num_colors);

		if (!info->num_colors) {
			continue;
		}

		TC_PRINT(" color_mapping: ");

		for (col = 0; col < info->num_colors; col++) {
			zassert_equal(info->color_mapping[col],
				test_led_info[led].color_mapping[col],
				"LED %d - color_mapping[%d]=%d instead of %d",
				led, col, info->color_mapping[col],
				test_led_info[led].color_mapping[col]);
			TC_PRINT("%d", info->color_mapping[col]);
		}
		TC_PRINT("\n");
	}
}

void test_led_on(void)
{
	uint8_t led;
	int ret;

	if (!led_ctrl || !num_leds) {
		ztest_test_skip();
	}

	for (led = 0; led < num_leds; led++) {
		ret = led_on(led_ctrl, led);
		zassert_equal(ret, 0, "LED %d - failed to turn on", led);
	}
}

void test_led_off(void)
{
	uint8_t led;
	int ret;

	if (!led_ctrl || !num_leds) {
		ztest_test_skip();
	}

	for (led = 0; led < num_leds; led++) {
		ret = led_off(led_ctrl, led);
		zassert_equal(ret, 0, "LED %d - failed to turn off", led);
	}
}

void test_led_set_color(void)
{
	uint8_t led;
	uint8_t colors[TEST_MAX_COLORS + 1];
	int ret;

	if (!led_ctrl || !num_leds) {
		ztest_test_skip();
	}

	for (led = 0; led < num_leds; led++) {
		uint8_t num_colors = test_led_info[led].num_colors;
		uint8_t col;

		if (num_colors > TEST_MAX_COLORS) {
			TC_PRINT("LED %d - skip set_color test, num_colors: %d"
				 " (test limit is %d)",
				 led, num_colors, TEST_MAX_COLORS);
			continue;
		}

		memset(colors, 0, sizeof(colors));

		/* Try to set more colors than supported. */
		ret = led_set_color(led_ctrl, led, num_colors + 1, colors);
		zassert_not_equal(ret, 0, "LED %d - setting %d"
				  " colors should fail (%d supported)",
				  led, num_colors + 1, num_colors);

		if (!num_colors) {
			continue;
		}

		/* Try to set less colors than supported. */
		ret = led_set_color(led_ctrl, led, num_colors - 1, colors);
		zassert_not_equal(ret, 0, "LED %d - setting %d"
				  " colors should fail (%d supported)",
				  led, num_colors - 1, num_colors);

		/* Ensure the LED is on to get a visual feedback. */
		led_set_brightness(led_ctrl, led, BRIGHTNESS_MAX / 2);

		/* Set each color gradually to its maximum level. */
		for (col = 0; col < num_colors; col++) {
			uint16_t level;

			memset(colors, 0, sizeof(colors));

			for (level = 0; level <= COLOR_FULL; level++) {
				colors[col] = level;

				ret = led_set_color(led_ctrl, led,
						    num_colors, colors);
				zassert_equal(ret, 0,
					"LED %d - failed to set color[%d] to %d",
					led, level);
			}
		}
	}
}

void test_led_set_brightness(void)
{
	uint8_t led;
	int ret;

	if (!led_ctrl || !num_leds) {
		ztest_test_skip();
	}

	for (led = 0; led < num_leds; led++) {
		uint16_t level;

		for (level = 0; level <= BRIGHTNESS_MAX; level++) {
			ret = led_set_brightness(led_ctrl, led, level);
			zassert_equal(ret, 0,
				      "LED %d - failed to set brightness to %d",
				      led, level);
		}
		for (level = BRIGHTNESS_MAX + 1; level <= 255; level++) {
			ret = led_set_brightness(led_ctrl, led, level);
			zassert_not_equal(ret, 0,
					  "LED %d - setting brightness to %d"
					  " should fail (maximum: %d)",
					  led, level, BRIGHTNESS_MAX);
		}
	}
}
