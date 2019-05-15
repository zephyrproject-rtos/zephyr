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

static const struct zio_dev_chan synth_chans[2] = {
	{
		.name = "Left",
		.type = SYNTH_AUDIO_TYPE,
		.bit_width = 16,
		.byte_size = 2,
		.byte_order = ZIO_BYTEORDER_ARCH,
		.sign_bit = ZIO_SIGN_MSB,
	},
	{
		.name = "Right",
		.type = SYNTH_AUDIO_TYPE,
		.bit_width = 16,
		.byte_size = 2,
		.byte_order = ZIO_BYTEORDER_ARCH,
		.sign_bit = ZIO_SIGN_MSB,
	}
};

static struct synth_data {
	u32_t last_timestamp;
	u32_t t; /* samples index */
	struct device *counter;
	struct zio_attr dev_attrs[1];
	struct zio_attr chan_attrs[2][2];

	ZIO_FIFO_BUF_DECLARE(fifo, struct synth_datum, CONFIG_SYNTH_FIFO_SIZE);
}
synth_data = {
	.last_timestamp = 0,
	.t = 0,
	.dev_attrs = {
		{
			.type = ZIO_SAMPLE_RATE,
			.data = zio_variant_wrap_u32(CONFIG_SYNTH_SAMPLE_RATE),
		}
	},
	.chan_attrs = {
		{
			{
				.type = SYNTH_FREQUENCY,
				.data = zio_variant_wrap_u32(CONFIG_SYNTH_0_FREQ),
			},
			{
				.type = SYNTH_PHASE,
				.data = zio_variant_wrap_u32(CONFIG_SYNTH_0_PHASE),
			},
		},
		{
			{
				.type = SYNTH_FREQUENCY,
				.data = zio_variant_wrap_u32(CONFIG_SYNTH_1_FREQ),
			},
			{
				.type = SYNTH_PHASE,
				.data = zio_variant_wrap_u32(CONFIG_SYNTH_1_PHASE),
			}
		},
	},
	.fifo = ZIO_FIFO_BUF_INITIALIZER(synth_data.fifo, struct synth_datum, CONFIG_SYNTH_FIFO_SIZE),
};

static int synth_set_sample_rate(struct device *dev, u32_t sample_rate)
{
	return 0;
}

static int synth_set_attr(struct device *dev, const u32_t attr_idx,
		const struct zio_variant val)
{
	u32_t sample_rate = 0;
	int res = 0;

	switch (attr_idx) {
	case SYNTH_SAMPLE_RATE_IDX:
		res = zio_variant_unwrap(val, sample_rate);
		if (res != 0) {
			return -EINVAL;
		}
		return synth_set_sample_rate(dev, sample_rate);
	default:
		return -EINVAL;
	}
}

static u32_t synth_get_sample_rate(struct device *dev)
{
	struct synth_data *drv_data = dev->driver_data;
	u32_t sample_rate = 0;

	zio_variant_unwrap(drv_data->dev_attrs[0].data, sample_rate);
	return sample_rate;
}

static u32_t synth_chan_get_frequency(struct device *dev, u32_t chan_idx)
{
	struct synth_data *drv_data = dev->driver_data;
	u32_t frequency = 0;

	zio_variant_unwrap(drv_data->chan_attrs[chan_idx][0].data, frequency);
	return frequency;
}

static u32_t synth_chan_get_phase(struct device *dev, u32_t chan_idx)
{
	struct synth_data *drv_data = dev->driver_data;
	u32_t phase = 0;

	zio_variant_unwrap(drv_data->chan_attrs[chan_idx][1].data, phase);
	return phase;
}

static int synth_get_attr(struct device *dev, u32_t attr_idx,
		struct zio_variant *var)
{
	u32_t sample_rate = 0;

	switch (SYNTH_SAMPLE_RATE_IDX) {
	case 0:
		sample_rate = synth_get_sample_rate(dev);
		*var = zio_variant_wrap(sample_rate);
		return 0;
	default:
		return -EINVAL;
	}
}


static int synth_get_attrs(struct device *dev, struct zio_attr **attrs,
		u32_t *num_attrs)
{
	struct synth_data *drv_data = dev->driver_data;

	*attrs = drv_data->dev_attrs;
	*num_attrs = sizeof(drv_data->dev_attrs);
	return 0;
}

static int synth_get_chans(struct device *dev,
		const struct zio_dev_chan **chans,
		u32_t *num_chans)
{
	*chans = synth_chans;
	*num_chans = sizeof(synth_chans);
	return 0;
}

static int synth_generate(struct device *dev, u32_t n)
{
	struct synth_data *drv_data = dev->driver_data;
	float sample_rate = (float)(synth_get_sample_rate(dev));
	struct synth_datum datum;

	for (u32_t i = 0; i < n; i++) {
		for (int j = 0; j < 2; j++) {
			float freq = (float)(synth_chan_get_frequency(dev, j));
			float chan_phase = (float) synth_chan_get_phase(dev, j)
			float phase = (M_PI*chan_phase)/180.0;
			float sample = sin(2.0*M_PI*(freq/sample_rate)*drv_data->t + phase);

			datum.samples[j] = (s16_t)(sample*(float)(SHRT_MAX));
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
	float tdiff_s = (float)tdiff_ns/1000000.0;
	float n_gen_f = (float)(synth_get_sample_rate(dev))/tdiff_s;
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

static const struct zio_dev_api synth_driver_api = {
	.set_attr = synth_set_attr,
	.get_attr = synth_get_attr,
	.get_attrs = synth_get_attrs,
	.get_chans = synth_get_chans,
	.trigger = synth_trigger,
	.attach_buf = synth_attach_buf,
	.detach_buf = synth_detach_buf
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
		    CONFIG_ZIO_INIT_PRIORITY, &synth_driver_api);
