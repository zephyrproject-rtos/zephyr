/*
 * Copyright (c) 2023 by Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lowrisc_opentitan_aontimer

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/watchdog.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_opentitan, CONFIG_WDT_LOG_LEVEL);

#define OT_REG_WDOG_REGWEN_OFFSET 0x10
#define OT_REG_WDOG_CTRL_OFFSET 0x14
#define OT_REG_WDOG_BARK_THOLD_OFFSET 0x18
#define OT_REG_WDOG_BITE_THOLD_OFFSET 0x1C
#define OT_REG_WDOG_COUNT_OFFSET 0x20
#define OT_REG_INTR_STATE_OFFSET 0x24

struct wdt_ot_aontimer_cfg {
	uintptr_t regs;
	uint32_t clk_freq;
	bool wdog_lock;
};

struct wdt_ot_aontimer_data {
	wdt_callback_t bark;
};

static int ot_aontimer_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_ot_aontimer_cfg *const cfg = dev->config;
	volatile uintptr_t regs = cfg->regs;

	sys_write32(0, regs + OT_REG_WDOG_COUNT_OFFSET);
	sys_write32(1, regs + OT_REG_WDOG_CTRL_OFFSET);
	if (cfg->wdog_lock) {
		/* Force a read to ensure the timer was enabled. */
		(void) sys_read32(regs + OT_REG_WDOG_CTRL_OFFSET);
		sys_write32(0, regs + OT_REG_WDOG_REGWEN_OFFSET);
	}
	return 0;
}

static int ot_aontimer_disable(const struct device *dev)
{
	const struct wdt_ot_aontimer_cfg *const cfg = dev->config;
	volatile uintptr_t regs = cfg->regs;

	if (!sys_read32(regs + OT_REG_WDOG_REGWEN_OFFSET)) {
		LOG_ERR("Cannot disable - watchdog settings locked.");
		return -EPERM;
	}

	uint32_t ctrl_val = sys_read32(regs + OT_REG_WDOG_CTRL_OFFSET);

	if (!(ctrl_val & BIT(0))) {
		return -EFAULT;
	}
	sys_write32(ctrl_val & ~BIT(0), regs + OT_REG_WDOG_CTRL_OFFSET);

	return 0;
}

/*
 * The OpenTitan AON Timer includes a multi-level watchdog timer.
 * While the first stage supports an interrupt callback, the second does not.
 * The second stage is mandatory to adjust the "bite" time window.
 *
 * Some boundaries are enforced to prevent behavior that is technically correct
 * but is not likely intended.
 * Bark:
 * Minimum must be 0. Maximum must be > min.
 * The bark interrupt occurs at max (or if the timeout is too long to be
 * supported, the value x s.t. min < x < max and x is the largest valid timeout)
 * Bite:
 * Minimum must be >= bark.min, and maximum >= bark.max. If the timeout is too
 * long to fit, it tries to find the value x s.t. min < x < max where x is the
 * largest valid timeout.
 * The bite action occurs max.
 */
static int ot_aontimer_install_timeout(const struct device *dev,
					const struct wdt_timeout_cfg *cfg)
{
	struct wdt_ot_aontimer_data *data = dev->data;
	const struct wdt_ot_aontimer_cfg *const dev_cfg = dev->config;
	volatile uintptr_t reg_base = dev_cfg->regs;
	const uint64_t max_window = (uint64_t) UINT32_MAX * 1000 / dev_cfg->clk_freq;
	uint64_t bite_thold;
#ifdef CONFIG_WDT_MULTISTAGE
	/* When multistage is selected, add an intermediate bark stage */
	uint64_t bark_thold;
	struct wdt_timeout_cfg *bite = cfg->next;

	if (bite == NULL || bite->window.max < cfg->window.max ||
			(uint64_t) bite->window.min > max_window) {
		return -EINVAL;
	}
	/*
	 * Flag must be clear for stage 1, and reset SOC for stage 2.
	 * CPU not supported
	 */
	if (cfg->flags != WDT_FLAG_RESET_NONE || bite->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}
#endif

	if (cfg->window.min > cfg->window.max || (uint64_t) cfg->window.min > max_window) {
		return -EINVAL;
	}

	if (!sys_read32(reg_base + OT_REG_WDOG_REGWEN_OFFSET)) {
		LOG_ERR("Cannot install timeout - watchdog settings locked.");
		return -ENOMEM;
	}

	/* Watchdog is already enabled! */
	if (sys_read32(reg_base + OT_REG_WDOG_CTRL_OFFSET) & BIT(0)) {
		return -EBUSY;
	}

#ifdef CONFIG_WDT_MULTISTAGE
	/* Force 64-bit ops to ensure thresholds fits in the timer reg. */
	bark_thold = ((uint64_t) cfg->window.max * dev_cfg->clk_freq / 1000);
	bite_thold = ((uint64_t) bite->window.max * dev_cfg->clk_freq / 1000);
	/* Saturate these config values; min is verified to be < max_window */
	if (bark_thold > UINT32_MAX) {
		bark_thold = UINT32_MAX;
	}
	if (bite_thold > UINT32_MAX) {
		bite_thold = UINT32_MAX;
	}
	data->bark = cfg->callback;
	sys_write32((uint32_t) bark_thold, reg_base + OT_REG_WDOG_BARK_THOLD_OFFSET);
	sys_write32((uint32_t) bite_thold, reg_base + OT_REG_WDOG_BITE_THOLD_OFFSET);
#else
	bite_thold = ((uint64_t) cfg->window.max * dev_cfg->clk_freq / 1000);
	/* Saturate this config value; min is verified to be < max_window */
	if (bite_thold > UINT32_MAX) {
		bite_thold = UINT32_MAX;
	}
	if (cfg->flags == WDT_FLAG_RESET_NONE) {
		/* Set bite -> bark, so we generate an interrupt instead of resetting */
		sys_write32((uint32_t) bite_thold, reg_base + OT_REG_WDOG_BARK_THOLD_OFFSET);
		/* Disable bite by writing it to max. Edge case is the bark = max. */
		sys_write32(UINT32_MAX, reg_base + OT_REG_WDOG_BITE_THOLD_OFFSET);
		data->bark = cfg->callback;
	} else {
		data->bark = NULL;
		/* Effectively disable bark by setting it to max */
		sys_write32(UINT32_MAX, reg_base + OT_REG_WDOG_BARK_THOLD_OFFSET);
		sys_write32((uint32_t) bite_thold, reg_base + OT_REG_WDOG_BITE_THOLD_OFFSET);
	}
#endif

	return 0;
}

static int ot_aontimer_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);
	const struct wdt_ot_aontimer_cfg *const cfg = dev->config;
	volatile uintptr_t regs = cfg->regs;

	sys_write32(0, regs + OT_REG_WDOG_COUNT_OFFSET);

	/* Deassert the interrupt line */
	sys_write32(BIT(1), regs + OT_REG_INTR_STATE_OFFSET);
	return 0;
}

static void wdt_ot_isr(const struct device *dev)
{
	const struct wdt_ot_aontimer_cfg *const cfg = dev->config;
	struct wdt_ot_aontimer_data *data = dev->data;
	volatile uintptr_t regs = cfg->regs;

	if (data->bark != NULL) {
		data->bark(dev, 0);
	}

	/* Deassert the interrupt line */
	sys_write32(BIT(1), regs + OT_REG_INTR_STATE_OFFSET);
}

static int ot_aontimer_init(const struct device *dev)
{
	IRQ_CONNECT(
		DT_INST_IRQ_BY_IDX(0, 0, irq),
		DT_INST_IRQ_BY_IDX(0, 0, priority), wdt_ot_isr,
		DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 0, irq));

	return 0;
}

static struct wdt_ot_aontimer_data ot_aontimer_data;

static struct wdt_ot_aontimer_cfg ot_aontimer_cfg = {
	.regs = (volatile uintptr_t) DT_INST_REG_ADDR(0),
	.clk_freq = DT_INST_PROP(0, clock_frequency),
	.wdog_lock = DT_INST_PROP(0, wdog_lock),
};

static DEVICE_API(wdt, ot_aontimer_api) = {
	.setup = ot_aontimer_setup,
	.disable = ot_aontimer_disable,
	.install_timeout = ot_aontimer_install_timeout,
	.feed = ot_aontimer_feed,
};

DEVICE_DT_INST_DEFINE(0, ot_aontimer_init, NULL,
	&ot_aontimer_data, &ot_aontimer_cfg, PRE_KERNEL_1,
	CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	&ot_aontimer_api);
