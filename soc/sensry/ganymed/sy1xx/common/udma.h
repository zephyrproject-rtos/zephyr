/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#ifndef GANYMED_SY1XX_UDMA_H
#define GANYMED_SY1XX_UDMA_H

#include <stdint.h>
#include <zephyr/arch/common/sys_io.h>
#include <soc.h>

/* UDMA */
#define ARCHI_UDMA_ADDR (ARCHI_SOC_PERIPHERALS_ADDR + ARCHI_UDMA_OFFSET)

#define UDMA_PERIPH_AREA_SIZE_LOG2 7
#define UDMA_PERIPH_OFFSET(id)     (((id) << UDMA_PERIPH_AREA_SIZE_LOG2))

#define ARCHI_UDMA_UART_ID(id)  (0 + (id))
#define ARCHI_UDMA_SPIM_ID(id)  (3 + (id))
#define ARCHI_UDMA_I2C_ID(id)   (10 + (id))
#define ARCHI_UDMA_I2S_ID(id)   (14 + (id))
#define ARCHI_UDMA_HYPER_ID(id) (19 + (id))
#define ARCHI_UDMA_TSN_ID(id)   (20 + (id))

#define UDMA_CHANNEL_RX_OFFSET     0x00
#define UDMA_CHANNEL_TX_OFFSET     0x10
#define UDMA_CHANNEL_CUSTOM_OFFSET 0x20

/*
 * For each channel, the RX and TX part have the following registers
 * The offsets are given relative to the offset of the RX or TX part
 */

/* Start address register */
#define UDMA_CHANNEL_SADDR_OFFSET  0x0
/* Size register */
#define UDMA_CHANNEL_SIZE_OFFSET   0x4
/* Configuration register */
#define UDMA_CHANNEL_CFG_OFFSET    0x8
/* Int configuration register */
#define UDMA_CHANNEL_INTCFG_OFFSET 0xC

/*
 * The configuration register of the RX and TX parts for each channel can be accessed using the
 * following bits
 */

#define UDMA_CHANNEL_CFG_SHADOW_BIT (5)
#define UDMA_CHANNEL_CFG_CLEAR_BIT  (5)
#define UDMA_CHANNEL_CFG_EN_BIT     (4)
#define UDMA_CHANNEL_CFG_SIZE_BIT   (1)
#define UDMA_CHANNEL_CFG_CONT_BIT   (0)

/* Indicates if a shadow transfer is there */
#define UDMA_CHANNEL_CFG_SHADOW  (1 << UDMA_CHANNEL_CFG_SHADOW_BIT)
/* Stop and clear all pending transfers */
#define UDMA_CHANNEL_CFG_CLEAR   (1 << UDMA_CHANNEL_CFG_CLEAR_BIT)
/* Start a transfer */
#define UDMA_CHANNEL_CFG_EN      (1 << UDMA_CHANNEL_CFG_EN_BIT)
/* Configure for 8-bits transfer */
#define UDMA_CHANNEL_CFG_SIZE_8  (0 << UDMA_CHANNEL_CFG_SIZE_BIT)
/* Configure for 16-bits transfer */
#define UDMA_CHANNEL_CFG_SIZE_16 (1 << UDMA_CHANNEL_CFG_SIZE_BIT)
/* Configure for 32-bits transfer */
#define UDMA_CHANNEL_CFG_SIZE_32 (2 << UDMA_CHANNEL_CFG_SIZE_BIT)
/* Configure for continuous mode */
#define UDMA_CHANNEL_CFG_CONT    (1 << UDMA_CHANNEL_CFG_CONT_BIT)

/* Configuration area offset */
#define UDMA_CONF_OFFSET    0xF80
/* Clock-gating control register */
#define UDMA_CONF_CG_OFFSET 0x00

static inline void plp_udma_cg_set(unsigned int value)
{
	sys_write32(value, ARCHI_SOC_PERIPHERALS_ADDR + ARCHI_UDMA_OFFSET + UDMA_CONF_OFFSET +
				   UDMA_CONF_CG_OFFSET);
}

typedef enum {
	DRIVERS_UDMA_UART,
	DRIVERS_UDMA_I2C,
	DRIVERS_UDMA_SPI,
	DRIVERS_UDMA_MAC,
	DRIVERS_MAX_UDMA_COUNT
} udma_module_t;

void drivers_udma_enable_clock(udma_module_t module, uint32_t instance);
void drivers_udma_disable_clock(udma_module_t module, uint32_t instance);

int32_t drivers_udma_cancel(uint32_t base, uint32_t channel);
int32_t drivers_udma_is_ready(uint32_t base, uint32_t channel);
int32_t drivers_udma_wait_for_finished(uint32_t base, uint32_t channel);
int32_t drivers_udma_wait_for_status(uint32_t base);
int32_t drivers_udma_start(uint32_t base, uint32_t channel, uint32_t saddr, uint32_t size,
			   uint32_t optional_cfg);
int32_t drivers_udma_get_remaining(uint32_t base, uint32_t channel);

typedef enum {
	UDMA_SADDR_REG = 0x00,
	UDMA_SIZE_REG = 0x04,
	UDMA_CFG_REG = 0x08,

} udma_regs_t;

typedef enum {
	UDMA_RX_SADDR_REG = 0x00,
	UDMA_RX_SIZE_REG = 0x04,
	UDMA_RX_CFG_REG = 0x08,

	UDMA_TX_SADDR_REG = 0x10,
	UDMA_TX_SIZE_REG = 0x14,
	UDMA_TX_CFG_REG = 0x18,

	UDMA_STATUS = 0x20,
	UDMA_SETUP_REG = 0x24,
} udma_reg_t;

#define UDMA_RX_DATA_ADDR_INC_SIZE_8  (0x0 << 1)
#define UDMA_RX_DATA_ADDR_INC_SIZE_16 (0x1 << 1)
#define UDMA_RX_DATA_ADDR_INC_SIZE_32 (0x2 << 1)

#define UDMA_RX_CHANNEL 0
#define UDMA_TX_CHANNEL 1

#define UDMA_READ_REG(udma_base, reg)         sys_read32(udma_base + reg)
#define UDMA_WRITE_REG(udma_base, reg, value) sys_write32(value, udma_base + reg)

#define UDMA_CANCEL_RX(udma_base) drivers_udma_cancel(udma_base, UDMA_RX_CHANNEL)
#define UDMA_CANCEL_TX(udma_base) drivers_udma_cancel(udma_base, UDMA_TX_CHANNEL)

#define UDMA_IS_FINISHED_RX(udma_base) drivers_udma_is_ready(udma_base, UDMA_RX_CHANNEL)
#define UDMA_IS_FINISHED_TX(udma_base) drivers_udma_is_ready(udma_base, UDMA_TX_CHANNEL)

#define UDMA_WAIT_FOR_FINISHED_RX(udma_base)                                                       \
	drivers_udma_wait_for_finished(udma_base, UDMA_RX_CHANNEL)
#define UDMA_WAIT_FOR_FINISHED_TX(udma_base)                                                       \
	drivers_udma_wait_for_finished(udma_base, UDMA_TX_CHANNEL)

#define UDMA_START_RX(base, addr, size, cfg)                                                       \
	drivers_udma_start(base, UDMA_RX_CHANNEL, addr, size, cfg)
#define UDMA_START_TX(base, addr, size, cfg)                                                       \
	drivers_udma_start(base, UDMA_TX_CHANNEL, addr, size, cfg)

#define UDMA_GET_REMAINING_RX(base) drivers_udma_get_remaining(base, UDMA_RX_CHANNEL)
#define UDMA_GET_REMAINING_TX(base) drivers_udma_get_remaining(base, UDMA_TX_CHANNEL)

#define UDMA_WAIT_FOR_STATUS_IDLE(udma_base) drivers_udma_wait_for_status(udma_base)

#endif /* GANYMED_SY1XX_UDMA_H */
