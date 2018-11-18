/*
 * Copyright (c) 2018 Xilinx, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include "irq.h"
#include "legacy_api.h"

#define TIMER_FREQ          CONFIG_SYS_CLOCK_TICKS_PER_SEC

#if (CONFIG_XLNX_PSTTC_TIMER_INDEX == 0)
#define TIMER_INPUT_CLKHZ   DT_INST_0_CDNS_TTC_CLOCK_FREQUENCY
#define TIMER_IRQ           DT_INST_0_CDNS_TTC_IRQ_0
#define TIMER_BASEADDR      DT_INST_0_CDNS_TTC_BASE_ADDRESS
#else
#error ("No timer is specified")
#endif

#define XTTCPS_CLK_CNTRL_OFFSET	   0x00000000U  /**< Clock Control Register */
#define XTTCPS_CNT_CNTRL_OFFSET	   0x0000000CU  /**< Counter Control Register*/
#define XTTCPS_COUNT_VALUE_OFFSET  0x00000018U  /**< Current Counter Value */
#define XTTCPS_INTERVAL_VAL_OFFSET 0x00000024U  /**< Interval Count Value */
#define XTTCPS_MATCH_0_OFFSET	   0x00000030U  /**< Match 1 value */
#define XTTCPS_MATCH_1_OFFSET	   0x0000003CU  /**< Match 2 value */
#define XTTCPS_MATCH_2_OFFSET	   0x00000048U  /**< Match 3 value */
#define XTTCPS_ISR_OFFSET	   0x00000054U  /**< Interrupt Status Register */
#define XTTCPS_IER_OFFSET	   0x00000060U  /**< Interrupt Enable Register */

/* Clock Control Register definitions */
#define XTTCPS_CLK_CNTRL_PS_EN_MASK	0x00000001U  /**< Prescale enable */
#define XTTCPS_CLK_CNTRL_PS_VAL_MASK	0x0000001EU  /**< Prescale value */
#define XTTCPS_CLK_CNTRL_PS_VAL_SHIFT	1U  /**< Prescale shift */
#define XTTCPS_CLK_CNTRL_PS_DISABLE	16U  /**< Prescale disable */
#define XTTCPS_CLK_CNTRL_SRC_MASK	0x00000020U  /**< Clock source */
#define XTTCPS_CLK_CNTRL_EXT_EDGE_MASK	0x00000040U  /**< External Clock edge */

/* Counter Control Register definitions */
#define XTTCPS_CNT_CNTRL_DIS_MASK	0x00000001U /**< Disable the counter */
#define XTTCPS_CNT_CNTRL_INT_MASK	0x00000002U /**< Interval mode */
#define XTTCPS_CNT_CNTRL_DECR_MASK	0x00000004U /**< Decrement mode */
#define XTTCPS_CNT_CNTRL_MATCH_MASK	0x00000008U /**< Match mode */
#define XTTCPS_CNT_CNTRL_RST_MASK	0x00000010U /**< Reset counter */
#define XTTCPS_CNT_CNTRL_EN_WAVE_MASK	0x00000020U /**< Enable waveform */
#define XTTCPS_CNT_CNTRL_POL_WAVE_MASK	0x00000040U /**< Waveform polarity */
#define XTTCPS_CNT_CNTRL_RESET_VALUE	0x00000021U /**< Reset value */

/* Interrupt register masks */
#define XTTCPS_IXR_INTERVAL_MASK	0x00000001U  /**< Interval Interrupt */
#define XTTCPS_IXR_MATCH_0_MASK		0x00000002U  /**< Match 1 Interrupt */
#define XTTCPS_IXR_MATCH_1_MASK		0x00000004U  /**< Match 2 Interrupt */
#define XTTCPS_IXR_MATCH_2_MASK		0x00000008U  /**< Match 3 Interrupt */
#define XTTCPS_IXR_CNT_OVR_MASK		0x00000010U  /**< Counter Overflow */
#define XTTCPS_IXR_ALL_MASK		0x0000001FU  /**< All valid Interrupts */

#define XTTC_MAX_INTERVAL_COUNT 0xFFFFFFFFU /**< Maximum value of interval counter */

static u32_t accumulated_cycles;
static s32_t _sys_idle_elapsed_ticks = 1;

static int xttc_calculate_interval(u32_t *interval, u8_t *prescaler)
{
	u32_t tmpinterval = 0;
	u8_t tmpprescaler = 0;
	unsigned int tmpval;

	tmpval = (u32_t)(TIMER_INPUT_CLKHZ / TIMER_FREQ);

	if (tmpval < (u32_t)65536U) {
		/* no prescaler is required */
		tmpinterval = tmpval;
		tmpprescaler = 0;
	} else {
		for (tmpprescaler = 1U; tmpprescaler < 16; tmpprescaler++) {
			tmpval = (u32_t)(TIMER_INPUT_CLKHZ /
					 (TIMER_FREQ * (1U << tmpprescaler)));
			if (tmpval < (u32_t)65536U) {
				tmpinterval = tmpval;
				break;
			}
		}
	}

	if (tmpinterval != 0) {
		*interval = tmpinterval;
		*prescaler = tmpprescaler;
		return 0;
	}

	/* TBD: Is there a way to adjust the sys clock parameters such as
	 * ticks per sec if it failed to configure the timer as specified
	 */
	return -EINVAL;
}

/**
 * @brief System timer tick handler
 *
 * This routine handles the system clock tick interrupt. A TICK_EVENT event
 * is pushed onto the kernel stack.
 *
 * The symbol for this routine is either _timer_int_handler.
 *
 * @return N/A
 */
void _timer_int_handler(void *unused)
{
	ARG_UNUSED(unused);

	u32_t regval;

	regval = sys_read32(TIMER_BASEADDR + XTTCPS_ISR_OFFSET);
	accumulated_cycles += sys_clock_hw_cycles_per_tick();
	z_clock_announce(_sys_idle_elapsed_ticks);
}

/**
 * @brief Initialize and enable the system clock
 *
 * This routine is used to program the systick to deliver interrupts at the
 * rate specified via the 'sys_clock_us_per_tick' global variable.
 *
 * @return 0
 */
int z_clock_driver_init(struct device *device)
{
	int ret;
	u32_t interval;
	u8_t prescaler;
	u32_t regval;

	/* Stop timer */
	sys_write32(XTTCPS_CNT_CNTRL_DIS_MASK,
		    TIMER_BASEADDR + XTTCPS_CNT_CNTRL_OFFSET);

	/* Calculate prescaler */
	ret = xttc_calculate_interval(&interval, &prescaler);
	if (ret < 0) {
		printk("Failed to calculate prescaler.\n");
		return ret;
	}

	/* Reset registers */
	sys_write32(XTTCPS_CNT_CNTRL_RESET_VALUE,
		    TIMER_BASEADDR + XTTCPS_CNT_CNTRL_OFFSET);
	sys_write32(0, TIMER_BASEADDR + XTTCPS_CLK_CNTRL_OFFSET);
	sys_write32(0, TIMER_BASEADDR + XTTCPS_INTERVAL_VAL_OFFSET);
	sys_write32(0, TIMER_BASEADDR + XTTCPS_MATCH_0_OFFSET);
	sys_write32(0, TIMER_BASEADDR + XTTCPS_MATCH_1_OFFSET);
	sys_write32(0, TIMER_BASEADDR + XTTCPS_MATCH_2_OFFSET);
	sys_write32(0, TIMER_BASEADDR + XTTCPS_IER_OFFSET);
	sys_write32(XTTCPS_IXR_ALL_MASK, TIMER_BASEADDR + XTTCPS_ISR_OFFSET);
	/* Reset counter value */
	regval = sys_read32(TIMER_BASEADDR + XTTCPS_CNT_CNTRL_OFFSET);
	regval |= XTTCPS_CNT_CNTRL_RST_MASK;
	sys_write32(regval, TIMER_BASEADDR + XTTCPS_CNT_CNTRL_OFFSET);

	/* Set options */
	regval = sys_read32(TIMER_BASEADDR + XTTCPS_CNT_CNTRL_OFFSET);
	regval |= XTTCPS_CNT_CNTRL_INT_MASK;
	sys_write32(regval, TIMER_BASEADDR + XTTCPS_CNT_CNTRL_OFFSET);

	/* Set interval and prescaller */
	sys_write32(interval, TIMER_BASEADDR + XTTCPS_INTERVAL_VAL_OFFSET);
	regval = (u32_t)((prescaler & 0xFU) << 1);
	sys_write32(regval, TIMER_BASEADDR + XTTCPS_CLK_CNTRL_OFFSET);

	/* Enable timer interrupt */
	IRQ_CONNECT(TIMER_IRQ, 0, _timer_int_handler, 0, 0);
	irq_enable(TIMER_IRQ);
	regval = sys_read32(TIMER_BASEADDR + XTTCPS_IER_OFFSET);
	regval |= XTTCPS_IXR_INTERVAL_MASK;
	sys_write32(regval, TIMER_BASEADDR + XTTCPS_IER_OFFSET);

	/* Start timer */
	regval = sys_read32(TIMER_BASEADDR + XTTCPS_CNT_CNTRL_OFFSET);
	regval &= (~XTTCPS_CNT_CNTRL_DIS_MASK);
	sys_write32(regval, TIMER_BASEADDR + XTTCPS_CNT_CNTRL_OFFSET);

	return 0;
}


/**
 * @brief Read the platform's timer hardware
 *
 * This routine returns the current time in terms of timer hardware clock
 * cycles.
 *
 * @return up counter of elapsed clock cycles
 */
u32_t z_timer_cycle_get_32(void)
{
	return accumulated_cycles;
}
