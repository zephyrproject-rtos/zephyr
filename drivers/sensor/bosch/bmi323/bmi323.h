/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_H_

#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/kernel.h>
#ifdef CONFIG_SENSOR_ASYNC_API
#include <zephyr/drivers/sensor.h>
#endif

#define IMU_BOSCH_BMI323_REG_CHIP_ID    (0x00)

#define IMU_BOSCH_DIE_TEMP_OFFSET_MICRO_DEG_CELSIUS (23000000LL)
#define IMU_BOSCH_DIE_TEMP_MICRO_DEG_CELSIUS_LSB    (1953L)

#define IMU_BOSCH_BMI323_REG_ACC_DATA_X (0x03)
#define IMU_BOSCH_BMI323_REG_ACC_DATA_Y (0x04)
#define IMU_BOSCH_BMI323_REG_ACC_DATA_Z (0x05)

#define IMU_BOSCH_BMI323_REG_GYRO_DATA_X (0x06)
#define IMU_BOSCH_BMI323_REG_GYRO_DATA_Y (0x07)
#define IMU_BOSCH_BMI323_REG_GYRO_DATA_Z (0x08)

#define IMU_BOSCH_BMI323_REG_TEMP_DATA (0x09)

#define IMU_BOSCH_BMI323_REG_FEATURE_IO0 (0x10)

#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_X_EN_OFFSET  (0x03)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_X_EN_SIZE    (0x01)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_X_EN_VAL_DIS (0x00)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_X_EN_VAL_EN  (0x01)

#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_Y_EN_OFFSET  (0x04)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_Y_EN_SIZE    (0x01)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_Y_EN_VAL_DIS (0x00)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_Y_EN_VAL_EN  (0x01)

#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_Z_EN_OFFSET  (0x05)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_Z_EN_SIZE    (0x01)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_Z_EN_VAL_DIS (0x00)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO0_MOTION_Z_EN_VAL_EN  (0x01)

#define IMU_BOSCH_BMI323_REG_FEATURE_IO2 (0x12)

#define IMU_BOSCH_BMI323_REG_FEATURE_IO_STATUS (0x14)

#define IMU_BOSCH_BMI323_REG_FEATURE_IO_STATUS_STATUS_OFFSET  (0x00)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO_STATUS_STATUS_SIZE    (0x01)
#define IMU_BOSCH_BMI323_REG_FEATURE_IO_STATUS_STATUS_VAL_SET (0x01)

#define IMU_BOSCH_BMI323_REG_INT_STATUS_INT1 (0x0D)
#define IMU_BOSCH_BMI323_REG_INT_STATUS_INT2 (0x0E)

#define IMU_BOSCH_BMI323_REG_ACC_CONF (0x20)

#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_OFFSET	(0x00)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_SIZE		(0x04)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ0P78125 (0x01)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ1P5625	(0x02)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ3P125	(0x03)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ6P25	(0x04)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ12P5	(0x05)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ25	(0x06)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ50	(0x07)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ100	(0x08)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ200	(0x09)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ400	(0x0A)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ800	(0x0B)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ1600	(0x0C)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ3200	(0x0D)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ6400	(0x0E)

#define IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_OFFSET  (0x04)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_SIZE    (0x03)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_VAL_G2  (0x00)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_VAL_G4  (0x01)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_VAL_G8  (0x02)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_VAL_G16 (0x03)

#define IMU_BOSCH_BMI323_REG_ACC_CONF_MODE_OFFSET   (0x0C)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_MODE_SIZE	    (0x03)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_MODE_VAL_DIS  (0x00)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_MODE_VAL_LPWR (0x04)
#define IMU_BOSCH_BMI323_REG_ACC_CONF_MODE_VAL_HPWR (0x07)

#define IMU_BOSCH_BMI323_REG_GYRO_CONF (0x21)

#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_OFFSET	 (0x00)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_SIZE		 (0x04)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ0P78125 (0x01)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ1P5625	 (0x02)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ3P125	 (0x03)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ6P25	 (0x04)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ12P5	 (0x05)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ25	 (0x06)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ50	 (0x07)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ100	 (0x08)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ200	 (0x09)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ400	 (0x0A)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ800	 (0x0B)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ1600	 (0x0C)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ3200	 (0x0D)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ6400	 (0x0E)

#define IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_OFFSET	 (0x04)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_SIZE	 (0x03)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS125	 (0x00)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS250	 (0x01)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS500	 (0x02)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS1000 (0x03)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS2000 (0x04)

#define IMU_BOSCH_BMI323_REG_GYRO_CONF_MODE_OFFSET   (0x0C)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_MODE_SIZE     (0x03)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_MODE_VAL_DIS  (0x00)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_MODE_VAL_LPWR (0x04)
#define IMU_BOSCH_BMI323_REG_GYRO_CONF_MODE_VAL_HPWR (0x07)

#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL (0x38)

#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_LVL_OFFSET	(0x00)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_LVL_SIZE		(0x01)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_LVL_VAL_ACT_LOW	(0x00)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_LVL_VAL_ACT_HIGH	(0x01)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_OD_OFFSET		(0x01)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_OD_SIZE		(0x01)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_OD_VAL_PUSH_PULL	(0x00)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_OD_VAL_OPEN_DRAIN (0x01)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_OUTPUT_EN_OFFSET	(0x02)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_OUTPUT_EN_SIZE	(0x01)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_OUTPUT_EN_VAL_DIS (0x00)
#define IMU_BOSCH_BMI323_REG_IO_INT_CTRL_INT1_OUTPUT_EN_VAL_EN	(0x01)

#define IMU_BOSCH_BMI323_REG_INT_CONF (0x39)

#define IMU_BOSCH_BMI323_REG_INT_CONF_INT_LATCH_OFFSET		(0x01)
#define IMU_BOSCH_BMI323_REG_INT_CONF_INT_LATCH_SIZE		(0x01)
#define IMU_BOSCH_BMI323_REG_INT_CONF_INT_LATCH_VAL_NON_LATCHED (0x00)
#define IMU_BOSCH_BMI323_REG_INT_CONF_INT_LATCH_VAL_LATCHED	(0x01)

#define IMU_BOSCH_BMI323_REG_INT_MAP1 (0x3A)

#define IMU_BOSCH_BMI323_REG_INT_MAP1_MOTION_OUT_OFFSET	  (0x02)
#define IMU_BOSCH_BMI323_REG_INT_MAP1_MOTION_OUT_SIZE	  (0x02)
#define IMU_BOSCH_BMI323_REG_INT_MAP1_MOTION_OUT_VAL_DIS  (0x00)
#define IMU_BOSCH_BMI323_REG_INT_MAP1_MOTION_OUT_VAL_INT1 (0x01)
#define IMU_BOSCH_BMI323_REG_INT_MAP1_MOTION_OUT_VAL_INT2 (0x02)

#define IMU_BOSCH_BMI323_REG_INT_MAP2 (0x3B)

#define IMU_BOSCH_BMI323_REG_INT_MAP2_ACC_DRDY_INT_OFFSET   (0x0A)
#define IMU_BOSCH_BMI323_REG_INT_MAP2_ACC_DRDY_INT_SIZE	    (0x02)
#define IMU_BOSCH_BMI323_REG_INT_MAP2_ACC_DRDY_INT_VAL_DIS  (0x00)
#define IMU_BOSCH_BMI323_REG_INT_MAP2_ACC_DRDY_INT_VAL_INT1 (0x01)
#define IMU_BOSCH_BMI323_REG_INT_MAP2_ACC_DRDY_INT_VAL_INT2 (0x02)

#define IMU_BOSCH_BMI323_REG_FEATURE_CTRL (0x40)

#define IMU_BOSCH_BMI323_REG_FEATURE_CTRL_ENABLE_OFFSET	 (0x00)
#define IMU_BOSCH_BMI323_REG_FEATURE_CTRL_ENABLE_SIZE	 (0x01)
#define IMU_BOSCH_BMI323_REG_FEATURE_CTRL_ENABLE_VAL_DIS (0x00)
#define IMU_BOSCH_BMI323_REG_FEATURE_CTRL_ENABLE_VAL_EN	 (0x01)

#define IMU_BOSCH_BMI323_REG_CMD		    (0x7E)
#define IMU_BOSCH_BMI323_REG_CMD_CMD_OFFSET	    (0x00)
#define IMU_BOSCH_BMI323_REG_CMD_CMD_SIZE	    (0x10)
#define IMU_BOSCH_BMI323_REG_CMD_CMD_VAL_SOFT_RESET (0xDEAF)

#define IMU_BOSCH_BMI323_REG_MASK(reg, field)                                                      \
	(BIT_MASK(IMU_BOSCH_BMI323_REG_##reg##_##field##_SIZE)                                     \
	 << IMU_BOSCH_BMI323_REG_##reg##_##field##_OFFSET)

#define IMU_BOSCH_BMI323_REG_VALUE(reg, field, val)                                                \
	(IMU_BOSCH_BMI323_REG_##reg##_##field##_VAL_##val                                          \
	 << IMU_BOSCH_BMI323_REG_##reg##_##field##_OFFSET)

#define IMU_BOSCH_BMI323_REG_VALUE_GET_FIELD(reg_value, reg, field)                                \
	((reg_value >> IMU_BOSCH_BMI323_REG_##reg##_##field##_OFFSET) &                            \
	 BIT_MASK(IMU_BOSCH_BMI323_REG_##reg##_##field##_SIZE))

struct bosch_bmi323_bus_api {
	/* Read up to multiple words from the BMI323 */
	int (*read_words)(const void *context, uint8_t offset, uint16_t *words,
			  uint16_t words_count);

	/* Write up to multiple words to the BLI323 */
	int (*write_words)(const void *context, uint8_t offset, uint16_t *words,
			   uint16_t words_count);

	/* Initialize the bus */
	int (*init)(const void *context);
};

struct bosch_bmi323_bus {
	const void *context;
	const struct bosch_bmi323_bus_api *api;
};

enum bmi323_bus_type {
	BMI323_BUS_SPI = 0,
	BMI323_BUS_I2C,
	BMI323_BUS_I3C,
};

struct bosch_bmi323_data {
	struct k_mutex lock;

	struct sensor_value acc_samples[3];
	struct sensor_value gyro_samples[3];
	struct sensor_value temperature;

	bool acc_samples_valid;
	bool gyro_samples_valid;
	bool temperature_valid;

	uint32_t acc_full_scale;
	uint32_t gyro_full_scale;

	struct gpio_callback gpio_callback;
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t trigger_handler;
	struct k_work callback_work;
	const struct device *dev;

	enum bmi323_bus_type async_bus_type;
	/* 1 for SPI, 2 for I2C/I3C */
	uint8_t raw_data_offset;

#ifdef CONFIG_SENSOR_ASYNC_API
	struct rtio *r;
	struct rtio_iodev *bus_iodev;
	struct rtio_iodev_sqe *pending_sqe;
	struct k_spinlock mpsc_lock;
	/*
	 * Staging buffer: [conf_addr(1), ACC_CONF(2), GYRO_CONF(2), data_addr(1), sensor_data(14)]
	 * Plus offset for bus-specific dummy bytes. Max 22 bytes.
	 * Must remain valid until the completion callback fires.
	 */
	uint8_t raw_buffer[22];

	struct mpsc io_q;
#endif
};

/* Raw sensor readings (not sensor_value) - in micro units
 * Defined outside CONFIG_SENSOR_ASYNC_API as it's used by sample_fetch_impl
 * for both sync and async code paths.
 */
struct bmi323_reading {
	int64_t accel_x;
	int64_t accel_y;
	int64_t accel_z;
	int64_t gyro_x;
	int64_t gyro_y;
	int64_t gyro_z;
	int32_t temperature;
};

/* RTIO support structures */
#ifdef CONFIG_SENSOR_ASYNC_API

/* In bmi323.h - bus raw data offsets */
#define BMI323_SPI_RAW_OFFSET   1U  /* SPI dummy byte */
#define BMI323_I2C_RAW_OFFSET   2U  /* I2C no offset */
#define BMI323_I3C_RAW_OFFSET   2U  /* I3C no offset */

/* Decoder header with timestamp */
struct bmi323_decoder_header {
	uint64_t timestamp;
} __attribute__((__packed__));

struct bmi323_encoded_data {
	struct bmi323_decoder_header header;
	bool has_accel;
	bool has_gyro;
	bool has_temp;
	uint16_t acce_range;
	uint16_t gyro_range;
	struct bmi323_reading reading;
} __attribute__((__packed__));

/* Q31 conversion constants */
#define BMI323_ACCEL_SHIFT  10  /* Q21.10 for acceleration (m/s^2) */
#define BMI323_GYRO_SHIFT   10  /* Q21.10 for gyro (rad/s) */
#define BMI323_TEMP_SHIFT   10  /* Q21.10 for temperature */

/* Function declarations for async API */
void bmi323_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

int bmi323_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* CONFIG_SENSOR_ASYNC_API */

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_H_ */
