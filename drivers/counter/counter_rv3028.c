/*
 * Copyright (c) 2025 BayLibre, SAS
 * Copyright (c) 2026 Janez Ugovsek <janez@ugovsek.info>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv3028_counter

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <rv3028.h>

LOG_MODULE_REGISTER(counter_rv3028, CONFIG_COUNTER_LOG_LEVEL);

struct rv3028_counter_config {
	struct counter_config_info counter_info;
	const struct device *mfd;
	const bool repeat_mode;
	const uint32_t frequency;
};

struct rv3028_counter_data {
	struct counter_alarm_cfg alarm_cfg0;
	struct counter_top_cfg top_cfg;
	bool used_top;
	bool used_alarm;
};

/* Control 1 TD register frequencies:
 * 0 - 4096 Hz, default value
 * 1 - 64 Hz
 * 2 - 1 Hz
 * 3 - 1/60 Hz, represented as 0
 */
static const uint32_t td2freq[4] = {4096, 64, 1, 0};

int rv3028_counter_start(const struct device *dev)
{
	int err;
	const struct rv3028_counter_config *config = dev->config;

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL1, RV3028_CONTROL1_TE,
				     RV3028_CONTROL1_TE);
	if (err) {
		goto unlock;
	}

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

int rv3028_counter_stop(const struct device *dev)
{
	int err;
	const struct rv3028_counter_config *config = dev->config;
	struct rv3028_counter_data *data = dev->data;

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL1, RV3028_CONTROL1_TE, 0);
	if (err) {
		goto unlock;
	}

unlock:
	if (!err) {
		data->used_top = false;
	}

	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

int rv3028_counter_get_value(const struct device *dev, uint32_t *ticks)
{
	int err;
	const struct rv3028_counter_config *config = dev->config;
	uint8_t val[4];

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_read_regs(config->mfd, RV3028_REG_TIMER_VALUE_0, val, 4);
	if (err) {
		goto unlock;
	}

	/* Calculate from down counter to up counter */
	*ticks = ((val[1] << 8) | val[0]) - (((val[3] << 8) | val[2]));

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

int rv3028_counter_reset(const struct device *dev)
{
	const struct rv3028_counter_config *config = dev->config;
	int err;

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL1, RV3028_CONTROL1_TE, 0);
	if (err) {
		goto unlock;
	}

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL1, RV3028_CONTROL1_TE,
				     RV3028_CONTROL1_TE);

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

void counter_isr(const struct device *dev)
{
	struct rv3028_counter_data *data = dev->data;

	if (data->alarm_cfg0.callback && data->used_alarm) {
		data->alarm_cfg0.callback(dev, 0, data->alarm_cfg0.ticks,
					  data->alarm_cfg0.user_data);
	}
	if (data->top_cfg.callback && data->used_top) {
		data->top_cfg.callback(dev, data->top_cfg.user_data);
	}
}

int rv3028_counter_set_alarm(const struct device *dev, uint8_t chan_id,
			     const struct counter_alarm_cfg *cfg)
{
	const struct rv3028_counter_config *config = dev->config;
	struct rv3028_counter_data *data = dev->data;
	uint8_t timer[2];
	uint8_t status;
	int err;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id, only 0 is supported");
		return -ENOTSUP;
	}

	if (cfg->ticks > config->counter_info.max_top_value) {
		LOG_WRN("Timer ticks value out of range (max: %d)",
			config->counter_info.max_top_value);
		return -EINVAL;
	}

	if (data->used_top) {
		LOG_WRN("Cannot use top and alarm at the same time - HW limitation");
		return -ENOTSUP;
	}

	timer[0] = cfg->ticks & 0xff;
	timer[1] = (cfg->ticks >> 8) & 0x0f;

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_read_reg8(config->mfd, RV3028_REG_CONTROL1, &status);
	if (err) {
		goto unlock;
	}

	if (status & RV3028_CONTROL1_TE) {
		LOG_WRN("Cannot set alarm value because counter is running");
		err = -EBUSY;
		goto unlock;
	}

	err = mfd_rv3028_write_regs(config->mfd, RV3028_REG_TIMER_VALUE_0, timer, 2);
	if (err) {
		goto unlock;
	}

	data->alarm_cfg0 = *cfg;

	if (cfg->callback == NULL) {
		err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL2, RV3028_CONTROL2_TIE,
					     0);
		if (err) {
			goto unlock;
		}
	} else {
		err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL2, RV3028_CONTROL2_TIE,
					     RV3028_CONTROL2_TIE);
		if (err) {
			goto unlock;
		}
	}

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_STATUS, RV3028_STATUS_TF, 0);
	if (err) {
		goto unlock;
	}

	data->used_alarm = true;

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

int rv3028_counter_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct rv3028_counter_data *data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id, only 0 is supported");
		return -ENOTSUP;
	}

	if (data->used_top) {
		LOG_WRN("Can not use top and alarm in the same time - HW limitation");
		return -ENOTSUP;
	}

	data->used_alarm = false;
	return rv3028_counter_stop(dev);
}

uint32_t rv3028_counter_get_pending_int(const struct device *dev)
{
	const struct rv3028_counter_config *config = dev->config;
	uint8_t status;
	int err = 0;

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_read_reg8(config->mfd, RV3028_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

	/* Check timer bit in status register. */
	if (status & RV3028_STATUS_TF) {
		err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_STATUS, RV3028_STATUS_TF, 0);
		if (err) {
			goto unlock;
		}
		err = 1;
	}

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

int rv3028_counter_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct rv3028_counter_config *config = dev->config;
	struct rv3028_counter_data *data = dev->data;
	uint8_t timer[2];
	uint8_t status;
	int err;

	if (cfg->ticks > config->counter_info.max_top_value) {
		LOG_WRN("Timer ticks value out of range (max: %d)",
			config->counter_info.max_top_value);
		return -EINVAL;
	}

	if (data->used_alarm) {
		LOG_WRN("Cannot use top and alarm at the same time - HW limitation");
		return -ENOTSUP;
	}

	timer[0] = cfg->ticks & 0xff;
	timer[1] = (cfg->ticks >> 8) & 0x0f;

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_read_reg8(config->mfd, RV3028_REG_CONTROL1, &status);
	if (err) {
		goto unlock;
	}

	if (status & RV3028_CONTROL1_TE) {
		LOG_WRN("Cannot set top value because counter is running");
		err = -EBUSY;
		goto unlock;
	}

	err = mfd_rv3028_write_regs(config->mfd, RV3028_REG_TIMER_VALUE_0, timer, 2);
	if (err) {
		goto unlock;
	}

	data->top_cfg = *cfg;

	if (cfg->callback == NULL) {
		err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL2, RV3028_CONTROL2_TIE,
					     0);
		if (err) {
			goto unlock;
		}
	} else {
		err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL2, RV3028_CONTROL2_TIE,
					     RV3028_CONTROL2_TIE);
		if (err) {
			goto unlock;
		}
	}

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_STATUS, RV3028_STATUS_TF, 0);
	if (err) {
		goto unlock;
	}

	data->used_top = true;

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

uint32_t rv3028_counter_get_top_value(const struct device *dev)
{
	const struct rv3028_counter_config *config = dev->config;
	uint8_t timer[2];
	uint32_t val = 0;
	int err;

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_read_regs(config->mfd, RV3028_REG_TIMER_VALUE_0, timer, 2);
	if (err) {
		goto unlock;
	}

	val = timer[0] | (timer[1] << 8);

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return val;
}

uint32_t rv3028_counter_get_guard_period(const struct device *dev, uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);

	return -ENOTSUP;
}

int rv3028_counter_set_guard_period(const struct device *dev, uint32_t ticks, uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ticks);
	ARG_UNUSED(flags);

	return -ENOTSUP;
}

uint32_t rv3028_counter_get_freq(const struct device *dev)
{
	const struct rv3028_counter_config *config = dev->config;
	uint8_t control;
	uint32_t freq;
	int err;

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_read_reg8(config->mfd, RV3028_REG_CONTROL1, &control);
	if (err) {
		freq = ~0;
		goto unlock;
	}

	control &= RV3028_CONTROL1_TD;
	freq = td2freq[control];

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return freq;
}

static int rv3028_counter_init(const struct device *dev)
{
	const struct rv3028_counter_config *config = dev->config;
	uint8_t val = 0;
	int err;

	val = 0xff;
	for (uint8_t i = 0; i < ARRAY_SIZE(td2freq); i++) {
		if (td2freq[i] == config->frequency) {
			val = i;
		}
	}

	if (val == 0xff) {
		LOG_ERR("Invalid frequency");
		return -EINVAL;
	}

	if (config->repeat_mode) {
		val |= RV3028_CONTROL1_TRPT;
	}

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL1,
				     RV3028_CONTROL1_TD | RV3028_CONTROL1_TRPT, val);
	if (err) {
		return -ENODEV;
	}

	mfd_rv3028_set_irq_handler(config->mfd, dev, RV3028_DEV_COUNTER, counter_isr);

	return 0;
}

static DEVICE_API(counter, rv3028_counter_api) = {
	.start = rv3028_counter_start,
	.stop = rv3028_counter_stop,
	.get_value = rv3028_counter_get_value,
	.reset = rv3028_counter_reset,
	.set_alarm = rv3028_counter_set_alarm,
	.cancel_alarm = rv3028_counter_cancel_alarm,
	.set_top_value = rv3028_counter_set_top_value,
	.get_pending_int = rv3028_counter_get_pending_int,
	.get_top_value = rv3028_counter_get_top_value,
	.get_freq = rv3028_counter_get_freq,
};

#define INIT(inst)                                                                                 \
	static const struct rv3028_counter_config rv3028_counter_config_##inst = {                 \
		.counter_info =                                                                    \
			{                                                                          \
				.max_top_value = 4095,                                             \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1,                                                     \
			},                                                                         \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.repeat_mode = DT_INST_PROP(inst, repeat_mode),                                    \
		.frequency = DT_INST_PROP_OR(inst, frequency, 4096),                               \
	};                                                                                         \
                                                                                                   \
	static struct rv3028_counter_data rv3028_counter_data_##inst;                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &rv3028_counter_init, NULL, &rv3028_counter_data_##inst,       \
			      &rv3028_counter_config_##inst, POST_KERNEL,                          \
			      CONFIG_COUNTER_INIT_PRIORITY, &rv3028_counter_api);

DT_INST_FOREACH_STATUS_OKAY(INIT)
