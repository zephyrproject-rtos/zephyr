/*
 * Copyright (c) 2021 Tokita, Hiroshi <toita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <gd32vf103_rcu.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <sys/__assert.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control);

static const uint32_t en_offset[] = { 0, AHBEN_REG_OFFSET, APB1EN_REG_OFFSET, APB2EN_REG_OFFSET };

static inline bool check_bus_periph(uint32_t bus, uint32_t periph)
{
	rcu_periph_enum ph = (rcu_periph_enum)RCU_REGIDX_BIT(en_offset[bus], RCU_BIT_POS(periph));

	if (bus == CK_AHB  && RCU_DMA0 <= ph && ph <= RCU_USBFS) {
		return true;
	}
	if (bus == CK_APB1 && RCU_TIMER1 <= ph && ph <= RCU_RTC) {
		return true;
	}
	if (bus == CK_APB2 && RCU_AF <= ph  && ph <= RCU_USART0) {
		return true;
	}
	return false;
}

static inline void periph_clock_enable(uint32_t bus, uint32_t periph)
{
	rcu_periph_clock_enable((rcu_periph_enum)
				RCU_REGIDX_BIT(en_offset[bus], RCU_BIT_POS(periph)));
}

static inline void periph_clock_disable(uint32_t bus, uint32_t periph)
{
	rcu_periph_clock_disable((rcu_periph_enum)
				 RCU_REGIDX_BIT(en_offset[bus], RCU_BIT_POS(periph)));
}

static int gd32_clock_control_on(const struct device *dev,
				 clock_control_subsys_t sub_system)
{
	struct gd32_pclken *pclken = (struct gd32_pclken *)sub_system;

	if (!check_bus_periph(pclken->bus, pclken->enr)) {
		return -ENOTSUP;
	}
	periph_clock_enable(pclken->bus, pclken->enr);

	return 0;
}


static int gd32_clock_control_off(const struct device *dev,
				  clock_control_subsys_t sub_system)
{
	struct gd32_pclken *pclken = (struct gd32_pclken *)sub_system;

	if (!check_bus_periph(pclken->bus, pclken->enr)) {
		return -ENOTSUP;
	}
	periph_clock_disable(pclken->bus, pclken->enr);
	return 0;
}


static int gd32_clock_control_get_subsys_rate(const struct device *dev,
					      clock_control_subsys_t sub_system,
					      uint32_t *rate)
{
	struct gd32_pclken *pclken = (struct gd32_pclken *)sub_system;

	*rate = rcu_clock_freq_get(pclken->bus);

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
