/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_wdt

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include "r_wdt.h"
#include <math.h>

LOG_MODULE_REGISTER(wdt_renesas_rz, CONFIG_WDT_LOG_LEVEL);

struct wdt_rz_config {
	const wdt_api_t *fsp_api;
	IRQn_Type irq;
};

struct wdt_rz_data {
	struct wdt_timeout_cfg timeout;
	wdt_instance_ctrl_t *fsp_ctrl;
	wdt_cfg_t *fsp_cfg;
	struct k_mutex inst_lock;
	uint32_t clock_rate;
	atomic_t device_state;
#ifdef CONFIG_WDT_RENESAS_RZ_USE_ICU
	struct k_work interrupt_work;
#endif
};

#define WDT_RZ_ATOMIC_ENABLE      (0)
#define WDT_RZ_ATOMIC_TIMEOUT_SET (1)

#ifdef CONFIG_WDT_RENESAS_RZ_USE_ICU
void wdt_underflow_isr(void *irq);
#define WDT_RZ_ISR                      wdt_underflow_isr
#define WDT_RZ_TIMEOUT_UNSUPPORT        0xFF
#define WDT_RZ_CLOCK_DIVISION_UNSUPPORT 0xFF

/* Lookup table for WDT period raw cycle */
static const float timeout_period_lut[] = {[WDT_TIMEOUT_128] = WDT_RZ_TIMEOUT_UNSUPPORT,
					   [WDT_TIMEOUT_512] = WDT_RZ_TIMEOUT_UNSUPPORT,
					   [WDT_TIMEOUT_1024] = 1024,
					   [WDT_TIMEOUT_2048] = WDT_RZ_TIMEOUT_UNSUPPORT,
					   [WDT_TIMEOUT_4096] = 4096,
					   [WDT_TIMEOUT_8192] = 8192,
					   [WDT_TIMEOUT_16384] = 16384};

/* Lookup table for the division value of the input clock count */
static const float clock_div_lut[] = {[WDT_CLOCK_DIVISION_1] = WDT_RZ_CLOCK_DIVISION_UNSUPPORT,
				      [WDT_CLOCK_DIVISION_4] = 4,
				      [WDT_CLOCK_DIVISION_16] = WDT_RZ_CLOCK_DIVISION_UNSUPPORT,
				      [WDT_CLOCK_DIVISION_32] = WDT_RZ_CLOCK_DIVISION_UNSUPPORT,
				      [WDT_CLOCK_DIVISION_64] = 64,
				      [WDT_CLOCK_DIVISION_128] = 128,
				      [WDT_CLOCK_DIVISION_256] = WDT_RZ_CLOCK_DIVISION_UNSUPPORT,
				      [WDT_CLOCK_DIVISION_512] = 512,
				      [WDT_CLOCK_DIVISION_2048] = 2048,
				      [WDT_CLOCK_DIVISION_8192] = 8192};

/* Lookup table for the window start position setting */
static const int window_start_lut[] = {[0] = WDT_WINDOW_START_25,
				       [1] = WDT_WINDOW_START_50,
				       [2] = WDT_WINDOW_START_75,
				       [3] = WDT_WINDOW_START_100};

/* Lookup table for the window end position setting */
static const int window_end_lut[] = {[0] = WDT_WINDOW_END_0,
				     [1] = WDT_WINDOW_END_25,
				     [2] = WDT_WINDOW_END_50,
				     [3] = WDT_WINDOW_END_75};

#else
void wdt_overflow_isr(void *irq);
#define WDT_RZ_ISR        wdt_overflow_isr
#define WDT_RZ_PERIOD_MAX 0xFFF
#define WDT_RZ_PERIOD_MIN 0x0
#endif /* CONFIG_WDT_RENESAS_RZ_USE_ICU */

static inline void wdt_rz_inst_lock(const struct device *dev)
{
	struct wdt_rz_data *data = dev->data;

	k_mutex_lock(&data->inst_lock, K_FOREVER);
}

static inline void wdt_rz_inst_unlock(const struct device *dev)
{
	struct wdt_rz_data *data = dev->data;

	k_mutex_unlock(&data->inst_lock);
}

static int wdt_rz_timeout_calculate(const struct device *dev, const struct wdt_timeout_cfg *config)
{
	struct wdt_rz_data *data = dev->data;

	if (atomic_test_bit(&data->device_state, WDT_RZ_ATOMIC_TIMEOUT_SET)) {
		if (config->window.min != data->timeout.window.min ||
		    config->window.max != data->timeout.window.max ||
		    config->flags != data->timeout.flags) {
			LOG_ERR("wdt support only one timeout setting value");
			return -EINVAL;
		}

		data->timeout.callback = config->callback;
		return 0;
	}

#ifndef CONFIG_WDT_RENESAS_RZ_USE_ICU
	float best_period_cycle;
	wdt_extended_cfg_t *wdt_rz_cfg_extend = (wdt_extended_cfg_t *)data->fsp_cfg->p_extend;
	float convert_window_to_sec = (float)config->window.max / 1000;

	/*
	 * Calculate period for watchdog's counter.
	 * For more details, please refer to section 21.3.2 of the RZA3UL User’s Manual: Hardware.
	 */
	best_period_cycle = ((convert_window_to_sec * data->clock_rate) / (1024 * 1024)) - 1;
	if ((best_period_cycle > WDT_RZ_PERIOD_MAX) || (best_period_cycle < WDT_RZ_PERIOD_MIN)) {
		LOG_ERR("wdt timeout out of range");
		return -EINVAL;
	}
	wdt_rz_cfg_extend->wdt_timeout = (uint16_t)round(best_period_cycle);
	data->fsp_cfg->p_extend = wdt_rz_cfg_extend;
#else
	unsigned int window_start_idx;
	unsigned int window_end_idx;
	unsigned int best_divisor = WDT_CLOCK_DIVISION_1;
	unsigned int best_timeout = WDT_TIMEOUT_128;
	unsigned int best_period_ms = UINT_MAX;
	unsigned int min_delta = UINT_MAX;

	for (unsigned int divisor = WDT_CLOCK_DIVISION_1; divisor < ARRAY_SIZE(clock_div_lut);
	     divisor++) {
		if (clock_div_lut[divisor] == WDT_RZ_CLOCK_DIVISION_UNSUPPORT) {
			continue;
		}
		for (unsigned int timeout = WDT_TIMEOUT_128;
		     timeout < ARRAY_SIZE(timeout_period_lut); timeout++) {
			if (timeout_period_lut[timeout] == WDT_RZ_TIMEOUT_UNSUPPORT) {
				continue;
			}
			unsigned int period_ms = (unsigned int)(1000.0F * clock_div_lut[divisor] *
								timeout_period_lut[timeout] /
								(float)data->clock_rate);
			unsigned int delta = period_ms > config->window.max
						     ? period_ms - config->window.max
						     : config->window.max - period_ms;

			if (delta < min_delta) {
				min_delta = delta;
				best_divisor = divisor;
				best_timeout = timeout;
				best_period_ms = period_ms;
			}
		}
	}

	if (min_delta == UINT_MAX) {
		LOG_ERR("wdt timeout out of range");
		return -EINVAL;
	}

	if (config->window.max >= best_period_ms) {
		window_start_idx = 3;
	} else {
		window_start_idx =
			((config->window.max * 4 + best_period_ms) / best_period_ms) - 1;
	}

	if (config->window.min > best_period_ms) {
		LOG_ERR("window_min invalid");
		return -EINVAL;
	} else if (config->window.min == 0) {
		window_end_idx = 0;
	} else {
		window_end_idx = ((float)config->window.min / best_period_ms) * 4;
	}

	data->fsp_cfg->timeout = (wdt_timeout_t)best_timeout;
	data->fsp_cfg->clock_division = (wdt_clock_division_t)best_divisor;
	data->fsp_cfg->window_start = (wdt_window_start_t)window_start_lut[window_start_idx];
	data->fsp_cfg->window_end = (wdt_window_end_t)window_end_lut[window_end_idx];

	LOG_INF("actual window min = %d%%", window_end_idx * 25);
	LOG_INF("actual window max = %d%%", (window_start_idx + 1) * 25);

#endif /* CONFIG_WDT_RENESAS_RZ_USE_ICU */

	data->timeout = *config;

	return 0;
}

static int wdt_rz_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_rz_config *cfg = dev->config;
	struct wdt_rz_data *data = dev->data;
	int ret = 0;

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		LOG_ERR("wdt pause in sleep mode not supported");
		return -ENOTSUP;
	}

	wdt_rz_inst_lock(dev);

	if (atomic_test_bit(&data->device_state, WDT_RZ_ATOMIC_ENABLE)) {
		LOG_ERR("wdt has been already setup");
		ret = -EBUSY;
		goto end;
	}

	if (!atomic_test_bit(&data->device_state, WDT_RZ_ATOMIC_TIMEOUT_SET)) {
		LOG_ERR("wdt timeout should be installed before");
		ret = -EFAULT;
		goto end;
	}

#ifdef CONFIG_WDT_RENESAS_RZ_USE_ICU
	/* Clear the error status of the watchdog */
	R_ICU->PERIERR_CLR0 |= R_ICU_PERIERR_CLR0_ER_CL7_Msk;
	/* Release captured watchdog error status as an PERI_ERR0 event*/
	R_ICU->PERIERR_E0MSK0 &= ~R_ICU_PERIERR_E0MSK0_E0_MK7_Msk;

	if (R_SYSC_NS->RSTSR0_b.SWR0F) {
		/* Clear CPU0 software reset flag */
		volatile uint32_t dummy = R_SYSC_NS->RSTSR0;

		R_SYSC_NS->RSTSR0_b.SWR0F = 0x00000000U;
		dummy = R_SYSC_NS->RSTSR0;
	}
#endif
	if (cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg) != FSP_SUCCESS) {
		LOG_ERR("wdt setup failed!");
		ret = -EIO;
		goto end;
	}

	if (cfg->fsp_api->refresh(data->fsp_ctrl) != FSP_SUCCESS) {
		LOG_ERR("wdt start failed!");
		ret = -EIO;
		goto end;
	}

	atomic_set_bit(&data->device_state, WDT_RZ_ATOMIC_ENABLE);

end:
	wdt_rz_inst_unlock(dev);

	return ret;
}

static int wdt_rz_disable(const struct device *dev)
{
	struct wdt_rz_data *data = dev->data;

	if (!atomic_test_bit(&data->device_state, WDT_RZ_ATOMIC_ENABLE)) {
		LOG_ERR("wdt has not been enabled yet");
		return -EFAULT;
	}

	LOG_ERR("watchdog cannot be stopped once started unless SOC gets a reset");
	return -EPERM;
}

static int wdt_rz_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *config)
{
	struct wdt_rz_data *data = dev->data;
	int ret = 0;

	if (config->window.min > config->window.max || config->window.max == 0) {
		return -EINVAL;
	}

	if (config->callback == NULL && (config->flags & WDT_FLAG_RESET_MASK) == 0) {
		LOG_ERR("no timeout response was chosen");
		return -EINVAL;
	}

	wdt_rz_inst_lock(dev);

	if (atomic_test_bit(&data->device_state, WDT_RZ_ATOMIC_ENABLE)) {
		LOG_ERR("cannot change timeout settings after wdt setup");
		ret = -EBUSY;
		goto end;
	}

	ret = wdt_rz_timeout_calculate(dev, config);
	if (ret < 0) {
		goto end;
	}

	atomic_set_bit(&data->device_state, WDT_RZ_ATOMIC_TIMEOUT_SET);

	LOG_INF("wdt timeout was set successfully");

end:
	wdt_rz_inst_unlock(dev);

	return ret;
}

static int wdt_rz_feed(const struct device *dev, int channel_id)
{
	const struct wdt_rz_config *cfg = dev->config;
	struct wdt_rz_data *data = dev->data;

	if (!atomic_test_bit(&data->device_state, WDT_RZ_ATOMIC_ENABLE)) {
		LOG_ERR("WDT has not been enabled yet!");
		return -EINVAL;
	}

	if (channel_id != 0) {
		LOG_ERR("Incorrect channel_id!");
		return -EINVAL;
	}

	if (cfg->fsp_api->refresh(data->fsp_ctrl) != FSP_SUCCESS) {
		LOG_ERR("Fail to refresh watchdog!");
		return -EIO;
	}

	return 0;
}

void wdt_rz_callback(wdt_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct wdt_rz_data *data = dev->data;
	wdt_callback_t callback = data->timeout.callback;

	if (callback != NULL) {
		callback(dev, 0);
	}
}

static void wdt_rz_isr_adapter(const struct device *dev)
{
	const struct wdt_rz_config *cfg = dev->config;
	struct wdt_rz_data *data = dev->data;

	WDT_RZ_ISR((void *)cfg->irq);

	atomic_test_and_clear_bit(&data->device_state, WDT_RZ_ATOMIC_ENABLE);
	atomic_test_and_clear_bit(&data->device_state, WDT_RZ_ATOMIC_TIMEOUT_SET);

#ifdef CONFIG_WDT_RENESAS_RZ_USE_ICU
	wdt_status_t status = WDT_STATUS_NO_ERROR;

	/* Clear watchdog status's interrupt flag */
	cfg->fsp_api->statusGet(data->fsp_ctrl, &status);
	cfg->fsp_api->statusClear(data->fsp_ctrl, status);

	/* Clear the error status of the watchdog */
	R_ICU->PERIERR_CLR0 |= R_ICU_PERIERR_CLR0_ER_CL7_Msk;
	k_work_submit(&data->interrupt_work);
#endif
}

#ifdef CONFIG_WDT_RENESAS_RZ_USE_ICU
static void wdt_rz_interrupt_work(struct k_work *work)
{
	ARG_UNUSED(work);

	/* CR52_0 software reset. */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_RESET);
	R_SYSC_S->SWRCPU0 = BSP_PRV_RESET_KEY_AUTO_RELEASE;

	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_RESET);

	__asm__ volatile("wfi");
}
#endif

static DEVICE_API(wdt, wdt_rz_api) = {
	.setup = wdt_rz_setup,
	.disable = wdt_rz_disable,
	.install_timeout = wdt_rz_install_timeout,
	.feed = wdt_rz_feed,
};

#ifndef CONFIG_WDT_RENESAS_RZ_USE_ICU
static wdt_extended_cfg_t g_wdt_extend_cfg = {
	.overflow_ipl = DT_IRQ(DT_NODELABEL(wdt0), priority),
	.overflow_irq = DT_IRQ(DT_NODELABEL(wdt0), irq),
};
#define WDT_RZ_CONFIG_EXTEND &g_wdt_extend_cfg
#define WDT_RZ_INIT_WORK_QUEUE
#else
#define WDT_RZ_CONFIG_EXTEND   NULL
#define WDT_RZ_INIT_WORK_QUEUE k_work_init(&data->interrupt_work, wdt_rz_interrupt_work)
#endif

#define WDT_RZ_INIT(inst)                                                                          \
	static wdt_cfg_t g_wdt_##inst##_cfg = {                                                    \
		.timeout = WDT_TIMEOUT_16384,                                                      \
		.clock_division = WDT_CLOCK_DIVISION_8192,                                         \
		.window_start = WDT_WINDOW_START_100,                                              \
		.window_end = WDT_WINDOW_END_0,                                                    \
		.reset_control = WDT_RESET_CONTROL_NMI,                                            \
		.stop_control = WDT_STOP_CONTROL_DISABLE,                                          \
		.p_callback = wdt_rz_callback,                                                     \
		.p_context = (void *)DEVICE_DT_INST_GET(inst),                                     \
		.p_extend = WDT_RZ_CONFIG_EXTEND,                                                  \
	};                                                                                         \
                                                                                                   \
	static wdt_instance_ctrl_t g_wdt##inst##_ctrl;                                             \
                                                                                                   \
	static struct wdt_rz_data wdt_rz_data_##inst = {                                           \
		.fsp_ctrl = &g_wdt##inst##_ctrl,                                                   \
		.fsp_cfg = &g_wdt_##inst##_cfg,                                                    \
		.clock_rate = DT_INST_PROP(inst, clock_freq),                                      \
		.device_state = ATOMIC_INIT(0),                                                    \
	};                                                                                         \
                                                                                                   \
	static struct wdt_rz_config wdt_rz_config_##inst = {.fsp_api = &g_wdt_on_wdt,              \
							    .irq = DT_INST_IRQN(inst)};            \
                                                                                                   \
	static int wdt_rz_init_##inst(const struct device *dev)                                    \
	{                                                                                          \
		struct wdt_rz_data *data = dev->data;                                              \
                                                                                                   \
		k_mutex_init(&data->inst_lock);                                                    \
		WDT_RZ_INIT_WORK_QUEUE;                                                            \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), wdt_rz_isr_adapter,   \
			    DEVICE_DT_INST_GET(inst), DT_INST_IRQ(inst, flags));                   \
		irq_enable(DT_INST_IRQN(inst));                                                    \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &wdt_rz_init_##inst, NULL, &wdt_rz_data_##inst,                \
			      &wdt_rz_config_##inst, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_rz_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_RZ_INIT)
