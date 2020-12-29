/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BME280_BME280_H_
#define ZEPHYR_DRIVERS_SENSOR_BME280_BME280_H_

#include <zephyr/types.h>
#include <device.h>

#define BME280_REG_PRESS_MSB            0xF7
#define BME280_REG_COMP_START           0x88
#define BME280_REG_HUM_COMP_PART1       0xA1
#define BME280_REG_HUM_COMP_PART2       0xE1
#define BME280_REG_ID                   0xD0
#define BME280_REG_CONFIG               0xF5
#define BME280_REG_CTRL_MEAS            0xF4
#define BME280_REG_CTRL_HUM             0xF2
#define BME280_REG_STATUS               0xF3

#define BMP280_CHIP_ID_SAMPLE_1         0x56
#define BMP280_CHIP_ID_SAMPLE_2         0x57
#define BMP280_CHIP_ID_MP               0x58
#define BME280_CHIP_ID                  0x60
#define BME280_MODE_SLEEP               0x00
#define BME280_MODE_FORCED              0x01
#define BME280_MODE_NORMAL              0x03
#define BME280_SPI_3W_DISABLE           0x00

#if defined CONFIG_BME280_MODE_NORMAL
#define BME280_MODE BME280_MODE_NORMAL
#elif defined CONFIG_BME280_MODE_FORCED
#define BME280_MODE BME280_MODE_FORCED
#endif

#if defined CONFIG_BME280_TEMP_OVER_1X
#define BME280_TEMP_OVER                (1 << 5)
#elif defined CONFIG_BME280_TEMP_OVER_2X
#define BME280_TEMP_OVER                (2 << 5)
#elif defined CONFIG_BME280_TEMP_OVER_4X
#define BME280_TEMP_OVER                (3 << 5)
#elif defined CONFIG_BME280_TEMP_OVER_8X
#define BME280_TEMP_OVER                (4 << 5)
#elif defined CONFIG_BME280_TEMP_OVER_16X
#define BME280_TEMP_OVER                (5 << 5)
#endif

#if defined CONFIG_BME280_PRESS_OVER_1X
#define BME280_PRESS_OVER               (1 << 2)
#elif defined CONFIG_BME280_PRESS_OVER_2X
#define BME280_PRESS_OVER               (2 << 2)
#elif defined CONFIG_BME280_PRESS_OVER_4X
#define BME280_PRESS_OVER               (3 << 2)
#elif defined CONFIG_BME280_PRESS_OVER_8X
#define BME280_PRESS_OVER               (4 << 2)
#elif defined CONFIG_BME280_PRESS_OVER_16X
#define BME280_PRESS_OVER               (5 << 2)
#endif

#if defined CONFIG_BME280_HUMIDITY_OVER_1X
#define BME280_HUMIDITY_OVER            1
#elif defined CONFIG_BME280_HUMIDITY_OVER_2X
#define BME280_HUMIDITY_OVER            2
#elif defined CONFIG_BME280_HUMIDITY_OVER_4X
#define BME280_HUMIDITY_OVER            3
#elif defined CONFIG_BME280_HUMIDITY_OVER_8X
#define BME280_HUMIDITY_OVER            4
#elif defined CONFIG_BME280_HUMIDITY_OVER_16X
#define BME280_HUMIDITY_OVER            5
#endif

#if defined CONFIG_BME280_STANDBY_05MS
#define BME280_STANDBY                  0
#elif defined CONFIG_BME280_STANDBY_62MS
#define BME280_STANDBY                  (1 << 5)
#elif defined CONFIG_BME280_STANDBY_125MS
#define BME280_STANDBY                  (2 << 5)
#elif defined CONFIG_BME280_STANDBY_250MS
#define BME280_STANDBY                  (3 << 5)
#elif defined CONFIG_BME280_STANDBY_500MS
#define BME280_STANDBY                  (4 << 5)
#elif defined CONFIG_BME280_STANDBY_1000MS
#define BME280_STANDBY                  (5 << 5)
#elif defined CONFIG_BME280_STANDBY_2000MS
#define BME280_STANDBY                  (6 << 5)
#elif defined CONFIG_BME280_STANDBY_4000MS
#define BME280_STANDBY                  (7 << 5)
#endif

#if defined CONFIG_BME280_FILTER_OFF
#define BME280_FILTER                   0
#elif defined CONFIG_BME280_FILTER_2
#define BME280_FILTER                   (1 << 2)
#elif defined CONFIG_BME280_FILTER_4
#define BME280_FILTER                   (2 << 2)
#elif defined CONFIG_BME280_FILTER_8
#define BME280_FILTER                   (3 << 2)
#elif defined CONFIG_BME280_FILTER_16
#define BME280_FILTER                   (4 << 2)
#endif

#define BME280_CTRL_MEAS_VAL            (BME280_PRESS_OVER | \
					 BME280_TEMP_OVER |  \
					 BME280_MODE)
#define BME280_CONFIG_VAL               (BME280_STANDBY | \
					 BME280_FILTER |  \
					 BME280_SPI_3W_DISABLE)


#define BME280_CTRL_MEAS_OFF_VAL		(BME280_PRESS_OVER | \
					 BME280_TEMP_OVER |  \
					 BME280_MODE_SLEEP)

#endif /* ZEPHYR_DRIVERS_SENSOR_BME280_BME280_H_ */
