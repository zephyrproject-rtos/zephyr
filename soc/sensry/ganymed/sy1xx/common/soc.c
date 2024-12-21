/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc);

#include "soc.h"

/* ITC */
#define ARCHI_ITC_MASK_OFFSET       0x0
#define ARCHI_ITC_MASK_SET_OFFSET   0x4
#define ARCHI_ITC_MASK_CLR_OFFSET   0x8
#define ARCHI_ITC_STATUS_OFFSET     0xc
#define ARCHI_ITC_STATUS_SET_OFFSET 0x10
#define ARCHI_ITC_STATUS_CLR_OFFSET 0x14
#define ARCHI_ITC_ACK_OFFSET        0x18
#define ARCHI_ITC_ACK_SET_OFFSET    0x1c
#define ARCHI_ITC_ACK_CLR_OFFSET    0x20
#define ARCHI_ITC_FIFO_OFFSET       0x24

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}

#define ARCHI_REF_CLOCK (32768)
#define ARCHI_PER_CLOCK (125000000)

uint32_t soc_get_rts_clock_frequency(void)
{
	return ARCHI_REF_CLOCK;
}

uint32_t soc_get_peripheral_clock(void)
{
	return ARCHI_PER_CLOCK;
}

void riscv_clic_irq_priority_set(uint32_t irq, uint32_t prio, uint32_t flags)
{
	/* we do not support priorities */
}

void soc_enable_irq(uint32_t idx)
{
	uint32_t current = sys_read32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_MASK_SET_OFFSET);

	sys_write32(current | (1 << (idx & 0x1f)), ARCHI_FC_ITC_ADDR + ARCHI_ITC_MASK_SET_OFFSET);
}

void soc_disable_irq(uint32_t idx)
{
	uint32_t current = sys_read32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_MASK_CLR_OFFSET);

	sys_write32(current & (~(1 << (idx & 0x1f))),
		    ARCHI_FC_ITC_ADDR + ARCHI_ITC_MASK_CLR_OFFSET);
}

/*
 * SoC-level interrupt initialization. Clear any pending interrupts or
 * events, and find the INTMUX device if necessary.
 *
 * This gets called as almost the first thing z_cstart() does, so it
 * will happen before any calls to the _arch_irq_xxx() routines above.
 */
void soc_interrupt_init(void)
{
}

/**
 * @brief Perform basic hardware initialization
 *
 * Initializes the base clocks and LPFLL using helpers provided by the HAL.
 *
 * @return 0
 */
static int soc_sy1xx_init(void)
{

	return 0;
}

SYS_INIT(soc_sy1xx_init, PRE_KERNEL_1, 0);
