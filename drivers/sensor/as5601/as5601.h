/* as5601.h - header file for AS5601 magnetic rotary position sensor driver */

/*
 * Copyright ...
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AS5601_H_
#define ZEPHYR_DRIVERS_SENSOR_AS5601_H_

#include <zephyr/types.h>
#include <drivers/i2c.h>
#include <sys/util.h>

// Configuration registers
#define AS5601_REG_ZMCO    0x00
#define AS5601_REG_ZPOS    0x01
#define AS5601_REG_CONF    0x07
#define AS5601_REG_ABN     0x09
#define AS5601_REG_PUSHTHR 0x0A

// Output registers (read only)
#define AS5601_REG_RAW_ANGLE 0x0C
#define AS5601_REG_ANGLE     0x0E

// Status registers (read only)
#define AS5601_REG_STATUS 0x0B

#define AS5601_MASK_REG_STATUS_MH BIT(3)
#define AS5601_MASK_REG_STATUS_ML BIT(4)
#define AS5601_MASK_REG_STATUS_MD BIT(5)

#define AS5601_REG_VAL_ABN_8    (0b0000)
#define AS5601_REG_VAL_ABN_16   (0b0001)
#define AS5601_REG_VAL_ABN_32   (0b0010)
#define AS5601_REG_VAL_ABN_64   (0b0011)
#define AS5601_REG_VAL_ABN_128  (0b0100)
#define AS5601_REG_VAL_ABN_256  (0b0101)
#define AS5601_REG_VAL_ABN_512  (0b0110)
#define AS5601_REG_VAL_ABN_1024 (0b0111)
#define AS5601_REG_VAL_ABN_2048 (0b1000)


struct as5601_config {
	char *i2c_master_dev_name;
	uint16_t i2c_slave_addr;
};

struct as5601_data {
	const struct device *i2c_master;

	int16_t sample_angle;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AS5601_H_ */
