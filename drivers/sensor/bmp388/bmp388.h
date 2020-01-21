/*
 * Copyright (c) 2020 Ioannis Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP388_BMP388_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP388_BMP388_H_

#include <zephyr/types.h>
#include <device.h>

#define BMP388_REG_CHIP_ID              0x00
#define BMP388_REG_ERR                  0x02
#define BMP388_REG_STATUS               0x03
#define BMP388_REG_PRESS_DATA_0         0x04
#define BMP388_REG_PRESS_DATA_1         0x05
#define BMP388_REG_PRESS_DATA_2         0x06
#define BMP388_REG_TEMP_DATA_0          0x07
#define BMP388_REG_TEMP_DATA_1          0x08
#define BMP388_REG_TEMP_DATA_2          0x09
#define BMP388_REG_SENSOR_TIME_0        0x0C
#define BMP388_REG_SENSOR_TIME_1        0x0D
#define BMP388_REG_SENSOR_TIME_2        0x0E
#define BMP388_REG_EVENT                0x10
#define BMP388_REG_INT_STATUS           0x11
#define BMP388_REG_INT_CTRL             0x19
#define BMP388_REG_PWR_CTRL             0x1B
#define BMP388_REG_OSR                  0x1C
#define BMP388_REG_ODR                  0x1D
#define BMP388_REG_CONFIG               0x1F
#define BMP388_REG_CMD                  0x7E

#define BMP388_NVM_TRIMMING_COEF        0x31

#define BMP388_CHIP_ID                  0x50

#define BMP388_ERR_FATAL                0x01
#define BMP388_ERR_CMD                  0x02
#define BMP388_ERR_CONF                 0x04

#define BMP388_STATUS_CMD_RDY           0x10
#define BMP388_STATUS_DRDY_PRESS        0x20
#define BMP388_STATUS_DRDY_TEMP         0x40

#define BMP388_PWR_SLEEP                0
#define BMP388_PWR_FORCED               (1 << 4)
#define BMP388_PWR_NORMAL               (3 << 4)

#define BMP388_SOFT_RESET               0xB6

#if defined(CONFIG_BMP388_PRESSURE_ENABLE)
#define BMP388_PRESS_EN                 1
#else
#define BMP388_PRESS_EN                 0
#endif /* CONFIG_BMP388_PRESSURE_ENABLE */

#if defined(CONFIG_BMP388_TEMPERATURE_ENABLE)
#define BMP388_TEMP_EN                  (1 << 1)
#else
#define BMP388_TEMP_EN                  0
#endif /* CONFIG_BMP388_TEMPERATURE_ENABLE */

#if defined CONFIG_BMP388_PRESS_OVER_1X
#define BMP388_PRESS_OVER               0
#elif defined CONFIG_BMP388_PRESS_OVER_2X
#define BMP388_PRESS_OVER               1
#elif defined CONFIG_BMP388_PRESS_OVER_4X
#define BMP388_PRESS_OVER               2
#elif defined CONFIG_BMP388_PRESS_OVER_8X
#define BMP388_PRESS_OVER               3
#elif defined CONFIG_BMP388_PRESS_OVER_16X
#define BMP388_PRESS_OVER               4
#elif defined CONFIG_BMP388_PRESS_OVER_32X
#define BMP388_PRESS_OVER               5
#endif /* CONFIG_BMP388_PRESS_OVER_* */

#if defined(CONFIG_BMP388_TEMP_OVER_1X)
#define BMP388_TEMP_OVER                0
#elif defined(CONFIG_BMP388_TEMP_OVER_2X)
#define BMP388_TEMP_OVER                (1 << 3)
#elif defined(CONFIG_BMP388_TEMP_OVER_4X)
#define BMP388_TEMP_OVER                (2 << 3)
#elif defined(CONFIG_BMP388_TEMP_OVER_8X)
#define BMP388_TEMP_OVER                (3 << 3)
#elif defined(CONFIG_BMP388_TEMP_OVER_16X)
#define BMP388_TEMP_OVER                (4 << 3)
#elif defined(CONFIG_BMP388_TEMP_OVER_32X)
#define BMP388_TEMP_OVER                (5 << 3)
#endif /* CONFIG_BMP388_TEMP_OVER_* */

#if defined(CONFIG_BMP388_FILTER_OFF)
#define BMP388_FILTER                   0
#elif defined(CONFIG_BMP388_FILTER_1)
#define BMP388_FILTER                   (1 << 1)
#elif defined(CONFIG_BMP388_FILTER_3)
#define BMP388_FILTER                   (2 << 1)
#elif defined(CONFIG_BMP388_FILTER_7)
#define BMP388_FILTER                   (3 << 1)
#elif defined(CONFIG_BMP388_FILTER_15)
#define BMP388_FILTER                   (4 << 1)
#elif defined(CONFIG_BMP388_FILTER_31)
#define BMP388_FILTER                   (5 << 1)
#elif defined(CONFIG_BMP388_FILTER_63)
#define BMP388_FILTER                   (6 << 1)
#elif defined(CONFIG_BMP388_FILTER_127)
#define BMP388_FILTER                   (7 << 1)
#endif /* CONFIG_BMP388_FILTER_* */

#if defined(CONFIG_BMP388_ODR_200)
#define BMP388_ODR                      0
#elif defined(CONFIG_BMP388_ODR_100)
#define BMP388_ODR                      1
#elif defined(CONFIG_BMP388_ODR_50)
#define BMP388_ODR                      2
#elif defined(CONFIG_BMP388_ODR_25)
#define BMP388_ODR                      3
#elif defined(CONFIG_BMP388_ODR_12p5)
#define BMP388_ODR                      4
#elif defined(CONFIG_BMP388_ODR_6p25)
#define BMP388_ODR                      5
#elif defined(CONFIG_BMP388_ODR_3p1)
#define BMP388_ODR                      6
#elif defined(CONFIG_BMP388_ODR_1p5)
#define BMP388_ODR                      7
#elif defined(CONFIG_BMP388_ODR_0p78)
#define BMP388_ODR                      8
#elif defined(CONFIG_BMP388_ODR_0p39)
#define BMP388_ODR                      9
#elif defined(CONFIG_BMP388_ODR_0p2)
#define BMP388_ODR                      10
#elif defined(CONFIG_BMP388_ODR_0p1)
#define BMP388_ODR                      11
#elif defined(CONFIG_BMP388_ODR_0p05)
#define BMP388_ODR                      12
#elif defined(CONFIG_BMP388_ODR_0p02)
#define BMP388_ODR                      13
#elif defined(CONFIG_BMP388_ODR_0p01)
#define BMP388_ODR                      14
#elif defined(CONFIG_BMP388_ODR_0p006)
#define BMP388_ODR                      15
#elif defined(CONFIG_BMP388_ODR_0p003)
#define BMP388_ODR                      16
#elif defined(CONFIG_BMP388_ODR_0p0015)
#define BMP388_ODR                      17
#endif /* CONFIG_BMP388_ODR_* */

/* Trimming coefficients for output compensation. */
struct bmp388_trimming_coef {
	u16_t par_t1;
	u16_t par_t2;
	s8_t par_t3;
	s16_t par_p1;
	s16_t par_p2;
	s8_t par_p3;
	s8_t par_p4;
	u16_t par_p5;
	u16_t par_p6;
	s8_t par_p7;
	s8_t par_p8;
	s16_t par_p9;
	s8_t par_p10;
	s8_t par_p11;
};

/* Converted coefficients for output compensation. */
struct bmp388_converted_coef {
	float par_t1;
	float par_t2;
	float par_t3;
	float par_p1;
	float par_p2;
	float par_p3;
	float par_p4;
	float par_p5;
	float par_p6;
	float par_p7;
	float par_p8;
	float par_p9;
	float par_p10;
	float par_p11;
};

struct bmp388_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);

#if defined(DT_BOSCH_BMP388_BUS_I2C)
	u16_t i2c_slave_addr;
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
};

struct bmp388_data {
	struct device *bus;

	int (*reg_read)(struct device *dev, u8_t reg_addr, u8_t *value,
			u16_t len);
	int (*reg_write)(struct device *dev, u8_t reg_addr, u8_t value);

	struct bmp388_trimming_coef tr_coef;
	struct bmp388_converted_coef conv_coef;

	/* Compensated values */
	float comp_press;
	float comp_temp;

	/* Chip ID */
	u8_t chip_id;

	/* Measurement time in milliseconds */
	int meas_time;

	/* Measurement time in milliseconds */
	int freq_max_forced_mode;
	int freq_max_normal_mode;
};

int bmp388_i2c_init(struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP388_BMP388_H_ */
