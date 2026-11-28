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

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/spi.h>

void stmemsc_mdelay(uint32_t millisec);

#ifdef CONFIG_I2C
/*
 * Populate the stmdev_ctx_t structure pointed by stmdev_ctx_ptr with
 * standard stmemsc i2c APIs.
 */
#define STMEMSC_CTX_I2C(stmdev_ctx_ptr)					\
	.ctx = {							\
		.read_reg = stmemsc_i2c_read,				\
		.write_reg = stmemsc_i2c_write,				\
		.mdelay = stmemsc_mdelay,				\
		.handle = (void *)stmdev_ctx_ptr			\
	}

int32_t stmemsc_i2c_read(void *ctx, uint8_t reg_addr,
			 uint8_t *value, uint16_t len);
int32_t stmemsc_i2c_write(void *ctx, uint8_t reg_addr,
			  const uint8_t *value, uint16_t len);

/*
 * Populate the stmdev_ctx_t structure pointed by stmdev_ctx_ptr with
 * specific stmemsc i2c APIs that set reg_addr MSB to '1' in order to allow
 * multiple read/write operations. This is common in some STMEMSC drivers
 */
#define STMEMSC_CTX_I2C_INCR(stmdev_ctx_ptr)				\
	.ctx = {							\
		.read_reg = stmemsc_i2c_read_incr,			\
		.write_reg = stmemsc_i2c_write_incr,			\
		.mdelay = stmemsc_mdelay,				\
		.handle = (void *)stmdev_ctx_ptr			\
	}

int32_t stmemsc_i2c_read_incr(void *ctx, uint8_t reg_addr,
			      uint8_t *value, uint16_t len);
int32_t stmemsc_i2c_write_incr(void *ctx, uint8_t reg_addr,
			       const uint8_t *value, uint16_t len);

/*
 * Populate the stmdev_ctx_t structure pointed by stmdev_ctx_ptr with
 * custom stmemsc i2c APIs specified by driver.
 */
#define STMEMSC_CTX_I2C_CUSTOM(stmdev_ctx_ptr, i2c_rd_api, i2c_wr_api)	\
	.ctx = {							\
		.read_reg = i2c_rd_api,					\
		.write_reg = i2c_wr_api,				\
		.mdelay = stmemsc_mdelay,				\
		.handle = (void *)stmdev_ctx_ptr			\
	}

#endif

#ifdef CONFIG_I3C
/*
 * Populate the stmdev_ctx_t structure pointed by stmdev_ctx_ptr with
 * standard stmemsc i3c APIs.
 */
#define STMEMSC_CTX_I3C(stmdev_ctx_ptr)					\
	.ctx = {							\
		.read_reg = stmemsc_i3c_read,				\
		.write_reg = stmemsc_i3c_write,				\
		.mdelay = stmemsc_mdelay,				\
		.handle = (void *)stmdev_ctx_ptr			\
	}

int32_t stmemsc_i3c_read(void *ctx, uint8_t reg_addr,
			 uint8_t *value, uint16_t len);
int32_t stmemsc_i3c_write(void *ctx, uint8_t reg_addr,
			  const uint8_t *value, uint16_t len);
#endif

#ifdef CONFIG_SPI
/*
 * Populate the stmdev_ctx_t structure pointed by stmdev_ctx_ptr with
 * stmemsc spi APIs.
 */
#define STMEMSC_CTX_SPI(stmdev_ctx_ptr)					\
	.ctx = {							\
		.read_reg = stmemsc_spi_read,				\
		.write_reg = stmemsc_spi_write,				\
		.mdelay = stmemsc_mdelay,				\
		.handle = (void *)stmdev_ctx_ptr			\
	}

int32_t stmemsc_spi_read(void *ctx, uint8_t reg_addr,
			 uint8_t *value, uint16_t len);
int32_t stmemsc_spi_write(void *ctx, uint8_t reg_addr,
			  const uint8_t *value, uint16_t len);

/*
 * Populate the stmdev_ctx_t structure pointed by stmdev_ctx_ptr with
 * specific stmemsc sp APIs that set reg_addr bit6 to '1' in order to allow
 * multiple read/write operations. This is common in some STMEMSC drivers
 */
#define STMEMSC_CTX_SPI_INCR(stmdev_ctx_ptr)				\
	.ctx = {							\
		.read_reg = stmemsc_spi_read_incr,			\
		.write_reg = stmemsc_spi_write_incr,			\
		.mdelay = stmemsc_mdelay,				\
		.handle = (void *)stmdev_ctx_ptr			\
	}

int32_t stmemsc_spi_read_incr(void *ctx, uint8_t reg_addr,
			      uint8_t *value, uint16_t len);
int32_t stmemsc_spi_write_incr(void *ctx, uint8_t reg_addr,
			       const uint8_t *value, uint16_t len);

/*
 * Populate the stmdev_ctx_t structure pointed by stmdev_ctx_ptr with
 * custom stmemsc spi APIs specified by driver.
 */
#define STMEMSC_CTX_SPI_CUSTOM(stmdev_ctx_ptr, spi_rd_api, spi_wr_api)	\
	.ctx = {							\
		.read_reg = spi_rd_api,					\
		.write_reg = spi_wr_api,				\
		.mdelay = stmemsc_mdelay,				\
		.handle = (void *)stmdev_ctx_ptr			\
	}

#endif
#endif /* ZEPHYR_DRIVERS_SENSOR_STMEMSC_STMEMSC_H_ */
