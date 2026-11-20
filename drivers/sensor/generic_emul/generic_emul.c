/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_generic_emul_sensor

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/generic_emul.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "generic_emul.h"

void generic_emul_reset(const struct emul *target, bool reset_rc)
{
	struct generic_emul_data *data = target->data;

	K_SPINLOCK(&data->lock) {
		memset(data->channels_set, 0, sizeof(data->channels_set));
	}
	if (reset_rc) {
		data->resume_rc = 0;
		data->suspend_rc = 0;
		data->fetch_rc = 0;
	}
}

void generic_emul_func_rc(const struct emul *target, int resume_rc, int suspend_rc, int fetch_rc)
{
	struct generic_emul_data *data = target->data;

	data->resume_rc = resume_rc;
	data->suspend_rc = suspend_rc;
	data->fetch_rc = fetch_rc;
}

static int generic_emul_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct generic_emul_data *data = dev->data;

	if (chan > SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	return data->fetch_rc;
}

static int generic_emul_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	struct generic_emul_data *data = dev->data;

	if (chan >= SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	/* A three-axis channel reads the values of its X, Y and Z channels */
	if (SENSOR_CHANNEL_3_AXIS(chan)) {
		for (int i = 0; i < 3; i++) {
			enum sensor_channel axis = generic_emul_axis_channel(chan, i);

			if (!atomic_test_bit(data->channels_set, axis)) {
				return -ENOTSUP;
			}
		}

		for (int i = 0; i < 3; i++) {
			val[i] = data->channel_values[generic_emul_axis_channel(chan, i)];
		}

		return 0;
	}

	if (!atomic_test_bit(data->channels_set, chan)) {
		return -ENOTSUP;
	}

	*val = data->channel_values[chan];
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int generic_emul_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct generic_emul_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return data->suspend_rc;
	case PM_DEVICE_ACTION_RESUME:
		return data->resume_rc;
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
		return 0;
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, generic_emul_driver_api) = {
	.sample_fetch = generic_emul_sample_fetch,
	.channel_get = generic_emul_channel_get,
};

#ifdef CONFIG_SENSOR_ASYNC_API
/* For instances that stream; the others use the RTIO fallback */
static DEVICE_API(sensor, generic_emul_stream_api) = {
	.sample_fetch = generic_emul_sample_fetch,
	.channel_get = generic_emul_channel_get,
	.submit = generic_emul_submit,
	.get_decoder = generic_emul_get_decoder,
};
#endif /* CONFIG_SENSOR_ASYNC_API */

/* A three-axis channel sets each of its X, Y and Z channels */
static void set_value(struct generic_emul_data *data, enum sensor_channel chan,
		      struct sensor_value val, q31_t value, int8_t shift)
{
	int axes = SENSOR_CHANNEL_3_AXIS(chan) ? 3 : 1;

	K_SPINLOCK(&data->lock) {
		for (int i = 0; i < axes; i++) {
			enum sensor_channel axis = generic_emul_axis_channel(chan, i);

			data->channel_values[axis] = val;
			if (data->stream != NULL) {
				data->stream->q31[axis] = value;
				data->stream->shift[axis] = shift;
			}
			atomic_set_bit(data->channels_set, axis);
		}
	}
}

static int generic_emul_set_channel(const struct emul *target, struct sensor_chan_spec ch,
				    const q31_t *value, int8_t shift)
{
	struct generic_emul_data *data = target->data;
	struct sensor_value val;
	int64_t micro;

	if (!value) {
		return -EINVAL;
	}

	if (ch.chan_type >= SENSOR_CHAN_ALL || ch.chan_idx != 0) {
		return -ENOTSUP;
	}

	micro = (int64_t)*value * 1000000;
	if (shift > 0) {
		if (shift >= 63 ||
		    micro > (INT64_MAX >> shift) ||
		    micro < (INT64_MIN >> shift)) {
			return -ERANGE;
		}
		micro <<= shift;
	} else if (shift < 0) {
		if (shift <= -63) {
			micro = 0;
		} else {
			micro >>= -shift;
		}
	}
	micro /= INT64_C(1) << 31;

	if (sensor_value_from_micro(&val, micro) != 0) {
		return -ERANGE;
	}

	set_value(data, ch.chan_type, val, *value, shift);

	return 0;
}

static int generic_emul_get_sample_range(const struct emul *target, struct sensor_chan_spec ch,
					 q31_t *lower, q31_t *upper, q31_t *epsilon,
					 int8_t *shift)
{
	ARG_UNUSED(target);

	if (!lower || !upper || !epsilon || !shift) {
		return -EINVAL;
	}

	if (ch.chan_type >= SENSOR_CHAN_ALL || ch.chan_idx != 0) {
		return -ENOTSUP;
	}

	*shift = 0;
	*lower = INT32_MIN;
	*upper = INT32_MAX;
	*epsilon = 1;

	return 0;
}

static const struct emul_sensor_driver_api generic_emul_backend_api = {
	.set_channel = generic_emul_set_channel,
	.get_sample_range = generic_emul_get_sample_range,
};

static int generic_emul_init(const struct device *dev)
{
	const struct generic_emul_cfg *cfg = dev->config;

	if (cfg->init_rc != 0) {
		return cfg->init_rc;
	}

#ifdef CONFIG_SENSOR_ASYNC_API
	if (cfg->stream != NULL) {
		generic_emul_stream_init(dev);
	}
#endif /* CONFIG_SENSOR_ASYNC_API */

	return cfg->emul->init(cfg->emul, dev);
}

static int generic_emul_backend_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	generic_emul_reset(target, true);

	return 0;
}

#if defined(CONFIG_SENSOR_ASYNC_API)
#define GENERIC_EMUL_STREAMS(inst) DT_INST_NODE_HAS_PROP(inst, sampling_frequency)
#else
#define GENERIC_EMUL_STREAMS(inst) 0
#endif

#define GENERIC_EMUL_STREAM_DEFINE(inst)                                                           \
	BUILD_ASSERT(DT_INST_PROP(inst, sampling_frequency) > 0,                                   \
		     "sampling-frequency must be > 0");                                            \
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP(inst, fifo_watermark), 1,                               \
			      DT_INST_PROP(inst, fifo_depth)),                                     \
		     "fifo-watermark must be from 1 to fifo-depth");                               \
	static struct generic_emul_stream generic_emul_stream_##inst;                              \
	static const struct generic_emul_stream_cfg generic_emul_stream_cfg_##inst = {             \
		.period_ticks = MAX(1, DIV_ROUND_CLOSEST(CONFIG_SYS_CLOCK_TICKS_PER_SEC,           \
							 DT_INST_PROP(inst, sampling_frequency))), \
		.fifo_watermark = DT_INST_PROP(inst, fifo_watermark),                              \
		.fifo_depth = DT_INST_PROP(inst, fifo_depth),                                      \
	};

#define GENERIC_EMUL_DEFINE(inst)                                                                  \
	IF_ENABLED(GENERIC_EMUL_STREAMS(inst), (GENERIC_EMUL_STREAM_DEFINE(inst)))                 \
	static struct generic_emul_data generic_emul_data_##inst = {                               \
		.stream = COND_CODE_1(GENERIC_EMUL_STREAMS(inst),                                  \
				      (&generic_emul_stream_##inst), (NULL)),                      \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, generic_emul_pm_control);                                   \
	static const struct generic_emul_cfg generic_emul_cfg_##inst = {                           \
		.emul = EMUL_DT_GET(DT_DRV_INST(inst)),                                            \
		.init_rc = -DT_INST_PROP(inst, negated_init_rc),                                   \
		.stream = COND_CODE_1(GENERIC_EMUL_STREAMS(inst),                                  \
				      (&generic_emul_stream_cfg_##inst), (NULL)),                  \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, generic_emul_init, PM_DEVICE_DT_INST_GET(inst),         \
				     &generic_emul_data_##inst, &generic_emul_cfg_##inst,          \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     \
				     COND_CODE_1(GENERIC_EMUL_STREAMS(inst),                       \
						 (&generic_emul_stream_api),                       \
						 (&generic_emul_driver_api)));                     \
	EMUL_DT_INST_DEFINE(inst, generic_emul_backend_init, &generic_emul_data_##inst, NULL,      \
			    NULL, &generic_emul_backend_api);

DT_INST_FOREACH_STATUS_OKAY(GENERIC_EMUL_DEFINE)
