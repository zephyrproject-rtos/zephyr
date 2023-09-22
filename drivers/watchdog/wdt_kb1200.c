/* wdt_kb1200.c - ENE KB1200 watchdog driver */

#define DT_DRV_COMPAT ene_kb1200_watchdog

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/watchdog.h>
#include <soc.h>
#include <errno.h>

LOG_MODULE_REGISTER(wdog_kb1200, CONFIG_WDT_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "add exactly one wdog node to the devicetree");


struct wdt_kb1200_regs {
	volatile uint32_t WDTCFG;          // 0x4006_0000
	volatile uint32_t WDTIE;           // 0x4006_0004
	volatile uint32_t WDTPF;           // 0x4006_0008
	volatile uint32_t WDTM;            // 0x4006_000C
	volatile uint32_t WDTSCR;          // 0x4006_0010
	volatile uint32_t Reserved[0x6C];  // 0x4006_0014
	volatile uint32_t WDTC;            // 0x4006_0080
};

struct wdt_kb1200_config {
	struct wdt_kb1200_regs *regs;
};

struct wdt_kb1200_data {
	wdt_callback_t cb;
	bool timeout_installed;
};

static int wdt_kb1200_setup(const struct device *dev, uint8_t options)
{
	struct wdt_kb1200_config const *cfg = dev->config;
	struct wdt_kb1200_data *data = dev->data;
	struct wdt_kb1200_regs *regs = cfg->regs;

	//printk("%s\n", __func__);
	regs->WDTPF |= 0x0003;

	if (!data->timeout_installed) {
		LOG_ERR("No valid WDT timeout installed");
		return -EINVAL;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_WRN("WDT_OPT_PAUSE_IN_SLEEP is not supported");
		return -ENOTSUP;
	}

	regs->WDTCFG &= 0xFFFFFF0F;
	regs->WDTCFG |= 1;

	//printk("%s WDT Setup and enabled.\n", __func__);
	LOG_DBG("WDT Setup and enabled");

	return 0;
}

static int wdt_kb1200_disable(const struct device *dev)
{
	struct wdt_kb1200_config const *cfg = dev->config;
	struct wdt_kb1200_data *data = dev->data;
	struct wdt_kb1200_regs *regs = cfg->regs;

	//printk("%s\n", __func__);

	if (!(regs->WDTCFG & 1)) {
		return -EALREADY;
	}

	regs->WDTCFG = (regs->WDTCFG & 0xFFFFFF0E) | 0x00000090;  // bit 7~4 = 1001b
	data->timeout_installed = false;

	//printk("%s WDT Disabled.\n", __func__);
	LOG_DBG("WDT Disabled");

	return 0;
}

static int wdt_kb1200_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *config)
{
	struct wdt_kb1200_config const *cfg = dev->config;
	struct wdt_kb1200_data *data = dev->data;
	struct wdt_kb1200_regs *regs = cfg->regs;

	//printk("%s\n", __func__);
	if (config->window.min > 0U) {
		data->timeout_installed = false;
		return -EINVAL;
	}

	regs->WDTM = 0;

	data->cb = config->callback;
	if (data->cb) {
		regs->WDTIE |= 1;

		LOG_DBG("WDT callback enabled");
	} else {
		/* Setting WDT_FLAG_RESET_SOC or not will have no effect:
		 * even after the cb, if anything is done, SoC will reset
		 */
		regs->WDTIE &= ~1;

		LOG_DBG("WDT Reset enabled");
	}

	/* Since it almost takes 1ms to decrement the load register
	 * (See datasheet 18.6.1.4: 33/32.768 KHz = 1.007ms)
	 * Let's use the given window directly.
	 */
	regs->WDTM = config->window.max;

	data->timeout_installed = true;
	//printk("%s regs->WDTM = %d\n", __func__, config->window.max);

	return 0;
}

static int wdt_kb1200_feed(const struct device *dev, int channel_id)
{
	struct wdt_kb1200_config const *cfg = dev->config;
	struct wdt_kb1200_regs *regs = cfg->regs;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	//printk("%s\n", __func__);
	if (!(regs->WDTCFG & 1)) {
		return -EINVAL;
	}

	LOG_DBG("WDT Kicking");
	regs->WDTCFG |= 1;

	return 0;
}

static void wdt_kb1200_isr(const struct device *dev)
{
	struct wdt_kb1200_data *data = dev->data;

	//printk("%s\n", __func__);
	LOG_DBG("WDT ISR");

	if (data->cb) {
		data->cb(dev, 0);
	}
}

static const struct wdt_driver_api wdt_kb1200_api = {
	.setup = wdt_kb1200_setup,
	.disable = wdt_kb1200_disable,
	.install_timeout = wdt_kb1200_install_timeout,
	.feed = wdt_kb1200_feed,
};

static int wdt_kb1200_init(const struct device *dev)
{
	//printk("%s\n", __func__);
	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_kb1200_disable(dev);
	}

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    wdt_kb1200_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static const struct wdt_kb1200_config wdt_kb1200_config = {
	.regs = (struct wdt_kb1200_regs *)(DT_INST_REG_ADDR(0)),
};

static struct wdt_kb1200_data wdt_kb1200_dev_data;

DEVICE_DT_INST_DEFINE(0, wdt_kb1200_init, NULL,
		    &wdt_kb1200_dev_data, &wdt_kb1200_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdt_kb1200_api);
