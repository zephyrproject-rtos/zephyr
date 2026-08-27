/*
 * Copyright (c) 2025 Croxel
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ALS31300_H_
#define ZEPHYR_DRIVERS_SENSOR_ALS31300_H_

#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/rtio/rtio.h>

/* ALS31300 Register Definitions */
#define ALS31300_REG_EEPROM_02   0x02
#define ALS31300_REG_EEPROM_03   0x03
#define ALS31300_REG_VOLATILE_27 0x27
#define ALS31300_REG_DATA_28     0x28
#define ALS31300_REG_DATA_29     0x29

/* Customer Access Code */
#define ALS31300_ACCESS_ADDR 0x35
#define ALS31300_ACCESS_CODE 0x2C413534

/* Register 0x02 bit definitions */
#define ALS31300_BW_SELECT_MASK  GENMASK(23, 21)
#define ALS31300_BW_SELECT_SHIFT 21
#define ALS31300_HALL_MODE_MASK  GENMASK(20, 19)
#define ALS31300_HALL_MODE_SHIFT 19
#define ALS31300_CHAN_Z_EN       BIT(8)
#define ALS31300_CHAN_Y_EN       BIT(7)
#define ALS31300_CHAN_X_EN       BIT(6)

/* Register 0x27 bit definitions */
#define ALS31300_SLEEP_MASK   GENMASK(1, 0)
#define ALS31300_SLEEP_ACTIVE 0
#define ALS31300_SLEEP_MODE   1
#define ALS31300_SLEEP_LPDCM  2

/* Register 0x28 bit fields */
#define ALS31300_REG28_TEMP_MSB_MASK  GENMASK(5, 0) /* Bits 5:0 */
#define ALS31300_REG28_TEMP_MSB_SHIFT 0

#define ALS31300_REG28_INTERRUPT_MASK  GENMASK(6, 6) /* Bit 6 */
#define ALS31300_REG28_INTERRUPT_SHIFT 6

#define ALS31300_REG28_NEW_DATA_MASK  GENMASK(7, 7) /* Bit 7 */
#define ALS31300_REG28_NEW_DATA_SHIFT 7

#define ALS31300_REG28_Z_AXIS_MSB_MASK  GENMASK(15, 8) /* Bits 15:8 */
#define ALS31300_REG28_Z_AXIS_MSB_SHIFT 8

#define ALS31300_REG28_Y_AXIS_MSB_MASK  GENMASK(23, 16) /* Bits 23:16 */
#define ALS31300_REG28_Y_AXIS_MSB_SHIFT 16

#define ALS31300_REG28_X_AXIS_MSB_MASK  GENMASK(31, 24) /* Bits 31:24 */
#define ALS31300_REG28_X_AXIS_MSB_SHIFT 24

/* Register 0x29 bit fields */
#define ALS31300_REG29_TEMP_LSB_MASK  GENMASK(5, 0) /* Bits 5:0 */
#define ALS31300_REG29_TEMP_LSB_SHIFT 0

#define ALS31300_REG29_HALL_MODE_STATUS_MASK  GENMASK(7, 6) /* Bits 7:6 */
#define ALS31300_REG29_HALL_MODE_STATUS_SHIFT 6

#define ALS31300_REG29_Z_AXIS_LSB_MASK  GENMASK(11, 8) /* Bits 11:8 */
#define ALS31300_REG29_Z_AXIS_LSB_SHIFT 8

#define ALS31300_REG29_Y_AXIS_LSB_MASK  GENMASK(15, 12) /* Bits 15:12 */
#define ALS31300_REG29_Y_AXIS_LSB_SHIFT 12

#define ALS31300_REG29_X_AXIS_LSB_MASK  GENMASK(19, 16) /* Bits 19:16 */
#define ALS31300_REG29_X_AXIS_LSB_SHIFT 16

#define ALS31300_REG29_INTERRUPT_WRITE_MASK  GENMASK(20, 20) /* Bit 20 */
#define ALS31300_REG29_INTERRUPT_WRITE_SHIFT 20

#define ALS31300_REG29_RESERVED_MASK  GENMASK(31, 21) /* Bits 31:21 */
#define ALS31300_REG29_RESERVED_SHIFT 21

/* ALS31300 sensitivity and conversion constants */
#define ALS31300_FULL_SCALE_RANGE_GAUSS 500  /* 500 gauss full scale */
#define ALS31300_12BIT_RESOLUTION       4096 /* 2^12 for 12-bit resolution */
#define ALS31300_12BIT_SIGN_BIT_INDEX   11   /* Sign bit position for 12-bit values (0-based) */

/* ALS31300 EEPROM Register 0x02 bit field definitions */
#define ALS31300_EEPROM_CUSTOMER_EE_MASK  GENMASK(4, 0) /* Bits 4:0 */
#define ALS31300_EEPROM_CUSTOMER_EE_SHIFT 0

#define ALS31300_EEPROM_INT_LATCH_EN_MASK  BIT(5) /* Bit 5 */
#define ALS31300_EEPROM_INT_LATCH_EN_SHIFT 5

#define ALS31300_EEPROM_CHANNEL_X_EN_MASK  BIT(6) /* Bit 6 */
#define ALS31300_EEPROM_CHANNEL_X_EN_SHIFT 6

#define ALS31300_EEPROM_CHANNEL_Y_EN_MASK  BIT(7) /* Bit 7 */
#define ALS31300_EEPROM_CHANNEL_Y_EN_SHIFT 7

#define ALS31300_EEPROM_CHANNEL_Z_EN_MASK  BIT(8) /* Bit 8 */
#define ALS31300_EEPROM_CHANNEL_Z_EN_SHIFT 8

#define ALS31300_EEPROM_I2C_THRESHOLD_MASK  BIT(9) /* Bit 9 */
#define ALS31300_EEPROM_I2C_THRESHOLD_SHIFT 9

#define ALS31300_EEPROM_SLAVE_ADDR_MASK  GENMASK(16, 10) /* Bits 16:10 */
#define ALS31300_EEPROM_SLAVE_ADDR_SHIFT 10

#define ALS31300_EEPROM_DISABLE_SLAVE_ADC_MASK  BIT(17) /* Bit 17 */
#define ALS31300_EEPROM_DISABLE_SLAVE_ADC_SHIFT 17

#define ALS31300_EEPROM_I2C_CRC_EN_MASK  BIT(18) /* Bit 18 */
#define ALS31300_EEPROM_I2C_CRC_EN_SHIFT 18

#define ALS31300_EEPROM_HALL_MODE_MASK  GENMASK(20, 19) /* Bits 20:19 */
#define ALS31300_EEPROM_HALL_MODE_SHIFT 19

#define ALS31300_EEPROM_BW_SELECT_MASK  GENMASK(23, 21) /* Bits 23:21 */
#define ALS31300_EEPROM_BW_SELECT_SHIFT 21

#define ALS31300_EEPROM_RESERVED_MASK  GENMASK(31, 24) /* Bits 31:24 */
#define ALS31300_EEPROM_RESERVED_SHIFT 24

/* Timing constants */
#define ALS31300_POWER_ON_DELAY_US  600
#define ALS31300_REG_WRITE_DELAY_MS 50

/* Fixed-point conversion constants */
#define ALS31300_TEMP_SCALE_FACTOR 302  /* Temperature scale factor */
#define ALS31300_TEMP_OFFSET       1708 /* Temperature offset */
#define ALS31300_TEMP_DIVISOR      4096 /* Temperature divisor */

/* RTIO-specific constants */
#define ALS31300_MAGN_SHIFT 16 /* Q31 shift for magnetic field values */
#define ALS31300_TEMP_SHIFT 16 /* Q31 shift for temperature values */

/* Sensor readings structure */
struct als31300_readings {
	int16_t x;
	int16_t y;
	int16_t z;
	uint16_t temp;
};

/* Bus abstraction structure */
struct als31300_bus {
	struct rtio *ctx;
	struct rtio_iodev *iodev;
};

/* RTIO encoded data structures */
struct als31300_encoded_header {
	uint8_t channels;
	uint8_t reserved[3];
	uint64_t timestamp;
} __attribute__((__packed__));

struct als31300_encoded_data {
	struct als31300_encoded_header header;
	uint8_t payload[8]; /* Raw I2C data from registers 0x28-0x29 */
} __attribute__((__packed__));

/* Forward declarations for structures used in multiple files */
struct als31300_data {
	int16_t x_raw;
	int16_t y_raw;
	int16_t z_raw;
	int16_t temp_raw;
};

struct als31300_config {
	struct i2c_dt_spec i2c;
	struct als31300_bus bus;
};

/* Common helper functions shared between sync and async implementations */
int16_t als31300_convert_12bit_to_signed(uint16_t value);
void als31300_parse_registers(const uint8_t *buf, struct als31300_readings *readings);
int32_t als31300_convert_to_gauss(int16_t raw_value);
int32_t als31300_convert_temperature(uint16_t raw_temp);

#ifdef CONFIG_SENSOR_ASYNC_API
#include <zephyr/drivers/sensor.h>

struct rtio_sqe;
struct rtio_iodev_sqe;
struct sensor_read_config;
struct sensor_decoder_api;

/* Async I2C helper function */
int als31300_prep_i2c_read_async(const struct als31300_config *cfg, uint8_t reg, uint8_t *buf,
				 size_t size, struct rtio_sqe **out);

/* Encode function for async API */
int als31300_encode(const struct device *dev, const struct sensor_read_config *read_config,
		    uint8_t trigger_status, uint8_t *buf);

/* Async API functions */
void als31300_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int als31300_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ALS31300_H_ */
