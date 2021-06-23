/*
 * Copyright (c) 2021 Tokita, Hiroshi <toita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd32_rcu

#include <devicetree.h>
#include <device.h>

#include <soc.h>
#include <gd32vf103_rcu.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/gd32_clock_control.h>
#include <sys/util.h>
#include <sys/__assert.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control);

#define GD32_RCU_REG(periph)                 (REG32(RCU + ((uint32_t)(periph))))

static inline void periph_clock_enable(uint32_t bus, uint32_t periph)
{
	GD32_RCU_REG(bus) |= BIT(periph);
}

static inline void periph_clock_disable(uint32_t bus, uint32_t periph)
{
	GD32_RCU_REG(bus) &= ~BIT(periph);
}

static uint32_t clock_freq_get(uint32_t clock)
{
	uint32_t sws, ck_freq = 0U;
	uint32_t cksys_freq, ahb_freq, apb1_freq, apb2_freq;
	uint32_t pllsel, predv0sel, pllmf, ck_src, idx, clk_exp;
	uint32_t predv0, predv1, pll1mf;

	/* exponent of AHB, APB1 and APB2 clock divider */
	uint8_t ahb_exp[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
	uint8_t apb1_exp[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };
	uint8_t apb2_exp[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };

	sws = GET_BITS(RCU_CFG0, 2, 3);
	switch (sws) {
	/* IRC8M is selected as CK_SYS */
	case SEL_IRC8M:
		cksys_freq = IRC8M_VALUE;
		break;
	/* HXTAL is selected as CK_SYS */
	case SEL_HXTAL:
		cksys_freq = HXTAL_VALUE;
		break;
	/* PLL is selected as CK_SYS */
	case SEL_PLL:
		/* PLL clock source selection, HXTAL or IRC8M/2 */
		pllsel = (RCU_CFG0 & RCU_CFG0_PLLSEL);

		if (RCU_PLLSRC_HXTAL == pllsel) {
			/* PLL clock source is HXTAL */
			ck_src = HXTAL_VALUE;

			predv0sel = (RCU_CFG1 & RCU_CFG1_PREDV0SEL);
			/* source clock use PLL1 */
			if (RCU_PREDV0SRC_CKPLL1 == predv0sel) {
				predv1 = (uint32_t)((RCU_CFG1 & RCU_CFG1_PREDV1) >> 4) + 1U;
				pll1mf = (uint32_t)((RCU_CFG1 & RCU_CFG1_PLL1MF) >> 8) + 2U;
				if (17U == pll1mf) {
					pll1mf = 20U;
				}
				ck_src = (ck_src / predv1) * pll1mf;
			}
			predv0 = (RCU_CFG1 & RCU_CFG1_PREDV0) + 1U;
			ck_src /= predv0;
		} else {
			/* PLL clock source is IRC8M/2 */
			ck_src = IRC8M_VALUE / 2U;
		}

		/* PLL multiplication factor */
		pllmf = GET_BITS(RCU_CFG0, 18, 21);
		if ((RCU_CFG0 & RCU_CFG0_PLLMF_4)) {
			pllmf |= 0x10U;
		}
		if (pllmf < 15U) {
			pllmf += 2U;
		} else {
			pllmf += 1U;
		}

		cksys_freq = ck_src * pllmf;

		if (15U == pllmf) {
			/* PLL source clock multiply by 6.5 */
			cksys_freq = ck_src * 6U + ck_src / 2U;
		}

		break;
	/* IRC8M is selected as CK_SYS */
	default:
		cksys_freq = IRC8M_VALUE;
		break;
	}

	/* calculate AHB clock frequency */
	idx = GET_BITS(RCU_CFG0, 4, 7);
	clk_exp = ahb_exp[idx];
	ahb_freq = cksys_freq >> clk_exp;

	/* calculate APB1 clock frequency */
	idx = GET_BITS(RCU_CFG0, 8, 10);
	clk_exp = apb1_exp[idx];
	apb1_freq = ahb_freq >> clk_exp;

	/* calculate APB2 clock frequency */
	idx = GET_BITS(RCU_CFG0, 11, 13);
	clk_exp = apb2_exp[idx];
	apb2_freq = ahb_freq >> clk_exp;

	/* return the clocks frequency */
	switch (clock) {
	case GD32_CLOCK_BUS_SYS:
		ck_freq = cksys_freq;
		break;
	case GD32_CLOCK_BUS_AHB:
		ck_freq = ahb_freq;
		break;
	case GD32_CLOCK_BUS_APB1:
		ck_freq = apb1_freq;
		break;
	case GD32_CLOCK_BUS_APB2:
		ck_freq = apb2_freq;
		break;
	default:
		break;
	}
	return ck_freq;
}

static int gd32_clock_control_on(const struct device *dev,
				 clock_control_subsys_t sub_system)
{
	struct gd32_pclken *pclken = (struct gd32_pclken *)sub_system;

	periph_clock_enable(pclken->bus, pclken->enr);

	return 0;
}

static int gd32_clock_control_off(const struct device *dev,
				  clock_control_subsys_t sub_system)
{
	struct gd32_pclken *pclken = (struct gd32_pclken *)sub_system;

	periph_clock_disable(pclken->bus, pclken->enr);
	return 0;
}

static int gd32_clock_control_get_subsys_rate(const struct device *dev,
					      clock_control_subsys_t sub_system,
					      uint32_t *rate)
{
	struct gd32_pclken *pclken = (struct gd32_pclken *)sub_system;

	*rate = clock_freq_get(pclken->bus);

	return 0;
}

static struct clock_control_driver_api gd32_clock_control_api = {
	.on = gd32_clock_control_on,
	.off = gd32_clock_control_off,
	.get_rate = gd32_clock_control_get_subsys_rate,
};


static int gd32_clock_control_init(const struct device *dev)
{
	/* ToDo: add code to initialize the system
	 * Warn: do not use global variables because this function is called before
	 * reaching pre-main. RW section maybe overwritten afterwards.
	 */
	/* reset the RCC clock configuration to the default reset state */
	/* enable IRC8M */
	RCU_CTL |= RCU_CTL_IRC8MEN;

	/* reset SCS, AHBPSC, APB1PSC, APB2PSC, ADCPSC, CKOUT0SEL bits */
	RCU_CFG0 &= ~(RCU_CFG0_SCS | RCU_CFG0_AHBPSC | RCU_CFG0_APB1PSC |
		      RCU_CFG0_APB2PSC | RCU_CFG0_ADCPSC | RCU_CFG0_ADCPSC_2 |
		      RCU_CFG0_CKOUT0SEL);

	/* reset HXTALEN, CKMEN, PLLEN bits */
	RCU_CTL &= ~(RCU_CTL_HXTALEN | RCU_CTL_CKMEN | RCU_CTL_PLLEN);

	/* Reset HXTALBPS bit */
	RCU_CTL &= ~(RCU_CTL_HXTALBPS);

	/* reset PLLSEL, PREDV0_LSB, PLLMF, USBFSPSC bits */

	RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PREDV0_LSB | RCU_CFG0_PLLMF |
		      RCU_CFG0_USBFSPSC | RCU_CFG0_PLLMF_4);
	RCU_CFG1 = 0x00000000U;

	/* Reset HXTALEN, CKMEN, PLLEN, PLL1EN and PLL2EN bits */
	RCU_CTL &= ~(RCU_CTL_PLLEN | RCU_CTL_PLL1EN | RCU_CTL_PLL2EN |
		     RCU_CTL_CKMEN | RCU_CTL_HXTALEN);
	/* disable all interrupts */
	RCU_INT = 0x00FF0000U;

	/* Configure the System clock source, PLL Multiplier, */
	/* AHB/APBx prescalers and Flash settings             */
	uint32_t timeout = 0U;
	uint32_t stab_flag = 0U;

	/* enable HXTAL */
	RCU_CTL |= RCU_CTL_HXTALEN;

	/* wait until HXTAL is stable or the startup time is */
	/* longer than HXTAL_STARTUP_TIMEOUT                 */
	do {
		timeout++;
		stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
	} while ((stab_flag == 0U) && (timeout != HXTAL_STARTUP_TIMEOUT));

	/* if fail */
	if (0U == (RCU_CTL & RCU_CTL_HXTALSTB)) {
		while (1) {
		}
	}

	/* HXTAL is stable */
	/* AHB = SYSCLK */
	RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
	/* APB2 = AHB/1 */
	RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
	/* APB1 = AHB/2 */
	RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

	/* CK_PLL = (CK_PREDIV0) * 27 = 108 MHz */
	RCU_CFG0 &= ~(RCU_CFG0_PLLMF | RCU_CFG0_PLLMF_4);
	RCU_CFG0 |= (RCU_PLLSRC_HXTAL | RCU_PLL_MUL27);

	if (HXTAL_VALUE == 25000000) {
		/* CK_PREDIV0 = (CK_HXTAL)/5 *8 /10 = 4 MHz */
		RCU_CFG1 &= ~(RCU_CFG1_PREDV0SEL | RCU_CFG1_PREDV1 | RCU_CFG1_PLL1MF |
			      RCU_CFG1_PREDV0);
		RCU_CFG1 |= (RCU_PREDV0SRC_CKPLL1 | RCU_PREDV1_DIV5 | RCU_PLL1_MUL8 |
			     RCU_PREDV0_DIV10);

		/* enable PLL1 */
		RCU_CTL |= RCU_CTL_PLL1EN;
		/* wait till PLL1 is ready */
		while (0U == (RCU_CTL & RCU_CTL_PLL1STB)) {
		}

		/* enable PLL1 */
		RCU_CTL |= RCU_CTL_PLL2EN;
		/* wait till PLL1 is ready */
		while (0U == (RCU_CTL & RCU_CTL_PLL2STB)) {
		}
	} else if (HXTAL_VALUE == 8000000) {
		RCU_CFG1 &= ~(RCU_CFG1_PREDV0SEL | RCU_CFG1_PREDV1 | RCU_CFG1_PLL1MF |
			      RCU_CFG1_PREDV0);
		RCU_CFG1 |= (RCU_PREDV0SRC_HXTAL | RCU_PREDV0_DIV2 | RCU_PREDV1_DIV2 |
			     RCU_PLL1_MUL20 | RCU_PLL2_MUL20);

		/* enable PLL1 */
		RCU_CTL |= RCU_CTL_PLL1EN;
		/* wait till PLL1 is ready */
		while (0U == (RCU_CTL & RCU_CTL_PLL1STB)) {
		}

		/* enable PLL2 */
		RCU_CTL |= RCU_CTL_PLL2EN;
		/* wait till PLL1 is ready */
		while (0U == (RCU_CTL & RCU_CTL_PLL2STB)) {
		}
	}
	/* enable PLL */
	RCU_CTL |= RCU_CTL_PLLEN;

	/* wait until PLL is stable */
	while (0U == (RCU_CTL & RCU_CTL_PLLSTB)) {
	}

	/* select PLL as system clock */
	RCU_CFG0 &= ~RCU_CFG0_SCS;
	RCU_CFG0 |= RCU_CKSYSSRC_PLL;

	/* wait until PLL is selected as system clock */
	while (0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
	}

	return 0;
}

/**
 * @brief RCU device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcu),
		 &gd32_clock_control_init,
		 NULL,
		 NULL, NULL,
		 PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		 &gd32_clock_control_api);
