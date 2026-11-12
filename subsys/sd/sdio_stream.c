/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Optional device-side SDIO streaming (packet/poll) extension. Adds per-function
 * RX/TX FIFOs and a poll() wait on top of the SDIO device subsystem, carrying
 * payloads in net_buf packets. The packet/poll data-path model is adapted from
 * the NXP sd_dev proposal (zephyr#111009), re-homed onto struct
 * sdio_device_function instead of a separate card-centered stack.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/net_buf.h>
#include <zephyr/drivers/sdio_dc.h>
#include <zephyr/sd/sdio_stream.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sdio_device, CONFIG_SDIO_DEVICE_LOG_LEVEL);

#define STREAM_COUNT CONFIG_SDIO_STREAM_PKT_COUNT
#define STREAM_BUF   CONFIG_SDIO_STREAM_BUF_SIZE

/* Per-buffer bookkeeping carried in the net_buf user data: a self-reference and
 * the single-descriptor set submitted to the controller. The set lets the
 * owning buffer be recovered O(1) from a completion.
 */
struct sdio_stream_ud {
	struct net_buf *buf;
	struct sdio_dc_buf iov;
	struct sdio_dc_buf_set set;
};

NET_BUF_POOL_DEFINE(sdio_stream_pool, STREAM_COUNT, STREAM_BUF,
		    sizeof(struct sdio_stream_ud), NULL);

/* Build the single-descriptor buffer set submitted to the controller. */
static struct sdio_dc_buf_set *sdio_buf_set(struct net_buf *buf, uint32_t len)
{
	struct sdio_stream_ud *ud = net_buf_user_data(buf);

	ud->buf = buf;
	ud->iov.data = buf->data;
	ud->iov.len = len;
	ud->set.bufs = &ud->iov;
	ud->set.count = 1;
	return &ud->set;
}

/* Recover the owning buffer from a completed set. */
static struct net_buf *sdio_buf_from_set(const struct sdio_dc_buf_set *bufs)
{
	return CONTAINER_OF(bufs, struct sdio_stream_ud, set)->buf;
}

struct net_buf *sdio_stream_alloc(k_timeout_t timeout)
{
	return net_buf_alloc(&sdio_stream_pool, timeout);
}

/* Push received payload into the RX FIFO (shared by both receive sources). */
static int sdio_stream_rx(struct sdio_stream_function *sf, const uint8_t *data,
			  uint32_t len)
{
	struct net_buf *buf = net_buf_alloc(&sdio_stream_pool, K_NO_WAIT);

	if (buf == NULL) {
		LOG_WRN("stream RX pool exhausted, dropping %u bytes", len);
		return -ENOMEM;
	}
	net_buf_add_mem(buf, data, MIN(len, (uint32_t)STREAM_BUF));
	k_fifo_put(&sf->rx_fifo, buf);
	return 0;
}

/*
 * Function FIFO handler: a host write is an inbound frame; a host read drains
 * a queued outbound frame.
 */
static int sdio_stream_fifo_cb(struct sdio_device_function *func,
			       enum sdio_io_dir dir, uint8_t *data, uint32_t len,
			       void *user)
{
	struct sdio_stream_function *sf = user;
	struct net_buf *buf;
	uint32_t n;

	ARG_UNUSED(func);

	if (dir == SDIO_IO_WRITE) {
		return sdio_stream_rx(sf, data, len);
	}

	/* Host read: serve one queued TX packet, zero-pad the remainder. */
	buf = k_fifo_get(&sf->tx_fifo, K_NO_WAIT);
	if (buf != NULL) {
		n = MIN(len, (uint32_t)buf->len);
		memcpy(data, buf->data, n);
		if (n < len) {
			memset(data + n, 0, len - n);
		}
		net_buf_unref(buf);
	} else {
		memset(data, 0, len);
	}
	return 0;
}

/* Zero-copy RX completion: a posted buffer set is returned by the controller. */
static void sdio_stream_rx_done(struct sdio_device_function *func,
				const struct sdio_dc_buf_set *bufs,
				uint32_t len, int status)
{
	struct sdio_stream_function *sf = func->user;
	struct net_buf *buf = sdio_buf_from_set(bufs);
	struct net_buf *next;

	if (status == 0) {
		net_buf_add(buf, MIN(len, (uint32_t)STREAM_BUF));
		k_fifo_put(&sf->rx_fifo, buf);
	} else {
		LOG_WRN("stream RX failed (%d), dropping buffer", status);
		net_buf_unref(buf);
	}

	/* Replenish so another frame can be received. */
	next = net_buf_alloc(&sdio_stream_pool, K_NO_WAIT);
	if (next == NULL) {
		LOG_WRN("stream RX pool exhausted, not replenishing");
		return;
	}
	if (sdio_device_rx_post(&sf->base, sdio_buf_set(next, STREAM_BUF)) != 0) {
		net_buf_unref(next);
	}
}

/* Zero-copy TX completion: a submitted buffer set is returned by the controller. */
static void sdio_stream_tx_done(struct sdio_device_function *func,
				const struct sdio_dc_buf_set *bufs, int status)
{
	ARG_UNUSED(func);

	if (status != 0) {
		LOG_WRN("stream TX failed (%d)", status);
	}
	net_buf_unref(sdio_buf_from_set(bufs));
}

int sdio_stream_function_init(struct sdio_stream_function *sf,
			      enum sdio_func_num num, uint32_t fifo_reg)
{
	if (sf == NULL) {
		return -EINVAL;
	}
	memset(&sf->base, 0, sizeof(sf->base));
	sf->base.num = num;
	sf->base.fifo_reg = fifo_reg;
	sf->base.fifo_cb = sdio_stream_fifo_cb;
	sf->base.rx_done = sdio_stream_rx_done;
	sf->base.tx_done = sdio_stream_tx_done;
	sf->base.user = sf;
	sf->zero_copy = false;
	k_fifo_init(&sf->rx_fifo);
	k_fifo_init(&sf->tx_fifo);
	return 0;
}

int sdio_stream_function_start(struct sdio_stream_function *sf)
{
	int posted = 0;

	if (sf == NULL || sf->base.parent == NULL) {
		return -EINVAL;
	}
	sf->zero_copy = sdio_device_is_zero_copy(sf->base.parent);
	if (!sf->zero_copy) {
		/* Synchronous FIFO-handler path; nothing to post. */
		return 0;
	}
	/* Post receive buffers until the controller queue is full. */
	for (int i = 0; i < STREAM_COUNT; i++) {
		struct net_buf *buf = net_buf_alloc(&sdio_stream_pool, K_NO_WAIT);

		if (buf == NULL) {
			break;
		}
		if (sdio_device_rx_post(&sf->base, sdio_buf_set(buf, STREAM_BUF)) != 0) {
			net_buf_unref(buf);
			break;
		}
		posted++;
	}
	return posted ? 0 : -ENOMEM;
}

struct net_buf *sdio_stream_read_pkt(struct sdio_stream_function *sf,
				     k_timeout_t timeout)
{
	return k_fifo_get(&sf->rx_fifo, timeout);
}

int sdio_stream_read(struct sdio_stream_function *sf, uint8_t *data,
		     uint16_t maxlen, k_timeout_t timeout)
{
	struct net_buf *buf = k_fifo_get(&sf->rx_fifo, timeout);
	uint16_t n;

	if (buf == NULL) {
		return -EAGAIN;
	}
	n = MIN(maxlen, buf->len);
	memcpy(data, buf->data, n);
	net_buf_unref(buf);
	return n;
}

/* Send an owned packet: submit to the controller (zero-copy) or queue for the
 * synchronous FIFO handler, then nudge the host.
 */
static int sdio_stream_tx(struct sdio_stream_function *sf, struct net_buf *buf)
{
	if (sf->zero_copy) {
		int ret = sdio_device_tx_submit(&sf->base,
						sdio_buf_set(buf, buf->len));

		if (ret) {
			net_buf_unref(buf);
			return ret;
		}
	} else {
		k_fifo_put(&sf->tx_fifo, buf);
	}
	/* Nudge the host to come and read (ignored if unsupported). */
	(void)sdio_device_raise_interrupt(&sf->base);
	return 0;
}

int sdio_stream_write_pkt(struct sdio_stream_function *sf, struct net_buf *buf)
{
	if (sf == NULL || buf == NULL) {
		return -EINVAL;
	}
	return sdio_stream_tx(sf, buf);
}

int sdio_stream_write(struct sdio_stream_function *sf, const uint8_t *data,
		      uint16_t len)
{
	struct net_buf *buf;

	if (sf == NULL || (data == NULL && len)) {
		return -EINVAL;
	}
	buf = net_buf_alloc(&sdio_stream_pool, K_NO_WAIT);
	if (buf == NULL) {
		return -ENOMEM;
	}
	net_buf_add_mem(buf, data, MIN(len, (uint16_t)STREAM_BUF));
	return sdio_stream_tx(sf, buf);
}

int sdio_stream_poll(struct sdio_stream_function *sf, uint32_t events,
		     uint32_t *revents, k_timeout_t timeout)
{
	struct k_poll_event evs[1];
	int n = 0;
	int ret;

	if (revents == NULL) {
		return -EINVAL;
	}
	*revents = 0;

	/* TX space is treated as always available. */
	if (events & SDIO_STREAM_POLLOUT) {
		*revents |= SDIO_STREAM_POLLOUT;
	}
	if (events & SDIO_STREAM_POLLIN) {
		if (!k_fifo_is_empty(&sf->rx_fifo)) {
			*revents |= SDIO_STREAM_POLLIN;
			return 0;
		}
		evs[n++] = (struct k_poll_event)K_POLL_EVENT_INITIALIZER(
			K_POLL_TYPE_FIFO_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
			&sf->rx_fifo);
	}
	if (n == 0) {
		return 0;
	}

	ret = k_poll(evs, n, timeout);
	if (ret == -EAGAIN) {
		return (*revents) ? 0 : -EAGAIN;
	}
	if (ret < 0) {
		return ret;
	}
	if (evs[0].state == K_POLL_STATE_FIFO_DATA_AVAILABLE) {
		*revents |= SDIO_STREAM_POLLIN;
	}
	return 0;
}

int sdio_stream_rx_submit(struct sdio_stream_function *sf, const uint8_t *data,
			  uint16_t len)
{
	if (sf == NULL) {
		return -EINVAL;
	}
	return sdio_stream_rx(sf, data, len);
}
