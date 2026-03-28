/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#include "soc.h"
#include "udma.h"

#define SY1XX_UDMA_CTRL_PER_CG (SY1XX_ARCHI_UDMA_ADDR + SY1XX_UDMA_CONF_OFFSET)

#define SY1XX_MAX_UART_COUNT 3
#define SY1XX_MAX_I2C_COUNT  4
#define SY1XX_MAX_SPI_COUNT  7
#define SY1XX_MAX_ETH_COUNT  1

void sy1xx_udma_enable_clock(sy1xx_udma_module_t module, uint32_t instance)
{

	uint32_t udma_ctrl_per_cg = sys_read32(SY1XX_UDMA_CTRL_PER_CG);

	switch (module) {

	case SY1XX_UDMA_MODULE_UART:
		if (instance >= SY1XX_MAX_UART_COUNT) {
			return;
		}
		udma_ctrl_per_cg |= 1 << (instance + 0);
		break;

	case SY1XX_UDMA_MODULE_I2C:
		if (instance >= SY1XX_MAX_I2C_COUNT) {
			return;
		}
		udma_ctrl_per_cg |= 1 << (instance + 10);
		break;

	case SY1XX_UDMA_MODULE_SPI:
		if (instance >= SY1XX_MAX_SPI_COUNT) {
			return;
		}
		udma_ctrl_per_cg |= 1 << (instance + 3);
		break;

	case SY1XX_UDMA_MODULE_MAC:
		if (instance >= SY1XX_MAX_ETH_COUNT) {
			return;
		}
		udma_ctrl_per_cg |= 1 << (instance + 20);
		break;

	case SY1XX_UDMA_MAX_MODULE_COUNT:
		break;
	}

	sys_write32(udma_ctrl_per_cg, SY1XX_UDMA_CTRL_PER_CG);
}

void sy1xx_udma_disable_clock(sy1xx_udma_module_t module, uint32_t instance)
{

	uint32_t udma_ctrl_per_cg = sys_read32(SY1XX_UDMA_CTRL_PER_CG);

	switch (module) {

	case SY1XX_UDMA_MODULE_UART:
		if (instance >= SY1XX_MAX_UART_COUNT) {
			return;
		}
		udma_ctrl_per_cg &= ~(1 << (instance + 0));
		break;

	case SY1XX_UDMA_MODULE_I2C:
		if (instance >= SY1XX_MAX_I2C_COUNT) {
			return;
		}
		udma_ctrl_per_cg &= ~(1 << (instance + 10));
		break;

	case SY1XX_UDMA_MODULE_SPI:
		if (instance >= SY1XX_MAX_SPI_COUNT) {
			return;
		}
		udma_ctrl_per_cg &= ~(1 << (instance + 3));
		break;

	case SY1XX_UDMA_MODULE_MAC:
		if (instance >= SY1XX_MAX_ETH_COUNT) {
			return;
		}
		udma_ctrl_per_cg &= ~(1 << (instance + 20));
		break;

	case SY1XX_UDMA_MAX_MODULE_COUNT:
		break;
	}

	sys_write32(udma_ctrl_per_cg, SY1XX_UDMA_CTRL_PER_CG);
}

void sy1xx_udma_busy_delay(uint32_t msec)
{
	uint32_t sec = 250000000;
	uint32_t millis = (sec / 1000) * msec;

	for (uint32_t i = 0; i < millis; i++) {
		__asm__("nop");
	}
}

int32_t sy1xx_udma_cancel(uint32_t base, uint32_t channel)
{
	uint32_t channel_offset = channel == 0 ? 0x00 : 0x10;

	/* clear existing */
	SY1XX_UDMA_WRITE_REG(base, SY1XX_UDMA_CFG_REG + channel_offset,
			     SY1XX_UDMA_CHANNEL_CFG_CLEAR);
	return 0;
}

int32_t sy1xx_udma_is_ready(uint32_t base, uint32_t channel)
{
	uint32_t channel_offset = channel == 0 ? 0x00 : 0x10;

	int32_t isBusy = SY1XX_UDMA_READ_REG(base, SY1XX_UDMA_CFG_REG + channel_offset) &
			 (SY1XX_UDMA_CHANNEL_CFG_EN);

	return isBusy ? 0 : 1;
}

int32_t sy1xx_udma_wait_for_finished(uint32_t base, uint32_t channel)
{
	uint32_t channel_offset = channel == 0 ? 0x00 : 0x10;

	volatile uint32_t timeout = 200;

	while (SY1XX_UDMA_READ_REG(base, SY1XX_UDMA_CFG_REG + channel_offset) &
	       (SY1XX_UDMA_CHANNEL_CFG_EN)) {
		sy1xx_udma_busy_delay(1);
		timeout--;
		if (timeout == 0) {
			return -1;
		}
	}

	return 0;
}

int32_t sy1xx_udma_wait_for_status(uint32_t base)
{

	volatile uint32_t timeout = 200;

	while (SY1XX_UDMA_READ_REG(base, SY1XX_UDMA_STATUS) & (0x3)) {
		sy1xx_udma_busy_delay(1);
		timeout--;
		if (timeout == 0) {
			return -1;
		}
	}

	return 0;
}

int32_t sy1xx_udma_start(uint32_t base, uint32_t channel, uint32_t saddr, uint32_t size,
			 uint32_t optional_cfg)
{
	uint32_t channel_offset = channel == 0 ? 0x00 : 0x10;

	SY1XX_UDMA_WRITE_REG(base, SY1XX_UDMA_SADDR_REG + channel_offset, saddr);
	SY1XX_UDMA_WRITE_REG(base, SY1XX_UDMA_SIZE_REG + channel_offset, size);
	SY1XX_UDMA_WRITE_REG(base, SY1XX_UDMA_CFG_REG + channel_offset,
			     SY1XX_UDMA_CHANNEL_CFG_EN | optional_cfg);

	return 0;
}

int32_t sy1xx_udma_get_remaining(uint32_t base, uint32_t channel)
{
	uint32_t channel_offset = channel == 0 ? 0x00 : 0x10;

	int32_t size = SY1XX_UDMA_READ_REG(base, SY1XX_UDMA_SIZE_REG + channel_offset);

	return size;
}
