/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_DS3231_DS3231_H_
#define ZEPHYR_DRIVERS_SENSOR_DS3231_DS3231_H_

#define DT_DRV_COMPAT maxim_ds3231

/* Temperature registers */
#define DS3231_REG_TEMP_MSB 0x11
#define DS3231_REG_TEMP_LSB 0x12

/* Temperature bitmasks */
#define DS3231_BITS_TEMP_LSB GENMASK(7, 6) /* fractional portion */

#endif
