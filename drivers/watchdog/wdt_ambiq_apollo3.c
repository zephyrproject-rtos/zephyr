#include <drivers/watchdog.h>

#define LFRC_PRESCALER_OFF (0x0)
#define LFRC_PRESCALER_128_HZ (0x1)
#define LFRC_PRESCALER_16_HZ (0x2)
#define LFRC_PRESCALER_1_HZ (0x3)
#define LFRC_PRESCALER_1_OVER_16_HZ (0x4)
#define WDT_MAX_VALUE 255

/* Device constant configuration parameters */
struct wdt_ambiq_cfg {
	uintptr_t cfg;
	uintptr_t rstrt;
	uintptr_t lock;
	uintptr_t count;
	uintptr_t inten;
	uintptr_t intstat;
	uintptr_t intclr;
	uintptr_t intset;
};

struct wdt_ambiq_data {
	wdt_callback_t cb;
};

static struct wdt_ambiq_data = { 0 };

#define DEV_CFG(dev) ((const struct wdt_ambiq_cfg *const)(dev)->config)

static void wdt_ambiq_isr(const struct device *dev)
{
	uintptr_t intclr = DEV_CFG(dev)->intclr;
	sys_write8(1, intclr);

	data->cb(dev, 0);
}

static int wdt_ambiq_disable(const struct device *dev)
{
	uintptr_t cfg = DEV_CFG(dev)->cfg;
	// Disconnect the clock ource and turn off the actual WDT.
	uint32_t mask = (0x00ffffff | 0xfffffffe);
	sys_write32((sys_read32(cfg) | mask), cfg);

	return 0;
}

static int wdt_ambiq_setup(const struct device *dev, uint8_t options)
{
	return 0;
}

static int wdt_ambiq_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	return 0;
}

static int wdt_ambiq_feed(const struct device *dev, int channel_id)
{
	uintptr_t rstrt = DEV_CFG(dev)->rstrt;
	sys_write8(0xb2, rstrt);

	return 0;
}

static const struct wdt_driver_api wdt_ambiq_api = {
	.setup = wdt_ambiq_setup,
	.disable = wdt_ambiq_disable,
	.install_timeout = wdt_ambiq_install_timeout,
	.feed = wdt_ambiq_feed,
};

static int wdt_ambiq_irq_config(void)
{
	// TODO(michalc): watchdog uses IRQ 1. Brownout uses IRQ 0 in the irq table.
	// There might be errors here! Double check!
	IRQ_CONNECT(DT_INST_IRQN(1), DT_INST_IRQ(1, priority), wdt_ambiq_isr, DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQN(1));
}

static int wdt_ambiq_init(const struct device *dev)
{
#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_ambiq_disable(dev);
#endif
	wdt_ambiq_irq_config();
	return 0;
}

DEVICE_DT_INST_DEFINE(0, wdt_ambiq_init, NULL, &wdt_ambiq_data, &wdt_ambiq_cfg, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_ambiq_api);
