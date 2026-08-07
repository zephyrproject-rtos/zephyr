/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME app_led

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "modules.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/net/lwm2m.h>

#define LIGHT_NAME "Test light"

/* If led0 gpios doesn't exist the relevant IPSO object will simply not be created. */
static const struct gpio_dt_spec led_gpio = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {});
static uint32_t led_state;

/* TODO: Move to a pre write hook that can handle ret codes once available */
static int led_on_off_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			 uint16_t data_len, bool last_block, size_t total_size, size_t offset)
{
	int ret = 0;
	uint32_t led_val;

	led_val = *(uint8_t *)data;
	if (led_val != led_state) {
		ret = gpio_pin_set_dt(&led_gpio, (int)led_val);
		if (ret) {
			/*
			 * We need an extra hook in LWM2M to better handle
			 * failures before writing the data value and not in
			 * post_write_cb, as there is not much that can be
			 * done here.
			 */
			LOG_ERR("Fail to write to GPIO %d", led_gpio.pin);
			return ret;
		}

		led_state = led_val;
		/* TODO: Move to be set by an internal post write function */
		lwm2m_set_s32(&LWM2M_OBJ(3311, 0, 5852), 0);
	}

	return ret;
}

int init_led_device(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	lwm2m_create_object_inst(&LWM2M_OBJ(3311, 0));
	lwm2m_register_post_write_callback(&LWM2M_OBJ(3311, 0, 5850), led_on_off_cb);
	lwm2m_set_res_buf(&LWM2M_OBJ(3311, 0, 5750), LIGHT_NAME, sizeof(LIGHT_NAME),
			  sizeof(LIGHT_NAME), LWM2M_RES_DATA_FLAG_RO);

	return 0;
}
