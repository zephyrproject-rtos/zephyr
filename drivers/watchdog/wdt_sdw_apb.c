/*
 * Copyright 2022 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT synopsys_sdw_watchdog

/**
 * @brief Driver for sdw APB Watchdog.
 */

#include <errno.h>
#include <drivers/watchdog.h>

struct wdog_sdw_apb {
	/* offset: 0x000 (r/w) watchdog control register */
	volatile uint32_t  ctrl;
	/* offset: 0x004 (r/ ) watchdog timeout range register */
	volatile uint32_t  torr;
	/* offset: 0x008 (r/w) watchdog current conter value register */
	volatile uint32_t  ccvr;
	/* offset: 0x00c ( /w) watchdog counter restart register */
	volatile uint32_t  crr;
	/* offset: 0x010 (r/ ) watchdog interrupt status register */
	volatile uint32_t  intstat;
	/* offset: 0x014 (r/ ) watchdog interrupt clear register */
	volatile uint32_t  eoi;
	/* offset: 0x01c (r/ ) watchdog protection level register */
	volatile uint32_t  prot_level;
	volatile uint32_t  reserved0[49];
	/* offset: 0xe4 (r/ ) watchdog component parameter 5 register */
	volatile uint32_t  comp_param_5;
	/* offset: 0xe8 (r/ ) watchdog component parameter 4 register */
	volatile uint32_t  comp_param_4;
	/* offset: 0xec (r/ ) watchdog component parameter 3 register */
	volatile uint32_t  comp_param_3;
	/* offset: 0xf0 (r/ ) watchdog component parameter 2 register */
	volatile uint32_t  comp_param_2;
	/* offset: 0xf4 (r/ ) watchdog component parameter 1 register */
	volatile uint32_t  comp_param_1;
	/* offset: 0xf8 (r/ ) watchdog component version register */
	volatile uint32_t  comp_version;
	/* offset: 0xfc (r/ ) watchdog component type register */
	volatile uint32_t  comp_type;
};

#define WDOG_STRUCT \
	((volatile struct wdog_sdw_apb *)(DT_INST_REG_ADDR(0)))

#define WDOG_0 DT_DRV_INST(0)
#define WDOG_CONTROL_REG_WDT_EN_MASK	    0x01
#define WDOG_CONTROL_RPL		    0x1c
#define WDOG_CONTROL_REG_RESP_MODE_MASK	    0x02
#define WDOG_TIMEOUT_RANGE_TOPINIT_SHIFT    4
#define WDOG_COUNTER_RESTART_KICK_VALUE	    0x76
#define WDOG_COMP_PARAMS_1_USE_FIX_TOP      BIT(6)

#define WDOG_CTRL_RPL_SHIFT		   2

#define WDT_SDW_NUM_TOPS		16
#define WDT_SDW_FIX_TOP(_idx)	(1U << (16 + (_idx)))

#define WDT_SDW_DEFAULT_SECONDS	30

static const uint32_t wdt_sdw_fix_tops[WDT_SDW_NUM_TOPS] = {
	WDT_SDW_FIX_TOP(0), WDT_SDW_FIX_TOP(1), WDT_SDW_FIX_TOP(2),
	WDT_SDW_FIX_TOP(3), WDT_SDW_FIX_TOP(4), WDT_SDW_FIX_TOP(5),
	WDT_SDW_FIX_TOP(6), WDT_SDW_FIX_TOP(7), WDT_SDW_FIX_TOP(8),
	WDT_SDW_FIX_TOP(9), WDT_SDW_FIX_TOP(10), WDT_SDW_FIX_TOP(11),
	WDT_SDW_FIX_TOP(12), WDT_SDW_FIX_TOP(13), WDT_SDW_FIX_TOP(14),
	WDT_SDW_FIX_TOP(15)
};

static uint32_t wdt_sdw_user_tops[WDT_SDW_NUM_TOPS] = { 0 };

enum wdog_sdw_rmod {
	WDOG_RMOD_RESET = 0,
	WDOG_RMOD_IRQ = 1
};

enum wdog_ctrl_rpl {
	WDOG_RPL_PCLK_CYCLES_2 = 0,
	WDOG_RPL_PCLK_CYCLES_4,
	WDOG_RPL_PCLK_CYCLES_8,
	WDOG_RPL_PCLK_CYCLES_16,
	WDOG_RPL_PCLK_CYCLES_32,
	WDOG_RPL_PCLK_CYCLES_64,
	WDOG_RPL_PCLK_CYCLES_128,
	WDOG_RPL_PCLK_CYCLES_256,
};

struct wdog_sdw_timeout {
	uint32_t top_val;
	unsigned int sec;
	unsigned int msec;
};

static struct wdog_sdw_timeout	timeouts[WDT_SDW_NUM_TOPS];

static void (*user_cb)(const struct device *dev, int channel_id);

static void wdog_sdw_update_mode(const struct device *dev,
				 const enum wdog_sdw_rmod rmod)
{
	volatile struct wdog_sdw_apb *wdog = WDOG_STRUCT;
	uint32_t val;

	ARG_UNUSED(dev);

	val = wdog->ctrl;
	if (rmod == WDOG_RMOD_IRQ)
		val |= WDOG_CONTROL_REG_RESP_MODE_MASK;
	else
		val &= ~WDOG_CONTROL_REG_RESP_MODE_MASK;

	wdog->ctrl = val;
}

static unsigned int wdog_sdw_find_best_top(const struct device *dev,
					   unsigned int timeout,
					   uint32_t *top_val)
{
	int idx;

	/*
	 * TOP will be selected based on the timeout value.
	 * based on TOP value timeout will be set to wdog
	 */
	for (idx = 0; idx < WDT_SDW_NUM_TOPS; ++idx) {
		if (timeouts[idx].msec >= timeout)
			break;
	}

	if (idx == WDT_SDW_NUM_TOPS)
		--idx;

	*top_val = timeouts[idx].top_val;

	return timeouts[idx].sec;
}

static int wdog_sdw_set_timeout(const struct device *dev,
				const unsigned int top_s)
{
	volatile struct wdog_sdw_apb *wdog = WDOG_STRUCT;
	unsigned int timeout;
	uint32_t top_val;

	timeout = wdog_sdw_find_best_top(dev, top_s, &top_val);

	/* Set the new timeout value using selected top */
	wdog->torr = (top_val | (top_val << WDOG_TIMEOUT_RANGE_TOPINIT_SHIFT));

	/* kick the value to torr register */
	wdog->crr = WDOG_COUNTER_RESTART_KICK_VALUE;

	return 0;
}

void do_swap(struct wdog_sdw_timeout *a, struct wdog_sdw_timeout *b)
{
	struct wdog_sdw_timeout temp = *a;

	*a = *b;
	*b = temp;
}

static void wdog_sdw_handle_tops(const struct device *dev,
				 unsigned long clk_rate,
				 const uint32_t *tops)
{
	struct wdog_sdw_timeout tout, *dst;
	int val, tidx;

	/*
	 * Based on provided clock rate, calculate the timeout values for
	 * each top value. arrange the timeout values in ascending order.
	 */
	for (val = 0; val < WDT_SDW_NUM_TOPS; ++val) {
		tout.top_val = val;
		tout.sec = tops[val] / clk_rate;
		tout.msec = (uint64_t)tops[val] * MSEC_PER_SEC / clk_rate;

		/* Arrange the timeouts in ascending order */
		for (tidx = 0; tidx < val; ++tidx) {
			dst = &timeouts[tidx];
			if (tout.sec > dst->sec ||
			    (tout.sec == dst->sec &&
			     tout.msec >= dst->msec))
				continue;
			else
				do_swap(dst, &tout);
		}

		timeouts[val] = tout;
	}
}

uint32_t *get_user_tops(void)
{
	wdt_sdw_user_tops[0] = DT_PROP_HAS_IDX(WDOG_0, utops, 0);
	wdt_sdw_user_tops[1] = DT_PROP_HAS_IDX(WDOG_0, utops, 1);
	wdt_sdw_user_tops[2] = DT_PROP_HAS_IDX(WDOG_0, utops, 2);
	wdt_sdw_user_tops[3] = DT_PROP_HAS_IDX(WDOG_0, utops, 3);
	wdt_sdw_user_tops[4] = DT_PROP_HAS_IDX(WDOG_0, utops, 4);
	wdt_sdw_user_tops[5] = DT_PROP_HAS_IDX(WDOG_0, utops, 5);
	wdt_sdw_user_tops[6] = DT_PROP_HAS_IDX(WDOG_0, utops, 6);
	wdt_sdw_user_tops[7] = DT_PROP_HAS_IDX(WDOG_0, utops, 7);
	wdt_sdw_user_tops[8] = DT_PROP_HAS_IDX(WDOG_0, utops, 8);
	wdt_sdw_user_tops[9] = DT_PROP_HAS_IDX(WDOG_0, utops, 9);
	wdt_sdw_user_tops[10] = DT_PROP_HAS_IDX(WDOG_0, utops, 10);
	wdt_sdw_user_tops[11] = DT_PROP_HAS_IDX(WDOG_0, utops, 11);
	wdt_sdw_user_tops[12] = DT_PROP_HAS_IDX(WDOG_0, utops, 12);
	wdt_sdw_user_tops[13] = DT_PROP_HAS_IDX(WDOG_0, utops, 13);
	wdt_sdw_user_tops[14] = DT_PROP_HAS_IDX(WDOG_0, utops, 14);
	wdt_sdw_user_tops[15] = DT_PROP_HAS_IDX(WDOG_0, utops, 15);

	return wdt_sdw_user_tops;
}


static int wdog_sdw_init_timeouts(const struct device *dev,
				  unsigned long clk_rate)
{
	volatile struct wdog_sdw_apb *wdog = WDOG_STRUCT;
	uint32_t data;
	const uint32_t *tops;

	/*
	 * use fixed top if WDT_USE_FIX_TOP set 1
	 * otherwise use custom tops
	 */
	data = wdog->comp_param_1;
	if (data & WDOG_COMP_PARAMS_1_USE_FIX_TOP) {
		tops = wdt_sdw_fix_tops;
	} else {
		if (DT_NODE_HAS_PROP(WDOG_0, utops)) {
			tops = get_user_tops();
		} else {
			tops = wdt_sdw_fix_tops;
		}
	}

	/* calculatei timeouts using top values */
	wdog_sdw_handle_tops(dev, clk_rate, tops);
	if (!timeouts[WDT_SDW_NUM_TOPS - 1].sec) {
		printk("No any valid TOP detected\n");
		return -EINVAL;
	}

	return 0;
}

static int wdog_sdw_apb_setup(const struct device *dev, uint8_t options)
{
	volatile struct wdog_sdw_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);

	/* set reset pulse length */
	if ((options >= WDOG_RPL_PCLK_CYCLES_2) &&
	    (options <= WDOG_RPL_PCLK_CYCLES_256)) {
		wdog->ctrl |= (options << WDOG_CTRL_RPL_SHIFT);
	}

	/* Enable the watchdog counter with INTEN bit */
	wdog->ctrl |= WDOG_CONTROL_REG_WDT_EN_MASK;

	return 0;
}

static int wdog_sdw_apb_disable(const struct device *dev)
{
	volatile struct wdog_sdw_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);

	/* Stop the watchdog counter */
	wdog->ctrl = ~(WDOG_CONTROL_REG_WDT_EN_MASK);

	return 0;
}

static int wdog_sdw_apb_install_timeout(const struct device *dev,
					const struct wdt_timeout_cfg *config)
{
	int ret;

	ARG_UNUSED(dev);

	ret = wdog_sdw_init_timeouts(dev,
				     DT_INST_PROP_BY_PHANDLE(0,
							     clocks,
							     clock_frequency));
	if (ret < 0)
		return ret;
	ret = wdog_sdw_set_timeout(dev, config->window.max);
	if (ret < 0)
		return ret;

	wdog_sdw_update_mode(dev, config->flags);

	/* Configure only the callback */
	user_cb = config->callback;

	return 0;
}

static int wdog_sdw_apb_feed(const struct device *dev, int channel_id)
{
	volatile struct wdog_sdw_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	/* Reload */
	wdog->crr = WDOG_COUNTER_RESTART_KICK_VALUE;

	return 0;
}

static const struct wdt_driver_api wdog_sdw_apb_api = {
	.setup = wdog_sdw_apb_setup,
	.disable = wdog_sdw_apb_disable,
	.install_timeout = wdog_sdw_apb_install_timeout,
	.feed = wdog_sdw_apb_feed,
};

static int wdog_sdw_apb_init(const struct device *dev)
{
	/* Configure the wdog mode */
	wdog_sdw_update_mode(dev, WDOG_RMOD_RESET);

#ifdef CONFIG_WDOG_SDW_APB_START_AT_BOOT
	wdog_sdw_apb_setup(dev, 0);
#endif
	return 0;
}

DEVICE_DT_INST_DEFINE(0,
		      wdog_sdw_apb_init,
		      NULL,
		      NULL, NULL,
		      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &wdog_sdw_apb_api);
