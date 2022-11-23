/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log_frontend.h>
#include <zephyr/logging/log_internal.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/sys/mpsc_pbuf.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

static uint32_t dbuf[CONFIG_LOG_FRONTEND_DICT_UART_BUFFER_SIZE / sizeof(uint32_t)];

struct log_frontend_uart_pkt_hdr {
	MPSC_PBUF_HDR;
	uint16_t len: 12;
	uint32_t noff: 2;
} __packed;

BUILD_ASSERT(sizeof(struct log_frontend_uart_pkt_hdr) == sizeof(uint16_t));

struct log_frontend_uart_generic_pkt {
	struct log_frontend_uart_pkt_hdr hdr;
	uint8_t data[0];
} __packed;

struct log_frontend_uart_dropped_pkt {
	struct log_frontend_uart_pkt_hdr hdr;
	struct log_dict_output_dropped_msg_t data;
} __packed;

struct log_frontend_uart_pkt {
	struct log_frontend_uart_pkt_hdr hdr;
	struct log_dict_output_normal_msg_hdr_t data_hdr;
	uint8_t data[0];
} __packed;

/* Union needed to avoid warning when casting to packed structure. */
union log_frontend_pkt {
	struct log_frontend_uart_generic_pkt *generic;
	struct log_frontend_uart_dropped_pkt *dropped;
	struct log_frontend_uart_pkt *pkt;
	const union mpsc_pbuf_generic *ro_pkt;
	union mpsc_pbuf_generic *rw_pkt;
};

static uint32_t get_wlen(const union mpsc_pbuf_generic *packet)
{
	struct log_frontend_uart_generic_pkt *hdr = (struct log_frontend_uart_generic_pkt *)packet;

	return (uint32_t)hdr->hdr.len;
}

static void notify_drop(const struct mpsc_pbuf_buffer *buffer,
			const union mpsc_pbuf_generic *packet)
{
}

static const struct mpsc_pbuf_buffer_config config = {
	.buf = dbuf,
	.size = ARRAY_SIZE(dbuf),
	.notify_drop = notify_drop,
	.get_wlen = get_wlen,
	.flags = 0
};
static const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static struct mpsc_pbuf_buffer buf;
static atomic_t active_cnt; /* Counts number of buffered messages. */
static atomic_t dropped; /* Counter storing number of dropped messages to be reported. */
static atomic_t adding_drop; /* Indicates that drop report message is being added. */
static volatile bool in_panic; /* Indicates that logging is in panic state. */
static bool dropped_notify; /* Indicate that drop report message should be added. */

static void tx(void);
static atomic_val_t add_drop_msg(void);

static void timeout(struct k_timer *timer)
{
	if (dropped != 0) {
		dropped_notify = true;
		if (add_drop_msg() == 0) {
			tx();
		}
	}
}

K_TIMER_DEFINE(dropped_timer, timeout, NULL);

/* Attempts to get pending message and initiate UART transfer. In panic it polls
 * out the message in the blocking mode.
 */
static void tx(void)
{
	union log_frontend_pkt generic_pkt;
	struct log_frontend_uart_generic_pkt *pkt;

	if (!IS_ENABLED(CONFIG_UART_ASYNC_API) && !in_panic) {
		uart_irq_tx_enable(dev);
		return;
	}

	generic_pkt.ro_pkt = mpsc_pbuf_claim(&buf);
	pkt = generic_pkt.generic;
	__ASSERT_NO_MSG(pkt == NULL);

	size_t len = sizeof(uint32_t) * pkt->hdr.len - pkt->hdr.noff -
			sizeof(struct log_frontend_uart_pkt_hdr);

	if (in_panic) {
		for (int i = 0; i < len; i++) {
			uart_poll_out(dev, pkt->data[i]);
		}
		atomic_dec(&active_cnt);
	} else {
		int err = uart_tx(dev, pkt->data, len, SYS_FOREVER_US);

		(void)err;
		__ASSERT_NO_MSG(err == 0);
	}
}

/* Add drop message and reset drop message count. */
static atomic_val_t add_drop_msg(void)
{
	union log_frontend_pkt generic_pkt;
	struct log_frontend_uart_dropped_pkt *pkt;
	size_t len = sizeof(struct log_frontend_uart_dropped_pkt);
	size_t wlen = DIV_ROUND_UP(len, sizeof(uint32_t));

	if (atomic_cas(&adding_drop, 0, 1) == false) {
		return 1;
	}

	generic_pkt.rw_pkt = mpsc_pbuf_alloc(&buf, wlen, K_NO_WAIT);
	pkt = generic_pkt.dropped;
	if (pkt == NULL) {
		return 1;
	}

	dropped_notify = false;
	pkt->hdr.len = wlen;
	pkt->hdr.noff = sizeof(uint32_t) * wlen - len;
	pkt->data.type = MSG_DROPPED_MSG;
	pkt->data.num_dropped_messages = atomic_set(&dropped, 0);
	mpsc_pbuf_commit(&buf, generic_pkt.rw_pkt);

	return atomic_inc(&active_cnt);
}

static void uart_callback(const struct device *dev,
			  struct uart_event *evt,
			  void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
	{
		union log_frontend_pkt generic_pkt;

		generic_pkt.generic = CONTAINER_OF(evt->data.tx.buf,
						   struct log_frontend_uart_generic_pkt, hdr);

		mpsc_pbuf_free(&buf, generic_pkt.ro_pkt);
		atomic_val_t rem_pkts = atomic_dec(&active_cnt);

		if (dropped_notify) {
			rem_pkts = add_drop_msg();
		}

		if (rem_pkts > 1) {
			tx();
		}
	}
	break;

	case UART_TX_ABORTED:
		break;
	default:
		break;
	}
}

static void uart_isr_callback(const struct device *dev, void *user_data)
{
	static size_t curr_offset;
	static union log_frontend_pkt isr_pkt;

	if (uart_irq_update(dev) && uart_irq_tx_ready(dev)) {
		if (isr_pkt.ro_pkt == NULL) {
			isr_pkt.ro_pkt = mpsc_pbuf_claim(&buf);
			__ASSERT_NO_MSG(isr_pkt.ro_pkt != NULL);

			curr_offset = 0;
		}

		if (isr_pkt.ro_pkt != NULL) {
			struct log_frontend_uart_generic_pkt *pkt = isr_pkt.generic;
			size_t len = sizeof(uint32_t) * pkt->hdr.len - pkt->hdr.noff -
				sizeof(struct log_frontend_uart_pkt_hdr);
			size_t rem = len - curr_offset;

			if (rem > 0) {
				uint8_t *d = &pkt->data[curr_offset];

				curr_offset += uart_fifo_fill(dev, d, rem);
			} else {
				mpsc_pbuf_free(&buf, isr_pkt.ro_pkt);
				isr_pkt.ro_pkt = NULL;

				static struct k_spinlock lock;
				k_spinlock_key_t key = k_spin_lock(&lock);

				active_cnt--;
				if (active_cnt == 0) {
					uart_irq_tx_disable(dev);
				}
				k_spin_unlock(&lock, key);
			}
		}
	}
}

static inline void hdr_fill(struct log_dict_output_normal_msg_hdr_t *hdr,
			    const void *source,
			    const struct log_msg_desc desc)
{
	hdr->type = MSG_NORMAL;
	hdr->domain = desc.domain;
	hdr->level = desc.level;
	hdr->package_len = desc.package_len;
	hdr->data_len = desc.data_len;
	hdr->timestamp = z_log_timestamp();
	hdr->source = (source != NULL) ?
			(IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
				log_dynamic_source_id((void *)source) :
				log_const_source_id((void *)source)) :
			0U;
}

/* Handle logging message in synchronous manner, in panic mode. */
static void sync_msg(const void *source,
		     const struct log_msg_desc desc,
		     uint8_t *package, const void *data)
{
	struct log_dict_output_normal_msg_hdr_t hdr;
	uint8_t *datas[3] = {(uint8_t *)&hdr, package, (uint8_t *)data};
	size_t len[3] = {sizeof(hdr), desc.package_len, desc.data_len};

	hdr_fill(&hdr, source, desc);

	for (int i = 0; i < ARRAY_SIZE(datas); i++) {
		for (int j = 0; j < len[i]; j++) {
			uart_poll_out(dev, datas[i][j]);
		}
	}
}

void log_frontend_msg(const void *source,
		      const struct log_msg_desc desc,
		      uint8_t *package, const void *data)
{
	uint16_t strl[4];
	struct log_msg_desc outdesc = desc;
	int plen = cbprintf_package_copy(package, desc.package_len, NULL, 0,
					 CBPRINTF_PACKAGE_CONVERT_RW_STR,
					 strl, ARRAY_SIZE(strl));
	size_t dlen = desc.data_len;
	bool dev_ready = device_is_ready(dev);
	size_t total_len = plen + dlen + sizeof(struct log_frontend_uart_pkt);
	size_t total_wlen = DIV_ROUND_UP(total_len, sizeof(uint32_t));

	if (in_panic) {
		sync_msg(source, desc, package, data);
	}

	union log_frontend_pkt generic_pkt;
	struct log_frontend_uart_pkt *pkt;

	generic_pkt.rw_pkt = mpsc_pbuf_alloc(&buf, total_wlen, K_NO_WAIT);

	pkt = generic_pkt.pkt;
	if (pkt == NULL) {
		/* Dropping */
		atomic_inc(&dropped);
		return;
	}

	pkt->hdr.len = total_wlen;
	pkt->hdr.noff = sizeof(uint32_t) * total_wlen - total_len;
	outdesc.package_len = plen;
	hdr_fill(&pkt->data_hdr, source, outdesc);

	plen = cbprintf_package_copy(package, desc.package_len,
				     pkt->data, plen,
				     CBPRINTF_PACKAGE_CONVERT_RW_STR,
				     strl, ARRAY_SIZE(strl));
	if (plen < 0) {
		/* error */
	}

	if (dlen != 0) {
		memcpy(&pkt->data[plen], data, dlen);
	}

	mpsc_pbuf_commit(&buf, generic_pkt.rw_pkt);

	if (dev_ready && (atomic_inc(&active_cnt) == 0)) {
		tx();
	}
}

void log_frontend_panic(void)
{
	in_panic = true;

	/* Flush all data */
	while (atomic_get(&active_cnt) > 0) {
		tx();
		/* empty */
	}
}

void log_frontend_init(void)
{
	int err;

	if (IS_ENABLED(CONFIG_UART_ASYNC_API)) {
		err = uart_callback_set(dev, uart_callback, NULL);
	} else if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
		uart_irq_callback_user_data_set(dev, uart_isr_callback, NULL);
		err = 0;
	}

	__ASSERT(err == 0, "Failed to set callback");
	if (err < 0) {
		return;
	}

	mpsc_pbuf_init(&buf, &config);
}

/* Cannot be started in log_frontend_init because it is called before kernel is ready. */
static int log_frontend_uart_start_timer(const struct device *unused)
{
	ARG_UNUSED(unused);
	k_timeout_t t = K_MSEC(CONFIG_LOG_FRONTEND_DICT_UART_DROPPED_NOTIFY_PERIOD);

	k_timer_start(&dropped_timer, t, t);

	return 0;
}

SYS_INIT(log_frontend_uart_start_timer, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
