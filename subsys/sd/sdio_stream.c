/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Optional device-side SDIO streaming (packet/poll) extension. Adds a pooled,
 * reference-counted packet layer with RX/TX FIFOs and a poll() wait on top of
 * the SDIO device subsystem. The packet-pool and poll data-path
 * model is adapted from the NXP sd_dev proposal (zephyr#111009), re-homed onto
 * struct sdio_device_function instead of a separate card-centered stack.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sd/sdio_stream.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sdio_device, CONFIG_SDIO_DEVICE_LOG_LEVEL);

#define STREAM_ALIGN CONFIG_SDIO_STREAM_DMA_ALIGN
#define STREAM_COUNT CONFIG_SDIO_STREAM_PKT_COUNT
#define STREAM_BUF   CONFIG_SDIO_STREAM_BUF_SIZE

K_MEM_SLAB_DEFINE(sdio_stream_pkt_pool, ROUND_UP(sizeof(struct sdio_pkt), STREAM_ALIGN),
		  STREAM_COUNT, STREAM_ALIGN);
K_MEM_SLAB_DEFINE(sdio_stream_data_pool, STREAM_BUF, STREAM_COUNT, STREAM_ALIGN);

struct sdio_pkt *sdio_pkt_alloc(enum sdio_pkt_dir dir)
{
	struct sdio_pkt *pkt = NULL;
	uint8_t *buf = NULL;

	if (dir != SDIO_PKT_RX && dir != SDIO_PKT_TX) {
		return NULL;
	}
	if (k_mem_slab_alloc(&sdio_stream_pkt_pool, (void **)&pkt, K_NO_WAIT) != 0) {
		return NULL;
	}
	if (k_mem_slab_alloc(&sdio_stream_data_pool, (void **)&buf, K_NO_WAIT) != 0) {
		k_mem_slab_free(&sdio_stream_pkt_pool, pkt);
		return NULL;
	}
	pkt->fifo_reserved = NULL;
	pkt->data = buf;
	pkt->len = 0;
	pkt->dir = dir;
	atomic_set(&pkt->ref, 1);
	return pkt;
}

struct sdio_pkt *sdio_pkt_ref(struct sdio_pkt *pkt)
{
	if (pkt) {
		atomic_inc(&pkt->ref);
	}
	return pkt;
}

void sdio_pkt_free(struct sdio_pkt *pkt)
{
	if (pkt == NULL) {
		return;
	}
	if (atomic_dec(&pkt->ref) == 1) {
		k_mem_slab_free(&sdio_stream_data_pool, pkt->data);
		k_mem_slab_free(&sdio_stream_pkt_pool, pkt);
	}
}

/* Push received payload into the RX FIFO (shared by both receive sources). */
static int sdio_stream_rx(struct sdio_stream_function *sf, const uint8_t *data,
			  uint32_t len)
{
	struct sdio_pkt *pkt = sdio_pkt_alloc(SDIO_PKT_RX);

	if (pkt == NULL) {
		LOG_WRN("stream RX pool exhausted, dropping %u bytes", len);
		return -ENOMEM;
	}
	pkt->len = MIN(len, (uint32_t)STREAM_BUF);
	memcpy(pkt->data, data, pkt->len);
	k_fifo_put(&sf->rx_fifo, pkt);
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
	struct sdio_pkt *pkt;
	uint32_t n;

	ARG_UNUSED(func);

	if (dir == SDIO_IO_WRITE) {
		return sdio_stream_rx(sf, data, len);
	}

	/* Host read: serve one queued TX packet, zero-pad the remainder. */
	pkt = k_fifo_get(&sf->tx_fifo, K_NO_WAIT);
	if (pkt != NULL) {
		n = MIN(len, (uint32_t)pkt->len);
		memcpy(data, pkt->data, n);
		if (n < len) {
			memset(data + n, 0, len - n);
		}
		sdio_pkt_free(pkt);
	} else {
		memset(data, 0, len);
	}
	return 0;
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
	sf->base.user = sf;
	k_fifo_init(&sf->rx_fifo);
	k_fifo_init(&sf->tx_fifo);
	return 0;
}

struct sdio_pkt *sdio_stream_read_pkt(struct sdio_stream_function *sf,
				      k_timeout_t timeout)
{
	return k_fifo_get(&sf->rx_fifo, timeout);
}

int sdio_stream_read(struct sdio_stream_function *sf, uint8_t *data,
		     uint16_t maxlen, k_timeout_t timeout)
{
	struct sdio_pkt *pkt = k_fifo_get(&sf->rx_fifo, timeout);
	uint16_t n;

	if (pkt == NULL) {
		return -EAGAIN;
	}
	n = MIN(maxlen, pkt->len);
	memcpy(data, pkt->data, n);
	sdio_pkt_free(pkt);
	return n;
}

int sdio_stream_write(struct sdio_stream_function *sf, const uint8_t *data,
		      uint16_t len)
{
	struct sdio_pkt *pkt;

	if (sf == NULL || (data == NULL && len)) {
		return -EINVAL;
	}
	pkt = sdio_pkt_alloc(SDIO_PKT_TX);
	if (pkt == NULL) {
		return -ENOMEM;
	}
	pkt->len = MIN(len, (uint16_t)STREAM_BUF);
	memcpy(pkt->data, data, pkt->len);
	k_fifo_put(&sf->tx_fifo, pkt);
	/* Nudge the host to come and read (ignored if unsupported). */
	(void)sdio_device_raise_interrupt(&sf->base);
	return 0;
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
