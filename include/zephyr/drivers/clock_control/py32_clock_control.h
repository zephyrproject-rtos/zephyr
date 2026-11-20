/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal clock control definitions for Puya PY32 devices.
 * @ingroup clock_control_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_PY32_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_PY32_H_

/** @cond INTERNAL_HIDDEN */

#include <zephyr/types.h>

/* RCC clock-enable register offset and peripheral enable mask. */
struct py32_pclken {
	uint32_t bus;
	uint32_t enr;
};

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_PY32_H_ */
