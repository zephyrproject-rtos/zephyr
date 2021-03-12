/*
 * Copyright (c) 2019 Microchip Technology Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <power/power.h>
#include <soc.h>
#include "device_power.h"

/*
 * Light sleep: PLL remains on. Fastest wake latency.
 */
void soc_lite_sleep_enable(void)
{
	SCB->SCR &= ~(1ul << 2);
	PCR_REGS->SYS_SLP_CTRL = MCHP_PCR_SYS_SLP_LIGHT;
}

/*
 * Deep sleep: PLL is turned off. Wake is fast. PLL requires
 * a minimum of 3ms to lock. During this time the main clock
 * will be ramping up from ~16 to 24 MHz.
 */
void soc_deep_sleep_enable(void)
{
	SCB->SCR = (1ul << 2); /* Cortex-M4 SLEEPDEEP */
	PCR_REGS->SYS_SLP_CTRL = MCHP_PCR_SYS_SLP_HEAVY;
}

void soc_deep_sleep_disable(void)
{
	SCB->SCR &= ~(1ul << 2); /* disable Cortex-M4 SLEEPDEEP */
}


void soc_deep_sleep_wait_clk_idle(void)
{
#ifdef DEEP_SLEEP_WAIT_ON_CLK_REQ_ENABLE
	uint32_t clkreq, cnt;

	cnt = DEEP_SLEEP_WAIT_SPIN_CLK_REQ;
	do {
		clkreq = PCR_REGS->CLK_REQ0 | PCR_REGS->CLK_REQ1
			 | PCR_REGS->CLK_REQ2 | PCR_REGS->CLK_REQ3
			 | PCR_REGS->CLK_REQ4;
	} while ((clkreq != (1ul << MCHP_PCR1_CPU_POS)) && (cnt-- != 0));
#endif

#ifdef DEEP_SLEEP_CLK_REQ_DUMP
	/* Save status to debug LPM been blocked */
	VBATM_REGS->MEM.u32[0] = PCR_REGS->CLK_REQ0;
	VBATM_REGS->MEM.u32[1] = PCR_REGS->CLK_REQ1;
	VBATM_REGS->MEM.u32[2] = PCR_REGS->CLK_REQ2;
	VBATM_REGS->MEM.u32[3] = PCR_REGS->CLK_REQ3;
	VBATM_REGS->MEM.u32[4] = PCR_REGS->CLK_REQ4;
#endif
}

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
	GIRQ22_REGS->EN_SET = (1ul << 9);
#endif
}

void soc_deep_sleep_non_wake_dis(void)
{
#ifdef CONFIG_ESPI_XEC
	GIRQ22_REGS->EN_CLR = 0xfffffffful;
	GIRQ22_REGS->SRC = 0xfffffffful;
#endif
}

/* Variables used to save various HW state */
#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE

const struct ds_timer_info ds_timer_tbl[] = {
	{
		(uintptr_t)&B16TMR0_REGS->CTRL, 0
	},
	{
		(uintptr_t)&B16TMR1_REGS->CTRL, 0
	},
	{
		(uintptr_t)&B32TMR0_REGS->CTRL, 0
	},
	{
		(uintptr_t)&B32TMR1_REGS->CTRL, 0
	},
	{
		(uintptr_t)&CCT_REGS->CTRL,
		(MCHP_CCT_CTRL_COMP1_SET | MCHP_CCT_CTRL_COMP0_SET),
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
	ds_ctx.uart_info[2] = UART2_REGS->ACTV;
	if (ds_ctx.uart_info[2]) {
		while ((UART2_REGS->LSR & MCHP_UART_LSR_TEMT) == 0) {
		}
	}

	UART2_REGS->ACTV = 0;
	UART1_REGS->ACTV = 0;
	UART0_REGS->ACTV = 0;
}

static void deep_sleep_save_timers(void)
{
	const struct ds_timer_info *p;
	uint32_t i;

	p = &ds_timer_tbl[0];
	for (i = 0; i < NUM_DS_TIMER_ENTRIES; i++) {
		ds_ctx.timers[i] = REG32(p->addr);
		REG32(p->addr) = 0;
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

static void deep_sleep_restore_uarts(void)
{
	UART0_REGS->ACTV = ds_ctx.uart_info[0];
	if (ds_ctx.uart_info[0]) {
		mchp_pcr_periph_slp_ctrl(PCR_UART0, MCHP_PCR_SLEEP_DIS);
	}

	UART1_REGS->ACTV = ds_ctx.uart_info[1];
	if (ds_ctx.uart_info[1]) {
		mchp_pcr_periph_slp_ctrl(PCR_UART1, MCHP_PCR_SLEEP_DIS);
	}

	UART2_REGS->ACTV = ds_ctx.uart_info[2];
	if (ds_ctx.uart_info[2]) {
		mchp_pcr_periph_slp_ctrl(PCR_UART2, MCHP_PCR_SLEEP_DIS);
	}
}

static void deep_sleep_restore_timers(void)
{
	const struct ds_timer_info *p;
	uint32_t i;

	p = &ds_timer_tbl[0];
	for (i = 0; i < NUM_DS_TIMER_ENTRIES; i++) {
		REG32(p->addr) = ds_ctx.timers[i] & ~p->restore_mask;
		p++;
	}
}

#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED

static void deep_sleep_save_blocks(void)
{
	/* ADC Power saving feature is enabled */
	ds_ctx.adc_info.adc_ctrl = ADC_REGS->CONTROL;
	ds_ctx.adc_info.adc_cfg = ADC_REGS->CONFIG;
	ADC_REGS->CONTROL = 0;
	ADC_REGS->CONFIG = 0;

	ds_ctx.peci_info.peci_ctrl = PECI_REGS->CONTROL;
	ds_ctx.peci_info.peci_dis = ECS_REGS->PECI_DIS;
	PECI_REGS->CONTROL = 0x08;
	PECI_REGS->CONTROL = 0x09;
	ECS_REGS->PECI_DIS = 0x01;

	/* SMBUS */
	ds_ctx.smb_info[0] = SMB0_REGS->CTRLSTS;
	ds_ctx.smb_info[1] = SMB1_REGS->CTRLSTS;
	ds_ctx.smb_info[2] = SMB2_REGS->CTRLSTS;
	ds_ctx.smb_info[3] = SMB3_REGS->CTRLSTS;
	ds_ctx.smb_info[4] = SMB4_REGS->CTRLSTS;

	mchp_pcr_periph_slp_ctrl(PCR_SMB1, MCHP_PCR_SLEEP_EN);
	mchp_pcr_periph_slp_ctrl(PCR_SMB2, MCHP_PCR_SLEEP_EN);
	mchp_pcr_periph_slp_ctrl(PCR_SMB2, MCHP_PCR_SLEEP_EN);
	mchp_pcr_periph_slp_ctrl(PCR_SMB3, MCHP_PCR_SLEEP_EN);
	mchp_pcr_periph_slp_ctrl(PCR_SMB4, MCHP_PCR_SLEEP_EN);

	mchp_pcr_periph_slp_ctrl(PCR_ESPI, MCHP_PCR_SLEEP_EN);

	/* Comparators */
	ds_ctx.comp_info = ECS_REGS->CMP_SLP_CTRL;
	ECS_REGS->CMP_SLP_CTRL = 0;

	/* Disable VCI_INPUT_ENABLE */
	ds_ctx.vci_info = VCI_REGS->INPUT_EN;
	VCI_REGS->INPUT_EN = 0;

	/* Disable WEEK TIMER */
	ds_ctx.wktmr_info = WKTMR_REGS->BGPO_PWR;
	WKTMR_REGS->BGPO_PWR = 0;

	/* Set SLOW_CLOCK_DIVIDE = CLKOFF */
	ds_ctx.slwclk_info = PCR_REGS->SLOW_CLK_CTRL;
	PCR_REGS->SLOW_CLK_CTRL &= (~MCHP_PCR_SLOW_CLK_CTRL_100KHZ & MCHP_PCR_SLOW_CLK_CTRL_MASK);

	ds_ctx.tfdp_info = TFDP_REGS->CTRL;
	TFDP_REGS->CTRL = 0;

	/* Port 80 */
	ds_ctx.port80_info[0] = PORT80_CAP0_REGS->ACTV;
	ds_ctx.port80_info[1] = PORT80_CAP1_REGS->ACTV;
	PORT80_CAP0_REGS->ACTV = 0;
	PORT80_CAP1_REGS->ACTV = 0;
}

static void deep_sleep_restore_blocks(void)
{
	ADC_REGS->CONTROL = ds_ctx.adc_info.adc_ctrl;
	ADC_REGS->CONFIG = ds_ctx.adc_info.adc_cfg;

	ECS_REGS->PECI_DIS = ds_ctx.peci_info.peci_dis;
	PECI_REGS->CONTROL = MCHP_PECI_CTRL_PD;
	PECI_REGS->CONTROL = ds_ctx.peci_info.peci_ctrl;
	PECI_REGS->CONTROL &= ~MCHP_PECI_CTRL_PD;

	SMB0_REGS->CTRLSTS = ds_ctx.smb_info[0];
	SMB1_REGS->CTRLSTS = ds_ctx.smb_info[1];
	SMB2_REGS->CTRLSTS = ds_ctx.smb_info[2];
	SMB3_REGS->CTRLSTS = ds_ctx.smb_info[3];
	SMB4_REGS->CTRLSTS = ds_ctx.smb_info[4];

	mchp_pcr_periph_slp_ctrl(PCR_SMB1, MCHP_PCR_SLEEP_DIS);
	mchp_pcr_periph_slp_ctrl(PCR_SMB2, MCHP_PCR_SLEEP_DIS);
	mchp_pcr_periph_slp_ctrl(PCR_SMB2, MCHP_PCR_SLEEP_DIS);
	mchp_pcr_periph_slp_ctrl(PCR_SMB3, MCHP_PCR_SLEEP_DIS);
	mchp_pcr_periph_slp_ctrl(PCR_SMB4, MCHP_PCR_SLEEP_DIS);

	mchp_pcr_periph_slp_ctrl(PCR_ESPI, MCHP_PCR_SLEEP_DIS);

	/* Comparators */
	ECS_REGS->CMP_SLP_CTRL = ds_ctx.comp_info;

	/* Disable VCI_INPUT_ENABLE */
	VCI_REGS->INPUT_EN = ds_ctx.vci_info;

	/* Disable WEEK TIMER */
	WKTMR_REGS->BGPO_PWR = ds_ctx.wktmr_info;

	PCR_REGS->SLOW_CLK_CTRL = ds_ctx.slwclk_info;
	TFDP_REGS->CTRL = ds_ctx.tfdp_info;

	/* Port 80 */
	PORT80_CAP0_REGS->ACTV = ds_ctx.port80_info[0];
	PORT80_CAP1_REGS->ACTV = ds_ctx.port80_info[1];
}
#endif /* DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED */

void soc_deep_sleep_periph_save(void)
{
#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED
	deep_sleep_save_blocks();
#endif
	deep_sleep_save_ecs();
	deep_sleep_save_timers();
	deep_sleep_save_uarts();
}

void soc_deep_sleep_periph_restore(void)
{
	deep_sleep_restore_ecs();
	deep_sleep_restore_uarts();
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
