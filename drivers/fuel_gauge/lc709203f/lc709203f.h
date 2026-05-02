/*
 * Copyright (c) 2025 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LC709203F_LC709203F_H_
#define ZEPHYR_DRIVERS_SENSOR_LC709203F_LC709203F_H_

#include <zephyr/drivers/i2c.h>

enum lc709203f_regs {
	LC709203F_REG_BEFORE_RSOC = 0x04,       /* Initialize before RSOC */
	LC709203F_REG_THERMISTOR_B = 0x06,      /* Read/write thermistor B */
	LC709203F_REG_INITIAL_RSOC = 0x07,      /* Initialize RSOC calculation */
	LC709203F_REG_CELL_TEMPERATURE = 0x08,  /* Read/write cell temperature */
	LC709203F_REG_CELL_VOLTAGE = 0x09,      /* Read batt voltage */
	LC709203F_REG_CURRENT_DIRECTION = 0x0A, /* Read/write current direction */
	LC709203F_REG_APA = 0x0B,               /* Adjustment Pack Application */
	LC709203F_REG_APT = 0x0C,               /* Read/write Adjustment Pack Thermistor */
	LC709203F_REG_RSOC = 0x0D,              /* Read state of charge; 1% 0−100 scale */
	LC709203F_REG_CELL_ITE = 0x0F,          /* Read batt indicator to empty */
	LC709203F_REG_IC_VERSION = 0x11,        /* Read IC version */
	LC709203F_REG_BAT_PROFILE = 0x12,       /* Set the battery profile */
	LC709203F_REG_ALARM_LOW_RSOC = 0x13,    /* Alarm on percent threshold */
	LC709203F_REG_ALARM_LOW_VOLTAGE = 0x14, /* Alarm on voltage threshold */
	LC709203F_REG_IC_POWER_MODE = 0x15,     /* Sets sleep/power mode */
	LC709203F_REG_STATUS_BIT = 0x16,        /* Temperature obtaining method */
	LC709203F_REG_NUM_PARAMETER = 0x1A      /* Batt profile code */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_LC709203F_LC709203F_H_ */
