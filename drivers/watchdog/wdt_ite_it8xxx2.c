/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_watchdog

#include <drivers/watchdog.h>
#include <errno.h>
#include <soc.h>

#include <logging/log.h>
#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
LOG_MODULE_REGISTER(wdt_ite_it8xxx2);

#define IT8XXX2_WATCHDOG_MAGIC_BYTE			0x5c
#define WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT(ms)	((ms) * 1024 / 1000)

/* enter critical period or not */
static int wdt_warning_fired;

/* device config */
struct wdt_it8xxx2_config {
	/* wdt register base address */
	uintptr_t base;
};

/* driver data */
struct wdt_it8xxx2_data {
	/* timeout callback used to handle watchdog event */
	wdt_callback_t callback;
	/* indicate whether a watchdog timeout is installed */
	bool timeout_installed;
	/* watchdog feed timeout in milliseconds */
	uint32_t timeout;
};

/* driver convenience defines */
#define DRV_CONFIG(dev) ((const struct wdt_it8xxx2_config *)(dev)->config)
#define DRV_DATA(dev) ((struct wdt_it8xxx2_data *)(dev)->data)
#define DRV_REG(dev) (struct wdt_it8xxx2_regs *)(DRV_CONFIG(dev)->base)

static int wdt_it8xxx2_install_timeout(const struct device *dev,
					  const struct wdt_timeout_cfg *config)
{
	struct wdt_it8xxx2_data *data = DRV_DATA(dev);
	struct wdt_it8xxx2_regs *const inst = DRV_REG(dev);

	/* if watchdog is already running */
	if ((inst->ETWCFG) & IT8XXX2_WDT_LEWDCNTL) {
		return -EBUSY;
	}

	/* no window watchdog support */
	if (config->window.min != 0) {
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

static int wdt_it8xxx2_setup(const struct device *dev, uint8_t options)
{
	struct wdt_it8xxx2_data *data = DRV_DATA(dev);
	struct wdt_it8xxx2_regs *const inst = DRV_REG(dev);
	uint16_t cnt0 = WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT(data->timeout);
	uint16_t cnt1 = WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT((data->timeout
			+ CONFIG_WDT_ITE_WARNING_LEADING_TIME_MS));

	/* disable pre-warning timer1 interrupt */
	irq_disable(DT_INST_IRQN(0));

	if (!data->timeout_installed) {
		LOG_ERR("No valid WDT timeout installed");
		return -EINVAL;
	}

	if ((inst->ETWCFG) & IT8XXX2_WDT_LEWDCNTL) {
		LOG_ERR("WDT is already running");
		return -EBUSY;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		LOG_ERR("WDT_OPT_PAUSE_IN_SLEEP is not supported");
		return -ENOTSUP;
	}

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0) {
		LOG_ERR("WDT_OPT_PAUSE_HALTED_BY_DBG is not supported");
		return -ENOTSUP;
	}

	/* pre-warning timer1 is 16-bit counter down timer */
	inst->ET1CNTLHR = (cnt0 >> 8) & 0xff;
	inst->ET1CNTLLR = cnt0 & 0xff;

	/* clear pre-warning timer1 interrupt status */
	ite_intc_isr_clear(DT_INST_IRQN(0));

	/* enable pre-warning timer1 interrupt */
	irq_enable(DT_INST_IRQN(0));

	/* set watchdog timer count */
	inst->EWDCNTHR = (cnt1 >> 8) & 0xff;
	inst->EWDCNTLR = cnt1 & 0xff;

	/* allow to write timer1 count register */
	inst->ETWCFG &= ~IT8XXX2_WDT_LET1CNTL;

	/*
	 * bit5 = 1: enable key match function to touch watchdog
	 * bit4 = 1: select watchdog clock source from prescaler
	 * bit3 = 1: lock watchdog count register
	 * bit1 = 1: lock timer1 prescaler register
	 * bit0 = 1: lock watchdog and timer1 config register
	 */
	inst->ETWCFG = (IT8XXX2_WDT_EWDKEYEN |
			IT8XXX2_WDT_EWDSRC |
			IT8XXX2_WDT_LEWDCNTL |
			IT8XXX2_WDT_LET1PS |
			IT8XXX2_WDT_LETWCFG);

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
static int wdt_it8xxx2_feed(const struct device *dev, int channel_id)
{
	struct wdt_it8xxx2_data *data = DRV_DATA(dev);
	struct wdt_it8xxx2_regs *const inst = DRV_REG(dev);
	uint16_t cnt0 = WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT(data->timeout);

	ARG_UNUSED(channel_id);

	/* reset pre-warning timer1 */
	inst->ETWCTRL |= IT8XXX2_WDT_ET1RST;

	/* restart watchdog timer */
	inst->EWDKEYR = IT8XXX2_WATCHDOG_MAGIC_BYTE;

	/* reset pre-warning timer1 to default if time is touched */
	if (wdt_warning_fired) {
		wdt_warning_fired = 0;

		/* pre-warning timer1 is 16-bit counter down timer */
		inst->ET1CNTLHR = (cnt0 >> 8) & 0xff;
		inst->ET1CNTLLR = cnt0 & 0xff;

		/* clear timer1 interrupt status */
		ite_intc_isr_clear(DT_INST_IRQN(0));

		/* enable timer1 interrupt */
		irq_enable(DT_INST_IRQN(0));
	}

	LOG_DBG("WDT Kicking");

	return 0;
}

static int wdt_it8xxx2_disable(const struct device *dev)
{
	struct wdt_it8xxx2_data *data = DRV_DATA(dev);
	struct wdt_it8xxx2_regs *const inst = DRV_REG(dev);

	/* stop watchdog timer counting */
	inst->ETWCTRL |= IT8XXX2_WDT_EWDSCEN;

	/* disable pre-warning timer1 interrupt */
	irq_disable(DT_INST_IRQN(0));

	/* mark uninstalled */
	data->timeout_installed = false;

	LOG_DBG("WDT Disabled");

	return 0;
}

static void wdt_it8xxx2_isr(const struct device *dev)
{
	struct wdt_it8xxx2_data *data = DRV_DATA(dev);
	struct wdt_it8xxx2_regs *const inst = DRV_REG(dev);
	uint16_t cnt0 = WARNING_TIMER_PERIOD_MS_TO_1024HZ_COUNT(data->timeout);

	/* clear pre-warning timer1 interrupt status */
	ite_intc_isr_clear(DT_INST_IRQN(0));

	/* reset pre-warning timer1 */
	inst->ETWCTRL |= IT8XXX2_WDT_ET1RST;

	/* callback function, ex. print warning message */
	if (data->callback) {
		data->callback(dev, 0);
	}

	/*
	 * Reduce interval of warning timer, so we can print more
	 * warning messages during critical period.
	 */
	if (!wdt_warning_fired++) {
		/* pre-warning timer1 is 16-bit counter down timer */
		inst->ET1CNTLHR = (cnt0 >> 8) & 0xff;
		inst->ET1CNTLLR = cnt0 & 0xff;

		/* clear pre-warning timer1 interrupt status */
		ite_intc_isr_clear(DT_INST_IRQN(0));
	}

	LOG_DBG("WDT ISR");
}

static const struct wdt_driver_api wdt_it8xxx2_api = {
	.setup = wdt_it8xxx2_setup,
	.disable = wdt_it8xxx2_disable,
	.install_timeout = wdt_it8xxx2_install_timeout,
	.feed = wdt_it8xxx2_feed,
};

static int wdt_it8xxx2_init(const struct device *dev)
{
	struct wdt_it8xxx2_regs *const inst = DRV_REG(dev);

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_it8xxx2_disable(dev);
	}

	/* unlock access to watchdog registers */
	inst->ETWCFG = 0x00;

	/* set WDT and timer1 to use 1.024kHz clock */
	inst->ET1PSR = IT8XXX2_WDT_ETPS_1P024_KHZ;

	/* set WDT key match enabled and WDT clock to use ET1PSR */
	inst->ETWCFG = (IT8XXX2_WDT_EWDKEYEN |
			IT8XXX2_WDT_EWDSRC);

	/* watchdog can be stopped */
	inst->ETWCTRL |= IT8XXX2_WDT_EWDSCMS;

	IRQ_CONNECT(DT_INST_IRQN(0), 0, wdt_it8xxx2_isr,
		    DEVICE_DT_INST_GET(0), 0);
	return 0;
}

static const struct wdt_it8xxx2_config wdt_it8xxx2_cfg_0 = {
	.base = DT_INST_REG_ADDR(0),
};

static struct wdt_it8xxx2_data wdt_it8xxx2_dev_data;

DEVICE_DT_INST_DEFINE(0, wdt_it8xxx2_init, NULL,
			&wdt_it8xxx2_dev_data, &wdt_it8xxx2_cfg_0,
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&wdt_it8xxx2_api);
