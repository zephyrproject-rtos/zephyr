/* Bosch BMG160 gyro driver */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMG160_BMG160_H_
#define ZEPHYR_DRIVERS_SENSOR_BMG160_BMG160_H_

#include <i2c.h>
#include <gpio.h>
#include <misc/util.h>

/* registers */
#define BMG160_REG_CHIPID		0x00
#define BMG160_REG_RATE_X		0x02
#define BMG160_REG_RATE_Y		0x04
#define BMG160_REG_RATE_Z		0x06
#define BMG160_REG_TEMP			0x08
#define BMG160_REG_INT_STATUS0		0x09
#define BMG160_REG_INT_STATUS1		0x0A
#define BMG160_REG_INT_STATUS2		0x0B
#define BMG160_REG_INT_STATUS3		0x0C
#define BMG160_REG_FIFO_STATUS		0x0E
#define BMG160_REG_RANGE		0x0F
#define BMG160_REG_BW			0x10
#define BMG160_REG_LPM1			0x11
#define BMG160_REG_LPM2			0x12
#define BMG160_REG_RATE_HBW		0x13
#define BMG160_REG_BGW_SOFTRESET	0x14
#define BMG160_REG_INT_EN0		0x15
#define BMG160_REG_INT_EN1		0x16
#define BMG160_REG_INT_MAP0		0x17
#define BMG160_REG_INT_MAP1		0x18
#define BMG160_REG_INT_MAP2		0x19
#define BMG160_REG_FILTER		0x1A
#define BMG160_REG_THRES		0x1B
#define BMG160_REG_ANY_EN		0x1C
#define BMG160_REG_FIFO_WM		0x1E
#define BMG160_REG_INT_RST_LATCH	0x21
#define BMG160_REG_HIGH_TH_X		0x22
#define BMG160_REG_HIGH_DUR_X		0x23
#define BMG160_REG_HIGH_TH_Y		0x24
#define BMG160_REG_HIGH_DUR_Y		0x25
#define BMG160_REG_HIGH_TH_Z		0x26
#define BMG160_REG_HIGH_DUR_Z		0x27
#define BMG160_REG_SOC			0x31
#define BMG160_REG_A_FOC		0x32
#define BMG160_REG_TRIM_NVM_CTRL	0x33
#define BMG160_REG_BGW_SPI3_WDT		0x34
#define BMG160_REG_OFC1			0x36
#define BMG160_REG_OFC2			0x37
#define BMG160_REG_OFC3			0x38
#define BMG160_REG_OFC4			0x39
#define BMG160_REG_TRIM_GP0		0x3A
#define BMG160_REG_TRIM_GP1		0x3B
#define BMG160_REG_TRIM_BIST		0x3C
#define BMG160_REG_TRIM_FIFO_CONFIG0	0x3D
#define BMG160_REG_TRIM_FIFO_CONFIG1	0x3E
#define BMG160_REG_TRIM_FIFO_DATA	0x3F

/* bitfields */

/* BMG160_REG_INT_STATUS0 */
#define BMG160_HIGH_INT			BIT(1)
#define BMG160_ANY_INT			BIT(2)

/* BMG160_REG_INT_STATUS1 */
#define BMG160_FIFO_INT			BIT(4)
#define BMG160_FAST_OFFSET_INT		BIT(5)
#define BMG160_AUTO_OFFSET_INT		BIT(6)
#define BMG160_DATA_INT			BIT(7)

/* BMG160_REG_INT_STATUS2 */
#define BMG160_ANY_FIRST_X		BIT(0)
#define BMG160_ANY_FIRST_Y		BIT(1)
#define BMG160_ANY_FIRST_Z		BIT(2)
#define BMG160_ANY_SIGN			BIT(3)

/* BMG160_REG_INT_STATUS3 */
#define BMG160_HIGH_FIRST_X		BIT(0)
#define BMG160_HIGH_FIRST_Y		BIT(1)
#define BMG160_HIGH_FIRST_Z		BIT(2)
#define BMG160_HIGH_SIGN		BIT(3)

/* BMG160_REG_FIFO_STATUS */
#define BMG160_FIFO_FRAME_COUNTER_MASK	0x7F
#define BMG160_FIFO_OVERRUN		BIT(7)

/* BMG160_REG_INT_EN_0 */
#define BMG160_AUTO_OFFSET_EN		BIT(2)
#define BMG160_FIFO_EN			BIT(6)
#define BMG160_DATA_EN			BIT(7)

/* BMG160_REG_INT_EN_1 */
#define BMG160_INT1_LVL			BIT(0)
#define BMG160_INT1_OD			BIT(1)
#define BMG160_INT2_LVL			BIT(2)
#define BMG160_INT2_OD			BIT(3)

/* BMG160_REG_INT_MAP0 */
#define BMG160_INT1_ANY			BIT(1)
#define BMG160_INT1_HIGH		BIT(3)

/* BMG160_REG_INT_MAP1 */
#define BMG160_INT1_DATA		BIT(0)
#define BMG160_INT1_FAST_OFFSET		BIT(1)
#define BMG160_INT1_FIFO		BIT(2)
#define BMG160_INT1_AUTO_OFFSET		BIT(3)
#define BMG160_INT2_AUTO_OFFSET		BIT(4)
#define BMG160_INT2_FIFO		BIT(5)
#define BMG160_INT2_FAST_OFFSET		BIT(6)
#define BMG160_INT2_DATA		BIT(7)

/* BMG160_REG_ANY_EN */
#define BMG160_AWAKE_DUR_POS		6
#define BMG160_AWAKE_DUR_MASK		(0x3 << 6)
#define BMG160_ANY_DURSAMPLE_POS	4
#define BMG160_ANY_DURSAMPLE_MASK	(0x3 << 4)
#define BMG160_ANY_EN_Z			BIT(2)
#define BMG160_ANY_EN_Y			BIT(1)
#define BMG160_ANY_EN_X			BIT(0)
#define BMG160_ANY_EN_MASK		0x7

/* BMG160_REG_INT_RST_LATCH */
#define BMG160_RESET_INT		BIT(7)
#define BMG160_OFFSET_RESET		BIT(6)
#define BMG160_LATCH_STATUS_BIT		BIT(4)
#define BMG160_LATCH_INT_MASK		0x0F

/* BMG160_REG_THRES */
#define BMG160_THRES_MASK		0x7F

/* other */
#define BMG160_CHIP_ID			0x0F
#define BMG160_RESET			0xB6

#define BMG160_RANGE_TO_SCALE(range_dps) \
		((2 * range_dps * SENSOR_PI) / 180LL / 65536LL)
#define BMG160_SCALE_TO_RANGE(scale) \
		(((scale * 90LL * 65536LL) + SENSOR_PI / 2) / SENSOR_PI)

/* default settings, based on menuconfig options */
#if defined(CONFIG_BMG160_RANGE_RUNTIME) ||\
		defined(CONFIG_BMG160_RANGE_2000DPS)
#	define BMG160_DEFAULT_RANGE		0
#elif defined(CONFIG_BMG160_RANGE_1000DPS)
#	define BMG160_DEFAULT_RANGE		1
#elif defined(CONFIG_BMG160_RANGE_500DPS)
#	define BMG160_DEFAULT_RANGE		2
#elif defined(CONFIG_BMG160_RANGE_250DPS)
#	define BMG160_DEFAULT_RANGE		3
#else
#	define BMG160_DEFAULT_RANGE		4
#endif

#if defined(CONFIG_BMG160_ODR_RUNTIME) ||\
		defined(CONFIG_BMG160_ODR_100)
#	define BMG160_DEFAULT_ODR		5
#elif defined(CONFIG_BMG160_ODR_200)
#	define BMG160_DEFAULT_ODR		4
#elif defined(CONFIG_BMG160_ODR_400)
#	define BMG160_DEFAULT_ODR		3
#elif defined(CONFIG_BMG160_ODR_1000)
#	define BMG160_DEFAULT_ODR		2
#else
#	define BMG160_DEFAULT_ODR		1
#endif

#if defined(CONFIG_BMG160_I2C_SPEED_STANDARD)
#define BMG160_BUS_SPEED	I2C_SPEED_STANDARD
#elif defined(CONFIG_BMG160_I2C_SPEED_FAST)
#define BMG160_BUS_SPEED	I2C_SPEED_FAST
#endif

/* end of default settigns */

struct bmg160_device_config {
	const char *i2c_port;
#ifdef CONFIG_BMG160_TRIGGER
	const char *gpio_port;
#endif
	u16_t i2c_addr;
	u8_t i2c_speed;
#ifdef CONFIG_BMG160_TRIGGER
	u8_t int_pin;
#endif
};

struct bmg160_device_data {
	struct device *i2c;
#ifdef CONFIG_BMG160_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;
#endif
#ifdef CONFIG_BMG160_TRIGGER_OWN_THREAD
	struct k_sem trig_sem;
#endif
	struct k_sem sem;
#ifdef CONFIG_BMG160_TRIGGER_GLOBAL_THREAD
	struct k_work work;
	struct device *dev;
#endif
#ifdef CONFIG_BMG160_TRIGGER
	sensor_trigger_handler_t anymotion_handler;
	sensor_trigger_handler_t drdy_handler;
#endif
	s16_t raw_gyro_xyz[3];
	u16_t scale;
	u8_t range_idx;

	s8_t raw_temp;
};

int bmg160_trigger_init(struct device *dev);
int bmg160_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
int bmg160_read(struct device *dev, u8_t reg_addr, u8_t *data,
		u8_t len);
int bmg160_read_byte(struct device *dev, u8_t reg_addr, u8_t *byte);
int bmg160_update_byte(struct device *dev, u8_t reg_addr, u8_t mask,
		       u8_t value);
int bmg160_write_byte(struct device *dev, u8_t reg_addr, u8_t data);
int bmg160_slope_config(struct device *dev, enum sensor_attribute attr,
			const struct sensor_value *val);

#define SYS_LOG_DOMAIN "BMG160"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* ZEPHYR_DRIVERS_SENSOR_BMG160_BMG160_H_ */
