/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zio.h>
#include <init.h>
#include <misc/__assert.h>
#include <misc/byteorder.h>
#include <sensor.h>
#include <string.h>
#include <logging/log.h>
#include <counter.h>
#include <sys_clock.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <drivers/zio/synth.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(SYNTH);

static struct synth_data {
	u32_t last_timestamp;
	u32_t t; /* samples index */
	struct device *counter;
	u32_t sample_rate;
	float frequencies[2];
	float phases[2];

	ZIO_FIFO_BUF_DECLARE(fifo, struct synth_datum, CONFIG_SYNTH_FIFO_SIZE);
}
synth_data = {
	.last_timestamp = 0,
	.t = 0,
	.sample_rate = (float)CONFIG_SYNTH_SAMPLE_RATE,
	.frequencies = {
		(float)(CONFIG_SYNTH_0_FREQ),
		(float)(CONFIG_SYNTH_1_FREQ)
	},
	.phases = {
		(float)(CONFIG_SYNTH_0_PHASE),
		(float)(CONFIG_SYNTH_1_PHASE)
	},
	.fifo = ZIO_FIFO_BUF_INITIALIZER(synth_data.fifo, struct synth_datum, CONFIG_SYNTH_FIFO_SIZE),
};

static int synth_generate(struct device *dev, u32_t n)
{
	struct synth_data *drv_data = dev->driver_data;
	float sample_rate = drv_data->sample_rate;
	struct synth_datum datum;

	for (u32_t i = 0; i < n; i++) {
		for (int j = 0; j < 2; j++) {
			float freq = drv_data->frequencies[j];
			float chan_phase = drv_data->phases[j];
			float phase = (M_PI * chan_phase) / 180.0;
			float sample = sin(2.0 * M_PI * (freq / sample_rate) * drv_data->t + phase);

			datum.samples[j] = (s16_t)(sample * (float)(SHRT_MAX));
		}
		zio_fifo_buf_push(&drv_data->fifo, datum);
		drv_data->t += 1;
	}
	return 0;
}

static int synth_trigger(struct device *dev)
{
	struct synth_data *drv_data = dev->driver_data;

	/* determine number of samples to generate */
	u32_t now = k_cycle_get_32();
	u32_t tstamp_diff = now - drv_data->last_timestamp;
	u64_t tdiff_ns = SYS_CLOCK_HW_CYCLES_TO_NS64(tstamp_diff);

	/* keep it simple, do all this in floats for simplicity, no speed */
	float tdiff_s = (float)tdiff_ns / 1000000.0;
	float n_gen_f = drv_data->sample_rate / tdiff_s;
	u32_t n_gen = round(n_gen_f);

	drv_data->last_timestamp = now;
	return synth_generate(dev, n_gen);
}

static int synth_attach_buf(struct device *dev, struct zio_buf *buf)
{
	struct synth_data *drv_data = dev->driver_data;

	return zio_fifo_buf_attach(&drv_data->fifo, buf);
}

static int synth_detach_buf(struct device *dev)
{
	struct synth_data *drv_data = dev->driver_data;

	return zio_fifo_buf_detach(&drv_data->fifo);
}

static int synth_dev_sample_set_attr(struct device *dev, const u16_t channel_idx, const u16_t attribute_idx, const struct zio_variant val)
{
	struct synth_data *drv_data = dev->driver_data;
	u32_t sample_rate = 0;

	if (zio_variant_unwrap(val, sample_rate) != 0) {
		return -EINVAL;
	}
	drv_data->sample_rate = sample_rate;
}

static int synth_dev_sample_get_attr(struct device *dev, const u16_t channel_idx, const u16_t attribute_idx, struct zio_variant *attr)
{
	struct synth_data *drv_data = dev->driver_data;

	*var = zio_variant_wrap(drv_data->sample_rate);
	return 0;
}


static int synth_channel_phase_set_attr(struct device *dev, const u16_t channel_idx, const u16_t attribute_idx, const struct zio_variant val)
{
	struct synth_data *drv_data = dev->driver_data;
	u32_t sample_rate = 0;

	if (zio_variant_unwrap(val, sample_rate) != 0) {
		return -EINVAL;
	}
	drv_data->sample_rate = sample_rate;
}

static int synth_channel_phase_get_attr(struct device *dev, const u16_t channel_idx, const u16_t attribute_idx, struct zio_variant *attr)
{
	struct synth_data *drv_data = dev->driver_data;

	switch (channel_idx) {
	case SYNTH_LEFT_CHANNEL:
		*var = zio_variant_wrap(drv_data->phases[0]);
	case SYNTH_RIGHT_CHANNEL:
		*var = zio_variant_wrap(drv_data->phases[1]);
	default:
		return -EINVAL;
	}
	return 0;
}

static int synth_channel_phase_set_attr(struct device *dev, const u16_t channel_idx, const u16_t attribute_idx, const struct zio_variant val)
{
	struct synth_data *drv_data = dev->driver_data;
	u32_t phase = 0;

	if (zio_variant_unwrap(val, phase) != 0) {
		return -EINVAL;
	}

	switch (channel_idx) {
	case SYNTH_LEFT_CHANNEL:
		drv_data->phases[0] = phase;
	case SYNTH_RIGHT_CHANNEL:
		drv_data->phases[1] = phase;
	default:
		return -EINVAL;
	}
	return 0;
}


static int synth_channel_frequency_get_attr(struct device *dev, const u16_t channel_idx, const u16_t attribute_idx, struct zio_variant *attr)
{
	struct synth_data *drv_data = dev->driver_data;

	switch (channel_idx) {
	case SYNTH_LEFT_CHANNEL:
		*var = zio_variant_wrap(drv_data->frequencies[0]);
	case SYNTH_RIGHT_CHANNEL:
		*var = zio_variant_wrap(drv_data->frequencies[1]);
	default:
		return -EINVAL;
	}
	return 0;
}

static int synth_channel_frequency_set_attr(struct device *dev, const u16_t channel_idx, const u16_t attribute_idx, const struct zio_variant val)
{
	struct synth_data *drv_data = dev->driver_data;
	u32_t phase = 0;

	if (zio_variant_unwrap(val, phase) != 0) {
		return -EINVAL;
	}

	switch (channel_idx) {
	case SYNTH_LEFT_CHANNEL:
		drv_data->frequencies[0] = phase;
	case SYNTH_RIGHT_CHANNEL:
		drv_data->frequencies[1] = phase;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct zio_synth_api {
	ZIO_DEFINE_CHANNEL(SYNTH_LEFT_CHANNEL,			       \
			   ZIO_DEFINE_ATTRIBUTE(SYNTH_FREQUENCY_ATTR); \
			   ZIO_DEFINE_ATTRIBUTE(SYNTH_PHASE_ATTR);     \
			   );
	ZIO_DEFINE_CHANNEL(SYNTH_RIGHT_CHANNEL,			       \
			   ZIO_DEFINE_ATTRIBUTE(SYNTH_FREQUENCY_ATTR); \
			   ZIO_DEFINE_ATTRIBUTE(SYNTH_PHASE_ATTR);     \
			   );
	ZIO_DEFINE_ATTRIBUTE(ZIO_SAMPLE_DEVICE_ATTR);
	ZIO_DEFINE_API();
} dev_api = {
	.ZIO_ATTR(ZIO_SAMPLE_DEVICE_ATTR) = {
		.data_type = zio_variant_float,
		.get_attr = synth_dev_sample_get_attr,
		.set_attr = synth_dev_sample_set_attr
	},
	.ZIO_CHANNEL(SYNTH_LEFT_CHANNEL) = {
		.channel = {
			.name = "Left",
			.bit_width = 16,
			.byte_size = 2,
			.byte_order = ZIO_BYTEORDER_ARCH,
			.sign_bit = ZIO_SIGN_MSB,
			.attributes = chans_attr_descs
		},
		.ZIO_ATTR(SYNTH_FREQUENCY_ATTR) = {
			.data_type = zio_variant_float,
			.get_attr = synth_channel_frequency_get_attr,
			.set_attr = synth_channel_frequency_set_attr
		},
		.ZIO_ATTR(SYNTH_PHASE_ATTR) = {
			.data_type = zio_variant_float,
			.get_attr = synth_channel_phase_get_attr,
			.set_attr = synth_channel_phase_set_attr
		}
	},
	.ZIO_CHANNEL(SYNTH_RIGHT_CHANNEL) = {
		.channel = {
			.name = "Right",
			.bit_width = 16,
			.byte_size = 2,
			.byte_order = ZIO_BYTEORDER_ARCH,
			.sign_bit = ZIO_SIGN_MSB,
			.attributes = chans_attr_descs
		},
		.ZIO_ATTR(SYNTH_FREQUENCY_ATTR) = {
			.data_type = zio_variant_float,
			.get_attr = synth_channel_frequency_get_attr,
			.set_attr = synth_channel_frequency_set_attr
		},
		.ZIO_ATTR(SYNTH_PHASE_ATTR) = {
			.data_type = zio_variant_float,
			.get_attr = synth_channel_phase_get_attr,
			.set_attr = synth_channel_phase_set_attr
		}
	},
	.ZIO_API() = {
		.trigger = synth_trigger,
		.attach_buf = synth_attach_buf,
		.detach_buf = synth_detach_buf
	}
};

int synth_init(struct device *dev)
{
	/* statically initialized, so nothing to do yet but set the timestamp
	 * and add an appropriate counter callback to generate samples
	 */
	struct synth_data *drv_data = dev->driver_data;

	drv_data->last_timestamp = k_cycle_get_32();
	return 0;
}

static struct synth_data synth_data;

DEVICE_AND_API_INIT(synth, "SYNTH", synth_init,
		    &synth_data, NULL, POST_KERNEL,
		    CONFIG_ZIO_INIT_PRIORITY, &dev_api);
