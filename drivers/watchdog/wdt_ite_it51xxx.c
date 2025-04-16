/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_watchdog

#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
LOG_MODULE_REGISTER(wdt_ite_it51xxx);

#define IT51XXX_WATCHDOG_MAGIC_BYTE                 0x5c
#define WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT(ms) ((ms) * 1024 / 1000)

/* 0x81: External Timer1/WDT Configuration */
#define REG_ETWCFG          0x01
#define WDT_EWDKEYEN        BIT(5)
#define WDT_EWDSRC          BIT(4)
#define WDT_LEWDCNTL        BIT(3)
#define WDT_LET1CNTL        BIT(2)
#define WDT_LET1PS          BIT(1)
#define WDT_LETWCFG         BIT(0)
/* 0x82: External Timer1 Prescaler */
#define REG_ET1PSR          0x02
#define WDT_ETPS_32P768_KHZ 0x00
#define WDT_ETPS_1P024_KHZ  0x01
#define WDT_ETPS_32_HZ      0x02
/* 0x83: External Timer1 Counter High Byte */
#define REG_ET1CNTLHR       0x03
/* 0x84: External Timer1 Counter Low Byte */
#define REG_ET1CNTLLR       0x04
/* 0x85: External Timer1/WDT Control */
#define REG_ETWCTRL         0x05
#define WDT_EWDSCEN         BIT(5)
#define WDT_EWDSCMS         BIT(4)
#define WDT_ET1TC           BIT(1)
#define WDT_ET1RST          BIT(0)
/* 0x86: External WDT Counter Low Byte */
#define REG_EWDCNTLR        0x06
/* 0x87: External WDT Key */
#define REG_EWDKEYR         0x07
/* 0x89: External WDT Counter High Byte */
#define REG_EWDCNTHR        0x09

/* device config */
struct wdt_it51xxx_config {
	/* wdt register base address */
	uintptr_t base;
};

/* driver data */
struct wdt_it51xxx_data {
	/* timeout callback used to handle watchdog event */
	wdt_callback_t callback;
	/* indicate whether a watchdog timeout is installed */
	bool timeout_installed;
	/* watchdog feed timeout in milliseconds */
	uint32_t timeout;
	/* pre-warning timer1 fired times */
	int wdt_warning_fired;
};

static int wdt_it51xxx_install_timeout(const struct device *dev,
				       const struct wdt_timeout_cfg *config)
{
	const struct wdt_it51xxx_config *const wdt_config = dev->config;
	struct wdt_it51xxx_data *data = dev->data;
	const uintptr_t base = wdt_config->base;

	/* if watchdog is already running */
	if (sys_read8(base + REG_ETWCFG) & WDT_LEWDCNTL) {
		return -EBUSY;
	}

	/*
	 * Not support lower limit window timeouts (min value must be equal to
	 * 0). Upper limit window timeouts can't be 0 when we install timeout.
	 */
	if ((config->window.min != 0) || (config->window.max == 0)) {
		data->timeout_installed = false;
		return -EINVAL;
	}

	/* save watchdog timeout */
	data->timeout = config->window.max;

	/* install user timeout isr */
	data->callback = config->callback;

	/* mark installed */
	data->timeout_installed = true;

	return 0;
}

static int wdt_it51xxx_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_it51xxx_config *const wdt_config = dev->config;
	struct wdt_it51xxx_data *data = dev->data;
	const uintptr_t base = wdt_config->base;
	uint16_t cnt0 = WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT(data->timeout);
	uint16_t cnt1 = WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT(
		(data->timeout + CONFIG_WDT_ITE_WARNING_LEADING_TIME_MS));
	uint8_t reg_val;

	/* disable pre-warning timer1 interrupt */
	irq_disable(DT_INST_IRQN(0));

	if (!data->timeout_installed) {
		LOG_ERR("No valid WDT timeout installed");
		return -EINVAL;
	}

	if (sys_read8(base + REG_ETWCFG) & WDT_LEWDCNTL) {
		LOG_ERR("WDT is already running");
		return -EBUSY;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		LOG_ERR("WDT_OPT_PAUSE_IN_SLEEP is not supported");
		return -ENOTSUP;
	}

	/* pre-warning timer1 is 16-bit counter down timer */
	sys_write8((cnt0 >> 8) & 0xff, base + REG_ET1CNTLHR);
	sys_write8(cnt0 & 0xff, base + REG_ET1CNTLLR);

	/* clear pre-warning timer1 interrupt status */
	ite_intc_isr_clear(DT_INST_IRQN(0));

	/* enable pre-warning timer1 interrupt */
	irq_enable(DT_INST_IRQN(0));

	/* don't stop watchdog timer counting */
	reg_val = sys_read8(base + REG_ETWCTRL);
	sys_write8(reg_val & ~WDT_EWDSCEN, base + REG_ETWCTRL);

	/* set watchdog timer count */
	sys_write8((cnt1 >> 8) & 0xff, base + REG_EWDCNTHR);
	sys_write8(cnt1 & 0xff, base + REG_EWDCNTLR);

	/* allow to write timer1 count register */
	reg_val = sys_read8(base + REG_ETWCFG);
	sys_write8(reg_val & ~WDT_LET1CNTL, base + REG_ETWCFG);

	/*
	 * bit5 = 1: enable key match function to touch watchdog
	 * bit4 = 1: select watchdog clock source from prescaler
	 * bit3 = 1: lock watchdog count register (also mark as watchdog running)
	 * bit1 = 1: lock timer1 prescaler register
	 */
	sys_write8((WDT_EWDKEYEN | WDT_EWDSRC | WDT_LEWDCNTL | WDT_LET1PS), base + REG_ETWCFG);

	LOG_DBG("WDT Setup and enabled");

	return 0;
}

/*
 * reload the WDT and pre-warning timer1 counter
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel_id Index of the fed channel, and we only support
 *                   channel_id = 0 now.
 */
static int wdt_it51xxx_feed(const struct device *dev, int channel_id)
{
	const struct wdt_it51xxx_config *const wdt_config = dev->config;
	struct wdt_it51xxx_data *data = dev->data;
	const uintptr_t base = wdt_config->base;
	uint16_t cnt0 = WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT(data->timeout);
	uint8_t reg_val;

	ARG_UNUSED(channel_id);

	/* reset pre-warning timer1 */
	reg_val = sys_read8(base + REG_ETWCTRL);
	sys_write8(reg_val | WDT_ET1RST, base + REG_ETWCTRL);

	/* restart watchdog timer */
	sys_write8(IT51XXX_WATCHDOG_MAGIC_BYTE, base + REG_EWDKEYR);

	/* reset pre-warning timer1 to default if time is touched */
	if (data->wdt_warning_fired) {
		data->wdt_warning_fired = 0;

		/* pre-warning timer1 is 16-bit counter down timer */
		sys_write8((cnt0 >> 8) & 0xff, base + REG_ET1CNTLHR);
		sys_write8(cnt0 & 0xff, base + REG_ET1CNTLLR);

		/* clear timer1 interrupt status */
		ite_intc_isr_clear(DT_INST_IRQN(0));

		/* enable timer1 interrupt */
		irq_enable(DT_INST_IRQN(0));
	}

	LOG_DBG("WDT Kicking");

	return 0;
}

static int wdt_it51xxx_disable(const struct device *dev)
{
	const struct wdt_it51xxx_config *const wdt_config = dev->config;
	struct wdt_it51xxx_data *data = dev->data;
	const uintptr_t base = wdt_config->base;
	uint8_t reg_val;

	/* stop watchdog timer counting */
	reg_val = sys_read8(base + REG_ETWCTRL);
	sys_write8(reg_val | WDT_EWDSCEN, base + REG_ETWCTRL);

	/* unlock watchdog count register (also mark as watchdog not running) */
	reg_val = sys_read8(base + REG_ETWCFG);
	sys_write8(reg_val & ~WDT_LEWDCNTL, base + REG_ETWCFG);

	/* disable pre-warning timer1 interrupt */
	irq_disable(DT_INST_IRQN(0));

	/* mark uninstalled */
	data->timeout_installed = false;

	LOG_DBG("WDT Disabled");

	return 0;
}

static void wdt_it51xxx_isr(const struct device *dev)
{
	const struct wdt_it51xxx_config *const wdt_config = dev->config;
	struct wdt_it51xxx_data *data = dev->data;
	const uintptr_t base = wdt_config->base;
	uint8_t reg_val;

	/* clear pre-warning timer1 interrupt status */
	ite_intc_isr_clear(DT_INST_IRQN(0));

	/* reset pre-warning timer1 */
	reg_val = sys_read8(base + REG_ETWCTRL);
	sys_write8(reg_val | WDT_ET1RST, base + REG_ETWCTRL);

	/* callback function, ex. print warning message */
	if (data->callback) {
		data->callback(dev, 0);
	}

	if (IS_ENABLED(CONFIG_WDT_ITE_REDUCE_WARNING_LEADING_TIME)) {
		/*
		 * Once warning timer triggered: if watchdog timer isn't reloaded,
		 * then we will reduce interval of warning timer to 30ms to print
		 * more warning messages before watchdog reset.
		 */
		if (!(data->wdt_warning_fired)) {
			uint16_t cnt0 = WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT(30);

			/* pre-warning timer1 is 16-bit counter down timer */
			sys_write8((cnt0 >> 8) & 0xff, base + REG_ET1CNTLHR);
			sys_write8(cnt0 & 0xff, base + REG_ET1CNTLLR);

			/* clear pre-warning timer1 interrupt status */
			ite_intc_isr_clear(DT_INST_IRQN(0));
		}
	}
	data->wdt_warning_fired++;

	LOG_DBG("WDT ISR");
}

static DEVICE_API(wdt, wdt_it51xxx_api) = {
	.setup = wdt_it51xxx_setup,
	.disable = wdt_it51xxx_disable,
	.install_timeout = wdt_it51xxx_install_timeout,
	.feed = wdt_it51xxx_feed,
};

static int wdt_it51xxx_init(const struct device *dev)
{
	const struct wdt_it51xxx_config *const wdt_config = dev->config;
	const uintptr_t base = wdt_config->base;
	uint8_t reg_val;

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_it51xxx_disable(dev);
	}

	/* unlock access to watchdog registers */
	sys_write8(0x00, base + REG_ETWCFG);

	/* set WDT and timer1 to use 1.024kHz clock */
	sys_write8(WDT_ETPS_1P024_KHZ, base + REG_ET1PSR);

	/* set WDT key match enabled and WDT clock to use ET1PSR */
	sys_write8(WDT_EWDKEYEN | WDT_EWDSRC, base + REG_ETWCFG);

	/*
	 * select the mode that watchdog can be stopped, this is needed for
	 * wdt_it51xxx_disable() api and WDT_OPT_PAUSE_HALTED_BY_DBG flag
	 */
	reg_val = sys_read8(base + REG_ETWCTRL);
	sys_write8(reg_val | WDT_EWDSCMS, base + REG_ETWCTRL);

	IRQ_CONNECT(DT_INST_IRQN(0), 0, wdt_it51xxx_isr, DEVICE_DT_INST_GET(0), 0);
	return 0;
}

static const struct wdt_it51xxx_config wdt_it51xxx_cfg_0 = {
	.base = DT_INST_REG_ADDR(0),
};

static struct wdt_it51xxx_data wdt_it51xxx_dev_data;

DEVICE_DT_INST_DEFINE(0, wdt_it51xxx_init, NULL, &wdt_it51xxx_dev_data, &wdt_it51xxx_cfg_0,
		      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_it51xxx_api);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one ite,it51xxx-watchdog compatible node can be supported");
