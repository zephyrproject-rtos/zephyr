/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_pulse_io_loopback

#include <zephyr/drivers/pulse_io.h>
#include <zephyr/device.h>
#include <string.h>

#define PULSE_IO_LB_MAX_SYMBOLS 256

struct pulse_io_lb_channel {
	bool in_use;
	bool configured;
	struct pulse_io_config cfg;
	struct pulse_symbol loop[PULSE_IO_LB_MAX_SYMBOLS];
	size_t loop_len;
};

struct pulse_io_lb_data {
	struct pulse_io_lb_channel *channels;
	uint8_t num_channels;
};

static struct pulse_io_lb_channel *to_channel(const struct device *dev,
					      struct pulse_io_channel *handle)
{
	struct pulse_io_lb_data *data = dev->data;
	uintptr_t idx = (uintptr_t)handle - 1U;

	if (idx >= data->num_channels) {
		return NULL;
	}
	return &data->channels[idx];
}

static int lb_get_capabilities(const struct device *dev, struct pulse_io_caps *caps)
{
	struct pulse_io_lb_data *data = dev->data;

	*caps = (struct pulse_io_caps){
		.modes = PULSE_IO_MODE_SYMBOL | PULSE_IO_MODE_CELL,
		.supports_tx = true,
		.supports_rx = true,
		.min_tick_ns = 1,
		.max_tick_ns = 1000000,
		.max_duration_ticks = INT32_MAX,
		.num_channels = data->num_channels,
	};
	return 0;
}

static int lb_channel_get(const struct device *dev, uint8_t channel_idx,
			  struct pulse_io_channel **chan)
{
	struct pulse_io_lb_data *data = dev->data;

	if (channel_idx >= data->num_channels) {
		return -ENODEV;
	}
	if (data->channels[channel_idx].in_use) {
		return -EBUSY;
	}
	data->channels[channel_idx].in_use = true;
	*chan = (struct pulse_io_channel *)(uintptr_t)(channel_idx + 1U);
	return 0;
}

static int lb_channel_release(const struct device *dev, struct pulse_io_channel *chan)
{
	struct pulse_io_lb_channel *ch = to_channel(dev, chan);

	if (ch == NULL) {
		return -EINVAL;
	}
	ch->in_use = false;
	ch->configured = false;
	ch->loop_len = 0;
	return 0;
}

static int lb_channel_configure(const struct device *dev, struct pulse_io_channel *chan,
				const struct pulse_io_config *cfg)
{
	struct pulse_io_lb_channel *ch = to_channel(dev, chan);

	if (ch == NULL) {
		return -EINVAL;
	}
	if ((cfg->mode != PULSE_IO_MODE_SYMBOL) && (cfg->mode != PULSE_IO_MODE_CELL)) {
		return -ENOTSUP;
	}
	if (cfg->mode == PULSE_IO_MODE_CELL && cfg->cell_period_ticks == 0U) {
		return -EINVAL;
	}
	ch->cfg = *cfg;
	ch->configured = true;
	return 0;
}

static int lb_transmit_sync(const struct device *dev, struct pulse_io_channel *chan,
			    const struct pulse_io_tx_req *req, k_timeout_t timeout)
{
	struct pulse_io_lb_channel *ch = to_channel(dev, chan);

	ARG_UNUSED(timeout);

	if (ch == NULL || !ch->configured) {
		return -EINVAL;
	}
	/* Infinite loops cannot complete synchronously; use the async path. */
	if (req->loop_count == UINT32_MAX) {
		return -ENOTSUP;
	}
	if (req->count > PULSE_IO_LB_MAX_SYMBOLS) {
		return -ENOMEM;
	}

	memcpy(ch->loop, req->symbols, req->count * sizeof(struct pulse_symbol));
	ch->loop_len = req->count;
	return 0;
}

static int lb_receive_sync(const struct device *dev, struct pulse_io_channel *chan,
			   const struct pulse_io_rx_req *req, size_t *count, k_timeout_t timeout)
{
	struct pulse_io_lb_channel *ch = to_channel(dev, chan);

	ARG_UNUSED(timeout);

	if (ch == NULL || !ch->configured || count == NULL) {
		return -EINVAL;
	}
	if (req->capacity < ch->loop_len) {
		return -ENOMEM;
	}

	memcpy(req->symbols, ch->loop, ch->loop_len * sizeof(struct pulse_symbol));
	*count = ch->loop_len;
	return 0;
}

static int lb_stop(const struct device *dev, struct pulse_io_channel *chan)
{
	struct pulse_io_lb_channel *ch = to_channel(dev, chan);

	if (ch == NULL) {
		return -EINVAL;
	}
	ch->loop_len = 0;
	return 0;
}

#ifdef CONFIG_RTIO
static const struct pulse_io_encoder_api lb_encoder = {
	.encode = pulse_io_encode_bytes,
};

static const struct pulse_io_decoder_api lb_decoder = {
	.decode = pulse_io_decode_bytes,
};

static int lb_get_encoder(const struct device *dev, const struct pulse_io_encoder_api **api)
{
	ARG_UNUSED(dev);
	*api = &lb_encoder;
	return 0;
}

static int lb_get_decoder(const struct device *dev, const struct pulse_io_decoder_api **api)
{
	ARG_UNUSED(dev);
	*api = &lb_decoder;
	return 0;
}

static void lb_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct pulse_io_lb_data *data = dev->data;
	/* The RTIO path uses a single channel for this test backend. */
	struct pulse_io_lb_channel *ch = &data->channels[0];
	struct rtio_sqe *sqe = &iodev_sqe->sqe;

	if (sqe->op == RTIO_OP_TX) {
		size_t nsyms = sqe->tx.buf_len / sizeof(struct pulse_symbol);

		if (nsyms > PULSE_IO_LB_MAX_SYMBOLS) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			return;
		}
		memcpy(ch->loop, sqe->tx.buf, nsyms * sizeof(struct pulse_symbol));
		ch->loop_len = nsyms;
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	} else if (sqe->op == RTIO_OP_RX) {
		uint8_t *buf = NULL;
		uint32_t buf_len = 0;
		size_t want = ch->loop_len * sizeof(struct pulse_symbol);
		int rc = rtio_sqe_rx_buf(iodev_sqe, want, want, &buf, &buf_len);

		if (rc != 0) {
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}
		memcpy(buf, ch->loop, want);
		rtio_iodev_sqe_ok(iodev_sqe, (int)want);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
#endif /* CONFIG_RTIO */

static DEVICE_API(pulse_io, pulse_io_lb_api) = {
	.get_capabilities = lb_get_capabilities,
	.channel_get = lb_channel_get,
	.channel_release = lb_channel_release,
	.channel_configure = lb_channel_configure,
	.transmit_sync = lb_transmit_sync,
	.receive_sync = lb_receive_sync,
	.stop = lb_stop,
#ifdef CONFIG_RTIO
	.submit = lb_submit,
	.get_encoder = lb_get_encoder,
	.get_decoder = lb_get_decoder,
#endif /* CONFIG_RTIO */
};

#define PULSE_IO_LB_INIT(idx)                                                                      \
	static struct pulse_io_lb_channel                                                          \
		pulse_io_lb_channels_##idx[DT_INST_PROP(idx, num_channels)];                       \
	static struct pulse_io_lb_data pulse_io_lb_data_##idx = {                                  \
		.channels = pulse_io_lb_channels_##idx,                                            \
		.num_channels = DT_INST_PROP(idx, num_channels),                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, NULL, NULL, &pulse_io_lb_data_##idx, NULL, POST_KERNEL,         \
			      CONFIG_PULSE_IO_INIT_PRIORITY, &pulse_io_lb_api);

DT_INST_FOREACH_STATUS_OKAY(PULSE_IO_LB_INIT)
