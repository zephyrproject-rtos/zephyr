/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tsi_input

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <fsl_tsi_v6.h>

LOG_MODULE_REGISTER(input_mcux_tsi, CONFIG_INPUT_LOG_LEVEL);

struct tsi_channel_state {
	uint16_t baseline;
	uint16_t counter;
	int16_t delta;
	bool touched;
	bool prev_touched;
};

struct mcux_tsi_config {
	TSI_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;

	/* Channel configuration */
	uint8_t num_channels;
	const uint16_t *input_codes;
	uint32_t channel_mask;

	/* Touch detection parameters */
	uint16_t touch_threshold;
	uint16_t release_threshold;

	/* Scan configuration */
	uint16_t scan_period_ms;
	bool continuous_scan;
};

struct mcux_tsi_data {
	const struct device *dev;

	/* Channel states */
	struct tsi_channel_state channels[FSL_FEATURE_TSI_CHANNEL_COUNT];

	/* Calibration data */
	tsi_calibration_data_t cal_data;

	/* Current scanning channel */
	uint8_t current_channel;
	uint8_t scan_channel_index;

	/* Scan control */
	struct k_work_delayable scan_work;
	struct k_sem scan_sem;
};

static int mcux_tsi_get_next_channel(const struct mcux_tsi_config *config, int current)
{
	for (int i = 0; i < config->num_channels; i++) {
		int ch = (current + 1 + i) % config->num_channels;

		if (config->channel_mask & BIT(ch)) {
			return ch;
		}
	}

	return -1;
}

static void mcux_tsi_process_channel(const struct device *dev, uint8_t channel_idx)
{
	const struct mcux_tsi_config *config = dev->config;
	struct mcux_tsi_data *data = dev->data;
	struct tsi_channel_state *ch = &data->channels[channel_idx];

	/* Determine touch state with hysteresis */
	if (!ch->touched && ch->delta > (int16_t)config->touch_threshold) {
		ch->touched = true;
	} else if (ch->touched && ch->delta < (int16_t)config->release_threshold) {
		ch->touched = false;
	}

	/* Report state change */
	if (ch->touched != ch->prev_touched) {
		uint16_t code = config->input_codes[channel_idx];

		input_report_key(dev, code, ch->touched ? 1 : 0, true, K_FOREVER);

		if (ch->touched) {
			LOG_DBG("Channel %d touched (code=%d, delta=%d)",
				channel_idx, code, ch->delta);
		} else {
			LOG_DBG("Channel %d released (code=%d, delta=%d)",
				channel_idx, code, ch->delta);
		}

		ch->prev_touched = ch->touched;
	}
}

static void mcux_tsi_isr(const struct device *dev)
{
	const struct mcux_tsi_config *config = dev->config;
	struct mcux_tsi_data *data = dev->data;
	TSI_Type *base = config->base;
	uint32_t status;

	status = TSI_GetStatusFlags(base);

	if (status & kTSI_EndOfScanFlag) {
		/* Read counter value */
		uint16_t counter = TSI_GetCounter(base);
		uint8_t ch_idx = data->scan_channel_index;

		if (ch_idx < FSL_FEATURE_TSI_CHANNEL_COUNT) {
			struct tsi_channel_state *ch = &data->channels[ch_idx];

			/* Update channel data */
			ch->counter = counter;
			ch->delta = (int16_t)(ch->baseline - counter);

			/* Process touch state */
			mcux_tsi_process_channel(dev, ch_idx);
		}

		/* Clear flag */
		TSI_ClearStatusFlags(base, kTSI_EndOfScanFlag);

		/* Signal scan complete */
		k_sem_give(&data->scan_sem);
	}

	if (status & kTSI_OutOfRangeFlag) {
		TSI_ClearStatusFlags(base, kTSI_OutOfRangeFlag);
	}
}

static void mcux_tsi_scan_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mcux_tsi_data *data = CONTAINER_OF(dwork, struct mcux_tsi_data, scan_work);
	const struct device *dev = data->dev;
	const struct mcux_tsi_config *config = dev->config;
	TSI_Type *base = config->base;
	int ret;

	/* Get next channel to scan */
	int next_ch = mcux_tsi_get_next_channel(config, data->current_channel);

	if (next_ch < 0) {
		LOG_ERR("No enabled channels");
		return;
	}

	data->current_channel = next_ch;
	data->scan_channel_index = next_ch;

	/* Start scan */
	TSI_SetSelfCapMeasuredChannel(base, next_ch);
	TSI_StartSoftwareTrigger(base);

	/* Wait for scan complete with timeout */
	ret = k_sem_take(&data->scan_sem, K_MSEC(10));
	if (ret < 0) {
		LOG_WRN("TSI scan timeout on channel %d", next_ch);
	}

	/* Schedule next scan */
	if (config->continuous_scan) {
		k_work_schedule(&data->scan_work, K_MSEC(config->scan_period_ms));
	}
}

static void mcux_tsi_start_scan(const struct device *dev)
{
	const struct mcux_tsi_config *config = dev->config;
	struct mcux_tsi_data *data = dev->data;

	if (config->continuous_scan) {
		k_work_schedule(&data->scan_work, K_NO_WAIT);
	}
}

static int mcux_tsi_init(const struct device *dev)
{
	const struct mcux_tsi_config *config = dev->config;
	struct mcux_tsi_data *data = dev->data;
	TSI_Type *base = config->base;
	int ret;

	LOG_INF("Initializing MCUX TSI input device");

	/* Store device pointer */
	data->dev = dev;

	/* Apply pin configuration */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d", ret);
		return ret;
	}

	/* Enable clock */
	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable clock: %d", ret);
		return ret;
	}

	/* Initialize semaphore */
	k_sem_init(&data->scan_sem, 0, 1);

	/* Initialize work queue */
	k_work_init_delayable(&data->scan_work, mcux_tsi_scan_work_handler);

	/* Initialize TSI in self-cap mode */
	tsi_selfCap_config_t tsi_config;

	TSI_GetSelfCapModeDefaultConfig(&tsi_config);

	TSI_InitSelfCapMode(base, &tsi_config);

	/* Enable TSI module */
	TSI_EnableModule(base, true);

	/* Calibrate channels */
	LOG_INF("Calibrating TSI channels...");
	TSI_SelfCapCalibrate(base, &data->cal_data);

	/* Initialize channel states */
	for (uint8_t i = 0; i < FSL_FEATURE_TSI_CHANNEL_COUNT; i++) {
		if (config->channel_mask & BIT(i)) {
			data->channels[i].baseline = data->cal_data.calibratedData[i];
			data->channels[i].counter = 0;
			data->channels[i].delta = 0;
			data->channels[i].touched = false;
			data->channels[i].prev_touched = false;

			LOG_INF("Channel %d baseline: %d", i, data->channels[i].baseline);
		}
	}

	/* Configure interrupts */
	config->irq_config_func(dev);

	/* Enable TSI interrupts */
	TSI_EnableInterrupts(base, kTSI_EndOfScanInterruptEnable);

	/* Start scanning */
	mcux_tsi_start_scan(dev);

	LOG_INF("MCUX TSI initialized: %d channels, scan period %d ms",
		config->num_channels, config->scan_period_ms);

	return 0;
}

#define MCUX_TSI_INIT(n)							\
										\
	PINCTRL_DT_INST_DEFINE(n);						\
										\
	static const uint16_t mcux_tsi_input_codes_##n[] =			\
		DT_INST_PROP(n, input_codes);					\
										\
	static void mcux_tsi_irq_config_##n(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    mcux_tsi_isr,					\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static const struct mcux_tsi_config mcux_tsi_config_##n = {		\
		.base = (TSI_Type *)DT_INST_REG_ADDR(n),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)			\
			DT_INST_CLOCKS_CELL(n, name),				\
		.irq_config_func = mcux_tsi_irq_config_##n,			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.num_channels = DT_INST_PROP(n, num_channels),			\
		.input_codes = mcux_tsi_input_codes_##n,			\
		.channel_mask = DT_INST_PROP(n, channel_mask),			\
		.touch_threshold = DT_INST_PROP(n, touch_threshold),		\
		.release_threshold = DT_INST_PROP_OR(n, release_threshold,	\
			DT_INST_PROP(n, touch_threshold) / 2),			\
		.scan_period_ms = DT_INST_PROP(n, scan_period_ms),		\
		.continuous_scan = DT_INST_PROP_OR(n, continuous_scan, true),	\
	};									\
										\
	static struct mcux_tsi_data mcux_tsi_data_##n;				\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      mcux_tsi_init,					\
			      NULL,						\
			      &mcux_tsi_data_##n,				\
			      &mcux_tsi_config_##n,				\
			      POST_KERNEL,					\
			      CONFIG_INPUT_INIT_PRIORITY,			\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MCUX_TSI_INIT)
