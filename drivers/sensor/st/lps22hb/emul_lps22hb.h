// SPDX-License-Identifier: Apache-2.0
// Copyright The Zephyr Project Contributors

#ifndef ZEPHYR_DRIVERS_SENSOR_LPS22HB_EMU_LPS22HB_H_
#define ZEPHYR_DRIVERS_SENSOR_LPS22HB_EMUL_LPS22HB_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include "lps22hb.h"

#define LPS22HB_REG_COUNT 1024

struct lps22hb_emul_data {
	uint32_t cur_reg;
	uint32_t reg[LPS22HB_REG_COUNT];
	uint8_t count;

};

struct lps22hb_emul_cfg {
	struct i2c_dt_spec i2c;
};

#endif