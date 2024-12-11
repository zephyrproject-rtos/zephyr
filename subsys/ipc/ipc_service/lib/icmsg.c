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


#define UNBOUND_DISABLED IS_ENABLED(CONFIG_IPC_SERVICE_ICMSG_UNBOUND_DISABLED_ALLOWED)
#define UNBOUND_ENABLED IS_ENABLED(CONFIG_IPC_SERVICE_ICMSG_UNBOUND_ENABLED_ALLOWED)
#define UNBOUND_DETECT IS_ENABLED(CONFIG_IPC_SERVICE_ICMSG_UNBOUND_DETECT_ALLOWED)

/** Get local session id request from RX handshake word.
 */
#define LOCAL_SID_REQ_FROM_RX(rx_handshake) ((rx_handshake) & 0xFFFF)

/** Get remote session id request from TX handshake word.
 */
#define REMOTE_SID_REQ_FROM_TX(tx_handshake) ((tx_handshake) & 0xFFFF)

/** Get local session id acknowledge from TX handshake word.
 */
#define LOCAL_SID_ACK_FROM_TX(tx_handshake) ((tx_handshake) >> 16)

/** Create RX handshake word from local session id request
 * and remote session id acknowledge.
 */
#define MAKE_RX_HANDSHAKE(local_sid_req, remote_sid_ack) \
	((local_sid_req) | ((remote_sid_ack) << 16))

/** Create TX handshake word from remote session id request
 * and local session id acknowledge.
 */
#define MAKE_TX_HANDSHAKE(remote_sid_req, local_sid_ack) \
	((remote_sid_req) | ((local_sid_ack) << 16))

/** Special session id indicating that peers are disconnected.
 */
#define SID_DISCONNECTED 0

#define SHMEM_ACCESS_TO		K_MSEC(CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_TO_MS)

static const uint8_t magic[] = {0x45, 0x6d, 0x31, 0x6c, 0x31, 0x4b,
				0x30, 0x72, 0x6e, 0x33, 0x6c, 0x69, 0x34};

#ifdef CONFIG_MULTITHREADING
#if defined(CONFIG_IPC_SERVICE_BACKEND_ICMSG_WQ_ENABLE)
static K_THREAD_STACK_DEFINE(icmsg_stack, CONFIG_IPC_SERVICE_BACKEND_ICMSG_WQ_STACK_SIZE);
static struct k_work_q icmsg_workq;
static struct k_work_q *const workq = &icmsg_workq;
#else /* defined(CONFIG_IPC_SERVICE_BACKEND_ICMSG_WQ_ENABLE) */
static struct k_work_q *const workq = &k_sys_work_q;
#endif /* defined(CONFIG_IPC_SERVICE_BACKEND_ICMSG_WQ_ENABLE) */
#endif /* def CONFIG_MULTITHREADING */

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

#ifdef CONFIG_MULTITHREADING
	(void)k_work_cancel(&dev_data->mbox_work);
#endif

	return 0;
}

static bool is_endpoint_ready(atomic_t state)
{
	return state >= MIN(ICMSG_STATE_CONNECTED_SID_DISABLED, ICMSG_STATE_CONNECTED_SID_ENABLED);
}

static inline int reserve_tx_buffer_if_unused(struct icmsg_data_t *dev_data)
{
#ifdef CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_SYNC
	return k_mutex_lock(&dev_data->tx_lock, SHMEM_ACCESS_TO);
#else
	return 0;
#endif
}

static inline int release_tx_buffer(struct icmsg_data_t *dev_data)
{
#ifdef CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_SYNC
	return k_mutex_unlock(&dev_data->tx_lock);
#else
	return 0;
#endif
}

static uint32_t data_available(struct icmsg_data_t *dev_data)
{
	return pbuf_read(dev_data->rx_pb, NULL, 0);
}

#ifdef CONFIG_MULTITHREADING
static void submit_mbox_work(struct icmsg_data_t *dev_data)
{
	if (k_work_submit_to_queue(workq, &dev_data->mbox_work) < 0) {
		/* The mbox processing work is never canceled.
		 * The negative error code should never be seen.
		 */
		__ASSERT_NO_MSG(false);
	}
}

#endif

static int initialize_tx_with_sid_disabled(struct icmsg_data_t *dev_data)
{
	int ret;

	ret = pbuf_tx_init(dev_data->tx_pb);

	if (ret < 0) {
		__ASSERT(false, "Incorrect Tx configuration");
		return ret;
	}

	ret = pbuf_write(dev_data->tx_pb, magic, sizeof(magic));

	if (ret < 0) {
		__ASSERT_NO_MSG(false);
		return ret;
	}

	if (ret < (int)sizeof(magic)) {
		__ASSERT_NO_MSG(ret == sizeof(magic));
		return -EINVAL;
	}

	return 0;
}

static bool callback_process(struct icmsg_data_t *dev_data)
{
	int ret;
	uint8_t rx_buffer[CONFIG_PBUF_RX_READ_BUF_SIZE] __aligned(4);
	uint32_t len = 0;
	uint32_t len_available;
	bool rerun = false;
	bool notify_remote = false;
	atomic_t state = atomic_get(&dev_data->state);

	switch (state) {

#if UNBOUND_DETECT
	case ICMSG_STATE_INITIALIZING_SID_DETECT: {
		/* Initialization with detection of remote session awareness */
		volatile char *magic_buf;
		uint16_t magic_len;

		ret = pbuf_get_initial_buf(dev_data->rx_pb, &magic_buf, &magic_len);

		if (ret == 0 && magic_len == sizeof(magic) &&
		    memcmp((void*)magic_buf, magic, sizeof(magic)) == 0) {
			/* Remote initalized in session-unaware mode, so we do old way of
			 * initialization.
			 */
			ret = initialize_tx_with_sid_disabled(dev_data);
			if (ret < 0) {
				if (dev_data->cb->error) {
					dev_data->cb->error("Incorrect Tx configuration",
							    dev_data->ctx);
				}
				__ASSERT(false, "Incorrect Tx configuration");
				atomic_set(&dev_data->state, ICMSG_STATE_OFF);
				return false;
			}
			/* We got magic data, so we can handle it later. */
			notify_remote = true;
			rerun = true;
			atomic_set(&dev_data->state, ICMSG_STATE_INITIALIZING_SID_DISABLED);
			break;
		}
		/* If remote did not initialize the RX in session-unaware mode, we can try
		 * with session-aware initialization.
		 */
		__fallthrough;
	}
#endif /* UNBOUND_DETECT */

#if UNBOUND_ENABLED || UNBOUND_DETECT
	case ICMSG_STATE_INITIALIZING_SID_ENABLED: {
		uint32_t tx_handshake = pbuf_handshake_read(dev_data->tx_pb);
		uint32_t remote_sid_req = REMOTE_SID_REQ_FROM_TX(tx_handshake);
		uint32_t local_sid_ack = LOCAL_SID_ACK_FROM_TX(tx_handshake);
		
		if (remote_sid_req != dev_data->remote_sid && remote_sid_req != SID_DISCONNECTED) {
			/* We can now initialize TX, since we know that remote, during receiving,
			 * will first read FIFO indexes and later, it will check if session has
			 * changed before using indexes to receive the message. Additionally,
			 * we know that remote after session request change will no try to receive
			 * more data.
			 */
			ret = pbuf_tx_init(dev_data->tx_pb);
			if (ret < 0) {
				if (dev_data->cb->error) {
					dev_data->cb->error("Incorrect Tx configuration",
						dev_data->ctx);
				}
				__ASSERT(false, "Incorrect Tx configuration");
				atomic_set(&dev_data->state, ICMSG_STATE_DISCONNECTED);
				return false;
			}
			/* Acknowledge the remote session. */
			dev_data->remote_sid = remote_sid_req;
			pbuf_handshake_write(dev_data->rx_pb,
				MAKE_RX_HANDSHAKE(dev_data->local_sid, dev_data->remote_sid));
			notify_remote = true;
		}

		if (local_sid_ack == dev_data->local_sid &&
		    dev_data->remote_sid != SID_DISCONNECTED) {
			/* We send acknowledge to remote, receive acknowledge from remote,
			 * so we are ready.
			 */
			atomic_set(&dev_data->state, ICMSG_STATE_CONNECTED_SID_ENABLED);

			if (dev_data->cb->bound) {
				dev_data->cb->bound(dev_data->ctx);
			}
			/* Re-run this handler, because remote may already send something. */
			rerun = true;
			notify_remote = true;
		}

		break;
	}
#endif /* UNBOUND_ENABLED || UNBOUND_DETECT */

#if UNBOUND_ENABLED || UNBOUND_DETECT
	case ICMSG_STATE_CONNECTED_SID_ENABLED:
#endif
#if UNBOUND_DISABLED || UNBOUND_DETECT
	case ICMSG_STATE_CONNECTED_SID_DISABLED:
#endif
#if UNBOUND_DISABLED
	case ICMSG_STATE_INITIALIZING_SID_DISABLED:
#endif

		len_available = data_available(dev_data);

		if (len_available > 0 && sizeof(rx_buffer) >= len_available) {
			len = pbuf_read(dev_data->rx_pb, rx_buffer, sizeof(rx_buffer));
		}

		if (state == ICMSG_STATE_CONNECTED_SID_ENABLED &&
		    (UNBOUND_ENABLED || UNBOUND_DETECT)) {
			/* The incoming message is valid only if remote session is as expected,
			 * so we need to check remote session now.
			 */
			uint32_t remote_sid_req = REMOTE_SID_REQ_FROM_TX(
				pbuf_handshake_read(dev_data->tx_pb));

			if (remote_sid_req != dev_data->remote_sid) {
				atomic_set(&dev_data->state, ICMSG_STATE_DISCONNECTED);
				if (dev_data->cb->unbound) {
					dev_data->cb->unbound(dev_data->ctx);
				}
				return false;
			}
		}

		if (len_available == 0) {
			/* Unlikely, no data in buffer. */
			return false;
		}

		__ASSERT_NO_MSG(len_available <= sizeof(rx_buffer));

		if (sizeof(rx_buffer) < len_available) {
			return false;
		}

		if (state != ICMSG_STATE_INITIALIZING_SID_DISABLED || !UNBOUND_DISABLED) {
			if (dev_data->cb->received) {
				dev_data->cb->received(rx_buffer, len, dev_data->ctx);
			}
		} else {
			/* Allow magic number longer than sizeof(magic) for future protocol
			 * version.
			 */
			bool endpoint_invalid = (len < sizeof(magic) ||
						memcmp(magic, rx_buffer, sizeof(magic)));

			if (endpoint_invalid) {
				__ASSERT_NO_MSG(false);
				return false;
			}

			if (dev_data->cb->bound) {
				dev_data->cb->bound(dev_data->ctx);
			}

			atomic_set(&dev_data->state, ICMSG_STATE_CONNECTED_SID_DISABLED);
			notify_remote = true;
		}

		rerun = (data_available(dev_data) > 0);
		break;

	case ICMSG_STATE_OFF:
	case ICMSG_STATE_DISCONNECTED:
	default:
		/* Nothing to do in this state. */
		return false;
	}

	if (notify_remote) {
		(void)mbox_send_dt(&dev_data->cfg->mbox_tx, NULL);
	}

	return rerun;
}

#ifdef CONFIG_MULTITHREADING
static void workq_callback_process(struct k_work *item)
{
	bool rerun;
	struct icmsg_data_t *dev_data = CONTAINER_OF(item, struct icmsg_data_t, mbox_work);
	
	rerun = callback_process(dev_data);
	if (rerun) {
		submit_mbox_work(dev_data);
	}
}
#endif /* def CONFIG_MULTITHREADING */

static void mbox_callback(const struct device *instance, uint32_t channel,
			  void *user_data, struct mbox_msg *msg_data)
{
	bool rerun;
	struct icmsg_data_t *dev_data = user_data;

#ifdef CONFIG_MULTITHREADING
	ARG_UNUSED(rerun);
	submit_mbox_work(dev_data);
#else
	do {
		rerun = callback_process(dev_data);
	} while (rerun);
#endif
}

static int mbox_init(const struct icmsg_config_t *conf,
		     struct icmsg_data_t *dev_data)
{
	int err;

#ifdef CONFIG_MULTITHREADING
	k_work_init(&dev_data->mbox_work, workq_callback_process);
#endif

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
	int ret;
	enum icmsg_state old_state;

	__ASSERT(conf->unbound_mode != ICMSG_UNBOUND_MODE_DISABLE || UNBOUND_DISABLED,
		 "Unbound mode \"disabled\" is was forbidden in Kconfig.");
	__ASSERT(conf->unbound_mode != ICMSG_UNBOUND_MODE_ENABLE || UNBOUND_ENABLED,
		 "Unbound mode \"enabled\" is was forbidden in Kconfig.");
	__ASSERT(conf->unbound_mode != ICMSG_UNBOUND_MODE_DETECT || UNBOUND_DETECT,
		 "Unbound mode \"detect\" is was forbidden in Kconfig.");

	if (conf->unbound_mode == ICMSG_UNBOUND_MODE_DISABLE ||
	    !(UNBOUND_ENABLED || UNBOUND_DETECT)) {
		if (!atomic_cas(&dev_data->state, ICMSG_STATE_OFF,
				ICMSG_STATE_INITIALIZING_SID_DISABLED)) {
			/* Already opened. */
			return -EALREADY;
		}
		old_state = ICMSG_STATE_OFF;
	} else {
		/* Unbound mode has the same values as ICMSG_STATE_INITIALIZING_* */
		old_state = atomic_set(&dev_data->state, conf->unbound_mode);
	}

	dev_data->cb = cb;
	dev_data->ctx = ctx;
	dev_data->cfg = conf;

#ifdef CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_SYNC
	k_mutex_init(&dev_data->tx_lock);
#endif

	ret = pbuf_rx_init(dev_data->rx_pb);

	if (ret < 0) {
		__ASSERT(false, "Incorrect Rx configuration");
		goto cleanup_and_exit;
	}

	if (conf->unbound_mode != ICMSG_UNBOUND_MODE_DISABLE &&
	    (UNBOUND_ENABLED || UNBOUND_DETECT)) {
		/* Increment local session id without conflicts with forbidden values. */
		uint32_t local_sid_ack =
			LOCAL_SID_ACK_FROM_TX(pbuf_handshake_read(dev_data->tx_pb));
		dev_data->local_sid =
			LOCAL_SID_REQ_FROM_RX(pbuf_handshake_read(dev_data->tx_pb));
		dev_data->remote_sid = SID_DISCONNECTED;
		do {
			dev_data->local_sid = (dev_data->local_sid + 1) & 0xFFFF;
		} while (dev_data->local_sid == local_sid_ack ||
			 dev_data->local_sid == SID_DISCONNECTED);
		/* Write local session id request without remote acknowledge */
		pbuf_handshake_write(dev_data->rx_pb,
			MAKE_RX_HANDSHAKE(dev_data->local_sid, SID_DISCONNECTED));
	} else if (UNBOUND_DISABLED) {
		ret = initialize_tx_with_sid_disabled(dev_data);
	}

	if (old_state == ICMSG_STATE_OFF && (UNBOUND_ENABLED || UNBOUND_DETECT)) {
		/* Initialize mbox only if we are doing first-time open (not re-open
		 * after unbound)
		 */
		ret = mbox_init(conf, dev_data);
		if (ret) {
			goto cleanup_and_exit;
		}
	}

	/* We need to send a notification to remote, it may not be delivered
	 * since it may be uninitialized yet, but when it finishes the initialization
	 * we get a notification from it. We need to send this notification in callback
	 * again to make sure that it arrived.
	 */
	ret = mbox_send_dt(&conf->mbox_tx, NULL);

	if (ret < 0) {
		__ASSERT(false, "Cannot send mbox notification");
		goto cleanup_and_exit;
	}

	return ret;

cleanup_and_exit:
	atomic_set(&dev_data->state, ICMSG_STATE_OFF);
	return ret;
}

int icmsg_close(const struct icmsg_config_t *conf,
		struct icmsg_data_t *dev_data)
{
	int ret = 0;
	enum icmsg_state old_state;

	if (conf->unbound_mode != ICMSG_UNBOUND_MODE_DISABLE &&
	    (UNBOUND_ENABLED || UNBOUND_DETECT)) {
		pbuf_handshake_write(dev_data->rx_pb,
			MAKE_RX_HANDSHAKE(SID_DISCONNECTED, SID_DISCONNECTED));
	}

	(void)mbox_send_dt(&conf->mbox_tx, NULL);

	old_state = atomic_set(&dev_data->state, ICMSG_STATE_OFF);

	if (old_state != ICMSG_STATE_OFF) {
		ret = mbox_deinit(conf, dev_data);
	}

	return ret;
}

int icmsg_send(const struct icmsg_config_t *conf,
	       struct icmsg_data_t *dev_data,
	       const void *msg, size_t len)
{
	int ret;
	int write_ret;
#ifdef CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_SYNC
	int release_ret;
#endif
	int sent_bytes;
	uint32_t state = atomic_get(&dev_data->state);

	if (!is_endpoint_ready(state)) {
		/* If instance was disconnected on the remote side, some threads may still
		 * don't know it yet and still may try to send messages.
		 */
		return (state == ICMSG_STATE_DISCONNECTED) ? len : -EBUSY;
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
