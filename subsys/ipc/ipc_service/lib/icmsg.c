/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ipc/icmsg.h>

#include <string.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/spsc_pbuf.h>

#define CB_BUF_SIZE	CONFIG_IPC_SERVICE_ICMSG_CB_BUF_SIZE
#define BOND_NOTIFY_REPEAT_TO K_MSEC(CONFIG_IPC_SERVICE_ICMSG_BOND_NOTIFY_REPEAT_TO_MS)

static const uint8_t magic[] = {0x45, 0x6d, 0x31, 0x6c, 0x31, 0x4b,
				0x30, 0x72, 0x6e, 0x33, 0x6c, 0x69, 0x34};
BUILD_ASSERT(sizeof(magic) <= CB_BUF_SIZE);
BUILD_ASSERT(CB_BUF_SIZE <= UINT16_MAX);

static int mbox_deinit(const struct icmsg_config_t *conf,
		       struct icmsg_data_t *dev_data)
{
	int err;

	err = mbox_set_enabled(&conf->mbox_rx, 0);
	if (err != 0) {
		return err;
	}

	err = mbox_register_callback(&conf->mbox_rx, NULL, NULL);
	if (err != 0) {
		return err;
	}

	(void)k_work_cancel(&dev_data->mbox_work);
	(void)k_work_cancel_delayable(&dev_data->notify_work);

	return 0;
}

static void notify_process(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct icmsg_data_t *dev_data =
		CONTAINER_OF(dwork, struct icmsg_data_t, notify_work);

	(void)mbox_send(&dev_data->cfg->mbox_tx, NULL);

	atomic_t state = atomic_get(&dev_data->state);

	if (state != ICMSG_STATE_READY) {
		int ret;

		ret = k_work_reschedule(dwork, BOND_NOTIFY_REPEAT_TO);
		__ASSERT_NO_MSG(ret >= 0);
		(void)ret;
	}
}

static void mbox_callback_process(struct k_work *item)
{
	struct icmsg_data_t *dev_data = CONTAINER_OF(item, struct icmsg_data_t, mbox_work);
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
		if (dev_data->cb->received) {
			dev_data->cb->received(cb_buffer, len, dev_data->ctx);
		}
	} else {
		int ret;

		__ASSERT_NO_MSG(state == ICMSG_STATE_BUSY);
		if (len != sizeof(magic) || memcmp(magic, cb_buffer, len)) {
			__ASSERT_NO_MSG(false);
			return;
		}

		if (dev_data->cb->bound) {
			dev_data->cb->bound(dev_data->ctx);
		}

		atomic_set(&dev_data->state, ICMSG_STATE_READY);
		ret = k_work_cancel_delayable(&dev_data->notify_work);
		__ASSERT_NO_MSG(ret >= 0);
		(void)ret;
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
	struct icmsg_data_t *dev_data = user_data;

	(void)k_work_submit(&dev_data->mbox_work);
}

static int mbox_init(const struct icmsg_config_t *conf,
		     struct icmsg_data_t *dev_data)
{
	int err;

	k_work_init(&dev_data->mbox_work, mbox_callback_process);
	k_work_init_delayable(&dev_data->notify_work, notify_process);

	err = mbox_register_callback(&conf->mbox_rx, mbox_callback, dev_data);
	if (err != 0) {
		return err;
	}

	return mbox_set_enabled(&conf->mbox_rx, 1);
}

int icmsg_init(const struct icmsg_config_t *conf,
	       struct icmsg_data_t *dev_data)
{
	__ASSERT_NO_MSG(conf->tx_shm_size > sizeof(struct spsc_pbuf));

	dev_data->tx_ib = spsc_pbuf_init((void *)conf->tx_shm_addr,
					 conf->tx_shm_size,
					 SPSC_PBUF_CACHE);
	dev_data->rx_ib = (void *)conf->rx_shm_addr;

	return 0;
}

int icmsg_open(const struct icmsg_config_t *conf,
	       struct icmsg_data_t *dev_data,
	       const struct ipc_service_cb *cb, void *ctx)
{
	int ret;

	if (!atomic_cas(&dev_data->state, ICMSG_STATE_OFF, ICMSG_STATE_BUSY)) {
		/* Already opened. */
		return -EALREADY;
	}

	dev_data->cb = cb;
	dev_data->ctx = ctx;
	dev_data->cfg = conf;

	ret = mbox_init(conf, dev_data);
	if (ret) {
		return ret;
	}

	ret = spsc_pbuf_write(dev_data->tx_ib, magic, sizeof(magic));
	if (ret < 0) {
		__ASSERT_NO_MSG(false);
		return ret;
	}

	if (ret < (int)sizeof(magic)) {
		__ASSERT_NO_MSG(ret == sizeof(magic));
		return ret;
	}

	ret = k_work_schedule(&dev_data->notify_work, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int icmsg_close(const struct icmsg_config_t *conf,
		struct icmsg_data_t *dev_data)
{
	int ret;

	ret = mbox_deinit(conf, dev_data);
	if (ret) {
		return ret;
	}

	atomic_set(&dev_data->state, ICMSG_STATE_OFF);

	return 0;
}

int icmsg_send(const struct icmsg_config_t *conf,
	       struct icmsg_data_t *dev_data,
	       const void *msg, size_t len)
{
	int ret;
	int sent_bytes;

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
	sent_bytes = ret;

	__ASSERT_NO_MSG(conf->mbox_tx.dev != NULL);

	ret = mbox_send(&conf->mbox_tx, NULL);
	if (ret) {
		return ret;
	}

	return sent_bytes;
}

int icmsg_clear_tx_memory(const struct icmsg_config_t *conf)
{
	/* Clear spsc_pbuf header and a part of the magic number. */
	memset((void *)conf->tx_shm_addr, 0, sizeof(struct spsc_pbuf) + sizeof(int));

	return 0;
}

int icmsg_clear_rx_memory(const struct icmsg_config_t *conf)
{
	/* Clear spsc_pbuf header and a part of the magic number. */
	memset((void *)conf->rx_shm_addr, 0, sizeof(struct spsc_pbuf) + sizeof(int));

	return 0;
}
