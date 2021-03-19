/* ST Microelectronics STMEMS hal i/f
 *
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * zephyrproject-rtos/modules/hal/st/sensor/stmemsc/
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STMEMSC_STMEMSC_H_
#define ZEPHYR_DRIVERS_SENSOR_STMEMSC_STMEMSC_H_

#include <drivers/i2c.h>
#include <drivers/spi.h>

#ifdef CONFIG_I2C
struct stmemsc_cfg_i2c {
	const struct device *bus;
	uint16_t i2c_slv_addr;
};

int stmemsc_i2c_read(const struct stmemsc_cfg_i2c *stmemsc,
		     uint8_t reg_addr, uint8_t *value, uint8_t len);
int stmemsc_i2c_write(const struct stmemsc_cfg_i2c *stmemsc,
		      uint8_t reg_addr, uint8_t *value, uint8_t len);
#endif

#ifdef CONFIG_SPI
struct stmemsc_cfg_spi {
	const struct device *bus;
	struct spi_config spi_cfg;
};

int stmemsc_spi_read(const struct stmemsc_cfg_spi *stmemsc,
		     uint8_t reg_addr, uint8_t *value, uint8_t len);
int stmemsc_spi_write(const struct stmemsc_cfg_spi *stmemsc,
		      uint8_t reg_addr, uint8_t *value, uint8_t len);
#endif
#endif /* ZEPHYR_DRIVERS_SENSOR_STMEMSC_STMEMSC_H_ */
