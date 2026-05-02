/*
 * Copyright 2021 Google LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_SYSCON_SYSCON_COMMON_H_
#define DRIVERS_SYSCON_SYSCON_COMMON_H_

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate register address alignment and bounds
 *
 * @param reg The register offset to validate.
 * @param reg_size The size of the syscon register region.
 * @param reg_width The width of a single register (in bytes).
 *
 * @retval 0 if the register address is valid.
 * @retval -EINVAL if the address is misaligned or out of bounds.
 */
static inline int syscon_sanitize_reg(uint16_t reg, size_t reg_size, uint8_t reg_width)
{
	if (reg % reg_width != 0) {
		return -EINVAL;
	}

	if (reg >= reg_size) {
		return -EINVAL;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_SYSCON_SYSCON_COMMON_H_ */
