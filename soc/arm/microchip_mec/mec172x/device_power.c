/*
 * Copyright (c) 2019 Microchip Technology Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <pm/pm.h>
#include <soc.h>
#include "device_power.h"

#ifdef DEBUG_DEEP_SLEEP_CLK_REQ
void soc_debug_sleep_clk_req(void)
{
	/* Save status to debug LPM been blocked */
	VBATM_REGS->MEM.u32[0] = PCR_REGS->CLK_REQ[0];
	VBATM_REGS->MEM.u32[1] = PCR_REGS->CLK_REQ[1];
	VBATM_REGS->MEM.u32[2] = PCR_REGS->CLK_REQ[2];
	VBATM_REGS->MEM.u32[3] = PCR_REGS->CLK_REQ[3];
	VBATM_REGS->MEM.u32[4] = PCR_REGS->CLK_REQ[4];
	VBATM_REGS->MEM.u32[5] = PCR_REGS->SYS_SLP_CTRL;
	VBATM_REGS->MEM.u32[6] = ECS_REGS->SLP_STS_MIRROR;
}
#endif

/*
 * Allow peripherals connected to external masters to wake the PLL but not
 * the EC. Once the peripheral has serviced the external master the PLL
 * will be turned back off. For example, if the eSPI master requests eSPI
 * configuration information or state of virtual wires the EC doesn't need
 * to be involved. The hardware can power on the PLL long enough to service
 * the request and then turn the PLL back off.  The SMBus and I2C peripherals
 * in slave mode can also make use of this feature.
 */
void soc_deep_sleep_non_wake_en(void)
{
#ifdef CONFIG_ESPI_XEC
	GIRQ22_REGS->SRC = 0xfffffffful;
	GIRQ22_REGS->EN_SET = MCHP_ESPI_WK_CLK_GIRQ_BIT;
#endif
}

void soc_deep_sleep_non_wake_dis(void)
{
#ifdef CONFIG_ESPI_XEC
	GIRQ22_REGS->EN_CLR = 0xfffffffful;
	GIRQ22_REGS->SRC = 0xfffffffful;
#endif
}

/* When MEC172x drivers are power-aware this should be move there */
void soc_deep_sleep_wake_en(void)
{
#if defined(CONFIG_KSCAN)
	/* Enable PLL wake via KSCAN  */
	GIRQ21_REGS->SRC = MCHP_KEYSCAN_GIRQ_BIT;
	GIRQ21_REGS->EN_SET = MCHP_KEYSCAN_GIRQ_BIT;
#endif
#if defined(CONFIG_PS2_XEC_0)
	/* Enable PS2_0B_WK */
	GIRQ21_REGS->SRC = MCHP_PS2_0_PORT0B_WK_GIRQ_BIT;
	GIRQ21_REGS->EN_SET = MCHP_PS2_0_PORT0B_WK_GIRQ_BIT;
#endif
}

void soc_deep_sleep_wake_dis(void)
{
#ifdef CONFIG_PS2_XEC_0
	/* Enable PS2_0B_WK */
	GIRQ21_REGS->EN_CLR = MCHP_PS2_0_PORT0B_WK_GIRQ_BIT;
	GIRQ21_REGS->SRC = MCHP_PS2_0_PORT0B_WK_GIRQ_BIT;
#endif
}


/* Variables used to save various HW state */
#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE

const struct ds_timer_info ds_timer_tbl[NUM_DS_TIMER_ENTRIES] = {
	{
		(uintptr_t)&BTMR16_0_REGS->CTRL, MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)&BTMR16_1_REGS->CTRL, MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)&BTMR16_2_REGS->CTRL, MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)&BTMR16_3_REGS->CTRL, MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)&BTMR32_0_REGS->CTRL, MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)&BTMR32_1_REGS->CTRL, MCHP_BTMR_CTRL_HALT, 0
	},
};

static struct ds_dev_info ds_ctx;

static void deep_sleep_save_ecs(void)
{
	ds_ctx.ecs[0] = ECS_REGS->ETM_CTRL;
	ds_ctx.ecs[1] = ECS_REGS->DEBUG_CTRL;
#ifdef DEEP_SLEEP_JTAG
	ECS_REGS->ETM_CTRL = 0;
	ECS_REGS->DEBUG_CTRL = 0x00;
#endif
}

#ifdef DEEP_SLEEP_UART_SAVE_RESTORE
static void deep_sleep_save_uarts(void)
{
	ds_ctx.uart_info[0] = UART0_REGS->ACTV;
	if (ds_ctx.uart_info[0]) {
		while ((UART0_REGS->LSR & MCHP_UART_LSR_TEMT) == 0) {
		}
	}
	UART0_REGS->ACTV = 0;
	ds_ctx.uart_info[1] = UART1_REGS->ACTV;
	if (ds_ctx.uart_info[1]) {
		while ((UART1_REGS->LSR & MCHP_UART_LSR_TEMT) == 0) {
		}
	}
	UART1_REGS->ACTV = 0;
}
#endif

static void deep_sleep_save_timers(void)
{
	const struct ds_timer_info *p;
	uint32_t i;

	p = &ds_timer_tbl[0];
	for (i = 0; i < NUM_DS_TIMER_ENTRIES; i++) {
		ds_ctx.timers[i] = REG32(p->addr);
		if (p->stop_mask) {
			REG32(p->addr) = ds_ctx.timers[i] | p->stop_mask;
		} else {
			REG32(p->addr) = 0U;
		}
		p++;
	}
}

static void deep_sleep_restore_ecs(void)
{
#ifdef DEEP_SLEEP_JTAG
	ECS_REGS->ETM_CTRL = ds_ctx.ecs[0];
	ECS_REGS->DEBUG_CTRL = ds_ctx.ecs[1];
#endif
}

#ifdef DEEP_SLEEP_UART_SAVE_RESTORE
static void deep_sleep_restore_uarts(void)
{
	UART0_REGS->ACTV = ds_ctx.uart_info[0];
	UART1_REGS->ACTV = ds_ctx.uart_info[1];
}
#endif

static void deep_sleep_restore_timers(void)
{
	const struct ds_timer_info *p;
	uint32_t i;

	p = &ds_timer_tbl[0];
	for (i = 0; i < NUM_DS_TIMER_ENTRIES; i++) {
		if (p->stop_mask) {
			REG32(p->addr) &= ~(p->stop_mask);
		} else {
			REG32(p->addr) = ds_ctx.timers[i] & ~p->restore_mask;
		}
		p++;
	}
}

#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED

static void deep_sleep_save_blocks(void)
{
#ifdef CONFIG_ADC
	/* ADC deactivate  */
	ADC_0_REGS->CONTROL &= ~(MCHP_ADC_CTRL_ACTV);
#endif

#ifdef CONFIG_PECI
	ds_ctx.peci_info.peci_ctrl = PECI_REGS->CONTROL;
	ds_ctx.peci_info.peci_dis = ECS_REGS->PECI_DIS;
	ECS_REGS->PECI_DIS |= MCHP_ECS_PECI_DISABLE;
#endif

#ifdef CONFIG_I2C
	for (size_t n = 0; n > MCHP_I2C_SMB_INSTANCES; n++) {
		uint32_t addr = MCHP_I2C_SMB_BASE_ADDR(n) +
				MCHP_I2C_SMB_CFG_OFS;
		uint32_t regval = REG32(addr);

		ds_ctx.smb_info[n] = regval;
		REG32(addr) = regval & ~(MCHP_I2C_SMB_CFG_ENAB);
	}
#endif

	/* Disable comparator if enabled */
	if (ECS_REGS->CMP_CTRL & BIT(0)) {
		ds_ctx.comp_en = 1;
		ECS_REGS->CMP_CTRL &= ~(MCHP_ECS_ACC_EN0);
	}

#if defined(CONFIG_TACH_XEC) || defined(CONFIG_PWM_XEC)
	/* This low-speed clock derived from the 48MHz clock domain is used as
	 * a time base for PWMs and TACHs
	 * Set SLOW_CLOCK_DIVIDE = CLKOFF to save additional power
	 */
	ds_ctx.slwclk_info = PCR_REGS->SLOW_CLK_CTRL;
	PCR_REGS->SLOW_CLK_CTRL &= (~MCHP_PCR_SLOW_CLK_CTRL_100KHZ &
				    MCHP_PCR_SLOW_CLK_CTRL_MASK);
#endif

	/* TFDP HW block is not expose to any Zephyr subsystem */
	if (TFDP_0_REGS->CTRL & MCHP_TFDP_CTRL_EN) {
		ds_ctx.tfdp_en = 1;
		TFDP_0_REGS->CTRL &= ~MCHP_TFDP_CTRL_EN;
	}

	/* Port 80 TODO Do we need to do anything? MEC172x BDP does not
	 * include a timer so it should de-assert its CLK_REQ in response
	 * to SLP_EN 0->1.
	 */
}

static void deep_sleep_restore_blocks(void)
{
#ifdef CONFIG_ADC
	ADC_0_REGS->CONTROL |= MCHP_ADC_CTRL_ACTV;
#endif

#ifdef CONFIG_PECI
	ECS_REGS->PECI_DIS = ds_ctx.peci_info.peci_dis;
	PECI_REGS->CONTROL = ds_ctx.peci_info.peci_ctrl;
#endif

#ifdef CONFIG_I2C
	for (size_t n = 0; n > MCHP_I2C_SMB_INSTANCES; n++) {
		uint32_t addr = MCHP_I2C_SMB_BASE_ADDR(n) +
				MCHP_I2C_SMB_CFG_OFS;

		REG32(addr) = ds_ctx.smb_info[n];
	}
#endif
	/* Restore comparator control values */
	if (ds_ctx.comp_en) {
		ECS_REGS->CMP_CTRL |= MCHP_ECS_ACC_EN0;
	}

#if defined(CONFIG_TACH_XEC) || defined(CONFIG_PWM_XEC)
	/* Restore slow clock control */
	PCR_REGS->SLOW_CLK_CTRL = ds_ctx.slwclk_info;
#endif

	/* TFDP HW block is not expose to any Zephyr subsystem */
	if (ds_ctx.tfdp_en) {
		TFDP_0_REGS->CTRL |= MCHP_TFDP_CTRL_EN;
	}
}
#endif /* DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED */

void soc_deep_sleep_periph_save(void)
{
#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED
	deep_sleep_save_blocks();
#endif
	deep_sleep_save_ecs();
	deep_sleep_save_timers();
#ifdef DEEP_SLEEP_UART_SAVE_RESTORE
	deep_sleep_save_uarts();
#endif
}

void soc_deep_sleep_periph_restore(void)
{
	deep_sleep_restore_ecs();
#ifdef DEEP_SLEEP_UART_SAVE_RESTORE
	deep_sleep_restore_uarts();
#endif
	deep_sleep_restore_timers();
#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED
	deep_sleep_restore_blocks();
#endif
}

#else

void soc_deep_sleep_periph_save(void)
{
}

void soc_deep_sleep_periph_restore(void)
{
}

#endif /* DEEP_SLEEP_PERIPH_SAVE_RESTORE */
