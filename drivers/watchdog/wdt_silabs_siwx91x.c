/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/irq.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <math.h>
#include "rsi_wwdt.h"
#include "rsi_sysrtc.h"

#define DT_DRV_COMPAT                       silabs_siwx91x_wdt
#define SIWX91X_WDT_SYSTEM_RESET_TIMER_MASK 0x0000001F

struct siwx91x_wdt_config {
	/* WDT register base address */
	MCU_WDT_Type *reg;
	/* Pointer to the clock device structure */
	const struct device *clock_dev;
	/* Clock control subsystem */
	clock_control_subsys_t clock_subsys;
	/* Function pointer for the IRQ (Interrupt Request) configuration */
	void (*irq_config)(void);
};

struct siwx91x_wdt_data {
	/* Callback function to be called on watchdog timer events */
	wdt_callback_t callback;
	/* WDT operating clock (LF-FSM) frequency */
	uint32_t clock_frequency;
	/* Timer system reset duration in ms */
	uint8_t delay_reset;
	/* Timer interrupt duration in ms */
	uint8_t delay_irq;
	/* Flag indicating the timeout install status */
	bool timeout_install_status;
	/* Flag indicating the setup status */
	bool setup_status;
};

/* Function to get the delay in milliseconds from the register value */
static uint32_t siwx91x_wdt_delay_from_hw(uint8_t value, int clock_frequency)
{
	uint32_t ticks = BIT(value);
	float timeout = (float)ticks / clock_frequency;

	timeout *= 1000;
	/* Return the timeout value as an unsigned 32-bit integer in milliseconds */
	return (uint32_t)timeout;
}

/* Function to get the register value from the delay in milliseconds */
static uint8_t siwx91x_wdt_delay_to_hw(uint32_t delay, int clock_frequency)
{
	/* reg_value = log((timeout * clock_frequency)/1000)base2 */
	float value = ((float)delay * (float)clock_frequency) / 1000;
	float result = log2f(value);

	/* Round the result to nearest integer */
	result = roundf(result);

	return (uint8_t)result;
}

static int siwx91x_wdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct siwx91x_wdt_data *data = dev->data;

	/* Check the WDT setup status */
	if (data->setup_status) {
		/* WDT setup is already done */
		return -EBUSY;
	}

	/* Check the WDT timeout  status */
	if (data->timeout_install_status) {
		/* Only single timeout can be installed */
		return -ENOMEM;
	}

	if (cfg->window.max > siwx91x_wdt_delay_from_hw(SIWX91X_WDT_SYSTEM_RESET_TIMER_MASK,
							data->clock_frequency) ||
	    cfg->window.max == 0) {
		/* Requested value is out of range */
		return -EINVAL;
	}

	if (cfg->window.min > 0) {
		/* This feature is currently not supported */
		return -ENOTSUP;
	}

	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
	case WDT_FLAG_RESET_CPU_CORE:
		if (cfg->callback != NULL) {
			/* Callback is not supported for reset flags */
			return -ENOTSUP;
		}
		data->delay_reset = siwx91x_wdt_delay_to_hw(cfg->window.max, data->clock_frequency);
		/* During a system or CPU core reset, interrupts are not needed. Thus, we set
		 * the interrupt time to 0 to ensure no interrupts occur while resetting.
		 */
		data->delay_irq = 0;
		/* Mask the WWDT interrupt */
		RSI_WWDT_IntrMask();
		break;

	case WDT_FLAG_RESET_NONE:
		/* Set the reset time to maximum value */
		data->delay_reset = SIWX91X_WDT_SYSTEM_RESET_TIMER_MASK;
		data->delay_irq = siwx91x_wdt_delay_to_hw(cfg->window.max, data->clock_frequency);
		if (cfg->callback != NULL) {
			data->callback = cfg->callback;
		}
		break;

	default:
		/* Unsupported WDT config options */
		return -ENOTSUP;
	}

	data->timeout_install_status = true;
	return 0;
}

/* Function to setup and start WDT */
static int siwx91x_wdt_setup(const struct device *dev, uint8_t options)
{
	const struct siwx91x_wdt_config *config = dev->config;
	struct siwx91x_wdt_data *data = dev->data;

	/* Check the WDT setup status */
	if (data->setup_status) {
		/* WDT is already running */
		return -EBUSY;
	}

	/* Check the WDT timeout  status */
	if (!data->timeout_install_status) {
		/* Timeout need to be set before setup */
		return -ENOTSUP;
	}

	if (options & (WDT_OPT_PAUSE_IN_SLEEP)) {
		return -ENOTSUP;
	}

	RSI_WWDT_ConfigSysRstTimer(config->reg, data->delay_reset);
	RSI_WWDT_ConfigIntrTimer(config->reg, data->delay_irq);

	RSI_WWDT_Start(config->reg);

	data->setup_status = true;

	return 0;
}

static int siwx91x_wdt_disable(const struct device *dev)
{
	const struct siwx91x_wdt_config *config = dev->config;
	struct siwx91x_wdt_data *data = dev->data;

	if (!data->timeout_install_status) {
		/* No timeout installed */
		return -EFAULT;
	}

	RSI_WWDT_Disable(config->reg);

	data->timeout_install_status = false;
	data->setup_status = false;

	return 0;
}

static int siwx91x_wdt_feed(const struct device *dev, int channel_id)
{
	const struct siwx91x_wdt_config *config = dev->config;
	struct siwx91x_wdt_data *data = dev->data;

	if (!(data->timeout_install_status && data->setup_status)) {
		/* WDT is not configured */
		return -EINVAL;
	}

	if (channel_id != 0) {
		/* Channel id must be 0 */
		return -EINVAL;
	}

	RSI_WWDT_ReStart(config->reg);
	return 0;
}

static void siwx91x_wdt_isr(const struct device *dev)
{
	const struct siwx91x_wdt_config *config = dev->config;
	struct siwx91x_wdt_data *data = dev->data;

	/* Clear WDT interrupt */
	RSI_WWDT_IntrClear();

	if (data->delay_irq) {
		/* Restart the timer */
		RSI_WWDT_ReStart(config->reg);
	}

	if (data->callback != NULL) {
		data->callback(dev, 0);
	}
}

static int siwx91x_wdt_init(const struct device *dev)
{
	const struct siwx91x_wdt_config *config = dev->config;
	struct siwx91x_wdt_data *data = dev->data;
	int ret;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret) {
		return ret;
	}
	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys,
				     &data->clock_frequency);
	if (ret) {
		return ret;
	}

	RSI_WWDT_Init(config->reg);

	config->irq_config();
	RSI_WWDT_IntrUnMask();

	return 0;
}

static DEVICE_API(wdt, siwx91x_wdt_driver_api) = {
	.setup = siwx91x_wdt_setup,
	.disable = siwx91x_wdt_disable,
	.install_timeout = siwx91x_wdt_install_timeout,
	.feed = siwx91x_wdt_feed,
};

#define siwx91x_WDT_INIT(inst)                                                                     \
	static struct siwx91x_wdt_data siwx91x_wdt_data_##inst;                                    \
	static void siwx91x_wdt_irq_configure_##inst(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(inst, irq), DT_INST_IRQ(inst, priority), siwx91x_wdt_isr,  \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ(inst, irq));                                                \
	}                                                                                          \
	static const struct siwx91x_wdt_config siwx91x_wdt_config_##inst = {                       \
		.reg = (MCU_WDT_Type *)DT_INST_REG_ADDR(inst),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clkid),          \
		.irq_config = siwx91x_wdt_irq_configure_##inst,                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &siwx91x_wdt_init, NULL, &siwx91x_wdt_data_##inst,             \
			      &siwx91x_wdt_config_##inst, PRE_KERNEL_1,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &siwx91x_wdt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(siwx91x_WDT_INIT)
