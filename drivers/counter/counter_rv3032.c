/*
 * Copyright (c) 2025 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv3032_counter

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/mfd/rv3032.h>

LOG_MODULE_REGISTER(rv3032_counter, CONFIG_COUNTER_LOG_LEVEL);

#define RV3032_CONTROL1_TD_4096 (RV3032_CONTROL1_TD & 0x0)
#define RV3032_CONTROL1_TD_64   (RV3032_CONTROL1_TD & 0x1)
#define RV3032_CONTROL1_TD_1    (RV3032_CONTROL1_TD & 0x2)
#define RV3032_CONTROL1_TD_1_60 (RV3032_CONTROL1_TD & 0x3)

struct rv3032_counter_config {
	struct counter_config_info counter_info;
	uint32_t base;
	const struct device *mfd;
};

struct rv3032_counter_data {
	struct counter_alarm_cfg alarm_cfg0;
	uint32_t freq;
};

int rv3032_counter_start(const struct device *dev)
{
	const struct rv3032_counter_config *config = dev->config;

	return mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL1, RV3032_CONTROL1_TE,
				      RV3032_CONTROL1_TE);
}

int rv3032_counter_stop(const struct device *dev)
{
	const struct rv3032_counter_config *config = dev->config;

	return mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL1, RV3032_CONTROL1_TE, 0);
}

int rv3032_counter_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct rv3032_counter_config *config = dev->config;
	uint8_t val[2];

	mfd_rv3032_read_regs(config->mfd, RV3032_REG_CONTROL1, val, 2);

	*ticks = (val[1]<<8) | val[0];

	return 0;
}

int rv3032_counter_get_value_64(const struct device *dev, uint64_t *ticks)
{
	return -ENOTSUP;
}

int rv3032_counter_reset(const struct device *dev)
{
	const struct rv3032_counter_config *config = dev->config;
	int ret;

	ret = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL1, RV3032_CONTROL1_TE, 0);
	if (ret) {
		goto exit;
	}

	ret = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL1, RV3032_CONTROL1_TE,
				     RV3032_CONTROL1_TE);
exit:

	return ret;
}

void rv3032_counter_isr(const struct device *dev)
{
	struct rv3032_counter_data *data = dev->data;

	if (data->alarm_cfg0.callback) {
		data->alarm_cfg0.callback(dev, 0, data->alarm_cfg0.ticks,
					  data->alarm_cfg0.user_data);
	}
}

int rv3032_counter_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct rv3032_counter_config *config = dev->config;
	struct rv3032_counter_data *data = dev->data;
	int err;
	uint8_t time_val[2];
	uint8_t freq_val;

	data->alarm_cfg0.user_data = alarm_cfg->user_data;
	data->alarm_cfg0.callback = alarm_cfg->callback;
	data->alarm_cfg0.flags = alarm_cfg->flags;
	data->alarm_cfg0.ticks = alarm_cfg->ticks;

	if (alarm_cfg->ticks > 4096) {
		LOG_ERR("alarm_cfg->ticks %d) Max value (4096)", alarm_cfg->ticks);
		return -ENOTSUP;
	}

	err = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL2, RV3032_CONTROL2_TIE, 0);
	if (err) {
		LOG_ERR("TIMER register read failed : %d", err);
		goto err_return;
	}

	err = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL1, RV3032_CONTROL1_TE, 0);
	if (err) {
		LOG_ERR("TIMER register read failed : %d", err);
		goto err_return;
	}

	time_val[0] = alarm_cfg->ticks & 0xff;
	time_val[1] = (alarm_cfg->ticks & 0xff00) >> 8;

	err = mfd_rv3032_write_regs(config->mfd, RV3032_REG_TIMER_VALUE_0, time_val,
				    sizeof(time_val));
	if (err) {
		LOG_ERR("TIMER register write failed : %d", err);
	}

	if (data->freq == 4096) {
		freq_val = 0x0;
	} else if (data->freq == 64) {
		freq_val = 0x1;
	} else if (data->freq == 1) {
		freq_val = 0x2;
	} else if (data->freq == 0) {
		freq_val = 0x3;
	} else {
		freq_val = 0x0;
	}

	printk("alarm_cfg->ticks [%d] freq_val[%d]\n", alarm_cfg->ticks, freq_val);

	/* Setup clock freq */
	err = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL1, RV3032_CONTROL1_TD,
				     freq_val);
	if (err) {
		LOG_ERR("TIMER register read failed : %d", err);
		goto err_return;
	}

	/* Clear Timer Flag from status if there was something leftover */
	err = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_STATUS, RV3032_STATUS_TF, 0);
	if (err) {
		LOG_ERR("TIMER register read failed : %d", err);
		goto err_return;
	}

	/* Enable Timer interrupts */
	err = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL2, RV3032_CONTROL2_TIE,
				     RV3032_CONTROL2_TIE);
	if (err) {
		LOG_ERR("TIMER register read failed : %d", err);
		goto err_return;
	}

	/* Enable Timer */
	err = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL1, RV3032_CONTROL1_TE,
				     RV3032_CONTROL1_TE);
	if (err) {
		LOG_ERR("TIMER register read failed : %d", err);
		goto err_return;
	}

	mfd_rv3032_set_irq_handler(config->mfd, dev, RV3032_DEV_COUNTER, rv3032_counter_isr);

err_return:

	return err;
}

int rv3032_counter_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct rv3032_counter_config *config = dev->config;
	int err;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id, only 0 is supported");
		return -ENOTSUP;
	}

	/* disable counter interrut */
	err = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_CONTROL1, RV3032_CONTROL1_TE, 0);
	if (err) {
		LOG_ERR("Status register read failed after EEPROM refresh: %d", err);
	}

	return err;
}

uint32_t rv3032_counter_get_pending_int(const struct device *dev)
{
	const struct rv3032_counter_config *config = dev->config;
	uint8_t status;
	int err;

	err = mfd_rv3032_read_reg8(config->mfd, RV3032_REG_STATUS, &status);
	if (err) {
		LOG_ERR("TIMER register read failed : %d", err);
		return err;
	}

	/* Check timer bit in status reg, if there is pending int fire ISR*/
	if (status & RV3032_STATUS_TF) {
		err = mfd_rv3032_update_reg8(config->mfd, RV3032_REG_STATUS, RV3032_STATUS_TF,
					     RV3032_STATUS_TF);
		if (err) {
			LOG_ERR("TIMER register read failed : %d", err);
			return err;
		}

		rv3032_counter_isr(dev);
	}

	return err;
}

int rv3032_counter_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct rv3032_counter_config *config = dev->config;
	uint8_t timer[2];
	int err;

	timer[0] = cfg->ticks & 0xff;
	timer[1] = (cfg->ticks >> 8) & 0xff;

	err = mfd_rv3032_write_regs(config->mfd, RV3032_REG_TIMER_VALUE_0, timer, 2);
	if (err) {
		LOG_ERR("TIMER register read failed : %d", err);
	}

	return 0;
}

uint32_t rv3032_counter_get_top_value(const struct device *dev)
{
	const struct rv3032_counter_config *config = dev->config;
	uint8_t timer[2];
	uint8_t val;
	int err;

	err = mfd_rv3032_read_regs(config->mfd, RV3032_REG_TIMER_VALUE_0, timer, 2);
	if (err) {
		LOG_ERR("TIMER register read failed : %d", err);
		return err;
	}

	val = timer[0] | (timer[1] << 8);

	return val;
}

uint32_t rv3032_counter_get_guard_period(const struct device *dev, uint32_t flags)
{
	return -ENOTSUP;
}

int rv3032_counter_set_guard_period(const struct device *dev, uint32_t ticks, uint32_t flags)
{
	return -ENOTSUP;
}

uint32_t rv3032_counter_get_freq(const struct device *dev)
{
	/*
	 * 4096 Hz – Default value
	 * 64 Hz
	 * 1 Hz
	 * 1/60 Hz
	 */

	const struct rv3032_counter_data *data = dev->data;

	return data->freq;
}

static int rv3032_counter_init(const struct device *dev)
{
	const struct rv3032_counter_config *config = dev->config;

	LOG_DBG("Counter [%s] mdf-parent [%s]\n", dev->name, config->mfd->name);

	return 0;
}

static DEVICE_API(counter, rv3032_counter_api) = {
	.start = rv3032_counter_start,
	.stop = rv3032_counter_stop,
	.get_value = rv3032_counter_get_value,
	.get_value_64 = rv3032_counter_get_value_64,
	.reset = rv3032_counter_reset,
	.set_alarm = rv3032_counter_set_alarm,
	.cancel_alarm = rv3032_counter_cancel_alarm,
	.set_top_value = rv3032_counter_set_top_value,
	.get_pending_int = rv3032_counter_get_pending_int,
	.get_top_value = rv3032_counter_get_top_value,
	.get_freq = rv3032_counter_get_freq,
};

#define rv3032_counter_INIT(inst)                                                                  \
	static const struct rv3032_counter_config rv3032_counter_config_##inst = {                 \
		.counter_info = {                                                                  \
			.max_top_value = 4096,                                                     \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,                                     \
			.channels = 1,                                                             \
		},                                                                                 \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
	};                                                                                         \
	static struct rv3032_counter_data rv3032_counter_data_##inst = {                           \
		.freq = DT_INST_PROP_OR(inst, frequency, 4096),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(0, &rv3032_counter_init, NULL, &rv3032_counter_data_##inst,          \
			      &rv3032_counter_config_##inst, POST_KERNEL,                          \
			      CONFIG_COUNTER_MICROCRYSTAL_RV3032_INIT_PRIORITY,                    \
			      &rv3032_counter_api);

DT_INST_FOREACH_STATUS_OKAY(rv3032_counter_INIT)
