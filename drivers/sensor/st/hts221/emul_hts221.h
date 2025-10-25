// SPDX-License-Identifier: Apache-2.0
// Copyright The Zephyr Project Contributors

#ifndef ZEPHYR_DRIVERS_SENSOR_HTS221_EMU_HTS221_H_
#define ZEPHYR_DRIVERS_SENSOR_HTS221_EMUL_HTS221_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include "hts221.h"

#define HTS221_REG_COUNT 1024

struct hts221_emul_data {
	uint32_t cur_reg;
	uint32_t reg[HTS221_REG_COUNT];
	uint8_t count;
};

struct hts221_emul_cfg {
};

#endif /* ZEPHYR_DRIVERS_SENSOR_HTS221_HTS221_H_ */