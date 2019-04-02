/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <counter.h>
#include <logging/log.h>

#define LOG_MODULE_NAME counter_generic
LOG_MODULE_REGISTER(counter_gen, CONFIG_COUNTER_LOG_LEVEL);

/* returns 1 if in range (including min value) */
#define IN_RANGE(val, min, max) (((val) >= (min) && (val) < (max)) ? 1 : 0)

#define DBG(...) LOG_INST_DBG(get_dev_data(dev)->log, __VA_ARGS__)
#define INF(...) LOG_INST_INF(get_dev_data(dev)->log, __VA_ARGS__)
#define WRN(...) LOG_INST_WRN(get_dev_data(dev)->log, __VA_ARGS__)
#define ERR(...) LOG_INST_ERR(get_dev_data(dev)->log, __VA_ARGS__)

/*
 * In this driver the data is located in flash and remains read-only,
 * while config is located in RAM and is updated in run-time. Such design
 * is forced by the fact that backend properties could be obtained only
 * in run-time.
 */

struct counter_generic_data {
	const char *backend_name;
	u8_t prescale;
	LOG_INSTANCE_PTR_DECLARE(log);
};

struct counter_channel_data {
	counter_alarm_callback_t clbk;
	void *user_data;
	u32_t ticks;
	bool frag;
};

struct counter_generic_config {
	struct counter_config_info info;
	struct device *backend;
	u32_t cnt;
	struct counter_channel_data chdata[1];
};

static inline struct counter_generic_config *get_dev_config(struct device *dev)
{
	return CONTAINER_OF(dev->config->config_info,
				struct counter_generic_config, info);
}

static inline const struct counter_generic_data *get_dev_data(
							struct device *dev)
{
	return dev->driver_data;
}

static int counter_generic_start(struct device *dev)
{
	struct counter_generic_config *config = get_dev_config(dev);

	return counter_start(config->backend);
}

static int counter_generic_stop(struct device *dev)
{
	struct counter_generic_config *config = get_dev_config(dev);

	/* Cancel potential sync tick. */
	counter_cancel_channel_alarm(config->backend, 0);

	config->cnt = 0;
	return counter_stop(config->backend);
}

static u32_t get_top_value(struct device *backend)
{
	return IS_ENABLED(CONFIG_COUNTER_GENERIC_TEST_MODE) ?
		counter_get_top_value(backend) :
		counter_get_max_top_value(backend);
}

static bool is_backend32(struct device *backend)
{
	return (get_top_value(backend) == UINT32_MAX);
}

static u32_t emu32_read(struct device *dev)
{
	struct counter_generic_config *config = get_dev_config(dev);
	const struct counter_generic_data *devdata = get_dev_data(dev);
	u32_t cnt_lo;
	u32_t cnt_hi;
	u32_t max;
	u32_t common_bit;

	/* TODO: What about counting down backend. */
	cnt_hi = config->cnt;
	cnt_lo = counter_read(config->backend) >> devdata->prescale;
	max = get_top_value(config->backend) >> devdata->prescale;

	common_bit = (max >> 1) + 1;
	if ((cnt_lo & common_bit) != (cnt_hi & common_bit)) {
		cnt_hi += common_bit;
		config->cnt = cnt_hi;
	}

	return cnt_hi | (cnt_lo & (max >> 1));
}

static u32_t counter_generic_read(struct device *dev)
{
	struct counter_generic_config *config = get_dev_config(dev);
	const struct counter_generic_data *devdata = get_dev_data(dev);

	return is_backend32(config->backend) ?
		counter_read(config->backend) >> devdata->prescale :
		emu32_read(dev);
}

static u32_t get_backend_ticks(struct device *dev, u32_t ticks32, u32_t *bticks)
{
	struct counter_generic_config *config = get_dev_config(dev);
	u32_t now = counter_read(dev);
	u32_t rel_ticks = ticks32 - now;
	u32_t htop = get_top_value(config->backend)/2;
	u32_t new_ticks = ticks32;
	bool frag = false;
	u32_t guard = counter_get_top_value(config->backend) -
			counter_get_max_relative_alarm(config->backend);

	if (IN_RANGE((int)rel_ticks, -guard, 0)) {
		new_ticks = ticks32;
	} else if (rel_ticks > htop + guard /*todo - make relative */) {
		new_ticks = now + htop - guard/2;
		frag = true;
	} else if (IN_RANGE(rel_ticks, guard, htop)) {
		new_ticks = now + rel_ticks/2;
		frag = true;
	} else {
		new_ticks = ticks32;
	}

	*bticks = new_ticks & get_top_value(config->backend);
	DBG("now:%d, rel_ticks:%d, qtop:%d , bticks %d",
			now, rel_ticks, htop, *bticks);

	return frag;
}

static void call_user_alarm(struct device *dev, u8_t chan_id)
{
	struct counter_generic_config *config = get_dev_config(dev);
	counter_alarm_callback_t clbk = config->chdata[chan_id].clbk;
	u32_t ticks = config->chdata[chan_id].ticks;
	void *user_data = config->chdata[chan_id].user_data;

	config->chdata[chan_id].clbk = NULL;
	clbk(dev, chan_id, ticks, user_data);
}

static bool user_alarm_handle(struct device *dev, u8_t chan_id,
			      struct counter_alarm_cfg *cfg)
{
	struct counter_generic_config *config = get_dev_config(dev);
	bool user_alarm = false;
	int err;

	if (config->chdata[chan_id].clbk) {
		if (config->chdata[chan_id].frag) {
			bool frag;

			frag = get_backend_ticks(dev,
						 config->chdata[chan_id].ticks,
						 &cfg->ticks);
			config->chdata[chan_id].frag = frag;
			err = counter_set_channel_alarm(config->backend,
							chan_id, cfg);
			__ASSERT_NO_MSG(err == 0);
			user_alarm = true;
		} else {
			call_user_alarm(dev, chan_id);
		}
	}

	return user_alarm;
}

static void set_sync_tick(struct device *dev, struct device *backend,
			  struct counter_alarm_cfg *cfg)
{
	int err;
	u32_t top = get_top_value(backend);
	u32_t now = counter_read(backend);

	cfg->ticks = IN_RANGE(now, top/4 - 100, 3*top/4 - 100) ?
							3*top/4 : top/4;
	err = counter_set_channel_alarm(backend, 0, cfg);
	__ASSERT((err == 0) || (err == -EBUSY), "Unexpected err: %d", err);
	DBG("Setting sync tick (now: %d, next %d)", now, cfg->ticks);
}

static void alarm_callback_frag(struct device *backend, u8_t chan_id,
				u32_t ticks, void *user_data)
{
	struct device *dev = (struct device *)user_data;
	struct counter_alarm_cfg cfg = {
		.callback = alarm_callback_frag,
		.user_data = dev,
		.absolute = true
	};

	INF("alarm_callback chan: %d, ticks:%d", chan_id, ticks);

	bool user_alarm = user_alarm_handle(dev, chan_id, &cfg);

	if (!user_alarm && (chan_id == 0)) {
		u32_t cnt = counter_read(dev);

		DBG("Sync tick (cnt:%d).", cnt);
		set_sync_tick(dev, backend, &cfg);
	}
}

static void alarm_callback32(struct device *backend, u8_t chan_id, u32_t ticks,
			     void *user_data)
{
	struct device *dev = (struct device *)user_data;

	DBG("alarm_callback chan: %d, ticks:%d", chan_id, ticks);
	call_user_alarm(dev, chan_id);
}

static int counter_generic_set_alarm(struct device *dev, u8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	__ASSERT(alarm_cfg->absolute, "Only absolute supported");
	int err;
	struct counter_generic_config *config = get_dev_config(dev);
	struct device *backend = config->backend;
	struct counter_alarm_cfg cfg = {
		.user_data = dev,
		.absolute = true,
	};

	if (is_backend32(backend)) {
		cfg.callback = alarm_callback32;
		cfg.ticks = alarm_cfg->ticks;
	} else {
		u32_t backend_ticks;
		bool frag;

		frag = get_backend_ticks(dev, alarm_cfg->ticks, &backend_ticks);
		cfg.ticks = backend_ticks;
		cfg.callback = alarm_callback_frag;

		if (chan_id == 0) {
			int err;

			err = counter_cancel_channel_alarm(backend, chan_id);
			if (err != 0) {
				ERR("Failed to cancel alarm (err: %d).", err);
				return err;
			}
		}

		config->chdata[chan_id].frag = frag;
		DBG("Setting alarm, ticks:%d", alarm_cfg->ticks);
		if (frag) {
			DBG("Fragmented, first part:%d", cfg.ticks);
		}
	}

	config->chdata[chan_id].clbk = alarm_cfg->callback;
	config->chdata[chan_id].user_data = alarm_cfg->user_data;
	config->chdata[chan_id].ticks = alarm_cfg->ticks;

	err = counter_set_channel_alarm(backend, chan_id, &cfg);
	if (err != 0) {
		ERR("Failed to set alarm (err %d).", err);
		return err;
	}

	return 0;
}

static int counter_generic_cancel_alarm(struct device *dev, u8_t chan_id)
{
	struct counter_generic_config *config = get_dev_config(dev);

	counter_cancel_channel_alarm(config->backend, chan_id);
	config->chdata[chan_id].clbk = NULL;

	DBG("Alarm canceled (chan: %d)", chan_id);

	return 0;
}

static int counter_generic_set_top_value(struct device *dev,
					 const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);

	return ((cfg->ticks != UINT32_MAX) ||
		!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) ? -ENOTSUP : 0;
}

static u32_t counter_generic_get_pending_int(struct device *dev)
{
	return 0;
}


static u32_t counter_generic_get_top_value(struct device *dev)
{
	return counter_get_max_top_value(dev);
}

static u32_t counter_generic_get_max_relative_alarm(struct device *dev)
{
	return UINT32_MAX;
}

COND_CODE_1(CONFIG_COUNTER_GENERIC_TEST_MODE, (), (static))
int counter_generic_init_with_max(struct device *dev, u32_t max)
{
	struct counter_generic_config *config = get_dev_config(dev);
	const struct counter_generic_data *devdata = get_dev_data(dev);
	struct device *backend;
	int err;
	struct counter_top_cfg top_cfg = {
		.callback = NULL,
		.flags = 0,
		.ticks = max
	};

	DBG("Initializing dev: %p %s", dev, devdata->backend_name);
	__ASSERT(devdata->backend_name != NULL, "Backend name is not defined!");
	backend = device_get_binding(devdata->backend_name);
	__ASSERT(backend != NULL, "Backend device cannot be found!");

	__ASSERT((max & (max + 1)) == 0,
		 "Maximum backend counter top value must be equal to 2^n-1");

	config->info.max_top_value = UINT32_MAX;
	config->info.freq = counter_get_frequency(backend);
	config->info.flags = COUNTER_CONFIG_INFO_COUNT_UP;
	config->info.channels = counter_get_num_of_channels(backend);
	config->backend = backend;
	config->cnt = 0;

	config->info.freq >>= devdata->prescale;
	__ASSERT(config->info.freq > 0, "Invalid prescaler settings!");

	if (config->info.channels == 0) {
		top_cfg.ticks = max / 3;
		top_cfg.flags = COUNTER_TOP_CFG_DONT_RESET;

		/*
		 * We cannot use compare channels, but we can emulate one
		 * if top value could be changed without resetting the counter.
		 */
		err = counter_set_top_value(backend, &top_cfg);

		__ASSERT(err == 0, "Selected backend cannot be used!");

		top_cfg.ticks = max;
		err = counter_set_top_value(backend, &top_cfg);
	} else {
		/* If counter has channels then top value will never be modified
		 * thus it can be set and reset.
		 */
		err = counter_set_top_value(backend, &top_cfg);
		__ASSERT(err == 0, "Selected backend cannot be used!");

		if (!is_backend32(backend)) {
			struct counter_alarm_cfg cfg = {
				.callback = alarm_callback_frag,
				.user_data = dev,
				.absolute = true
			};

			set_sync_tick(dev, backend, &cfg);
		}
	}

	__ASSERT(err == 0, "Could not configure backend counter!");
	DBG("Initialized dev: %s", devdata->backend_name);
	DBG("channels: %d", config->info.channels);

	return 0;
}

static int counter_generic_init(struct device *dev)
{
	const struct counter_generic_data *devdata = get_dev_data(dev);
	struct device *backend;
	u32_t max;

	__ASSERT(devdata->backend_name != NULL, "Backend name is not defined!");
	backend = device_get_binding(devdata->backend_name);
	__ASSERT(backend != NULL, "Backend device cannot be found!");

	max = counter_get_max_top_value(backend);

	return counter_generic_init_with_max(dev, max);
}

static const struct counter_driver_api counter_generic_driver_api = {
	.start = counter_generic_start,
	.stop = counter_generic_stop,
	.read = counter_generic_read,
	.set_alarm = counter_generic_set_alarm,
	.cancel_alarm = counter_generic_cancel_alarm,
	.set_top_value = counter_generic_set_top_value,
	.get_pending_int = counter_generic_get_pending_int,
	.get_top_value = counter_generic_get_top_value,
	.get_max_relative_alarm = counter_generic_get_max_relative_alarm,
};

#define COUNTER_GENERIC_DEVICE(_idx, _name, _backend_name)		       \
									       \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, _idx, CONFIG_COUNTER_LOG_LEVEL);\
	static const struct counter_generic_data counter_##_idx##_data = {     \
		.backend_name = _backend_name,				       \
		.prescale = 0,						       \
		LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, _idx)	       \
	};								       \
	static struct counter_generic_config counter_##_idx##_config;	       \
	DEVICE_AND_API_INIT(counter_generic_##_idx,			       \
			    _name,					       \
			    counter_generic_init,			       \
			    (void *)&counter_##_idx##_data,		       \
			    &counter_##_idx##_config.info,		       \
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,   \
			    &counter_generic_driver_api)

#ifdef DT_COUNTER_GENERIC_0_LABEL
COUNTER_GENERIC_DEVICE(0, DT_COUNTER_GENERIC_0_LABEL,
			DT_COUNTER_GENERIC_0_BUS_NAME);
#endif

#ifdef DT_COUNTER_GENERIC_1_LABEL
COUNTER_GENERIC_DEVICE(1, DT_COUNTER_GENERIC_1_LABEL,
			DT_COUNTER_GENERIC_1_BUS_NAME);
#endif
