/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UWB_BUS_BOARD_H__
#define __UWB_BUS_BOARD_H__

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <uwb_uwbs_tml_io.h>

typedef enum {
	WRITE_MODE = 0,
	READ_MODE
} op_mode_t;

typedef struct {
	/*master handle */
	op_mode_t op_mode;
	/* SPI DT specification */
	struct spi_dt_spec *masterHandle;
	/*GPIO DT Specification*/
	struct gpio_dt_spec *irq_gpio;
	/*GPIO DT Specification*/
	struct gpio_dt_spec *rstn_gpio;
	/* This semaphore is use to wait for read interrupt from helios */
	struct k_sem mIrqWaitSem;
} uwb_bus_board_ctx_t;

typedef struct {
	/* I2C device */
	const struct device *i2cMasterHandle;
	/* No of bytes read */
	int bytesRead;
} i2c_bus_board_ctx_t;

void AddDelayInMicroSec(int delay);
#endif /*__UWB_BUS_BOARD_H__*/
