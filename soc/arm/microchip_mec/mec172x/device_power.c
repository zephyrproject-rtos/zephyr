/*
 * Copyright (c) 2019 Microchip Technology Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include "device_power.h"

#define ADC_0_XEC_REG_BASE						\
	((struct adc_regs *)(DT_REG_ADDR(DT_NODELABEL(adc0))))
#define ECIA_XEC_REG_BASE						\
	((struct ecia_named_regs *)(DT_REG_ADDR(DT_NODELABEL(ecia))))
#define ECS_XEC_REG_BASE						\
	((struct ecs_regs *)(DT_REG_ADDR(DT_NODELABEL(ecs))))
#define PECI_XEC_REG_BASE						\
	((struct peci_regs *)(DT_REG_ADDR(DT_NODELABEL(peci0))))
#define PCR_XEC_REG_BASE						\
	((struct pcr_regs *)(DT_REG_ADDR(DT_NODELABEL(pcr))))
#define TFDP_0_XEC_REG_BASE						\
	((struct tfdp_regs *)(DT_REG_ADDR(DT_NODELABEL(tfdp0))))
#define UART_0_XEC_REG_BASE						\
	((struct uart_regs *)(DT_REG_ADDR(DT_NODELABEL(uart0))))
#define UART_1_XEC_REG_BASE						\
	((struct uart_regs *)(DT_REG_ADDR(DT_NODELABEL(uart1))))
#define VBATR_XEC_REG_BASE						\
	((struct vbatr_regs *)(DT_REG_ADDR_BY_NAME(DT_NODELABEL(pcr), vbatr)))
#define VBATM_XEC_BASE_ADDR						\
	((uintptr_t)(DT_REG_ADDR(DT_NODELABEL(bbram))))

#define BTMR16_0_ADDR	DT_REG_ADDR(DT_NODELABEL(timer0))
#define BTMR16_1_ADDR	DT_REG_ADDR(DT_NODELABEL(timer1))
#define BTMR16_2_ADDR	DT_REG_ADDR(DT_NODELABEL(timer2))
#define BTMR16_3_ADDR	DT_REG_ADDR(DT_NODELABEL(timer3))
#define BTMR32_0_ADDR	DT_REG_ADDR(DT_NODELABEL(timer4))
#define BTMR32_1_ADDR	DT_REG_ADDR(DT_NODELABEL(timer5))
#define VBATM_XEC_ADDR	DT_REG_ADDR(DT_NODELABEL(vbr))

#ifdef DEBUG_DEEP_SLEEP_CLK_REQ
void soc_debug_sleep_clk_req(void)
{
	struct ecs_regs *ecs = ECS_XEC_REG_BASE;
	struct pcr_regs *pcr = PCR_XEC_REG_BASE;
	uintptr_t vbm_addr = VBATM_XEC_BASE_ADDR;

	/* Save status to debug LPM been blocked */
	for (int i = 0; i < 5; i++) {
		sys_write32(pcr->CLK_REQ[i], vbm_addr);
		vbm_addr += 4;
	}

	sys_write32(pcr->SYS_SLP_CTRL, vbm_addr);
	vbm_addr += 4;
	sys_write32(ecs->SLP_STS_MIRROR, vbm_addr);
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
#ifdef CONFIG_ESPI
	struct ecia_named_regs *regs = ECIA_XEC_REG_BASE;

	regs->GIRQ22.SRC = UINT32_MAX;
	regs->GIRQ22.EN_SET = MCHP_ESPI_WK_CLK_GIRQ_BIT;
#endif
}

void soc_deep_sleep_non_wake_dis(void)
{
#ifdef CONFIG_ESPI
	struct ecia_named_regs *regs = ECIA_XEC_REG_BASE;

	regs->GIRQ22.EN_CLR = UINT32_MAX;
	regs->GIRQ22.SRC = UINT32_MAX;
#endif
}

/* When MEC172x drivers are power-aware this should be move there */
void soc_deep_sleep_wake_en(void)
{
#if defined(CONFIG_KSCAN) || DT_NODE_HAS_STATUS(DT_NODELABEL(ps2_0), okay)
	struct ecia_named_regs *regs = ECIA_XEC_REG_BASE;
#if defined(CONFIG_KSCAN)
	/* Enable PLL wake via KSCAN  */
	regs->GIRQ21.SRC = MCHP_KEYSCAN_GIRQ_BIT;
	regs->GIRQ21.EN_SET = MCHP_KEYSCAN_GIRQ_BIT;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(ps2_0), okay)
	/* Enable PS2_0B_WK */
	regs->GIRQ21.SRC = MCHP_PS2_0_PORT0B_WK_GIRQ_BIT;
	regs->GIRQ21.EN_SET = MCHP_PS2_0_PORT0B_WK_GIRQ_BIT;
#endif
#endif
}

void soc_deep_sleep_wake_dis(void)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(ps2_0), okay)
	struct ecia_named_regs *regs = ECIA_XEC_REG_BASE;

	/* Enable PS2_0B_WK */
	regs->GIRQ21.EN_CLR = MCHP_PS2_0_PORT0B_WK_GIRQ_BIT;
	regs->GIRQ21.SRC = MCHP_PS2_0_PORT0B_WK_GIRQ_BIT;
#endif
}


/* Variables used to save various HW state */
#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE

const struct ds_timer_info ds_timer_tbl[NUM_DS_TIMER_ENTRIES] = {
	{
		(uintptr_t)(BTMR16_0_ADDR + MCHP_BTMR_CTRL_OFS),
		MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)(BTMR16_1_ADDR + MCHP_BTMR_CTRL_OFS),
		MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)(BTMR16_2_ADDR + MCHP_BTMR_CTRL_OFS),
		MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)(BTMR16_3_ADDR + MCHP_BTMR_CTRL_OFS),
		MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)(BTMR32_0_ADDR + MCHP_BTMR_CTRL_OFS),
		MCHP_BTMR_CTRL_HALT, 0
	},
	{
		(uintptr_t)(BTMR32_1_ADDR + MCHP_BTMR_CTRL_OFS),
		MCHP_BTMR_CTRL_HALT, 0
	},
};

static struct ds_dev_info ds_ctx;

static void deep_sleep_save_ecs(void)
{
	struct ecs_regs *regs = ECS_XEC_REG_BASE;

	ds_ctx.ecs[0] = regs->ETM_CTRL;
	ds_ctx.ecs[1] = regs->DEBUG_CTRL;
#ifdef DEEP_SLEEP_JTAG
	regs->ETM_CTRL = 0;
	regs->DEBUG_CTRL = 0x00;
#endif
}

#ifdef DEEP_SLEEP_UART_SAVE_RESTORE
static void deep_sleep_save_uarts(void)
{
	struct uart_regs *regs = UART_0_XEC_REG_BASE;

	ds_ctx.uart_info[0] = regs->ACTV;
	if (ds_ctx.uart_info[0]) {
		while ((regs->LSR & MCHP_UART_LSR_TEMT) == 0) {
		}
	}
	regs->ACTV = 0;

	regs = UART_1_XEC_REG_BASE;
	ds_ctx.uart_info[1] = regs->ACTV;
	if (ds_ctx.uart_info[1]) {
		while ((regs->LSR & MCHP_UART_LSR_TEMT) == 0) {
		}
	}
	regs->ACTV = 0;
}
#endif

static void deep_sleep_save_timers(void)
{
	const struct ds_timer_info *p;
	uint32_t i;

	p = &ds_timer_tbl[0];
	for (i = 0; i < NUM_DS_TIMER_ENTRIES; i++) {
		ds_ctx.timers[i] = sys_read32(p->addr);
		if (p->stop_mask) {
			sys_write32(ds_ctx.timers[i] | p->stop_mask, p->addr);
		} else {
			sys_write32(0, p->addr);
		}
		p++;
	}
}

static void deep_sleep_restore_ecs(void)
{
#ifdef DEEP_SLEEP_JTAG
	struct ecs_regs *regs = ECS_XEC_REG_BASE;

	regs->ETM_CTRL = ds_ctx.ecs[0];
	regs->DEBUG_CTRL = ds_ctx.ecs[1];
#endif
}

#ifdef DEEP_SLEEP_UART_SAVE_RESTORE
static void deep_sleep_restore_uarts(void)
{
	struct uart_regs *regs0 = UART_0_XEC_REG_BASE;
	struct uart_regs *regs1 = UART_1_XEC_REG_BASE;

	regs0->ACTV = ds_ctx.uart_info[0];
	regs1->ACTV = ds_ctx.uart_info[1];
}
#endif

static void deep_sleep_restore_timers(void)
{
	const struct ds_timer_info *p;
	uint32_t i, temp;

	p = &ds_timer_tbl[0];
	for (i = 0; i < NUM_DS_TIMER_ENTRIES; i++) {
		if (p->stop_mask) {
			temp = sys_read32(p->addr) & ~(p->stop_mask);
			sys_write32(temp, p->addr);
		} else {
			sys_write32(ds_ctx.timers[i] & ~p->restore_mask,
				    p->addr);
		}
		p++;
	}
}

#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED

static void deep_sleep_save_blocks(void)
{
	struct tfdp_regs *tfdp = TFDP_0_XEC_REG_BASE;
	struct ecs_regs *ecs = ECS_XEC_REG_BASE;
#ifdef CONFIG_ADC
	struct adc_regs *adc0 = ADC_0_XEC_REG_BASE;

	/* ADC deactivate  */
	adc0->CONTROL &= ~(MCHP_ADC_CTRL_ACTV);
#endif

#ifdef CONFIG_PECI
	struct peci_regs *peci = PECI_XEC_REG_BASE;

	ds_ctx.peci_info.peci_ctrl = peci->CONTROL;
	ds_ctx.peci_info.peci_dis = ecs->PECI_DIS;
	ecs->PECI_DIS |= MCHP_ECS_PECI_DISABLE;
#endif

#ifdef CONFIG_I2C
	for (size_t n = 0; n < MCHP_I2C_SMB_INSTANCES; n++) {
		uint32_t addr = MCHP_I2C_SMB_BASE_ADDR(n) +
				MCHP_I2C_SMB_CFG_OFS;
		uint32_t regval = sys_read32(addr);

		ds_ctx.smb_info[n] = regval;
		sys_write32(regval & ~(MCHP_I2C_SMB_CFG_ENAB), addr);
	}
#endif

	/* Disable comparator if enabled */
	if (ecs->CMP_CTRL & BIT(0)) {
		ds_ctx.comp_en = 1;
		ecs->CMP_CTRL &= ~(MCHP_ECS_ACC_EN0);
	}

#if defined(CONFIG_TACH_XEC) || defined(CONFIG_PWM_XEC)
	struct pcr_regs *pcr = PCR_XEC_REG_BASE;

	/* This low-speed clock derived from the 48MHz clock domain is used as
	 * a time base for PWMs and TACHs
	 * Set SLOW_CLOCK_DIVIDE = CLKOFF to save additional power
	 */
	ds_ctx.slwclk_info = pcr->SLOW_CLK_CTRL;
	pcr->SLOW_CLK_CTRL &= (~MCHP_PCR_SLOW_CLK_CTRL_100KHZ &
				MCHP_PCR_SLOW_CLK_CTRL_MASK);
#endif

	/* TFDP HW block is not expose to any Zephyr subsystem */
	if (tfdp->CTRL & MCHP_TFDP_CTRL_EN) {
		ds_ctx.tfdp_en = 1;
		tfdp->CTRL &= ~MCHP_TFDP_CTRL_EN;
	}

	/* Port 80 TODO Do we need to do anything? MEC172x BDP does not
	 * include a timer so it should de-assert its CLK_REQ in response
	 * to SLP_EN 0->1.
	 */
}

static void deep_sleep_restore_blocks(void)
{
	struct tfdp_regs *tfdp = TFDP_0_XEC_REG_BASE;
	struct ecs_regs *ecs = ECS_XEC_REG_BASE;
#ifdef CONFIG_ADC
	struct adc_regs *adc0 = ADC_0_XEC_REG_BASE;

	adc0->CONTROL |= MCHP_ADC_CTRL_ACTV;
#endif

#ifdef CONFIG_PECI
	struct peci_regs *peci = PECI_XEC_REG_BASE;

	ecs->PECI_DIS = ds_ctx.peci_info.peci_dis;
	peci->CONTROL = ds_ctx.peci_info.peci_ctrl;
#endif

#ifdef CONFIG_I2C
	for (size_t n = 0; n < MCHP_I2C_SMB_INSTANCES; n++) {
		uint32_t addr = MCHP_I2C_SMB_BASE_ADDR(n) +
				MCHP_I2C_SMB_CFG_OFS;

		sys_write32(ds_ctx.smb_info[n], addr);
	}
#endif
	/* Restore comparator control values */
	if (ds_ctx.comp_en) {
		ecs->CMP_CTRL |= MCHP_ECS_ACC_EN0;
	}

#if defined(CONFIG_TACH_XEC) || defined(CONFIG_PWM_XEC)
	struct pcr_regs *pcr = PCR_XEC_REG_BASE;

	/* Restore slow clock control */
	pcr->SLOW_CLK_CTRL = ds_ctx.slwclk_info;
#endif

	/* TFDP HW block is not expose to any Zephyr subsystem */
	if (ds_ctx.tfdp_en) {
		tfdp->CTRL |= MCHP_TFDP_CTRL_EN;
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
