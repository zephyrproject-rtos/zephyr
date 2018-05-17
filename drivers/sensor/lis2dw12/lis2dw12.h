/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dw12.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_H_

#include <spi.h>
#include <gpio.h>
#include <misc/util.h>
#include <sensor.h>

/* COMMON DEFINE FOR ACCEL SENSOR */
#define LIS2DW12_EN_BIT		0x01
#define LIS2DW12_DIS_BIT		0x00
#define LIS2DW12_OUT_LEN		6

/* temperature sensor */
#define LIS2DW12_OUT_TEMP_L_ADDR	0x0d

/* Who Am I  */
#define LIS2DW12_WHO_AM_I_REG		0x0f
#define LIS2DW12_WHO_AM_I		0x44

#define LIS2DW12_CTRL1_ADDR		0x20
#define LIS2DW12_LOW_POWER_MASK	0x03
#define LIS2DW12_POWER_MODE_MASK	0x0c
#define LIS2DW12_LOW_POWER_MODE	0x00
#define LIS2DW12_HP_MODE		0x04
#define LIS2DW12_ODR_MASK		0xf0

enum lis2dh_powermode {
	LIS2DW12_LOW_POWER_M1,
	LIS2DW12_LOW_POWER_M2,
	LIS2DW12_LOW_POWER_M3,
	LIS2DW12_LOW_POWER_M4,
	LIS2DW12_HIGH_PERF
};

/* Acc data rate for Low Power mode */
#define LIS2DW12_MAX_ODR		1600

enum lis2dh_odr {
	LIS2DW12_ODR_POWER_OFF_VAL,
	LIS2DW12_ODR_1_6HZ_VAL,
	LIS2DW12_ODR_12_5HZ_VAL,
	LIS2DW12_ODR_25HZ_VAL,
	LIS2DW12_ODR_50HZ_VAL,
	LIS2DW12_ODR_100HZ_VAL,
	LIS2DW12_ODR_200HZ_VAL,
	LIS2DW12_ODR_400HZ_VAL,
	LIS2DW12_ODR_800HZ_VAL,
	LIS2DW12_ODR_1600HZ_VAL
};

#if defined(CONFIG_LIS2DW12_ODR_1_6)
	#define LIS2DW12_DEFAULT_ODR	LIS2DW12_ODR_1_6HZ_VAL
#elif defined(CONFIG_LIS2DW12_ODR_12_5)
	#define LIS2DW12_DEFAULT_ODR	LIS2DW12_ODR_12_5HZ_VAL
#elif defined(CONFIG_LIS2DW12_ODR_25)
	#define LIS2DW12_DEFAULT_ODR	LIS2DW12_ODR_25HZ_VAL
#elif defined(CONFIG_LIS2DW12_ODR_50)
	#define LIS2DW12_DEFAULT_ODR	LIS2DW12_ODR_50HZ_VAL
#elif defined(CONFIG_LIS2DW12_ODR_100) || \
	defined(CONFIG_LIS2DW12_ODR_RUNTIME)
	#define LIS2DW12_DEFAULT_ODR	LIS2DW12_ODR_100HZ_VAL
#elif defined(CONFIG_LIS2DW12_ODR_200)
	#define LIS2DW12_DEFAULT_ODR	LIS2DW12_ODR_200HZ_VAL
#elif defined(CONFIG_LIS2DW12_ODR_400)
	#define LIS2DW12_DEFAULT_ODR	LIS2DW12_ODR_400HZ_VAL
#elif defined(CONFIG_LIS2DW12_ODR_800)
	#define LIS2DW12_DEFAULT_ODR	LIS2DW12_ODR_800HZ_VAL
#elif defined(CONFIG_LIS2DW12_ODR_1600)
	#define LIS2DW12_DEFAULT_ODR	LIS2DW12_ODR_1600HZ_VAL
#endif

/* Return ODR reg value based on data rate set */
#define LIS2DW12_ODR_TO_REG(_odr) \
	((_odr <= 1) ? LIS2DW12_ODR_1_6HZ_VAL : \
	 (_odr <= 12) ? LIS2DW12_ODR_12_5HZ_VAL : \
	 ((31 - __builtin_clz(_odr / 25))) + 3)

#define LIS2DW12_CTRL2_ADDR		0x21
#define LIS2DW12_BDU_MASK		BIT(3)
#define LIS2DW12_RESET_MASK		BIT(6)
#define LIS2DW12_BOOT_MASK		BIT(7)
#define LIS2DW12_CTRL3_ADDR		0x22
#define LIS2DW12_LIR_MASK		BIT(4)

#define LIS2DW12_CTRL4_ADDR		0x23
#define LIS2DW12_INT1_DRDY		BIT(0)

#define LIS2DW12_CTRL5_ADDR		0x24
#define LIS2DW12_INT2_DRDY		BIT(0)

#define LIS2DW12_CTRL6_ADDR		0x25
#define LIS2DW12_FS_MASK		0x30

enum lis2dh_fs {
	LIS2DW12_FS_2G_VAL,
	LIS2DW12_FS_4G_VAL,
	LIS2DW12_FS_8G_VAL,
	LIS2DW12_FS_16G_VAL
};

/* FS reg value from Full Scale */
#define LIS2DW12_FS_TO_REG(_fs)	(30 - __builtin_clz(_fs))

#if defined(CONFIG_LIS2DW12_ACCEL_RANGE_RUNTIME) || \
	defined(CONFIG_LIS2DW12_ACCEL_RANGE_2G)
	#define LIS2DW12_ACC_FS		LIS2DW12_FS_2G_VAL
#elif defined(CONFIG_LIS2DW12_ACCEL_RANGE_4G)
	#define LIS2DW12_ACC_FS		LIS2DW12_FS_4G_VAL
#elif defined(CONFIG_LIS2DW12_ACCEL_RANGE_8G)
	#define LIS2DW12_ACC_FS		LIS2DW12_FS_8G_VAL
#elif defined(CONFIG_LIS2DW12_ACCEL_RANGE_16G)
	#define LIS2DW12_ACC_FS		LIS2DW12_FS_16G_VAL
#endif

#define LIS2DW12_OUT_T_REG		0x26

#define LIS2DW12_STATUS_REG		0x27
#define LIS2DW12_STS_XLDA_UP		0x01

#define LIS2DW12_OUT_X_L_ADDR		0x28

/* Acc Gain value in ug/LSB in High Perf mode */
#define LIS2DW12_FS_2G_GAIN		244
#define LIS2DW12_FS_4G_GAIN		488
#define LIS2DW12_FS_8G_GAIN		976
#define LIS2DW12_FS_16G_GAIN		1952

#define LIS2DW12_SHFT_GAIN_NOLP1	2
#define LIS2DW12_ACCEL_GAIN_DEFAULT_VAL LIS2DW12_FS_2G_GAIN
#define LIS2DW12_FS_TO_GAIN(_fs, _lp1) \
		(LIS2DW12_FS_2G_GAIN << ((_fs) + (_lp1)))

/* shift value for power mode */
#define LIS2DW12_SHIFT_PM1		4
#define LIS2DW12_SHIFT_PMOTHER		2

/**
 * struct lis2dw12_device_config - lis2dw12 hw configuration
 * @bus_name: Pointer to bus master identifier.
 * @pm: Power mode (lis2dh_powermode).
 * @int_gpio_port: Pointer to GPIO PORT identifier.
 * @int_gpio_pin: GPIO pin number connecter to sensor int pin.
 * @int_pin: Sensor int pin (int1/int2).
 */
struct lis2dw12_device_config {
	const char *bus_name;
	enum lis2dh_powermode pm;
#ifdef CONFIG_LIS2DW12_TRIGGER
	const char *int_gpio_port;
	u8_t int_gpio_pin;
	u8_t int_pin;
#endif /* CONFIG_LIS2DW12_TRIGGER */
};

/* sensor data forward declaration (member definition is below) */
struct lis2dw12_data;

/* transmission function interface */
struct lis2dw12_tf {
	int (*read_data)(struct lis2dw12_data *data, u8_t reg_addr,
			 u8_t *value, u8_t len);
	int (*write_data)(struct lis2dw12_data *data, u8_t reg_addr,
			 u8_t *value, u8_t len);
	int (*read_reg)(struct lis2dw12_data *data, u8_t reg_addr,
			u8_t *value);
	int (*write_reg)(struct lis2dw12_data *data, u8_t reg_addr,
			 u8_t value);
	int (*update_reg)(struct lis2dw12_data *data, u8_t reg_addr,
			  u8_t mask, u8_t value);
};

/* sensor data */
struct lis2dw12_data {
	struct device *bus;
	s16_t acc[3];

	 /* save sensitivity */
	u16_t gain;

	const struct lis2dw12_tf *hw_tf;
#ifdef CONFIG_LIS2DW12_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy;

#if defined(CONFIG_LIS2DW12_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_LIS2DW12_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif /* CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_LIS2DW12_TRIGGER */
#if defined(DT_ST_LIS2DW12_0_CS_GPIO_CONTROLLER)
	struct spi_cs_control cs_ctrl;
#endif
};

int lis2dw12_i2c_init(struct device *dev);
int lis2dw12_spi_init(struct device *dev);

#ifdef CONFIG_LIS2DW12_TRIGGER
int lis2dw12_init_interrupt(struct device *dev);
int lis2dw12_trigger_set(struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_LIS2DW12_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_H_ */
