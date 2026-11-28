/*
 * Copyright (c) 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/i3c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "bmi323_i3c.h"

LOG_MODULE_DECLARE(bosch_bmi323, CONFIG_SENSOR_LOG_LEVEL);

static int bosch_bmi323_i3c_read_words(const void *context, uint8_t offset, uint16_t *words,
				       uint16_t words_count)
{
	struct bosch_bmi323_i3c *i3c = (struct bosch_bmi323_i3c *)context;
	uint8_t dbuf[(words_count * 2) + IMU_BOSCH_BMI323_I3C_DUMMY_OFFSET];
	int ret;

	if (i3c->desc == NULL) {
		return -ENODEV;
	}

	ret = i3c_burst_read(i3c->desc, offset, dbuf, sizeof(dbuf));
	if (ret == 0) {
		memcpy(words, &dbuf[IMU_BOSCH_BMI323_I3C_DUMMY_OFFSET], (words_count * 2));
	}
	k_busy_wait(2);

	return ret;
}

static int bosch_bmi323_i3c_write_words(const void *context, uint8_t offset, uint16_t *words,
					uint16_t words_count)
{
	struct bosch_bmi323_i3c *i3c = (struct bosch_bmi323_i3c *)context;
	uint8_t dbuf[(words_count * 2) + sizeof(offset)];
	int ret;

	if (i3c->desc == NULL) {
		return -ENODEV;
	}

	dbuf[0] = offset;
	memcpy(&dbuf[sizeof(offset)], words, (words_count * 2));

	ret = i3c_write(i3c->desc, dbuf, sizeof(dbuf));
	k_busy_wait(2);

	return ret;
}

static int bosch_bmi323_i3c_init(const void *context)
{
	struct bosch_bmi323_i3c *i3c = (struct bosch_bmi323_i3c *)context;
	uint8_t sensor_id_raw[2 + IMU_BOSCH_BMI323_I3C_DUMMY_OFFSET];
	uint16_t sensor_id;
	int ret;

	if (device_is_ready(i3c->bus) == false) {
		return -ENODEV;
	}

	if (i3c->desc == NULL) {
		i3c->desc = i3c_device_find(i3c->bus, &i3c->id);
		if (i3c->desc == NULL) {
			return -ENODEV;
		}
	}

	ret = i3c_burst_read(i3c->desc, IMU_BOSCH_BMI323_REG_CHIP_ID, sensor_id_raw,
			     sizeof(sensor_id_raw));
	if (ret < 0) {
		return ret;
	}

	sensor_id = sys_get_le16(&sensor_id_raw[IMU_BOSCH_BMI323_I3C_DUMMY_OFFSET]);
	if ((sensor_id & 0xFF) != 0x43) {
		LOG_ERR("Unexpected CHIP_ID 0x%04x", sensor_id);
		return -ENODEV;
	}

	k_busy_wait(2);

	return 0;
}

static int bosch_bmi323_i3c_restore_da(const void *context)
{
	struct bosch_bmi323_i3c *i3c = (struct bosch_bmi323_i3c *)context;
	struct i3c_device_desc *desc = i3c->desc;
	struct i3c_config_controller ctrl_cfg;
	uint32_t prev_od_high_ns;
	uint32_t prev_tcas_ps;
	uint8_t da;
	int ret;
	int cret;

	if (desc == NULL) {
		return -ENODEV;
	}

	da = desc->init_dynamic_addr;
	if (da == 0U) {
		da = desc->dynamic_addr;
	}
	if (da == 0U) {
		da = desc->static_addr;
	}
	if (da == 0U) {
		return -EINVAL;
	}

	ret = i3c_config_get_controller(desc->bus, &ctrl_cfg);
	if (ret != 0) {
		return ret;
	}

	/*
	 * After soft-reset the target is back in I2C with the spike filter
	 * on. Same first-broadcast OD thigh and tCAS as i3c_bus_init()
	 * (MIPI 1.2 §4.3.2.2.2 / Table 49).
	 */
	prev_od_high_ns = ctrl_cfg.scl_od_min.high_ns;
	ctrl_cfg.scl_od_min.high_ns = MAX(I3C_OD_FIRST_BC_THIGH_MIN_NS,
					  ctrl_cfg.scl_od_min.high_ns);
	prev_tcas_ps = ctrl_cfg.tcas_ps;
	ctrl_cfg.tcas_ps = MAX(I3C_BUS_TCAS_FIRST_BC_MIN_PS, ctrl_cfg.tcas_ps);
	ret = i3c_configure_controller(desc->bus, &ctrl_cfg);
	if (ret != 0) {
		LOG_ERR("Open Drain Slow speed set failed");
		return ret;
	}

	ret = i3c_detach_i3c_device(desc);
	if ((ret != 0) && (ret != -EALREADY)) {
		goto restore_od;
	}
	desc->dynamic_addr = 0;

	ret = i3c_bus_setdasa(desc, da);
	if (ret != 0) {
		LOG_ERR("SETDASA to 0x%02x failed (%d)", da, ret);
	} else {
		LOG_DBG("Restored I3C DA 0x%02x", desc->dynamic_addr);
	}

restore_od:
	ctrl_cfg.scl_od_min.high_ns = prev_od_high_ns;
	ctrl_cfg.tcas_ps = prev_tcas_ps;
	cret = i3c_configure_controller(desc->bus, &ctrl_cfg);
	if (cret != 0) {
		LOG_ERR("Open Drain Normal speed set failed");
		if (ret == 0) {
			ret = cret;
		}
	}

	return ret;
}

const struct bosch_bmi323_bus_api bosch_bmi323_i3c_bus_api = {
	.read_words = bosch_bmi323_i3c_read_words,
	.write_words = bosch_bmi323_i3c_write_words,
	.init = bosch_bmi323_i3c_init,
	.restore_da = bosch_bmi323_i3c_restore_da,
};
