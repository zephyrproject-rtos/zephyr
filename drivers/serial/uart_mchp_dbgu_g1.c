/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file uart_mchp_dbgu_g1.c
 * @brief UART driver implementation for Microchip DBGU.
 */

#include <errno.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>

#define DT_DRV_COMPAT microchip_dbgu_g1_uart

/* Device constant configuration parameters */
struct mchp_dbgu_dev_cfg {
	dbgu_registers_t *const regs;
};

/* Device run time data */
struct mchp_dbgu_dev_data {
	uint32_t baud_rate;
};

static int mchp_dbgu_poll_in(const struct device *dev, unsigned char *c)
{
	const struct mchp_dbgu_dev_cfg *config = dev->config;

	if (!(config->regs->DBGU_SR & DBGU_SR_RXRDY_Msk)) {
		return -ENODATA;
	}

	/* got a character */
	*c = (unsigned char)config->regs->DBGU_RHR;

	return 0;
}

static void mchp_dbgu_poll_out(const struct device *dev, unsigned char c)
{
	const struct mchp_dbgu_dev_cfg *config = dev->config;
	dbgu_registers_t *dbgu = (dbgu_registers_t *)config->regs;

	/* Wait for transmitter to be ready */
	while (!(dbgu->DBGU_SR & DBGU_SR_TXRDY_Msk)) {
	}

	/* send a character */
	dbgu->DBGU_THR = (uint32_t)c;
}

static int mchp_dbgu_err_check(const struct device *dev)
{
	const struct mchp_dbgu_dev_cfg *config = dev->config;
	const dbgu_registers_t *dbgu = config->regs;
	int errors = 0;

	if (dbgu->DBGU_SR & DBGU_SR_OVRE_Msk) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (dbgu->DBGU_SR & DBGU_SR_PARE_Msk) {
		errors |= UART_ERROR_PARITY;
	}

	if (dbgu->DBGU_SR & DBGU_SR_FRAME_Msk) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

static int mchp_dbgu_init(const struct device *dev)
{
	return 0;
}

static DEVICE_API(uart, mchp_dbgu_driver_api) = {
	.poll_in = mchp_dbgu_poll_in,
	.poll_out = mchp_dbgu_poll_out,
	.err_check = mchp_dbgu_err_check,
};

#define DBGU_SAM_INIT(n)							\
		static const struct mchp_dbgu_dev_cfg dbgu##n##_sam_config = {	\
			.regs = (dbgu_registers_t *)DT_INST_REG_ADDR(n),	\
		};								\
										\
		static struct mchp_dbgu_dev_data dbgu##n##_sam_data = {		\
			.baud_rate = DT_INST_PROP(n, current_speed),		\
		};								\
										\
		DEVICE_DT_INST_DEFINE(n,					\
				      mchp_dbgu_init, NULL,			\
				      &dbgu##n##_sam_data,			\
				      &dbgu##n##_sam_config, PRE_KERNEL_1,	\
				      CONFIG_SERIAL_INIT_PRIORITY,		\
				      &mchp_dbgu_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DBGU_SAM_INIT)
