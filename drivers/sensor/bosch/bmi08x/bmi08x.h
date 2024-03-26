/* Bosch BMI08X inertial measurement unit header
 *
 * Copyright (c) 2022 Meta Platforms, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI08X_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI08X_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

/* Accel Chip Id register */
#define BMI08X_REG_ACCEL_CHIP_ID 0x00

/* Accel Error condition register */
#define BMI08X_REG_ACCEL_ERR 0x02

/* Accel Status flag register */
#define BMI08X_REG_ACCEL_STATUS 0x03

/* Accel X LSB data register */
#define BMI08X_REG_ACCEL_X_LSB 0x12

/* Accel X MSB data register */
#define BMI08X_REG_ACCEL_X_MSB 0x13

/* Accel Y LSB data register */
#define BMI08X_REG_ACCEL_Y_LSB 0x14

/* Accel Y MSB data register */
#define BMI08X_REG_ACCEL_Y_MSB 0x15

/* Accel Z LSB data register */
#define BMI08X_REG_ACCEL_Z_LSB 0x16

/* Accel Z MSB data register */
#define BMI08X_REG_ACCEL_Z_MSB 0x17

/* Sensor time byte 0 register */
#define BMI08X_REG_ACCEL_SENSORTIME_0 0x18

/* Sensor time byte 1 register */
#define BMI08X_REG_ACCEL_SENSORTIME_1 0x19

/* Sensor time byte 2 register */
#define BMI08X_REG_ACCEL_SENSORTIME_2 0x1A

/* Accel Interrupt status0 register */
#define BMI08X_REG_ACCEL_INT_STAT_0 0x1C

/* Accel Interrupt status1 register */
#define BMI08X_REG_ACCEL_INT_STAT_1 0x1D

/* Accel general purpose register 0*/
#define BMI08X_REG_ACCEL_GP_0 0x1E

/* Sensor temperature MSB data register */
#define BMI08X_REG_TEMP_MSB 0x22

/* Sensor temperature LSB data register */
#define BMI08X_REG_TEMP_LSB 0x23

/* Accel general purpose register 4*/
#define BMI08X_REG_ACCEL_GP_4 0x27

/* Accel Internal status register */
#define BMI08X_REG_ACCEL_INTERNAL_STAT 0x2A

/* Accel configuration register */
#define BMI08X_REG_ACCEL_CONF 0x40

/* Accel range setting register */
#define BMI08X_REG_ACCEL_RANGE 0x41

/* Accel Interrupt pin 1 configuration register */
#define BMI08X_REG_ACCEL_INT1_IO_CONF 0x53

/* Accel Interrupt pin 2 configuration register */
#define BMI08X_REG_ACCEL_INT2_IO_CONF 0x54

/* Accel Interrupt latch configuration register */
#define BMI08X_REG_ACCEL_INT_LATCH_CONF 0x55

/* Accel Interrupt pin1 mapping register */
#define BMI08X_REG_ACCEL_INT1_MAP 0x56

/* Accel Interrupt pin2 mapping register */
#define BMI08X_REG_ACCEL_INT2_MAP 0x57

/* Accel Interrupt map register */
#define BMI08X_REG_ACCEL_INT1_INT2_MAP_DATA 0x58

/* Accel Init control register */
#define BMI08X_REG_ACCEL_INIT_CTRL 0x59

/* Accel Self test register */
#define BMI08X_REG_ACCEL_SELF_TEST 0x6D

/* Accel Power mode configuration register */
#define BMI08X_REG_ACCEL_PWR_CONF 0x7C

/* Accel Power control (switch on or off  register */
#define BMI08X_REG_ACCEL_PWR_CTRL 0x7D

/* Accel Soft reset register */
#define BMI08X_REG_ACCEL_SOFTRESET 0x7E

/* BMI085 Accel unique chip identifier */
#define BMI085_ACCEL_CHIP_ID 0x1F

/* BMI088 Accel unique chip identifier */
#define BMI088_ACCEL_CHIP_ID 0x1E

/* Feature Config related Registers */
#define BMI08X_ACCEL_RESERVED_5B_REG 0x5B
#define BMI08X_ACCEL_RESERVED_5C_REG 0x5C
#define BMI08X_ACCEL_FEATURE_CFG_REG 0x5E

/* Interrupt masks */
#define BMI08X_ACCEL_DATA_READY_INT 0x80

/* Accel Bandwidth */
#define BMI08X_ACCEL_BW_OSR4   0x00
#define BMI08X_ACCEL_BW_OSR2   0x01
#define BMI08X_ACCEL_BW_NORMAL 0x02

/* BMI085 Accel Range */
#define BMI085_ACCEL_RANGE_2G  0x00
#define BMI085_ACCEL_RANGE_4G  0x01
#define BMI085_ACCEL_RANGE_8G  0x02
#define BMI085_ACCEL_RANGE_16G 0x03

/**\name  BMI088 Accel Range */
#define BMI088_ACCEL_RANGE_3G  0x00
#define BMI088_ACCEL_RANGE_6G  0x01
#define BMI088_ACCEL_RANGE_12G 0x02
#define BMI088_ACCEL_RANGE_24G 0x03

/* Accel Output data rate */
#define BMI08X_ACCEL_ODR_12_5_HZ 0x05
#define BMI08X_ACCEL_ODR_25_HZ	 0x06
#define BMI08X_ACCEL_ODR_50_HZ	 0x07
#define BMI08X_ACCEL_ODR_100_HZ	 0x08
#define BMI08X_ACCEL_ODR_200_HZ	 0x09
#define BMI08X_ACCEL_ODR_400_HZ	 0x0A
#define BMI08X_ACCEL_ODR_800_HZ	 0x0B
#define BMI08X_ACCEL_ODR_1600_HZ 0x0C

/* Accel Init Ctrl */
#define BMI08X_ACCEL_INIT_CTRL_DISABLE 0x00
#define BMI08X_ACCEL_INIT_CTRL_ENABLE  0x01

/* Accel Self test */
#define BMI08X_ACCEL_SWITCH_OFF_SELF_TEST 0x00
#define BMI08X_ACCEL_POSITIVE_SELF_TEST	  0x0D
#define BMI08X_ACCEL_NEGATIVE_SELF_TEST	  0x09

/* Accel Power mode */
#define BMI08X_ACCEL_PM_ACTIVE	0x00
#define BMI08X_ACCEL_PM_SUSPEND 0x03

/* Accel Power control settings */
#define BMI08X_ACCEL_POWER_DISABLE 0x00
#define BMI08X_ACCEL_POWER_ENABLE  0x04

/* Accel internal interrupt pin mapping */
#define BMI08X_ACCEL_INTA_DISABLE 0x00
#define BMI08X_ACCEL_INTA_ENABLE  0x01
#define BMI08X_ACCEL_INTB_DISABLE 0x00
#define BMI08X_ACCEL_INTB_ENABLE  0x02
#define BMI08X_ACCEL_INTC_DISABLE 0x00
#define BMI08X_ACCEL_INTC_ENABLE  0x04

/* Accel Soft reset delay */
#define BMI08X_ACCEL_SOFTRESET_DELAY_MS 1

/* Mask definitions for ACCEL_ERR_REG register */
#define BMI08X_FATAL_ERR_MASK 0x01
#define BMI08X_ERR_CODE_MASK  0x1C

/* Position definitions for ACCEL_ERR_REG register */
#define BMI08X_CMD_ERR_POS  1
#define BMI08X_ERR_CODE_POS 2

/* Mask definition for ACCEL_STATUS_REG register */
#define BMI08X_ACCEL_STATUS_MASK 0x80

/* Position definitions for ACCEL_STATUS_REG  */
#define BMI08X_ACCEL_STATUS_POS 7

/* Mask definitions for odr, bandwidth and range */
#define BMI08X_ACCEL_ODR_MASK	0x0F
#define BMI08X_ACCEL_BW_MASK	0x70
#define BMI08X_ACCEL_RANGE_MASK 0x03

/* Position definitions for odr, bandwidth and range */
#define BMI08X_ACCEL_BW_POS 4

/* Mask definitions for INT1_IO_CONF register */
#define BMI08X_ACCEL_INT_EDGE_MASK 0x01
#define BMI08X_ACCEL_INT_LVL_MASK  0x02
#define BMI08X_ACCEL_INT_OD_MASK   0x04
#define BMI08X_ACCEL_INT_IO_MASK   0x08
#define BMI08X_ACCEL_INT_IN_MASK   0x10

/* Position definitions for INT1_IO_CONF register */
#define BMI08X_ACCEL_INT_EDGE_POS 0
#define BMI08X_ACCEL_INT_LVL_POS  1
#define BMI08X_ACCEL_INT_OD_POS	  2
#define BMI08X_ACCEL_INT_IO_POS	  3
#define BMI08X_ACCEL_INT_IN_POS	  4

/* Mask definitions for INT1/INT2 mapping register */
#define BMI08X_ACCEL_MAP_INTA_MASK 0x01

/* Mask definitions for INT1/INT2 mapping register */
#define BMI08X_ACCEL_MAP_INTA_POS 0x00

/* Mask definitions for INT1_INT2_MAP_DATA register */
#define BMI08X_ACCEL_INT1_DRDY_MASK 0x04
#define BMI08X_ACCEL_INT2_DRDY_MASK 0x40

/* Position definitions for INT1_INT2_MAP_DATA register */
#define BMI08X_ACCEL_INT1_DRDY_POS 2
#define BMI08X_ACCEL_INT2_DRDY_POS 6

/* Asic Initialization value */
#define BMI08X_ASIC_INITIALIZED 0x01

#define BMI08X_TEMP_OFFSET 32

/*************************** BMI08 Gyroscope Macros *****************************/
/** Register map */
/* Gyro registers */

/* Gyro Chip Id register */
#define BMI08X_REG_GYRO_CHIP_ID 0x00

/* Gyro X LSB data register */
#define BMI08X_REG_GYRO_X_LSB 0x02

/* Gyro X MSB data register */
#define BMI08X_REG_GYRO_X_MSB 0x03

/* Gyro Y LSB data register */
#define BMI08X_REG_GYRO_Y_LSB 0x04

/* Gyro Y MSB data register */
#define BMI08X_REG_GYRO_Y_MSB 0x05

/* Gyro Z LSB data register */
#define BMI08X_REG_GYRO_Z_LSB 0x06

/* Gyro Z MSB data register */
#define BMI08X_REG_GYRO_Z_MSB 0x07

/* Gyro Interrupt status register */
#define BMI08X_REG_GYRO_INT_STAT_1 0x0A

/* Gyro Range register */
#define BMI08X_REG_GYRO_RANGE 0x0F

/* Gyro Bandwidth register */
#define BMI08X_REG_GYRO_BANDWIDTH 0x10

/* Gyro Power register */
#define BMI08X_REG_GYRO_LPM1 0x11

/* Gyro Soft reset register */
#define BMI08X_REG_GYRO_SOFTRESET 0x14

/* Gyro Interrupt control register */
#define BMI08X_REG_GYRO_INT_CTRL 0x15

/* Gyro Interrupt Pin configuration register */
#define BMI08X_REG_GYRO_INT3_INT4_IO_CONF 0x16

/* Gyro Interrupt Map register */
#define BMI08X_REG_GYRO_INT3_INT4_IO_MAP 0x18

/* Gyro Self test register */
#define BMI08X_REG_GYRO_SELF_TEST 0x3C

/* Gyro unique chip identifier */
#define BMI08X_GYRO_CHIP_ID 0x0F

/* Gyro Range */
#define BMI08X_GYRO_RANGE_2000_DPS 0x00
#define BMI08X_GYRO_RANGE_1000_DPS 0x01
#define BMI08X_GYRO_RANGE_500_DPS  0x02
#define BMI08X_GYRO_RANGE_250_DPS  0x03
#define BMI08X_GYRO_RANGE_125_DPS  0x04

/* Gyro Output data rate and bandwidth */
#define BMI08X_GYRO_BW_532_ODR_2000_HZ 0x00
#define BMI08X_GYRO_BW_230_ODR_2000_HZ 0x01
#define BMI08X_GYRO_BW_116_ODR_1000_HZ 0x02
#define BMI08X_GYRO_BW_47_ODR_400_HZ   0x03
#define BMI08X_GYRO_BW_23_ODR_200_HZ   0x04
#define BMI08X_GYRO_BW_12_ODR_100_HZ   0x05
#define BMI08X_GYRO_BW_64_ODR_200_HZ   0x06
#define BMI08X_GYRO_BW_32_ODR_100_HZ   0x07
#define BMI08X_GYRO_ODR_RESET_VAL      0x80

/* Gyro Power mode */
#define BMI08X_GYRO_PM_NORMAL	    0x00
#define BMI08X_GYRO_PM_DEEP_SUSPEND 0x20
#define BMI08X_GYRO_PM_SUSPEND	    0x80

/* Gyro data ready interrupt enable value */
#define BMI08X_GYRO_DRDY_INT_DISABLE_VAL 0x00
#define BMI08X_GYRO_DRDY_INT_ENABLE_VAL	 0x80

/* Gyro data ready map values */
#define BMI08X_GYRO_MAP_DRDY_TO_INT3	       0x01
#define BMI08X_GYRO_MAP_DRDY_TO_INT4	       0x80
#define BMI08X_GYRO_MAP_DRDY_TO_BOTH_INT3_INT4 0x81

/* Gyro Soft reset delay */
#define BMI08X_GYRO_SOFTRESET_DELAY 30

/* Gyro power mode config delay */
#define BMI08X_GYRO_POWER_MODE_CONFIG_DELAY 30

/** Mask definitions for range, bandwidth and power */
#define BMI08X_GYRO_RANGE_MASK 0x07
#define BMI08X_GYRO_BW_MASK    0x0F
#define BMI08X_GYRO_POWER_MASK 0xA0

/** Position definitions for range, bandwidth and power */
#define BMI08X_GYRO_POWER_POS 5

/* Mask definitions for BMI08X_GYRO_INT_CTRL_REG register */
#define BMI08X_GYRO_DATA_EN_MASK 0x80

/* Position definitions for BMI08X_GYRO_INT_CTRL_REG register */
#define BMI08X_GYRO_DATA_EN_POS 7

/* Mask definitions for BMI08X_GYRO_INT3_INT4_IO_CONF_REG register */
#define BMI08X_GYRO_INT3_LVL_MASK 0x01
#define BMI08X_GYRO_INT3_OD_MASK  0x02
#define BMI08X_GYRO_INT4_LVL_MASK 0x04
#define BMI08X_GYRO_INT4_OD_MASK  0x08

/* Position definitions for BMI08X_GYRO_INT3_INT4_IO_CONF_REG register */
#define BMI08X_GYRO_INT3_OD_POS	 1
#define BMI08X_GYRO_INT4_LVL_POS 2
#define BMI08X_GYRO_INT4_OD_POS	 3

/* Mask definitions for BMI08X_GYRO_INT_EN_REG register */
#define BMI08X_GYRO_INT_EN_MASK 0x80

/* Position definitions for BMI08X_GYRO_INT_EN_REG register */
#define BMI08X_GYRO_INT_EN_POS 7

/* Mask definitions for BMI088_GYRO_INT_MAP_REG register */
#define BMI08X_GYRO_INT3_MAP_MASK 0x01
#define BMI08X_GYRO_INT4_MAP_MASK 0x80

/* Position definitions for BMI088_GYRO_INT_MAP_REG register */
#define BMI08X_GYRO_INT3_MAP_POS 0
#define BMI08X_GYRO_INT4_MAP_POS 7

/* Mask definitions for BMI088_GYRO_INT_MAP_REG register */
#define BMI088_GYRO_INT3_MAP_MASK 0x01
#define BMI088_GYRO_INT4_MAP_MASK 0x80

/* Position definitions for BMI088_GYRO_INT_MAP_REG register */
#define BMI088_GYRO_INT3_MAP_POS 0
#define BMI088_GYRO_INT4_MAP_POS 7

/* Mask definitions for GYRO_SELF_TEST register */
#define BMI08X_GYRO_SELF_TEST_EN_MASK	    0x01
#define BMI08X_GYRO_SELF_TEST_RDY_MASK	    0x02
#define BMI08X_GYRO_SELF_TEST_RESULT_MASK   0x04
#define BMI08X_GYRO_SELF_TEST_FUNCTION_MASK 0x08

/* Position definitions for GYRO_SELF_TEST register */
#define BMI08X_GYRO_SELF_TEST_RDY_POS	   1
#define BMI08X_GYRO_SELF_TEST_RESULT_POS   2
#define BMI08X_GYRO_SELF_TEST_FUNCTION_POS 3

/*************************** Common Macros for both Accel and Gyro *****************************/
/** Soft reset Value */
#define BMI08X_SOFT_RESET_CMD 0xB6

/* Constant values macros */
#define BMI08X_SENSOR_DATA_SYNC_TIME_MS 1
#define BMI08X_DELAY_BETWEEN_WRITES_MS	1
#define BMI08X_SELF_TEST_DELAY_MS	3
#define BMI08X_POWER_CONFIG_DELAY	5
#define BMI08X_SENSOR_SETTLE_TIME_MS	30
#define BMI08X_SELF_TEST_DATA_READ_MS	50
#define BMI08X_ASIC_INIT_TIME_MS	150

/* allowed ODR values */
enum bmi08x_odr {
	BMI08X_ODR_25_2,
	BMI08X_ODR_25,
	BMI08X_ODR_50,
	BMI08X_ODR_100,
	BMI08X_ODR_200,
	BMI08X_ODR_400,
	BMI08X_ODR_800,
	BMI08X_ODR_1600,
};

/* Range values for accelerometer */
#define BMI08X_ACC_RANGE_2G_3G	 0x0
#define BMI08X_ACC_RANGE_4G_6G	 0x1
#define BMI08X_ACC_RANGE_8G_12G	 0x2
#define BMI08X_ACC_RANGE_16G_24G 0x3

/* Range values for gyro */
#define BMI08X_GYR_RANGE_2000DPS 0
#define BMI08X_GYR_RANGE_1000DPS 1
#define BMI08X_GYR_RANGE_500DPS	 2
#define BMI08X_GYR_RANGE_250DPS	 3
#define BMI08X_GYR_RANGE_125DPS	 4

#define BMI08X_ACC_SCALE(range_g)   ((2 * range_g * SENSOR_G) / 65536LL)
#define BMI08X_GYR_SCALE(range_dps) ((2 * range_dps * SENSOR_PI) / 180LL / 65536LL)

/* report of data sync is selected */
#define BMI08X_ACCEL_DATA_SYNC_EN(inst) DT_NODE_HAS_STATUS(DT_INST_PHANDLE(inst, data_sync), okay)
/* Macro used for compile time optimization to compile in/out code used for data-sync
 * if at least 1 bmi08x has data-sync enabled
 */
#define ACCEL_HELPER(inst)		    BMI08X_ACCEL_DATA_SYNC_EN(inst) ||
#define BMI08X_ACCEL_ANY_INST_HAS_DATA_SYNC DT_INST_FOREACH_STATUS_OKAY(ACCEL_HELPER) 0
#define GYRO_HELPER(inst)		    DT_INST_PROP(inst, data_sync) ||
#define BMI08X_GYRO_ANY_INST_HAS_DATA_SYNC  DT_INST_FOREACH_STATUS_OKAY(GYRO_HELPER) 0

struct bmi08x_range {
	uint16_t range;
	uint8_t reg_val;
};

union bmi08x_bus {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
};

struct bmi08x_accel_bus_io {
	int (*check)(const union bmi08x_bus *bus);
	int (*bus_init)(const struct device *dev);
	int (*transceive)(const struct device *dev, uint8_t reg, bool write, void *data,
			  size_t length);
#if BMI08X_ACCEL_ANY_INST_HAS_DATA_SYNC
	int (*write_config_file)(const struct device *dev);
#endif
};

struct bmi08x_gyro_bus_io {
	int (*check)(const union bmi08x_bus *bus);
	int (*transceive)(const struct device *dev, uint8_t reg, bool write, void *data,
			  size_t length);
};

struct bmi08x_accel_config {
	union bmi08x_bus bus;
	const struct bmi08x_accel_bus_io *api;
#if defined(CONFIG_BMI08X_ACCEL_TRIGGER)
	struct gpio_dt_spec int_gpio;
#endif
#if defined(CONFIG_BMI08X_ACCEL_TRIGGER) || BMI08X_ACCEL_ANY_INST_HAS_DATA_SYNC
	uint8_t int1_map;
	uint8_t int2_map;
	uint8_t int1_conf_io;
	uint8_t int2_conf_io;
#endif
	uint8_t accel_hz;
	uint8_t accel_fs;
#if BMI08X_ACCEL_ANY_INST_HAS_DATA_SYNC
	uint8_t data_sync;
#endif
};

struct bmi08x_gyro_config {
	union bmi08x_bus bus;
	const struct bmi08x_gyro_bus_io *api;
#if defined(CONFIG_BMI08X_GYRO_TRIGGER)
	struct gpio_dt_spec int_gpio;
#endif
#if defined(CONFIG_BMI08X_GYRO_TRIGGER) || BMI08X_GYRO_ANY_INST_HAS_DATA_SYNC
	uint8_t int3_4_map;
	uint8_t int3_4_conf_io;
#endif
	uint8_t gyro_hz;
	uint16_t gyro_fs;
};

struct bmi08x_accel_data {
#if defined(CONFIG_BMI08X_ACCEL_TRIGGER)
	struct gpio_callback gpio_cb;
#endif
	uint16_t acc_sample[3];
	uint16_t scale; /* micro m/s^2/lsb */

#if defined(CONFIG_BMI08X_ACCEL_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_BMI08X_ACCEL_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem sem;
#elif defined(CONFIG_BMI08X_ACCEL_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	const struct device *dev;
#endif

#ifdef CONFIG_BMI08X_ACCEL_TRIGGER
	sensor_trigger_handler_t handler_drdy_acc;
	const struct sensor_trigger *drdy_trig_acc;
#endif /* CONFIG_BMI08X_ACCEL_TRIGGER */
	uint8_t accel_chip_id;
};

struct bmi08x_gyro_data {
#if defined(CONFIG_BMI08X_GYRO_TRIGGER)
	struct gpio_callback gpio_cb;
#endif
	uint16_t gyr_sample[3];
	uint16_t scale; /* micro radians/s/lsb */

#if defined(CONFIG_BMI08X_GYRO_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_BMI08X_GYRO_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem sem;
#elif defined(CONFIG_BMI08X_GYRO_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	const struct device *dev;
#endif

#ifdef CONFIG_BMI08X_GYRO_TRIGGER
	sensor_trigger_handler_t handler_drdy_gyr;
	const struct sensor_trigger *drdy_trig_gyr;
#endif /* CONFIG_BMI08X_GYRO_TRIGGER */
};

/* common functions for accel and gyro */
int bmi08x_freq_to_odr_val(uint16_t freq_int, uint16_t freq_milli);
int32_t bmi08x_range_to_reg_val(uint16_t range, const struct bmi08x_range *range_map,
				uint16_t range_map_size);
int32_t bmi08x_reg_val_to_range(uint8_t reg_val, const struct bmi08x_range *range_map,
				       uint16_t range_map_size);

int bmi08x_accel_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, uint8_t len);
int bmi08x_accel_write(const struct device *dev, uint8_t reg_addr, uint8_t *data, uint16_t len);
int bmi08x_accel_byte_read(const struct device *dev, uint8_t reg_addr, uint8_t *byte);
int bmi08x_accel_byte_write(const struct device *dev, uint8_t reg_addr, uint8_t byte);
int bmi08x_accel_word_write(const struct device *dev, uint8_t reg_addr, uint16_t word);
int bmi08x_accel_reg_field_update(const struct device *dev, uint8_t reg_addr, uint8_t pos,
				  uint8_t mask, uint8_t val);
static inline int bmi08x_accel_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask,
					  uint8_t val)
{
	return bmi08x_accel_reg_field_update(dev, reg_addr, 0, mask, val);
}

int bmi08x_gyro_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, uint8_t len);
int bmi08x_gyro_byte_read(const struct device *dev, uint8_t reg_addr, uint8_t *byte);
int bmi08x_gyro_byte_write(const struct device *dev, uint8_t reg_addr, uint8_t byte);
int bmi08x_gyro_word_write(const struct device *dev, uint8_t reg_addr, uint16_t word);
int bmi08x_gyro_reg_field_update(const struct device *dev, uint8_t reg_addr, uint8_t pos,
				 uint8_t mask, uint8_t val);
static inline int bmi08x_gyro_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask,
					 uint8_t val)
{
	return bmi08x_gyro_reg_field_update(dev, reg_addr, 0, mask, val);
}

int bmi08x_acc_trigger_mode_init(const struct device *dev);
int bmi08x_trigger_set_acc(const struct device *dev, const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler);
int bmi08x_acc_slope_config(const struct device *dev, enum sensor_attribute attr,
			    const struct sensor_value *val);
int32_t bmi08x_acc_reg_val_to_range(uint8_t reg_val);

int bmi08x_gyr_trigger_mode_init(const struct device *dev);
int bmi08x_trigger_set_gyr(const struct device *dev, const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler);
int bmi08x_gyr_slope_config(const struct device *dev, enum sensor_attribute attr,
			    const struct sensor_value *val);
int32_t bmi08x_gyr_reg_val_to_range(uint8_t reg_val);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI08X_H_ */
