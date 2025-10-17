/*
 * Copyright (C) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT syna_watchdog

#define WDT_CTRL_WCS 0x00
#define WDT_CTRL_WOR 0x08

#define WDT_REF_WRR 0x00

#define WCS_ENABLED_BIT  (0)
#define WCS_ENABLED_MASK (1 << WCS_ENABLED_BIT)
#define WCS_STATUS_BIT   (1)
#define WCS_STATUS_MASK  (1 << WCS_STATUS_BIT)

#define AON_POR_RST_M4_WATCHDOG_BIT  (8)
#define AON_POR_RST_M55_WATCHDOG_BIT (9)

#define SYSCOUNT_CTRL_CNTCR      0x0
#define CNTCR_EN                 (1 << 0)
#define CNTCR_INTMASK            (1 << 3)
#define SYSCOUNT_CTRL_CNTCV_LOW  0x8
#define SYSCOUNT_CTRL_CNTCV_HIGH 0xC

#include <soc.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>

struct wdt_syna_dev_config {
	mem_addr_t aon_por_rst;
	mem_addr_t wdt_ctrl_base;
	mem_addr_t wdt_ref_base;
	mem_addr_t syscount_base;
	uint32_t irq_num;
	uint32_t clk_freq;
};

struct wdt_syna_dev_data {
	wdt_callback_t cb;
	uint32_t timeout_ticks;
	uint8_t mode;
	bool timeout_valid;
	bool is_serviced;
};

static void wdt_syna_irq_config(void);

/* When the watchdog counter rolls over, the SCU will generate a pre-warning
 * event which gets routed to the ISR below. If the watchdog is not serviced,
 * the SCU will only reset the MCU after the second time the counter rolls
 * over. Hence the reset will only happen after 2*cfg->window.max have
 * elapsed. We could potentially manually reset the MCU, but this way the
 * context information (i.e. that reset happened because of a watchdog) is
 * lost.
 */
static void wdt_syna_isr(const struct device *dev)
{
	const struct wdt_syna_dev_config *config = dev->config;
	struct wdt_syna_dev_data *data = dev->data;

	/* this is a level triggered interrupt. the event must be cleared */
	sys_clear_bit(config->wdt_ctrl_base + WDT_CTRL_WCS, WCS_STATUS_BIT);

	data->is_serviced = false;

	if (data->cb) {
		data->cb(dev, 0);
	}

	/* Ensure that watchdog is serviced if RESET_NONE mode is used */
	if (data->mode == WDT_FLAG_RESET_NONE && !data->is_serviced) {
		sys_write32(1, config->wdt_ref_base + WDT_REF_WRR);
		data->is_serviced = true;
	}

	/* If WDT_FLAG_RESET_CPU_CORE or WDT_FLAG_RESET_SOC is used, spin in a loop.
	 * The first event is triggered after config->window.max milliseconds, and
	 * the actual reset will occur after a second period of the same length
	 * passes.
	 */
	if (data->mode != WDT_FLAG_RESET_NONE && !data->is_serviced) {
		while (true) {
		}
	}
}

/* NMI on this platform is sourced from the watchdog expiry signal, so
 * realistically this should be the only NMI handler present on applicable
 * devices.
 */
static void wdt_syna_nmi_handler(void)
{
	const struct device *device = DEVICE_DT_INST_GET(0);
	const struct wdt_syna_dev_config *config = device->config;
	uint32_t status = sys_read32(config->wdt_ctrl_base + WDT_CTRL_WCS);

	if (status & WCS_STATUS_MASK) {
		wdt_syna_isr(device);
	} else {
		while (true) {
		}
	}
}

static int wdt_syna_disable(const struct device *dev)
{
	const struct wdt_syna_dev_config *config = dev->config;
	struct wdt_syna_dev_data *data = dev->data;

	/* Disable & Stop the watchdog timer */
	sys_clear_bit(config->wdt_ctrl_base + WDT_CTRL_WCS, WCS_ENABLED_BIT);
	data->timeout_valid = false;
	/* Clear AON reset mask */
	if (data->mode == WDT_FLAG_RESET_SOC) {
		sys_set_bit(config->aon_por_rst, AON_POR_RST_M4_WATCHDOG_BIT);
		sys_set_bit(config->aon_por_rst, AON_POR_RST_M55_WATCHDOG_BIT);
	}

	return 0;
}

static int wdt_syna_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_syna_dev_config *config = dev->config;
	struct wdt_syna_dev_data *data = dev->data;
	int rc = 0;

	ARG_UNUSED(options);

	if (data->timeout_valid == false) {
		rc = -EINVAL;
		goto exit;
	}

	/* Check if the System Counter has already been initialized */
	if (sys_test_bit(config->syscount_base + SYSCOUNT_CTRL_CNTCR, CNTCR_EN) == 0U) {
		uint32_t value;

		value = sys_read32(config->syscount_base + SYSCOUNT_CTRL_CNTCR);
		value &= ~CNTCR_EN;
		sys_write32(value, config->syscount_base + SYSCOUNT_CTRL_CNTCR);
		sys_write32(0U, config->syscount_base + SYSCOUNT_CTRL_CNTCV_HIGH);
		sys_write32(0U, config->syscount_base + SYSCOUNT_CTRL_CNTCV_LOW);
		value &= ~CNTCR_INTMASK;
		sys_write32(value, config->syscount_base + SYSCOUNT_CTRL_CNTCR);
		value |= CNTCR_EN;
		sys_write32(value, config->syscount_base + SYSCOUNT_CTRL_CNTCR);
	}

	/* Configure AON reset mask */
	if (data->mode == WDT_FLAG_RESET_SOC) {
		sys_clear_bit(config->aon_por_rst, AON_POR_RST_M4_WATCHDOG_BIT);
		sys_clear_bit(config->aon_por_rst, AON_POR_RST_M55_WATCHDOG_BIT);
	}

	/* Set timeout value */
	sys_write32(data->timeout_ticks, config->wdt_ctrl_base + WDT_CTRL_WOR);
	/* Enable & Start the watchdog timer */
	sys_set_bit(config->wdt_ctrl_base + WDT_CTRL_WCS, WCS_ENABLED_BIT);

exit:
	return rc;
}

static int wdt_syna_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_syna_dev_config *config = dev->config;
	struct wdt_syna_dev_data *data = dev->data;
	int rc = 0;
	uint64_t timeout_ticks;

	/* disable the watchdog if timeout was already installed */
	if (data->timeout_valid) {
		wdt_syna_disable(dev);
		data->timeout_valid = false;
	}

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		rc = -EINVAL;
		goto exit;
	}

	if (cfg->flags == WDT_FLAG_RESET_NONE && cfg->callback == NULL) {
		rc = -EINVAL;
		goto exit;
	}

	if (cfg->flags == WDT_FLAG_RESET_CPU_CORE) {
		rc = -EINVAL;
		goto exit;
	}

	/* Check if the timeout can be represented */
	timeout_ticks = (uint64_t)config->clk_freq * cfg->window.max;
	timeout_ticks /= 1000;
	if (timeout_ticks > UINT32_MAX) {
		rc = -EINVAL;
		goto exit;
	}

	data->cb = cfg->callback;
	data->mode = cfg->flags;
	data->timeout_valid = true;
	data->timeout_ticks = timeout_ticks;

exit:
	return rc;
}

static int wdt_syna_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);
	const struct wdt_syna_dev_config *config = dev->config;
	struct wdt_syna_dev_data *data = dev->data;

	sys_write32(1, config->wdt_ref_base + WDT_REF_WRR);
	data->is_serviced = true;

	return 0;
}

static const struct wdt_driver_api wdt_syna_api = {
	.setup = wdt_syna_setup,
	.disable = wdt_syna_disable,
	.install_timeout = wdt_syna_install_timeout,
	.feed = wdt_syna_feed,
};

static int wdt_syna_init(const struct device *dev)
{
	wdt_syna_irq_config();
	z_arm_nmi_set_handler(wdt_syna_nmi_handler);

#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	return 0;
#else
#ifdef CONFIG_WDT_SYNA_DEFAULT_TIMEOUT_MAX
	int ret;
	const struct wdt_timeout_cfg cfg = {.window.max = CONFIG_WDT_SYNA_DEFAULT_TIMEOUT_MAX,
					    .flags = WDT_FLAG_RESET_SOC};

	ret = wdt_syna_install_timeout(dev, &cfg);
	if (ret < 0) {
		return ret;
	}

	return wdt_syna_setup(dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
#else
	return 0;
#endif
#endif
}

#define SYNA_WDT_INIT(n)                                                                           \
	BUILD_ASSERT((n) == 0, "There should only be one instance of syna,watchdog!");             \
	static struct wdt_syna_dev_data wdt_syna_data;                                             \
	static const struct wdt_syna_dev_config wdt_syna_config = {                                \
		.aon_por_rst = DT_INST_REG_ADDR_BY_NAME(n, aon_por_rst),                           \
		.wdt_ctrl_base = DT_INST_REG_ADDR_BY_NAME(n, wdt_ctrl),                            \
		.wdt_ref_base = DT_INST_REG_ADDR_BY_NAME(n, wdt_ref),                              \
		.syscount_base = DT_INST_REG_ADDR_BY_NAME(n, syscount),                            \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.clk_freq = DT_INST_PROP(n, clock_frequency),                                      \
	};                                                                                         \
	static void wdt_syna_irq_config(void)                                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), wdt_syna_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, wdt_syna_init, NULL, &wdt_syna_data, &wdt_syna_config,            \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_syna_api);

DT_INST_FOREACH_STATUS_OKAY(SYNA_WDT_INIT)
