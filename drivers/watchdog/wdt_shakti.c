/*
 * Copyright (C) 2020 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Watchdog (WDT) Driver for SiFive Freedom
 */

#define DT_DRV_COMPAT shakti_wdt

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/watchdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(wdt_sifive);

#define WDOGCFG_SCALE_MAX     0xf
#define WDOGCFG_SCALE_SHIFT   0
#define WDOGCFG_SCALE_MASK    (WDOGCFG_SCALE_MAX << WDOGCFG_SCALE_SHIFT)
#define WDOGCFG_RSTEN         BIT(8)
#define WDOGCFG_ZEROCMP       BIT(9)
#define WDOGCFG_ENALWAYS      BIT(12)
#define WDOGCFG_COREAWAKE     BIT(13)
#define WDOGCFG_IP0           BIT(28)

#define WDOGCMP_MAX        0xffff

#define WDOG_KEY           0x51f15e
#define WDOG_FEED          0xd09f00d

#define WDOG_CLK           32768



#define WATCHDOG_EN 			1<<0
#define RESET_INTERRUPT_MODE	1<<1
#define SOFT_RESET				1<<2


#define INTERRUPT_MODE			1
#define SOFT_RESET_MODE			7
#define RESET_MODE				3

#define WD_CYCLES_REG_ADDR		0x40400
#define CONTROL_REG_ADDR		0x40408
#define RESET_CYCLES_REG_ADDR	0x40410

volatile uint16_t *control_reg = (uint16_t*) (CONTROL_REG_ADDR);
	 volatile uint32_t *wd_cycles = (uint32_t*) (WD_CYCLES_REG_ADDR);
	 volatile uint16_t *reset_cycles = (uint16_t*) (RESET_CYCLES_REG_ADDR);


struct wdt_shakti_device_config {
	uintptr_t regs;
	uint16_t mode;
	uint16_t rcycles;
	uint32_t wcycles;

}obj;


struct wdt_shakti_dev_data {
	wdt_callback_t cb;
	bool enable_cb;
	bool timeout_valid;
};

#define DEV_REG(dev) \
	((struct wdt_sifive_reg *) \
	 ((const struct wdt_sifive_device_config *const)(dev)->config)->regs)




static int wdt_shakti_disable(const struct device *dev)
{
	*control_reg=*control_reg && 0xFFFE;
	return 0;
}

static int wdt_shakti_setup(const struct device *dev, uint8_t options)
{
	*control_reg=options;
	*reset_cycles=obj.rcycles;
	*wd_cycles=obj.wcycles;
	return 0;
}


static int wdt_shakti_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	return 0;	
}

static int wdt_shakti_feed(const struct device *dev, int channel_id)
{
	wdt_shakti_disable(dev);
	wdt_shakti_setup(dev,3);
}

static const struct wdt_driver_api wdt_shakti_api = {
	.setup = wdt_shakti_setup,
	.disable = wdt_shakti_disable,
	.feed = wdt_shakti_feed,
};



static int wdt_shakti_init(const struct device *dev)
{
	return 0;
}

static struct wdt_shakti_dev_data wdt_shakti_data;

static const struct wdt_shakti_device_config wdt_shakti_cfg = {
	.regs = DT_INST_REG_ADDR(0), 
	.rcycles = DT_INST_PROP(0, rcycles), 
	.wcycles = DT_INST_PROP(0, wcycles), 
};

DEVICE_DT_INST_DEFINE(0, wdt_shakti_init, NULL,
		      &wdt_shakti_data, &wdt_shakti_cfg, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_shakti_api);
