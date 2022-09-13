/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/mbox.h>

#include <zephyr/ipc/ipc_service_backend.h>
#include "ipc_icmsg.h"

#define DT_DRV_COMPAT	zephyr_ipc_icmsg
#define CB_BUF_SIZE	CONFIG_IPC_SERVICE_BACKEND_ICMSG_CB_BUF_SIZE

static const uint8_t magic[] = {0x45, 0x6d, 0x31, 0x6c, 0x31, 0x4b,
				0x30, 0x72, 0x6e, 0x33, 0x6c, 0x69, 0x34};
BUILD_ASSERT(sizeof(magic) <= CB_BUF_SIZE);
BUILD_ASSERT(CB_BUF_SIZE <= UINT16_MAX);

struct backend_data_t {
	/* Tx/Rx buffers. */
	struct spsc_pbuf *tx_ib;
	struct spsc_pbuf *rx_ib;

	/* Backend ops for an endpoint. */
	const struct ipc_ept_cfg *cfg;

	/* General */
	struct k_work mbox_work;
	atomic_t state;
};

struct backend_config_t {
	uintptr_t tx_shm_addr;
	uintptr_t rx_shm_addr;
	size_t tx_shm_size;
	size_t rx_shm_size;
	struct mbox_channel mbox_tx;
	struct mbox_channel mbox_rx;
};


static void mbox_callback_process(struct k_work *item)
{
	struct backend_data_t *dev_data = CONTAINER_OF(item, struct backend_data_t, mbox_work);
	uint8_t cb_buffer[CB_BUF_SIZE] __aligned(4);

	atomic_t state = atomic_get(&dev_data->state);
	int len = spsc_pbuf_read(dev_data->rx_ib, cb_buffer, CB_BUF_SIZE);

	__ASSERT_NO_MSG(len <= CB_BUF_SIZE);

	if (len == -EAGAIN) {
		__ASSERT_NO_MSG(false);
		(void)k_work_submit(&dev_data->mbox_work);
		return;
	} else if (len <= 0) {
		return;
	}

	if (state == ICMSG_STATE_READY) {
		if (dev_data->cfg->cb.received) {
			dev_data->cfg->cb.received(cb_buffer, len, dev_data->cfg->priv);
		}
	} else {
		__ASSERT_NO_MSG(state == ICMSG_STATE_BUSY);
		if (len != sizeof(magic) || memcmp(magic, cb_buffer, len)) {
			__ASSERT_NO_MSG(false);
			return;
		}

		if (dev_data->cfg->cb.bound) {
			dev_data->cfg->cb.bound(dev_data->cfg->priv);
		}

		atomic_set(&dev_data->state, ICMSG_STATE_READY);
	}

	/* Reading with NULL buffer to know if there are data in the
	 * buffer to be read.
	 */
	len = spsc_pbuf_read(dev_data->rx_ib, NULL, 0);
	if (len > 0) {
		if (k_work_submit(&dev_data->mbox_work) < 0) {
			/* The mbox processing work is never canceled.
			 * The negative error code should never be seen.
			 */
			__ASSERT_NO_MSG(false);
		}
	}
}

static void mbox_callback(const struct device *instance, uint32_t channel,
			  void *user_data, struct mbox_msg *msg_data)
{
	struct backend_data_t *dev_data = user_data;

	(void)k_work_submit(&dev_data->mbox_work);
}

static int mbox_init(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;
	int err;

	k_work_init(&dev_data->mbox_work, mbox_callback_process);

	err = mbox_register_callback(&conf->mbox_rx, mbox_callback, dev_data);
	if (err != 0) {
		return err;
	}

	return mbox_set_enabled(&conf->mbox_rx, 1);
}

static int register_ept(const struct device *instance, void **token,
			const struct ipc_ept_cfg *cfg)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;
	struct ipc_ept *ep = (struct ipc_ept *)token;
	int ret;

	if (!atomic_cas(&dev_data->state, ICMSG_STATE_OFF, ICMSG_STATE_BUSY)) {
		/* Already registered. This backend supports single ep. */
		return -EALREADY;
	}

	ep->instance = instance;
	dev_data->cfg = cfg;

	ret = mbox_init(instance);
	if (ret) {
		return ret;
	}

	ret = spsc_pbuf_write(dev_data->tx_ib, magic, sizeof(magic));
	if (ret < sizeof(magic)) {
		__ASSERT_NO_MSG(ret == sizeof(magic));
		return ret;
	}

	ret = mbox_send(&conf->mbox_tx, NULL);
	if (ret) {
		return ret;
	}

	ret = spsc_pbuf_read(dev_data->rx_ib, NULL, 0);
	if (ret > 0) {
		(void)k_work_submit(&dev_data->mbox_work);
	}

	return 0;
}

static int send(const struct device *instance, void *token,
		const void *msg, size_t len)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;
	int ret;

	if (atomic_get(&dev_data->state) != ICMSG_STATE_READY) {
		return -EBUSY;
	}

	/* Empty message is not allowed */
	if (len == 0) {
		return -ENODATA;
	}

	ret = spsc_pbuf_write(dev_data->tx_ib, msg, len);
	if (ret < 0) {
		return ret;
	} else if (ret < len) {
		return -EBADMSG;
	}

	__ASSERT_NO_MSG(conf->mbox_tx.dev != NULL);

	return mbox_send(&conf->mbox_tx, NULL);
}

const static struct ipc_service_backend backend_ops = {
	.register_endpoint = register_ept,
	.send = send,
};

static int backend_init(const struct device *instance)
{
	const struct backend_config_t *conf = instance->config;
	struct backend_data_t *dev_data = instance->data;

	__ASSERT_NO_MSG(conf->tx_shm_size > sizeof(struct spsc_pbuf));

	dev_data->tx_ib = spsc_pbuf_init((void *)conf->tx_shm_addr,
					 conf->tx_shm_size,
					 SPSC_PBUF_CACHE);
	dev_data->rx_ib = (void *)conf->rx_shm_addr;

	return 0;
}

#define DEFINE_BACKEND_DEVICE(i)						\
	static const struct backend_config_t backend_config_##i = {		\
		.tx_shm_size = DT_REG_SIZE(DT_INST_PHANDLE(i, tx_region)),	\
		.tx_shm_addr = DT_REG_ADDR(DT_INST_PHANDLE(i, tx_region)),	\
		.rx_shm_size = DT_REG_SIZE(DT_INST_PHANDLE(i, rx_region)),	\
		.rx_shm_addr = DT_REG_ADDR(DT_INST_PHANDLE(i, rx_region)),	\
		.mbox_tx = MBOX_DT_CHANNEL_GET(DT_DRV_INST(i), tx),		\
		.mbox_rx = MBOX_DT_CHANNEL_GET(DT_DRV_INST(i), rx),		\
	};									\
										\
	static struct backend_data_t backend_data_##i;				\
										\
	DEVICE_DT_INST_DEFINE(i,						\
			 &backend_init,						\
			 NULL,							\
			 &backend_data_##i,					\
			 &backend_config_##i,					\
			 POST_KERNEL,						\
			 CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY,		\
			 &backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_DEVICE)
