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

#define RX_BUF_SIZE	CONFIG_IPC_SERVICE_ICMSG_CB_BUF_SIZE
#define BOND_NOTIFY_REPEAT_TO K_MSEC(CONFIG_IPC_SERVICE_ICMSG_BOND_NOTIFY_REPEAT_TO_MS)

#define RX_BUFFER_RELEASED	0
#define RX_BUFFER_HELD		1
#define SEND_BUFFER_UNUSED	0
#define SEND_BUFFER_RESERVED	1

static const uint8_t magic[] = {0x45, 0x6d, 0x31, 0x6c, 0x31, 0x4b,
				0x30, 0x72, 0x6e, 0x33, 0x6c, 0x69, 0x34};
BUILD_ASSERT(sizeof(magic) <= RX_BUF_SIZE);
BUILD_ASSERT(RX_BUF_SIZE <= UINT16_MAX);

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

static bool is_endpoint_ready(struct icmsg_data_t *dev_data)
{
	return atomic_get(&dev_data->state) == ICMSG_STATE_READY;
}

static bool is_send_buffer_reserved(struct icmsg_data_t *dev_data)
{
	return atomic_get(&dev_data->send_buffer_reserved) ==
			SEND_BUFFER_RESERVED;
}

static int reserve_send_buffer_if_unused(struct icmsg_data_t *dev_data)
{
	bool was_unused;

	was_unused = atomic_cas(&dev_data->send_buffer_reserved,
				  SEND_BUFFER_UNUSED, SEND_BUFFER_RESERVED);

	return was_unused ? 0 : -EALREADY;
}

static int release_send_buffer(struct icmsg_data_t *dev_data)
{
	bool was_reserved;

	was_reserved = atomic_cas(&dev_data->send_buffer_reserved,
				  SEND_BUFFER_RESERVED, SEND_BUFFER_UNUSED);
	return was_reserved ? 0 : -EALREADY;
}

static bool is_rx_buffer_free(struct icmsg_data_t *dev_data)
{
#ifdef CONFIG_IPC_SERVICE_ICMSG_NOCOPY_RX
	return atomic_get(&dev_data->rx_buffer_held) == RX_BUFFER_RELEASED;
#else
	return true;
#endif
}

static bool is_rx_data_available(struct icmsg_data_t *dev_data)
{
	int len = spsc_pbuf_read(dev_data->rx_ib, NULL, 0);

	return len > 0;
}

static void submit_mbox_work(struct icmsg_data_t *dev_data)
{
	if (k_work_submit(&dev_data->mbox_work) < 0) {
		/* The mbox processing work is never canceled.
		 * The negative error code should never be seen.
		 */
		__ASSERT_NO_MSG(false);
	}
}

static void submit_work_if_buffer_free(struct icmsg_data_t *dev_data)
{
	if (!is_rx_buffer_free(dev_data)) {
		return;
	}

	submit_mbox_work(dev_data);
}

static void submit_work_if_buffer_free_and_data_available(
		struct icmsg_data_t *dev_data)
{
	if (!is_rx_buffer_free(dev_data)) {
		return;
	}
	if (!is_rx_data_available(dev_data)) {
		return;
	}

	submit_mbox_work(dev_data);
}

static void mbox_callback_process(struct k_work *item)
{
	struct icmsg_data_t *dev_data = CONTAINER_OF(item, struct icmsg_data_t, mbox_work);

	atomic_t state = atomic_get(&dev_data->state);
	int len = spsc_pbuf_read(dev_data->rx_ib, dev_data->rx_buffer,
				 RX_BUF_SIZE);

	__ASSERT_NO_MSG(len <= RX_BUF_SIZE);

	if (len == -EAGAIN) {
		__ASSERT_NO_MSG(false);
		submit_mbox_work(dev_data);
		return;
	} else if (len <= 0) {
		return;
	}

	if (state == ICMSG_STATE_READY) {
		if (dev_data->cb->received) {
			dev_data->cb->received(dev_data->rx_buffer, len,
					       dev_data->ctx);
		}
	} else {
		int ret;

		__ASSERT_NO_MSG(state == ICMSG_STATE_BUSY);
		if (len != sizeof(magic) ||
		    memcmp(magic, dev_data->rx_buffer, len)) {
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

	submit_work_if_buffer_free_and_data_available(dev_data);
}

static void mbox_callback(const struct device *instance, uint32_t channel,
			  void *user_data, struct mbox_msg *msg_data)
{
	struct icmsg_data_t *dev_data = user_data;

	submit_work_if_buffer_free(dev_data);
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
	int write_ret;
	int release_ret;
	int sent_bytes;

	if (!is_endpoint_ready(dev_data)) {
		return -EBUSY;
	}

	/* Empty message is not allowed */
	if (len == 0) {
		return -ENODATA;
	}

	ret = reserve_send_buffer_if_unused(dev_data);
	if (ret) {
		return -ENOBUFS;
	}

	write_ret = spsc_pbuf_write(dev_data->tx_ib, msg, len);
	release_ret = release_send_buffer(dev_data);
	__ASSERT_NO_MSG(!release_ret);

	if (write_ret < 0) {
		return write_ret;
	} else if (write_ret < len) {
		return -EBADMSG;
	}
	sent_bytes = write_ret;

	__ASSERT_NO_MSG(conf->mbox_tx.dev != NULL);

	ret = mbox_send(&conf->mbox_tx, NULL);
	if (ret) {
		return ret;
	}

	return sent_bytes;
}

int icmsg_get_tx_buffer(const struct icmsg_config_t *conf,
			struct icmsg_data_t *dev_data,
			void **data, size_t *size)
{
	int ret;
	int release_ret;
	uint16_t requested_size;
	int allocated_len;
	char *allocated_buf;

	if (*size == 0) {
		/* Requested allocation of maximal size.
		 * Try to allocate maximal buffer size from spsc,
		 * potentially after wrapping marker.
		 */
		requested_size = SPSC_PBUF_MAX_LEN - 1;
	} else {
		requested_size = *size;
	}

	ret = reserve_send_buffer_if_unused(dev_data);
	if (ret) {
		return -ENOBUFS;
	}

	ret = spsc_pbuf_alloc(dev_data->tx_ib, requested_size, &allocated_buf);
	if (ret < 0) {
		release_ret = release_send_buffer(dev_data);
		__ASSERT_NO_MSG(!release_ret);
		return ret;
	}
	allocated_len = ret;

	if (*size == 0) {
		/* Requested allocation of maximal size.
		 * Pass the buffer that was allocated.
		 */
		*size = allocated_len;
		*data = allocated_buf;
		return 0;
	}

	if (*size == allocated_len) {
		/* Allocated buffer is of requested size. */
		*data = allocated_buf;
		return 0;
	}

	/* Allocated smaller buffer than requested.
	 * Silently stop using the allocated buffer what is allowed by SPSC API
	 */
	release_send_buffer(dev_data);
	*size = allocated_len;
	return -ENOMEM;
}

int icmsg_drop_tx_buffer(const struct icmsg_config_t *conf,
			 struct icmsg_data_t *dev_data,
			 const void *data)
{
	/* Silently stop using the allocated buffer what is allowed by SPSC API
	 */
	return release_send_buffer(dev_data);
}

int icmsg_send_nocopy(const struct icmsg_config_t *conf,
		      struct icmsg_data_t *dev_data,
		      const void *msg, size_t len)
{
	int ret;
	int sent_bytes;

	if (!is_endpoint_ready(dev_data)) {
		return -EBUSY;
	}

	/* Empty message is not allowed */
	if (len == 0) {
		return -ENODATA;
	}

	if (!is_send_buffer_reserved(dev_data)) {
		return -ENXIO;
	}

	spsc_pbuf_commit(dev_data->tx_ib, len);
	sent_bytes = len;

	ret = release_send_buffer(dev_data);
	__ASSERT_NO_MSG(!ret);

	__ASSERT_NO_MSG(conf->mbox_tx.dev != NULL);

	ret = mbox_send(&conf->mbox_tx, NULL);
	if (ret) {
		return ret;
	}

	return sent_bytes;
}

#ifdef CONFIG_IPC_SERVICE_ICMSG_NOCOPY_RX
int icmsg_hold_rx_buffer(const struct icmsg_config_t *conf,
			 struct icmsg_data_t *dev_data,
			 const void *data)
{
	bool was_released;

	if (!is_endpoint_ready(dev_data)) {
		return -EBUSY;
	}

	if (data != dev_data->rx_buffer) {
		return -EINVAL;
	}

	was_released = atomic_cas(&dev_data->rx_buffer_held,
				  RX_BUFFER_RELEASED, RX_BUFFER_HELD);
	if (!was_released) {
		return -EALREADY;
	}

	return 0;
}

int icmsg_release_rx_buffer(const struct icmsg_config_t *conf,
			    struct icmsg_data_t *dev_data,
			    const void *data)
{
	bool was_held;

	if (!is_endpoint_ready(dev_data)) {
		return -EBUSY;
	}

	if (data != dev_data->rx_buffer) {
		return -EINVAL;
	}

	was_held = atomic_cas(&dev_data->rx_buffer_held,
			      RX_BUFFER_HELD, RX_BUFFER_RELEASED);
	if (!was_held) {
		return -EALREADY;
	}

	submit_work_if_buffer_free_and_data_available(dev_data);

	return 0;
}
#endif /* CONFIG_IPC_SERVICE_ICMSG_NOCOPY_RX */

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
