/*
 * Copyright (c) 2026, Iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_DRIVERS_SENSOR_TMP112_EMUL_H_
#define TESTS_DRIVERS_SENSOR_TMP112_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

struct tmp112_emul_data {
	uint16_t regs[4];
	/* Count attempted configuration writes, including injected failures. */
	uint32_t config_write_count;
	uint32_t one_shot_count;
	bool fail_config_write;
};

#endif /* TESTS_DRIVERS_SENSOR_TMP112_EMUL_H_ */
