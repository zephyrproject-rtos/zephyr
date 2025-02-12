/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ipc/icmsg.h>

#include <string.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ipc/pbuf.h>
#include <zephyr/init.h>

#define BOND_NOTIFY_REPEAT_TO	K_MSEC(CONFIG_IPC_SERVICE_ICMSG_BOND_NOTIFY_REPEAT_TO_MS)
#define SHMEM_ACCESS_TO		K_MSEC(CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_TO_MS)


static const uint8_t magic[] = {0x45, 0x6d, 0x31, 0x6c, 0x31, 0x4b,
				0x30, 0x72, 0x6e, 0x33, 0x6c, 0x69, 0x34};

#if defined(CONFIG_IPC_SERVICE_BACKEND_ICMSG_WQ_ENABLE)
static K_THREAD_STACK_DEFINE(icmsg_stack, CONFIG_IPC_SERVICE_BACKEND_ICMSG_WQ_STACK_SIZE);
static struct k_work_q icmsg_workq;
static struct k_work_q *const workq = &icmsg_workq;
#else
static struct k_work_q *const workq = &k_sys_work_q;
#endif

static int mbox_deinit(const struct icmsg_config_t *conf,
		       struct icmsg_data_t *dev_data)
{
	int err;

	err = mbox_set_enabled_dt(&conf->mbox_rx, 0);
	if (err != 0) {
		return err;
	}

	err = mbox_register_callback_dt(&conf->mbox_rx, NULL, NULL);
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

	(void)mbox_send_dt(&dev_data->cfg->mbox_tx, NULL);

	atomic_t state = atomic_get(&dev_data->state);

	if (state != ICMSG_STATE_READY) {
		int ret;

		ret = k_work_reschedule_for_queue(workq, dwork, BOND_NOTIFY_REPEAT_TO);
		__ASSERT_NO_MSG(ret >= 0);
		(void)ret;
	}
}

static bool is_endpoint_ready(struct icmsg_data_t *dev_data)
{
	return atomic_get(&dev_data->state) == ICMSG_STATE_READY;
}

static int reserve_tx_buffer_if_unused(struct icmsg_data_t *dev_data)
{
#ifdef CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_SYNC
	int ret = k_mutex_lock(&dev_data->tx_lock, SHMEM_ACCESS_TO);

	if (ret < 0) {
		return ret;
	}
#endif
	return 0;
}

static int release_tx_buffer(struct icmsg_data_t *dev_data)
{
#ifdef CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_SYNC
	return k_mutex_unlock(&dev_data->tx_lock);
#endif
	return 0;
}

static uint32_t data_available(struct icmsg_data_t *dev_data)
{
	return pbuf_read(dev_data->rx_pb, NULL, 0);
}

static void submit_mbox_work(struct icmsg_data_t *dev_data)
{
	if (k_work_submit_to_queue(workq, &dev_data->mbox_work) < 0) {
		/* The mbox processing work is never canceled.
		 * The negative error code should never be seen.
		 */
		__ASSERT_NO_MSG(false);
	}
}

static void submit_work_if_buffer_free(struct icmsg_data_t *dev_data)
{
	submit_mbox_work(dev_data);
}

static void submit_work_if_buffer_free_and_data_available(
		struct icmsg_data_t *dev_data)
{
	if (!data_available(dev_data)) {
		return;
	}

	submit_mbox_work(dev_data);
}

static void mbox_callback_process(struct k_work *item)
{
	struct icmsg_data_t *dev_data = CONTAINER_OF(item, struct icmsg_data_t, mbox_work);

	atomic_t state = atomic_get(&dev_data->state);

	uint32_t len = data_available(dev_data);

	if (len == 0) {
		/* Unlikely, no data in buffer. */
		return;
	}

	uint8_t rx_buffer[len];

	len = pbuf_read(dev_data->rx_pb, rx_buffer, len);

	if (state == ICMSG_STATE_READY) {
		if (dev_data->cb->received) {
			dev_data->cb->received(rx_buffer, len,
					       dev_data->ctx);
		}
	} else {
		__ASSERT_NO_MSG(state == ICMSG_STATE_BUSY);

		/* Allow magic number longer than sizeof(magic) for future protocol version. */
		bool endpoint_invalid = (len < sizeof(magic) ||
					memcmp(magic, rx_buffer, sizeof(magic)));

		if (endpoint_invalid) {
			__ASSERT_NO_MSG(false);
			return;
		}

		if (dev_data->cb->bound) {
			dev_data->cb->bound(dev_data->ctx);
		}

		atomic_set(&dev_data->state, ICMSG_STATE_READY);
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

	err = mbox_register_callback_dt(&conf->mbox_rx, mbox_callback, dev_data);
	if (err != 0) {
		return err;
	}

	return mbox_set_enabled_dt(&conf->mbox_rx, 1);
}

int icmsg_open(const struct icmsg_config_t *conf,
	       struct icmsg_data_t *dev_data,
	       const struct ipc_service_cb *cb, void *ctx)
{
	if (!atomic_cas(&dev_data->state, ICMSG_STATE_OFF, ICMSG_STATE_BUSY)) {
		/* Already opened. */
		return -EALREADY;
	}

	dev_data->cb = cb;
	dev_data->ctx = ctx;
	dev_data->cfg = conf;

#ifdef CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_SYNC
	k_mutex_init(&dev_data->tx_lock);
#endif

	int ret = pbuf_init(dev_data->tx_pb);

	if (ret < 0) {
		__ASSERT(false, "Incorrect configuration");
		return ret;
	}

	/* Initialize local copies of rx_pb. */
	dev_data->rx_pb->data.wr_idx = 0;
	dev_data->rx_pb->data.rd_idx = 0;

	ret = pbuf_write(dev_data->tx_pb, magic, sizeof(magic));

	if (ret < 0) {
		__ASSERT_NO_MSG(false);
		return ret;
	}

	if (ret < (int)sizeof(magic)) {
		__ASSERT_NO_MSG(ret == sizeof(magic));
		return ret;
	}

	ret = mbox_init(conf, dev_data);
	if (ret) {
		return ret;
	}

	ret = k_work_schedule_for_queue(workq, &dev_data->notify_work, K_NO_WAIT);
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

	ret = reserve_tx_buffer_if_unused(dev_data);
	if (ret < 0) {
		return -ENOBUFS;
	}

	write_ret = pbuf_write(dev_data->tx_pb, msg, len);
	release_ret = release_tx_buffer(dev_data);
	__ASSERT_NO_MSG(!release_ret);

	if (write_ret < 0) {
		return write_ret;
	} else if (write_ret < len) {
		return -EBADMSG;
	}
	sent_bytes = write_ret;

	__ASSERT_NO_MSG(conf->mbox_tx.dev != NULL);

	ret = mbox_send_dt(&conf->mbox_tx, NULL);
	if (ret) {
		return ret;
	}

	return sent_bytes;
}

#if defined(CONFIG_IPC_SERVICE_BACKEND_ICMSG_WQ_ENABLE)

static int work_q_init(void)
{
	struct k_work_queue_config cfg = {
		.name = "icmsg_workq",
	};

	k_work_queue_start(&icmsg_workq,
			    icmsg_stack,
			    K_KERNEL_STACK_SIZEOF(icmsg_stack),
			    CONFIG_IPC_SERVICE_BACKEND_ICMSG_WQ_PRIORITY, &cfg);
	return 0;
}

SYS_INIT(work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
