/*
 * Copyright (c) 2024, MAKEEN Energy A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx7d_gpt

#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <gpt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>

#define GUARD_PERIOD 1200

/* bit indicating if an alarm is expecting to be enabled at the next overflow */
#define IMX_GPT_OVF_DELAYED 0x01
/* bit set when an alarm was rescheduled after an overflow */
#define IMX_GPT_OVF_PROCESSED 0x02

LOG_MODULE_REGISTER(imx7d_gpt, CONFIG_COUNTER_LOG_LEVEL);

struct imx_gpt_config {
	/* info must be first element */
	struct counter_config_info info;
	GPT_Type *base;
	uint8_t clock_source;
};

struct imx_gpt_alarm_cfg {
	struct counter_alarm_cfg alarm_cfg;
	uint8_t ovf_state;
};

struct imx_gpt_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
	struct imx_gpt_alarm_cfg alarm_cfgs[3];
};

static int imx_gpt_start(const struct device *dev)
{
	const struct imx_gpt_config *config = dev->config;
	unsigned int key;

	key = irq_lock();
	GPT_Enable(config->base);
	irq_unlock(key);
	LOG_DBG("GPT start counter: %u SR: %x CR: %x\n",
		GPT_ReadCounter(config->base), config->base->SR, config->base->CR);
	return 0;
}

static int imx_gpt_stop(const struct device *dev)
{
	const struct imx_gpt_config *config = dev->config;
	unsigned int key;

	key = irq_lock();
	GPT_Disable(config->base);
	irq_unlock(key);
	LOG_DBG("GPT stop counter: %u SR: %x CR: %x\n",
		GPT_ReadCounter(config->base), config->base->SR, config->base->CR);
	return 0;
}

static int imx_gpt_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct imx_gpt_config *config = dev->config;

	*ticks = GPT_ReadCounter(config->base);
	return 0;
}

static int imx_gpt_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct imx_gpt_config *config = dev->config;
	struct imx_gpt_data *data = dev->data;
	unsigned int key;

	uint32_t current = GPT_ReadCounter(config->base);
	uint32_t ticks = alarm_cfg->ticks;

	struct counter_alarm_cfg *alarm_data = NULL;
	struct imx_gpt_alarm_cfg *ext_alarm_data = NULL;

	if (chan_id > config->info.channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	ext_alarm_data = &data->alarm_cfgs[chan_id];
	alarm_data = &ext_alarm_data->alarm_cfg;

	/* tick delay too long */
	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0 &&
	   alarm_cfg->ticks >= config->info.max_top_value) {
		LOG_ERR("Error setting max %d / %d\n",
				alarm_cfg->ticks, config->info.max_top_value);
		return -EINVAL;
	}

	if (alarm_data->callback) {
		return -EBUSY;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
		ticks &= config->info.max_top_value;
	}

	/* it is not garanteed that we can match the TOP value, so match -1 tick from the TOP */
	/* Ticks can only match if they are bellow the MAX top */
	if (ticks > config->info.max_top_value) {
		LOG_ERR("Error ticks max %d / %d\n", alarm_cfg->ticks, config->info.max_top_value);
		return -EINVAL;
	}

	alarm_data->callback = alarm_cfg->callback;
	alarm_data->user_data = alarm_cfg->user_data;
	alarm_data->ticks = ticks;
	alarm_data->flags = alarm_cfg->flags;

	key = irq_lock();
	GPT_SetOutputCompareValue(config->base, chan_id, ticks);
	GPT_SetIntCmd(config->base, (1U<<chan_id), true);
	irq_unlock(key);
	LOG_DBG("GPT Set alarm [%d]: %u / %u IR:%x SR:%x CR:%x\n",
			chan_id, current, ticks, config->base->IR,
			config->base->SR, config->base->CR);


	return 0;
}

static int imx_gpt_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct imx_gpt_config *config = dev->config;
	struct imx_gpt_data *data = dev->data;
	struct counter_alarm_cfg *alarm_data = NULL;
	struct imx_gpt_alarm_cfg *ext_alarm_data = NULL;
	unsigned int key;

	if (chan_id > config->info.channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	ext_alarm_data = &data->alarm_cfgs[chan_id];
	alarm_data = &ext_alarm_data->alarm_cfg;

	key = irq_lock();
	GPT_SetIntCmd(config->base, (1UL<<chan_id), false);
	alarm_data->callback = NULL;
	irq_unlock(key);
	return 0;
}

static uint32_t imx_gpt_get_pending_int(const struct device *dev)
{
	const struct imx_gpt_config *config = dev->config;

	return	GPT_GetStatusFlag(config->base, (
					gptStatusFlagOutputCompare1 |
					gptStatusFlagOutputCompare2 |
					gptStatusFlagOutputCompare3 |
					gptStatusFlagRollOver));
}

void imx_gpt_isr(const struct device *dev)
{
	const struct imx_gpt_config *config = dev->config;
	struct imx_gpt_data *data = dev->data;
	unsigned int key = irq_lock();

	uint32_t current = GPT_ReadCounter(config->base);
	uint32_t status;
	uint32_t int_enable = (config->base->IR) & 0x3F;

	status =  imx_gpt_get_pending_int(dev);

	GPT_ClearStatusFlag(config->base, status);
	barrier_dsync_fence_full();

	if ((status & gptStatusFlagRollOver) && (int_enable & gptStatusFlagRollOver)) {

		if (data->top_callback) {
			data->top_callback(dev, data->top_user_data);
		}


		/* on overflow, all compare are triggered */
		uint32_t status_save = status;

		status = 0;

		for (uint8_t chan_id = 0; chan_id < config->info.channels; chan_id++) {
			uint8_t channel_flag = (1UL<<chan_id);
			struct imx_gpt_alarm_cfg *ext_alarm_data =  &data->alarm_cfgs[chan_id];
			struct counter_alarm_cfg *alarm_data = &ext_alarm_data->alarm_cfg;

			/* if channel is enabled and it was matching for the 0 count  */
			if ((status & channel_flag) != 0 && (int_enable & channel_flag) != 0
				&& GPT_GetOutputCompareValue(config->base, chan_id) == 0
				&& alarm_data->callback) {
				status |= channel_flag;
			}
		}
		/* Return here to clear the flags at overflow and */
		/* let the compare channel trigger again */
		LOG_DBG("GPT top: IR:%x SR:%x/%x/%x\n",
			config->base->IR, config->base->SR, status, status_save);
	}

	for (uint8_t chan_id = 0; chan_id < config->info.channels; chan_id++) {
		uint8_t channel_flag = (1UL<<chan_id);
		struct imx_gpt_alarm_cfg *ext_alarm_data = &data->alarm_cfgs[chan_id];
		struct counter_alarm_cfg *alarm_data = &ext_alarm_data->alarm_cfg;

		if ((status & channel_flag) &&
			(int_enable & channel_flag) &&
			alarm_data->callback) {

			GPT_SetIntCmd(config->base, channel_flag, false);

			counter_alarm_callback_t alarm_cb = alarm_data->callback;

			alarm_data->callback = NULL;
			alarm_cb(dev, chan_id, current, alarm_data->user_data);
			LOG_DBG("GPT alarm: IR:%x SR:%x/%x\n",
				config->base->IR, config->base->SR, status);
		}
	}

	irq_unlock(key);
}

static int imx_gpt_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct imx_gpt_config *config = dev->config;
	struct imx_gpt_data *data = dev->data;
	unsigned int key;
	int res = 0;

	if (cfg->ticks != config->info.max_top_value) {
		LOG_ERR("Wrap can only be set to 0x%x",
			config->info.max_top_value);
		res = -ENOTSUP;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	key = irq_lock();
	/* Always enable */
	GPT_SetIntCmd(config->base, gptStatusFlagRollOver, true);

	/* set_top_value resets the counter as per flags in Zephyr API */
	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0) {
		/* IMX GPT resets if ENMOD in GPT->CR is 1 */
		GPT_Disable(config->base);
		GPT_Enable(config->base);
	}
	irq_unlock(key);

	return res;
}

static uint32_t imx_gpt_get_top_value(const struct device *dev)
{
	const struct imx_gpt_config *config = dev->config;

	return config->info.max_top_value;
}

static uint32_t imx_gpt_get_frequency(const struct device *dev)
{
	const struct imx_gpt_config *config = dev->config;

	uint32_t clock_freq = get_gpt_clock_freq_soc(config->base);
	/* when using OSC source we must divide to make sure */
	/* the counting frequency is at maximum 1/2 of Periph clock */
	uint32_t osc_prescaler = 1;

	uint32_t clock_freq_pre = (clock_freq / (osc_prescaler+1));

	uint32_t prescaler = (clock_freq_pre / config->info.freq) - 1;

	uint32_t gpt_freq = clock_freq_pre / (prescaler+1);

	return gpt_freq;
}

static int imx_gpt_init(const struct device *dev)
{
	const struct imx_gpt_config *config = dev->config;
	struct imx_gpt_data *data = dev->data;
	gpt_init_config_t gptConfig;
	unsigned int key;
	uint32_t clock_freq;

	clock_freq = get_gpt_clock_freq_soc(config->base);

	/* when using OSC source we must divide to make sure */
	/* the counting frequency is at maximum 1/2 of Periph clock */
	/* See IMX7DRM */
	uint32_t osc_prescaler = 1;

	uint32_t clock_freq_pre = (clock_freq / (osc_prescaler+1));

	if (clock_freq_pre % config->info.freq) {
		LOG_ERR("Cannot Adjust GPT freq to %u\n", config->info.freq);
		LOG_ERR("clock src is %u\n", clock_freq_pre);
		return -EINVAL;
	}

	data->top_callback = NULL;
	data->top_user_data = NULL;

	/* Reference manual sequence page 4064*/
	/*!< true: FreeRun mode, false: Restart mode. */
	gptConfig.freeRun = true;
	/*!< GPT enabled in wait mode. */
	gptConfig.waitEnable = true;
	/*!< GPT enabled in stop mode. */
	gptConfig.stopEnable = true;
	/*!< GPT enabled in doze mode. */
	gptConfig.dozeEnable = true;
	/*!< GPT enabled in debug mode. */
	gptConfig.dbgEnable = false;
	/*!< true: counter reset to 0 when enabled, false: counter retain its value when enabled. */
	gptConfig.enableMode = true;


	uint32_t prescaler = (clock_freq_pre / config->info.freq) - 1;
	uint32_t gpt_freq = clock_freq_pre / (prescaler+1);

	LOG_DBG("GPT srcclock: %u gpt_freq: %u clock_source:%u\n",
			clock_freq_pre, config->info.freq, config->clock_source);

	key = irq_lock();
	GPT_Init(config->base, &gptConfig);

	GPT_SetClockSource(config->base, config->clock_source);

	GPT_ClearStatusFlag(config->base,
		gptStatusFlagOutputCompare1 |
		gptStatusFlagOutputCompare2 |
		gptStatusFlagOutputCompare3 |
		gptStatusFlagInputCapture1 |
		gptStatusFlagInputCapture2 |
		gptStatusFlagRollOver);

	GPT_SetIntCmd(config->base,
		gptStatusFlagOutputCompare1 |
		gptStatusFlagOutputCompare2 |
		gptStatusFlagOutputCompare3 |
		gptStatusFlagInputCapture1 |
		gptStatusFlagInputCapture2 |
		gptStatusFlagRollOver, false);



	GPT_SetOutputOperationMode(config->base,
			gptOutputCompareChannel1, gptOutputOperationDisconnected);
	GPT_SetOutputOperationMode(config->base,
			gptOutputCompareChannel2, gptOutputOperationDisconnected);
	GPT_SetOutputOperationMode(config->base,
			gptOutputCompareChannel3, gptOutputOperationDisconnected);

	GPT_SetOscPrescaler(config->base, osc_prescaler);
	GPT_SetPrescaler(config->base, prescaler);

	LOG_DBG("GPT oscprescaler: %u prescaler: %u cntFreq:%u CR:%u IR:%u\n",
	GPT_GetOscPrescaler(config->base),
	GPT_GetPrescaler(config->base),
	 gpt_freq,
	 config->base->CR, config->base->IR);


	/* enable by default */
	GPT_SetIntCmd(config->base, gptStatusFlagRollOver, true);

	irq_unlock(key);

	return 0;
}

static const struct counter_driver_api imx_gpt_driver_api = {
	.start = imx_gpt_start,
	.stop = imx_gpt_stop,
	.get_value = imx_gpt_get_value,
	.set_alarm = imx_gpt_set_alarm,
	.cancel_alarm = imx_gpt_cancel_alarm,
	.set_top_value = imx_gpt_set_top_value,
	.get_pending_int = imx_gpt_get_pending_int,
	.get_top_value = imx_gpt_get_top_value,
	.get_freq = imx_gpt_get_frequency,
	/* .set_guard_period = imx_gpt_set_guard_period, */
	/* .get_guard_period = imx_gpt_get_guard_period, */
};

#define GPT_DEVICE_INIT_IMX7D(n)						\
	static struct imx_gpt_data imx_gpt_data_ ## n = {0};		\
									\
	static const struct imx_gpt_config imx_gpt_config_ ## n = {	\
		.base = (void *)DT_INST_REG_ADDR(n),			\
		.clock_source = gptClockSourceOsc, \
		.info = {						\
			.max_top_value = UINT32_MAX,			\
			.freq = DT_INST_PROP(n, gptfreq),           \
			.channels = 3,					\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		\
		},							\
	};								\
									\
	static int imx_gpt_## n ##_init(const struct device *dev);	\
	DEVICE_DT_INST_DEFINE(n,					\
			    imx_gpt_## n ##_init,			\
			    NULL,					\
			    &imx_gpt_data_ ## n,			\
			    &imx_gpt_config_ ## n,			\
			    POST_KERNEL,				\
			    CONFIG_COUNTER_INIT_PRIORITY,		\
			    &imx_gpt_driver_api);			\
									\
	static int imx_gpt_## n ##_init(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    imx_gpt_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));				\
		return imx_gpt_init(dev);				\
	}								\

DT_INST_FOREACH_STATUS_OKAY(GPT_DEVICE_INIT_IMX7D)
