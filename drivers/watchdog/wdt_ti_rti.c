/*
 * Copyright (c) 2025 Ayush Singh, BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_j7_rti_wdt

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <stdint.h>

#define WDT_TI_RTI_ANY_CLOCKS DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)

#define WDENABLE_KEY  0xa98559da
#define WDDISABLE_KEY 0x5312aced

#define WDKEY_SEQ0 0xe51a
#define WDKEY_SEQ1 0xa35c

#define WDT_PRELOAD_SHIFT 13

#define WDT_PRELOAD_MAX 0xfff

/* Only the last 6 bits are used */
#define CLEAR_WDSTATUS 0x3f

#define RTIWWDRX_NMI   0xa
#define RTIWWDRX_RESET 0x5

#define RTIGCTRL_HALT_BY_DBG 0
#define RTIGCTRL_RUN_BY_DBG  BIT(15)

#define RTIWWDSIZE_100P  0x5
#define RTIWWDSIZE_50P   0x50
#define RTIWWDSIZE_25P   0x500
#define RTIWWDSIZE_12P5  0x5000
#define RTIWWDSIZE_6P25  0x50000
#define RTIWWDSIZE_3P125 0x500000

#define DEV_CFG(dev)  ((const struct wdt_ti_rti_config *)(dev)->config)
#define DEV_DATA(dev) ((struct wdt_ti_rti_data *)(dev)->data)
#define DEV_REGS(dev) ((struct wdt_ti_rti_regs *)DEVICE_MMIO_GET(dev))

struct wdt_ti_rti_regs {
	/* RTI Global Control Register, offset: 0x00 */
	volatile uint32_t GCTRL;
	uint32_t pad[35];
	/* Digital Watchdog Control Register, offset: 0x90 */
	volatile uint32_t DWDCTRL;
	/* Digital Watchdog Preload Register, offset: 0x94 */
	volatile uint32_t DWDPRLD;
	/* Watchdog Status Register, offset: 0x98 */
	volatile uint32_t WDSTATUS;
	/* Watchdog Key Register, offset: 0x9C */
	volatile uint32_t WDKEY;
	/* Digital Watchdog Down Counter, offset: 0xA0 */
	volatile uint32_t DWDCNTR;
	/* Digital Windowed Watchdog Reaction Control, offset: 0xA4 */
	volatile uint32_t WWDRXNCTRL;
	/* Digital Windowed Watchdog Window Size Control, offset: 0xA8 */
	volatile uint32_t WWDSIZECTRL;
};

struct wdt_ti_rti_data {
	DEVICE_MMIO_RAM;

	wdt_callback_t callback;
	uint64_t freq;
};

struct wdt_ti_rti_config {
	DEVICE_MMIO_ROM;

	uint64_t freq;
#if WDT_TI_RTI_ANY_CLOCKS
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	clock_control_subsys_t clk_assigned;
	uint32_t clk_parent_id;
	bool has_parent;
#endif
	bool nmi_supported;
	bool reset_supported;
	void (*irq_config_func)(void);
};

static int wdt_ti_rti_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(options);

	struct wdt_ti_rti_regs *regs = DEV_REGS(dev);

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		regs->GCTRL = RTIGCTRL_HALT_BY_DBG;
	} else {
		regs->GCTRL = RTIGCTRL_RUN_BY_DBG;
	}

	/* Clear the Watchdog status register before enabling */
	regs->WDSTATUS = CLEAR_WDSTATUS;
	regs->DWDCTRL = WDENABLE_KEY;

	return 0;
}

static int wdt_ti_rti_disable(const struct device *dev)
{
	struct wdt_ti_rti_regs *regs = DEV_REGS(dev);

	regs->DWDCTRL = WDDISABLE_KEY;
	regs->WDKEY = 0;

	return 0;
}

static int wdt_ti_rti_window_size(const struct wdt_window window)
{
	const int window_sizes[] = {RTIWWDSIZE_100P, RTIWWDSIZE_50P,  RTIWWDSIZE_25P,
				    RTIWWDSIZE_12P5, RTIWWDSIZE_6P25, RTIWWDSIZE_3P125};

	if (window.max < window.min || window.max == 0) {
		return -EINVAL;
	}

	for (uint32_t idx = 0; idx < ARRAY_SIZE(window_sizes); idx++) {
		uint32_t temp = (window.max - window.min) << idx;

		if (temp == window.max) {
			return window_sizes[idx];
		} else if (temp > window.max) {
			break;
		}
	}

	return -EINVAL;
}

static int wdt_ti_rti_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_ti_rti_config *config = DEV_CFG(dev);
	struct wdt_ti_rti_data *data = DEV_DATA(dev);
	struct wdt_ti_rti_regs *regs = DEV_REGS(dev);
	uint64_t ticks;
	int window_size;

	data->callback = cfg->callback;

	window_size = wdt_ti_rti_window_size(cfg->window);
	if (window_size < 0) {
		return window_size;
	}

	ticks = ((uint64_t)cfg->window.max * data->freq / MSEC_PER_SEC) >> WDT_PRELOAD_SHIFT;
	if (ticks == 0U || ticks > WDT_PRELOAD_MAX + 1U) {
		return -EINVAL;
	}

	if (cfg->flags == WDT_FLAG_RESET_SOC) {
		/* Reset mode requested */
		if (!config->reset_supported) {
			return -ENOTSUP;
		}
		regs->WWDRXNCTRL = RTIWWDRX_RESET;
	} else {
		/* NMI mode requested */
		if (!config->nmi_supported) {
			return -ENOTSUP;
		}
		regs->WWDRXNCTRL = RTIWWDRX_NMI;
	}

	regs->DWDPRLD = (uint32_t)(ticks - 1U);
	regs->WWDSIZECTRL = (uint32_t)window_size;

	return 0;
}

static int wdt_ti_rti_feed(const struct device *dev, int channel_id)
{
	struct wdt_ti_rti_regs *regs = DEV_REGS(dev);

	if (channel_id != 0) {
		return -EINVAL;
	}

	regs->WDKEY = WDKEY_SEQ0;
	regs->WDKEY = WDKEY_SEQ1;

	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
static void wdt_ti_rti_isr(const struct device *dev)
{
	struct wdt_ti_rti_data *data = DEV_DATA(dev);
	struct wdt_ti_rti_regs *regs = DEV_REGS(dev);
	volatile uint32_t status = regs->WDSTATUS;
	/* Clear status register before servicing the watchdog */
	regs->WDSTATUS = CLEAR_WDSTATUS;
	if (status & BIT(5) && data->callback) {
		data->callback(dev, 0);
	}
	regs->WDKEY = WDKEY_SEQ0;
	regs->WDKEY = WDKEY_SEQ1;
}
#endif

static int wdt_ti_rti_init(const struct device *dev)
{
	const struct wdt_ti_rti_config *config = DEV_CFG(dev);
	struct wdt_ti_rti_data *data = DEV_DATA(dev);
	struct wdt_ti_rti_regs *regs;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	regs = DEV_REGS(dev);
	if (!regs) {
		return -EINVAL;
	}

	data->freq = config->freq;

#if WDT_TI_RTI_ANY_CLOCKS
	if (config->clk_dev != NULL) {
		uint32_t rate;
		int ret;

		if (!device_is_ready(config->clk_dev)) {
			return -ENODEV;
		}

		if (config->has_parent) {
			uint32_t parent = config->clk_parent_id;

			ret = clock_control_configure(config->clk_dev, config->clk_assigned,
						      &parent);
			if (ret != 0) {
				return ret;
			}
		}

		ret = clock_control_get_rate(config->clk_dev, config->clk_id, &rate);
		if (ret != 0) {
			return ret;
		}

		if (rate == 0U) {
			return -EINVAL;
		}

		data->freq = rate;
	}
#endif

	if (config->nmi_supported) {
		config->irq_config_func();
	}
	return 0;
}

static DEVICE_API(wdt, wdt_ti_rti_api) = {
	.setup = wdt_ti_rti_setup,
	.disable = wdt_ti_rti_disable,
	.feed = wdt_ti_rti_feed,
	.install_timeout = wdt_ti_rti_timeout,
};

#define WDT_TI_RTI_CLK_ID(i, prop)                                                                 \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(i, prop),                                                \
		    ((clock_control_subsys_t)DT_INST_PHA_BY_IDX(i, prop, 0, name)), (NULL))

#define WDT_TI_RTI_CLK_INIT(i)                                                                     \
	.clk_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, clocks),                                   \
			       (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(i))), (NULL)),                   \
	.clk_id = WDT_TI_RTI_CLK_ID(i, clocks),                                                    \
	.clk_assigned = WDT_TI_RTI_CLK_ID(i, assigned_clocks),                                     \
	.clk_parent_id = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, assigned_clock_parents),             \
				     (DT_INST_PHA_BY_IDX(i, assigned_clock_parents, 0, name)),     \
				     (0)),                                                         \
	.has_parent = DT_INST_NODE_HAS_PROP(i, assigned_clock_parents),

#define WDT_TI_RTI_CLK_PROP_CHECK(i, prop)                                                         \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(i, prop),                                                 \
		   (BUILD_ASSERT(DT_INST_PROP_LEN(i, prop) == 1,                                   \
				 #prop " must have one entry");                                    \
		    IF_ENABLED(DT_INST_NODE_HAS_PROP(i, clocks),                                   \
			       (BUILD_ASSERT(DT_SAME_NODE(DT_INST_PHANDLE_BY_IDX(i, prop, 0),      \
							  DT_INST_CLOCKS_CTLR(i)),                 \
					     #prop " must use the clocks controller");))))

#define WDT_TI_RTI_CLK_CHECK(i)                                                                    \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(i, assigned_clock_parents),                               \
		   (BUILD_ASSERT(DT_INST_NODE_HAS_PROP(i, clocks) &&                               \
				 DT_INST_NODE_HAS_PROP(i, assigned_clocks),                        \
				 "assigned-clock-parents needs clocks and assigned-clocks");))     \
	WDT_TI_RTI_CLK_PROP_CHECK(i, assigned_clocks)                                              \
	WDT_TI_RTI_CLK_PROP_CHECK(i, assigned_clock_parents)

#define WDT_TI_RTI_INIT(i)                                                                         \
	WDT_TI_RTI_CLK_CHECK(i)                                                                    \
	static void wdt_ti_rti_irq_config_##i(void)                                                \
	{                                                                                          \
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(i, 0), (						   \
			IRQ_CONNECT(DT_INST_IRQN(i), DT_INST_IRQ(i, priority), wdt_ti_rti_isr,     \
				DEVICE_DT_INST_GET(i),                                             \
				COND_CODE_1(DT_INST_IRQ_HAS_CELL(i, flags),                        \
					(DT_INST_IRQ(i, flags)), (0)));                            \
			irq_enable(DT_INST_IRQN(i));                                               \
		));										   \
	};                                                                                         \
	static struct wdt_ti_rti_data wdt_ti_rti_data_##i = {};                                    \
                                                                                                   \
	static struct wdt_ti_rti_config wdt_ti_rti_config_##i = {                                  \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(i)),                                              \
		.freq = DT_INST_PROP(i, clock_frequency),                                          \
		.nmi_supported = DT_INST_IRQ_HAS_IDX(i, 0),					   \
		.reset_supported = DT_INST_PROP(i, reset_capable),				   \
		.irq_config_func = wdt_ti_rti_irq_config_##i,                                      \
		IF_ENABLED(WDT_TI_RTI_ANY_CLOCKS, (WDT_TI_RTI_CLK_INIT(i)))                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, wdt_ti_rti_init, NULL, &wdt_ti_rti_data_##i,                      \
			      &wdt_ti_rti_config_##i, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_ti_rti_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_TI_RTI_INIT)
