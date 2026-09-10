/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CHARGER_MAX20356_H_
#define ZEPHYR_INCLUDE_DRIVERS_CHARGER_MAX20356_H_

#include <zephyr/drivers/charger.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file max20356.h
 * @brief ADI MAX20356 PMIC charger driver custom properties
 * @defgroup charger_interface_max20356 MAX20356 Charger interface
 * @ingroup charger_interface_ext
 * @{
 */

/** MAX20356 charger custom properties. Extends enum charger_property. */
enum max20356_charger_prop {
	/**
	 * CC2 constant-charge (fast-charge) current in µA (ChgCur1/CC2IFChg).
	 * Shares the CC1 current encoding and range, 4000..500000 µA.
	 * Value is carried in charger_propval.custom_uint.
	 */
	MAX20356_CHARGER_PROP_CC2_CURRENT_UA = CHARGER_PROP_CUSTOM_BEGIN,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CHARGER_MAX20356_H_ */
