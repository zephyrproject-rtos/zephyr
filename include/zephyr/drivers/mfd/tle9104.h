/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_TLE9104_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_TLE9104_H_

#include <stdbool.h>
#include <zephyr/device.h>

#define TLE9104_GPIO_COUNT 4

enum tle9104_on_state_diagnostics {
	/* overtemperature */
	TLE9104_ONDIAG_OT = 5,
	/* overcurrent timeout */
	TLE9104_ONDIAG_OCTIME = 4,
	/* overtemperature during overcurrent */
	TLE9104_ONDIAG_OCOT = 3,
	/* short to battery */
	TLE9104_ONDIAG_SCB = 2,
	/* no failure */
	TLE9104_ONDIAG_NOFAIL = 1,
	/* no diagnosis done */
	TLE9104_ONDIAG_UNKNOWN = 0,
};

enum tle9104_off_state_diagnostics {
	/* short to ground */
	TLE9104_OFFDIAG_SCG = 3,
	/* open load */
	TLE9104_OFFDIAG_OL = 2,
	/* no failure */
	TLE9104_OFFDIAG_NOFAIL = 1,
	/* no diagnosis done */
	TLE9104_OFFDIAG_UNKNOWN = 0,
};

struct gpio_tle9104_channel_diagnostics {
	enum tle9104_on_state_diagnostics on: 3;
	enum tle9104_off_state_diagnostics off: 2;
};

/**
 * @brief get the diagnostics of the outputs
 *
 * @param dev instance of TLE9104
 * @param diag destination where the result is written to
 *
 * @retval 0 If successful.
 */
int tle9104_get_diagnostics(const struct device *dev,
			    struct gpio_tle9104_channel_diagnostics diag[TLE9104_GPIO_COUNT]);
/**
 * @brief clear the diagnostics of the outputs
 *
 * @param dev instance of TLE9104
 *
 * @retval 0 If successful.
 */
int tle9104_clear_diagnostics(const struct device *dev);
/*!
 * @brief write output state
 *
 * @param dev instance of TLE9104
 * @param state output state, each bit represents on output
 *
 * @retval 0 If successful.
 */
int tle9104_write_state(const struct device *dev, uint8_t state);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_TLE9104_H_ */
