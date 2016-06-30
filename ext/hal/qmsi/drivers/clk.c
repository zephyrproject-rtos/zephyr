/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "clk.h"
#include "flash_layout.h"
#if (!QM_SENSOR) || (UNIT_TEST)
#include <x86intrin.h>
#endif

#include "soc_watch.h"

#if (QM_SENSOR) && (!UNIT_TEST)
/* Timestamp counter for Sensor Subsystem is 32bit. */
#define get_ticks() __builtin_arc_lr(QM_SS_TSC_BASE + QM_SS_TIMER_COUNT)
#elif(QM_SENSOR) && (UNIT_TEST)
#define get_ticks() _rdtsc() % ((uint32_t)-1)
#else
/* 64bit Timestamp counter */
#define get_ticks() _rdtsc()
#endif

/* NOTE: Currently user space data / bss section overwrites the ROM data / bss
 * sections, so anything that is set in the ROM will be obliterated once we jump
 * into the user app.
 */
static uint32_t ticks_per_us = SYS_TICKS_PER_US_32MHZ;

int clk_sys_set_mode(const clk_sys_mode_t mode, const clk_sys_div_t div)
{
	QM_CHECK(div < CLK_SYS_DIV_NUM, -EINVAL);
	QM_CHECK(mode <= CLK_SYS_CRYSTAL_OSC, -EINVAL);
	uint16_t trim = 0;

	/* Store system ticks per us */
	uint32_t sys_ticks_per_us = 1;

	/*
	 * Get current settings, clear the clock divisor bits, and clock divider
	 * enable bit.
	 */
	uint32_t ccu_sys_clk_ctl =
	    QM_SCSS_CCU->ccu_sys_clk_ctl & CLK_SYS_CLK_DIV_DEF_MASK;

	/*
	 * Steps:
	 * 1. Enable the new oscillator and wait for it to stabilise.
	 * 2. Switch to the new oscillator
	 *    Note on registers:
	 *    - QM_OSC0_MODE_SEL:
	 *       - asserted: it switches to external crystal oscillator
	 *       - not asserted: it switches to silicon oscillator
	 *     - QM_CCU_SYS_CLK_SEL:
	 *       - asserted: it switches to hybrid (silicon or external)
	 *                   oscillator
	 *       - not asserted: it switches to RTC oscillator
	 * 3. Hybrid oscillator only: apply sysclk divisor
	 * 4. Disable mutually exclusive clock sources. For internal silicon
	 *    oscillator is disables the external crystal oscillator and vice
	 *    versa.
	 */
	switch (mode) {
	case CLK_SYS_HYB_OSC_32MHZ:
	case CLK_SYS_HYB_OSC_16MHZ:
	case CLK_SYS_HYB_OSC_8MHZ:
	case CLK_SYS_HYB_OSC_4MHZ:
		/* Calculate the system clock ticks per microsecond
		 * and get the shadowed trim code from the Data Region of Flash.
		 */
		if (CLK_SYS_HYB_OSC_32MHZ == mode) {
			sys_ticks_per_us = SYS_TICKS_PER_US_32MHZ / BIT(div);
			trim = QM_FLASH_DATA_TRIM_CODE->osc_trim_32mhz;
		} else if (CLK_SYS_HYB_OSC_16MHZ == mode) {
			sys_ticks_per_us = SYS_TICKS_PER_US_16MHZ / BIT(div);
			trim = QM_FLASH_DATA_TRIM_CODE->osc_trim_16mhz;
		} else if (CLK_SYS_HYB_OSC_8MHZ == mode) {
			sys_ticks_per_us = SYS_TICKS_PER_US_8MHZ / BIT(div);
			trim = QM_FLASH_DATA_TRIM_CODE->osc_trim_8mhz;
		} else {
			sys_ticks_per_us = SYS_TICKS_PER_US_4MHZ / BIT(div);
			trim = QM_FLASH_DATA_TRIM_CODE->osc_trim_4mhz;
		}
		/*
		 * Apply trim code for the selected mode if this has been
		 * written in the soc_data section.
		 * This is performed in rom on the first boot for each
		 * available frequency.
		 * If not present, something went wrong and trim code
		 * will not be applied.
		 */
		if ((trim & QM_FLASH_TRIM_PRESENT_MASK) ==
		    QM_FLASH_TRIM_PRESENT) {
			clk_trim_apply(trim);
		}
		/* Select the silicon oscillator frequency */
		QM_SCSS_CCU->osc0_cfg1 &= ~OSC0_CFG1_SI_FREQ_SEL_MASK;
		QM_SCSS_CCU->osc0_cfg1 |= (mode << OSC0_CFG1_SI_FREQ_SEL_OFFS);
		/* Enable the silicon oscillator */
		QM_SCSS_CCU->osc0_cfg1 |= QM_OSC0_EN_SI_OSC;
		/* Wait for the oscillator to lock */
		while (!(QM_SCSS_CCU->osc0_stat1 & QM_OSC0_LOCK_SI)) {
		};
		/* Switch to silicon oscillator mode */
		QM_SCSS_CCU->osc0_cfg1 &= ~QM_OSC0_MODE_SEL;
		/* Set the system clock divider */
		QM_SCSS_CCU->ccu_sys_clk_ctl =
		    ccu_sys_clk_ctl | QM_CCU_SYS_CLK_SEL |
		    (div << QM_CCU_SYS_CLK_DIV_OFFSET);
		/* Disable the crystal oscillator */
		QM_SCSS_CCU->osc0_cfg1 &= ~QM_OSC0_EN_CRYSTAL;
		break;

	case CLK_SYS_RTC_OSC:
		/* The RTC oscillator is on by hardware default */
		ccu_sys_clk_ctl |=
		    (QM_CCU_RTC_CLK_EN | (div << QM_CCU_SYS_CLK_DIV_OFFSET));

		QM_SCSS_CCU->ccu_sys_clk_ctl =
		    (ccu_sys_clk_ctl & ~(QM_CCU_SYS_CLK_SEL));
		break;

	case CLK_SYS_CRYSTAL_OSC:
		QM_SCSS_CCU->osc0_cfg1 |= QM_OSC0_EN_CRYSTAL;
		sys_ticks_per_us = SYS_TICKS_PER_US_XTAL / BIT(div);
		while (!(QM_SCSS_CCU->osc0_stat1 & QM_OSC0_LOCK_XTAL)) {
		};
		QM_SCSS_CCU->osc0_cfg1 |= QM_OSC0_MODE_SEL;
		QM_SCSS_CCU->ccu_sys_clk_ctl =
		    ccu_sys_clk_ctl | QM_CCU_SYS_CLK_SEL |
		    (div << QM_CCU_SYS_CLK_DIV_OFFSET);
		QM_SCSS_CCU->osc0_cfg1 &= ~QM_OSC0_EN_SI_OSC;
		break;
	}

	QM_SCSS_CCU->ccu_sys_clk_ctl |= QM_CCU_SYS_CLK_DIV_EN;
	ticks_per_us = (sys_ticks_per_us > 0 ? sys_ticks_per_us : 1);

	/* Log any clock changes. */
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_OSC0_CFG1);
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_CCU_SYS_CLK_CTL);
	return 0;
}

int clk_trim_read(uint32_t *const value)
{
	QM_CHECK(NULL != value, -EINVAL);

	*value = (QM_SCSS_CCU->osc0_cfg1 & OSC0_CFG1_FTRIMOTP_MASK) >>
		 OSC0_CFG1_FTRIMOTP_OFFS;

	return 0;
}

int clk_trim_apply(const uint32_t value)
{
	/* Enable trim mode */
	QM_SCSS_CCU->osc0_cfg0 |= BIT(1);

	/* Apply trim code */
	QM_SCSS_CCU->osc0_cfg1 &= ~OSC0_CFG1_FTRIMOTP_MASK;
	QM_SCSS_CCU->osc0_cfg1 |=
	    (value << OSC0_CFG1_FTRIMOTP_OFFS) & OSC0_CFG1_FTRIMOTP_MASK;

	/*
	 * Recommended wait time after setting up the trim code
	 * is 200us. Minimum wait time is 100us.
	 * The delay is running from of the silicon oscillator
	 * which is been trimmed. This induces a lack of precision
	 * in the delay.
	 */
	clk_sys_udelay(200);

	/* Disable trim mode */
	QM_SCSS_CCU->osc0_cfg0 &= ~BIT(1);

	return 0;
}

int clk_adc_set_div(const uint16_t div)
{
#if (QUARK_D2000)
	/*
	 * The driver adds 1 to the value, so to avoid confusion for the user,
	 * subtract 1 from the input value.
	 */
	QM_CHECK((div - 1) <= QM_ADC_DIV_MAX, -EINVAL);

	uint32_t reg = QM_SCSS_CCU->ccu_periph_clk_div_ctl0;
	reg &= CLK_ADC_DIV_DEF_MASK;
	reg |= ((div - 1) << QM_CCU_ADC_CLK_DIV_OFFSET);
	QM_SCSS_CCU->ccu_periph_clk_div_ctl0 = reg;
#else
	/* TODO this function should only be made available on D2000 */
	(void)div;
#endif

	return 0;
}

int clk_periph_set_div(const clk_periph_div_t div)
{
	QM_CHECK(div <= CLK_PERIPH_DIV_8, -EINVAL);

#if (QUARK_D2000)
	uint32_t reg =
	    QM_SCSS_CCU->ccu_periph_clk_div_ctl0 & CLK_PERIPH_DIV_DEF_MASK;
	reg |= (div << QM_CCU_PERIPH_PCLK_DIV_OFFSET);
	QM_SCSS_CCU->ccu_periph_clk_div_ctl0 = reg;
	/* CLK Div en bit must be written from 0 -> 1 to apply new value */
	QM_SCSS_CCU->ccu_periph_clk_div_ctl0 |= QM_CCU_PERIPH_PCLK_DIV_EN;

#elif(QUARK_SE)
	QM_SCSS_CCU->ccu_periph_clk_div_ctl0 =
	    (div << QM_CCU_PERIPH_PCLK_DIV_OFFSET);
	/* CLK Div en bit must be written from 0 -> 1 to apply new value */
	QM_SCSS_CCU->ccu_periph_clk_div_ctl0 |= QM_CCU_PERIPH_PCLK_DIV_EN;
#endif

	return 0;
}

int clk_gpio_db_set_div(const clk_gpio_db_div_t div)
{
	QM_CHECK(div <= CLK_GPIO_DB_DIV_128, -EINVAL);

	uint32_t reg =
	    QM_SCSS_CCU->ccu_gpio_db_clk_ctl & CLK_GPIO_DB_DIV_DEF_MASK;
	reg |= (div << QM_CCU_GPIO_DB_DIV_OFFSET);
	QM_SCSS_CCU->ccu_gpio_db_clk_ctl = reg;
	/* CLK Div en bit must be written from 0 -> 1 to apply new value */
	QM_SCSS_CCU->ccu_gpio_db_clk_ctl |= QM_CCU_GPIO_DB_CLK_DIV_EN;

	return 0;
}

int clk_ext_set_div(const clk_ext_div_t div)
{
	QM_CHECK(div <= CLK_EXT_DIV_8, -EINVAL);

	uint32_t reg = QM_SCSS_CCU->ccu_ext_clock_ctl & CLK_EXTERN_DIV_DEF_MASK;
	reg |= (div << QM_CCU_EXTERN_DIV_OFFSET);
	QM_SCSS_CCU->ccu_ext_clock_ctl = reg;
	/* CLK Div en bit must be written from 0 -> 1 to apply new value */
	QM_SCSS_CCU->ccu_ext_clock_ctl |= QM_CCU_EXT_CLK_DIV_EN;

	return 0;
}

int clk_rtc_set_div(const clk_rtc_div_t div)
{
	QM_CHECK(div <= CLK_RTC_DIV_32768, -EINVAL);

	uint32_t reg = QM_SCSS_CCU->ccu_sys_clk_ctl & CLK_RTC_DIV_DEF_MASK;
	reg |= (div << QM_CCU_RTC_CLK_DIV_OFFSET);
	QM_SCSS_CCU->ccu_sys_clk_ctl = reg;
	/* CLK Div en bit must be written from 0 -> 1 to apply new value */
	QM_SCSS_CCU->ccu_sys_clk_ctl |= QM_CCU_RTC_CLK_DIV_EN;

	return 0;
}

int clk_periph_enable(const clk_periph_t clocks)
{
	QM_CHECK(clocks <= CLK_PERIPH_ALL, -EINVAL);

	QM_SCSS_CCU->ccu_periph_clk_gate_ctl |= clocks;

	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER,
			    SOCW_REG_CCU_PERIPH_CLK_GATE_CTL);

	return 0;
}

int clk_periph_disable(const clk_periph_t clocks)
{
	QM_CHECK(clocks <= CLK_PERIPH_ALL, -EINVAL);

	QM_SCSS_CCU->ccu_periph_clk_gate_ctl &= ~clocks;

	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER,
			    SOCW_REG_CCU_PERIPH_CLK_GATE_CTL);

	return 0;
}

uint32_t clk_sys_get_ticks_per_us(void)
{
	return ticks_per_us;
}

void clk_sys_udelay(uint32_t microseconds)
{
	uint32_t timeout = ticks_per_us * microseconds;
#if (QM_SENSOR)
	uint32_t tsc_start;
#else
	unsigned long long tsc_start;
#endif
	tsc_start = get_ticks();
	/* We need to wait until timeout system clock ticks has occurred. */
	while (get_ticks() - tsc_start < timeout) {
	}
}
