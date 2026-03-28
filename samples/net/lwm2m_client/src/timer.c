/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME app_timer
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "modules.h"

#include <zephyr/net/lwm2m.h>

#define TIMER_NAME "Test timer"

/* An example data validation callback. */
static int timer_on_off_validate_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				    uint8_t *data, uint16_t data_len, bool last_block,
				    size_t total_size, size_t offset)
{
	LOG_INF("Validating On/Off data");

	if (data_len != 1) {
		return -EINVAL;
	}

	if (*data > 1) {
		return -EINVAL;
	}

	return 0;
}

static int timer_digital_state_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				  uint8_t *data, uint16_t data_len, bool last_block,
				  size_t total_size, size_t offset)
{
	bool *digital_state = (bool *)data;

	if (*digital_state) {
		LOG_INF("TIMER: ON");
	} else {
		LOG_INF("TIMER: OFF");
	}

	return 0;
}

void init_timer_object(void)
{
	lwm2m_create_object_inst(&LWM2M_OBJ(3340, 0));
	lwm2m_register_validate_callback(&LWM2M_OBJ(3340, 0, 5850), timer_on_off_validate_cb);
	lwm2m_register_post_write_callback(&LWM2M_OBJ(3340, 0, 5543), timer_digital_state_cb);
	lwm2m_set_res_buf(&LWM2M_OBJ(3340, 0, 5750), TIMER_NAME, sizeof(TIMER_NAME),
			  sizeof(TIMER_NAME), LWM2M_RES_DATA_FLAG_RO);
}
