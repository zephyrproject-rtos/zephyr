/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_test_spi_link

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

/* Transactions run on the controller buses */
atomic_t spi_link_transfers;

/* The peripheral is unplugged: the controller clocks nothing and reads an idle bus */
static bool spi_link_unplugged;

struct spi_link_config {
	/* The peripheral bus, NULL on the peripheral bus itself */
	const struct device *peer;
};

struct spi_link_data {
	struct k_spinlock lock;
	/* The peripheral transfer, armed until a controller transfer runs it */
	bool armed;
	const struct spi_buf_set *tx;
	const struct spi_buf_set *rx;
	spi_callback_t cb;
	void *userdata;
};

static size_t spi_link_total(const struct spi_buf_set *set)
{
	size_t len = 0;

	for (size_t i = 0; set != NULL && i < set->count; i++) {
		len += set->buffers[i].len;
	}

	return len;
}

/* Byte at offset of a buffer set, 0 past its end or in a NULL buffer */
static uint8_t spi_link_get(const struct spi_buf_set *set, size_t offset)
{
	for (size_t i = 0; set != NULL && i < set->count; i++) {
		const struct spi_buf *buf = &set->buffers[i];

		if (offset < buf->len) {
			return buf->buf != NULL ? ((const uint8_t *)buf->buf)[offset] : 0U;
		}

		offset -= buf->len;
	}

	return 0U;
}

static void spi_link_put(const struct spi_buf_set *set, size_t offset, uint8_t val)
{
	for (size_t i = 0; set != NULL && i < set->count; i++) {
		const struct spi_buf *buf = &set->buffers[i];

		if (offset < buf->len) {
			if (buf->buf != NULL) {
				((uint8_t *)buf->buf)[offset] = val;
			}
			return;
		}

		offset -= buf->len;
	}
}

static int spi_link_transceive(const struct device *dev, const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	const struct spi_link_config *cfg = dev->config;
	struct spi_link_data *peer;
	struct spi_link_data armed;
	size_t len = MAX(spi_link_total(tx_bufs), spi_link_total(rx_bufs));
	k_spinlock_key_t key;

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_CONTROLLER || cfg->peer == NULL) {
		return -ENOTSUP;
	}

	peer = cfg->peer->data;
	atomic_inc(&spi_link_transfers);

	key = k_spin_lock(&peer->lock);
	armed = *peer;
	if (spi_link_unplugged) {
		armed.armed = false;
	} else {
		peer->armed = false;
	}
	k_spin_unlock(&peer->lock, key);

	/* An unarmed peripheral leaves its SDO idle high and receives nothing */
	for (size_t i = 0; i < len; i++) {
		spi_link_put(rx_bufs, i, armed.armed ? spi_link_get(armed.tx, i) : 0xffU);

		if (armed.armed) {
			spi_link_put(armed.rx, i, spi_link_get(tx_bufs, i));
		}
	}

	if (armed.armed) {
		armed.cb(cfg->peer, (int)MIN(len, spi_link_total(armed.rx)), armed.userdata);
		/* The peripheral processes the transfer on its own processor: let its thread run
		 * before the controller goes on
		 */
		k_yield();
	}

	return 0;
}

static int spi_link_transceive_async(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				     void *userdata)
{
	struct spi_link_data *data = dev->data;
	k_spinlock_key_t key;

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_PERIPHERAL) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if (data->armed) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->tx = tx_bufs;
	data->rx = rx_bufs;
	data->cb = cb;
	data->userdata = userdata;
	data->armed = true;

	k_spin_unlock(&data->lock, key);

	return 0;
}

/* On the peripheral bus, completes the armed transfer without moving a byte */
static void spi_link_complete(const struct device *dev, int result)
{
	struct spi_link_data *data = dev->data;
	struct spi_link_data armed;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	armed = *data;
	data->armed = false;
	k_spin_unlock(&data->lock, key);

	if (armed.armed) {
		armed.cb(dev, result, armed.userdata);
	}
}

/* A chip select pulse without clocks: the peripheral driver reports nothing received */
void spi_link_pulse(const struct device *dev)
{
	spi_link_complete(dev, 0);
}

/* Unplugs the peripheral from the controller, or plugs it back in */
void spi_link_unplug(bool unplugged)
{
	spi_link_unplugged = unplugged;
}

/* On the peripheral bus, cancels the armed transfer */
static int spi_link_release(const struct device *dev, const struct spi_config *config)
{
	ARG_UNUSED(config);

	spi_link_complete(dev, -ECANCELED);

	return 0;
}

static DEVICE_API(spi, spi_link_api) = {
	.transceive = spi_link_transceive,
	.transceive_async = spi_link_transceive_async,
	.release = spi_link_release,
};

/* The peripheral bus a controller bus is wired to */
#define SPI_LINK_PEER(inst)                                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, peer),                                             \
		    (DEVICE_DT_GET(DT_INST_PHANDLE(inst, peer))), (NULL))

#define SPI_LINK_DEFINE(inst)                                                                      \
	static const struct spi_link_config spi_link_config_##inst = {                             \
		.peer = SPI_LINK_PEER(inst),                                                       \
	};                                                                                         \
                                                                                                   \
	static struct spi_link_data spi_link_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &spi_link_data_##inst, &spi_link_config_##inst,    \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_link_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_LINK_DEFINE)
