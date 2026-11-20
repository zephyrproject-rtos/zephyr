/*
 * Copyright (c) 2026 Embeint Inc
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_GENERIC_EMUL_GENERIC_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_GENERIC_EMUL_GENERIC_EMUL_H_

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/generic_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/mpsc_lockfree.h>

/* Guarded by generic_emul_data.lock, except the device and the timer */
struct generic_emul_stream {
	const struct device *dev;
	q31_t q31[SENSOR_CHAN_ALL];
	int8_t shift[SENSOR_CHAN_ALL];
	generic_emul_sample_fn sample_fn;
	void *sample_user_data;
	struct k_timer timer;
	/* Reads waiting for a watermark, oldest first, linked through their q node */
	struct mpsc reads;
	uint32_t pending;
	/* From the first read until a cancel leaves none */
	bool active;
	uint16_t fifo;
	uint32_t seq;
	uint32_t overruns;
};

struct generic_emul_stream_cfg {
	uint32_t period_ticks;
	uint16_t fifo_watermark;
	uint16_t fifo_depth;
};

struct generic_emul_data {
	struct sensor_value channel_values[SENSOR_CHAN_ALL];
	ATOMIC_DEFINE(channels_set, SENSOR_CHAN_ALL);
	int resume_rc;
	int suspend_rc;
	int fetch_rc;
	/* Makes a channel update, all its axes at once, atomic to stream deliveries */
	struct k_spinlock lock;
	struct generic_emul_stream *stream;
};

struct generic_emul_cfg {
	const struct emul *emul;
	int init_rc;
	const struct generic_emul_stream_cfg *stream;
};

/*
 * The X, Y or Z channel (axis 0, 1 or 2) of a three-axis channel. Any other channel is its own
 * axis 0.
 */
static inline enum sensor_channel generic_emul_axis_channel(enum sensor_channel chan, int axis)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return SENSOR_CHAN_ACCEL_X + axis;
	case SENSOR_CHAN_GYRO_XYZ:
		return SENSOR_CHAN_GYRO_X + axis;
	case SENSOR_CHAN_MAGN_XYZ:
		return SENSOR_CHAN_MAGN_X + axis;
	case SENSOR_CHAN_POS_DXYZ:
		return SENSOR_CHAN_POS_DX + axis;
	default:
		return chan;
	}
}

void generic_emul_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int generic_emul_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
void generic_emul_stream_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_GENERIC_EMUL_GENERIC_EMUL_H_ */
