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
#include <zephyr/init.h>

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
			const union mpsc_pbuf_generic *packet);

static const struct mpsc_pbuf_buffer_config config = {
	.buf = dbuf,
	.size = ARRAY_SIZE(dbuf),
	.notify_drop = notify_drop,
	.get_wlen = get_wlen,
	.flags = MPSC_PBUF_MODE_OVERWRITE
};

enum uart_state {
	UART_STATE_OFF,
	UART_STATE_IDLE,
	UART_STATE_ACTIVE,
};

static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static struct mpsc_pbuf_buffer pbuf;
static atomic_t dropped; /* Counter storing number of dropped messages to be reported. */
static volatile bool in_panic; /* Indicates that logging is in panic state. */
static atomic_t uart_state;
static bool dropped_notify; /* Indicate that drop report message should be added. */
static struct log_dict_output_dropped_msg_t drop_msg;
static const uint8_t *curr_tx_buf;
static size_t curr_tx_len;


static void timeout(struct k_timer *timer)
{
	dropped_notify = true;
}

K_TIMER_DEFINE(dropped_timer, timeout, NULL);

static void notify_drop(const struct mpsc_pbuf_buffer *buffer,
			const union mpsc_pbuf_generic *packet)
{
	if (atomic_inc(&dropped) == 0) {
		k_timeout_t t = K_MSEC(CONFIG_LOG_FRONTEND_DICT_UART_DROPPED_NOTIFY_PERIOD);

		k_timer_start(&dropped_timer, t, K_NO_WAIT);
	}
}

/* Called when UART TX is done. */
static void on_tx_done(const uint8_t *buf)
{
	if (curr_tx_buf != buf) {
		return;
	}

	union log_frontend_pkt generic_pkt;

	generic_pkt.generic = CONTAINER_OF(buf, struct log_frontend_uart_generic_pkt, data[0]);
	curr_tx_buf = NULL;

	mpsc_pbuf_free(&pbuf, generic_pkt.ro_pkt);
}

/* Attempts to get pending message and initiate UART transfer. In panic it polls
 * out the message in the blocking mode.
 */
static bool tx(const uint8_t *data, size_t in_len)
{
	union log_frontend_pkt generic_pkt;
	struct log_frontend_uart_generic_pkt *pkt;
	const uint8_t *buf;
	size_t len;

	if (!IS_ENABLED(CONFIG_UART_ASYNC_API) && !in_panic) {
		curr_tx_buf = data;
		curr_tx_len = in_len;
		uart_irq_tx_enable(uart_dev);
		return true;
	}

	if (data == NULL) {
		generic_pkt.ro_pkt = mpsc_pbuf_claim(&pbuf);
		pkt = generic_pkt.generic;
		if (pkt == NULL) {
			return false;
		}

		len = sizeof(uint32_t) * pkt->hdr.len - pkt->hdr.noff -
			sizeof(struct log_frontend_uart_pkt_hdr);
		buf = pkt->data;
		curr_tx_buf = buf;
	} else {
		len = in_len;
		buf = data;
	}

	if (in_panic) {
		for (int i = 0; i < len; i++) {
			uart_poll_out(uart_dev, buf[i]);
		}
		on_tx_done(buf);
	} else {
		int err = uart_tx(uart_dev, buf, len, SYS_FOREVER_US);

		(void)err;
		__ASSERT_NO_MSG(err == 0);
	}
	return true;
}

static void uart_callback(const struct device *dev,
			  struct uart_event *evt,
			  void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
	{
		on_tx_done(evt->data.tx.buf);

		if (dropped_notify) {
			drop_msg.num_dropped_messages = atomic_set(&dropped, 0);
			dropped_notify = false;
			tx((const uint8_t *)&drop_msg, sizeof(drop_msg));
		} else {
			if (tx(NULL, 0) == false) {
				atomic_set(&uart_state, UART_STATE_IDLE);
			}
		}
		break;
	}
	default:
		break;
	}
}

static void uart_isr_callback(const struct device *dev, void *user_data)
{
	static size_t curr_offset;
	static union log_frontend_pkt isr_pkt;

	uart_irq_update(dev);

	if (!uart_irq_tx_ready(dev)) {
		return;
	}
	if (curr_tx_buf != NULL && curr_tx_len == curr_offset) {
		curr_tx_buf = NULL;
		curr_tx_len = 0;
		curr_offset = 0;
		if (isr_pkt.ro_pkt != NULL) {
			mpsc_pbuf_free(&pbuf, isr_pkt.ro_pkt);
			isr_pkt.ro_pkt = NULL;
		}
	}

	if (curr_tx_buf == NULL) {
		if (dropped_notify) {
			drop_msg.num_dropped_messages = atomic_set(&dropped, 0);
			dropped_notify = false;
			curr_tx_buf = (const uint8_t *)&drop_msg;
			curr_tx_len = sizeof(drop_msg);
		} else if (isr_pkt.ro_pkt == NULL) {
			isr_pkt.ro_pkt = mpsc_pbuf_claim(&pbuf);
			if (isr_pkt.ro_pkt == NULL) {
				uart_irq_tx_disable(dev);
				atomic_set(&uart_state, UART_STATE_IDLE);
				return;
			}

			struct log_frontend_uart_generic_pkt *pkt = isr_pkt.generic;

			curr_tx_len = sizeof(uint32_t) * pkt->hdr.len - pkt->hdr.noff -
				sizeof(struct log_frontend_uart_pkt_hdr);
			curr_tx_buf = pkt->data;
		} else {
			/* Should not happen */
			__ASSERT_NO_MSG(false);
		}
	}

	size_t rem = curr_tx_len - curr_offset;

	curr_offset += uart_fifo_fill(dev, &curr_tx_buf[curr_offset], rem);
}

static inline void hdr_fill(struct log_dict_output_normal_msg_hdr_t *hdr,
			    const void *source,
			    const struct log_msg_desc desc, log_timestamp_t timestamp)
{
	uint32_t source_id = IS_ENABLED(CONFIG_LOG_MULTIDOMAIN) && desc.domain != 0 ?
		(uintptr_t)source : (source != NULL) ? log_source_id(source) : 0U;
	hdr->type = MSG_NORMAL;
	hdr->domain = desc.domain;
	hdr->level = desc.level;
	hdr->package_len = desc.package_len;
	hdr->data_len = desc.data_len;
	hdr->timestamp = timestamp;
	hdr->source = source_id;
}

/* Handle logging message in synchronous manner, in panic mode. */
static void sync_msg(const void *source,
		     const struct log_msg_desc desc,
		     uint8_t *package, const void *data, log_timestamp_t timestamp)
{
	struct log_dict_output_normal_msg_hdr_t hdr;
	uint8_t *datas[3] = {(uint8_t *)&hdr, package, (uint8_t *)data};
	size_t len[3] = {sizeof(hdr), desc.package_len, desc.data_len};

	hdr_fill(&hdr, source, desc, timestamp);

	for (int i = 0; i < ARRAY_SIZE(datas); i++) {
		for (int j = 0; j < len[i]; j++) {
			uart_poll_out(uart_dev, datas[i][j]);
		}
	}
}

void log_frontend_msg_with_timestamp(const void *source,
		      const struct log_msg_desc desc,
		      uint8_t *package, const void *data, log_timestamp_t timestamp)
{
	if (!mpsc_pbuf_is_init(&pbuf)) {
		return;
	}

	uint16_t strl[4];
	struct log_msg_desc outdesc = desc;
	int plen = cbprintf_package_copy(package, desc.package_len, NULL, 0,
					 CBPRINTF_PACKAGE_CONVERT_RW_STR,
					 strl, ARRAY_SIZE(strl));
	size_t dlen = desc.data_len;
	size_t total_len = plen + dlen + sizeof(struct log_frontend_uart_pkt);
	size_t total_wlen = DIV_ROUND_UP(total_len, sizeof(uint32_t));

	if (in_panic) {
		sync_msg(source, desc, package, data, timestamp);
	}

	union log_frontend_pkt generic_pkt;
	struct log_frontend_uart_pkt *pkt;

	generic_pkt.rw_pkt = mpsc_pbuf_alloc(&pbuf, total_wlen, K_NO_WAIT);

	pkt = generic_pkt.pkt;
	if (pkt == NULL) {
		/* Dropping */
		atomic_inc(&dropped);
		return;
	}

	pkt->hdr.len = total_wlen;
	pkt->hdr.noff = sizeof(uint32_t) * total_wlen - total_len;
	outdesc.package_len = plen;
	hdr_fill(&pkt->data_hdr, source, outdesc, timestamp);

	plen = cbprintf_package_copy(package, desc.package_len,
				     pkt->data, plen,
				     CBPRINTF_PACKAGE_CONVERT_RW_STR,
				     strl, ARRAY_SIZE(strl));
	__ASSERT_NO_MSG(plen >= 0);

	if (dlen != 0) {
		memcpy(&pkt->data[plen], data, dlen);
	}

	mpsc_pbuf_commit(&pbuf, generic_pkt.rw_pkt);

	if (atomic_cas(&uart_state, UART_STATE_IDLE, UART_STATE_ACTIVE) == true) {
		if (tx(NULL, 0) == false) {
			atomic_set(&uart_state, UART_STATE_IDLE);
		}
	}
}

void log_frontend_msg(const void *source,
		      const struct log_msg_desc desc,
		      uint8_t *package, const void *data)
{
	log_timestamp_t timestamp = IS_ENABLED(CONFIG_LOG_TIMESTAMP_64BIT) ?
		sys_clock_tick_get() : k_cycle_get_32();

	log_frontend_msg_with_timestamp(source, desc, package, data, timestamp);
}

void log_frontend_panic(void)
{
	in_panic = true;

	if (curr_tx_buf != NULL) {
		on_tx_done(curr_tx_buf);
	}

	/* Flush all data */
	while (tx(NULL, 0) == true) {
		/* empty */
	}
}

void log_frontend_init(void)
{
	drop_msg.type = MSG_DROPPED_MSG;
	mpsc_pbuf_init(&pbuf, &config);
}

/* Cannot be started in log_frontend_init because it is called before UART is ready. */
static int log_frontend_uart_init(void)
{
	int err;
	static const uint8_t init_msg[] = LOG_DICT_BIN_SEP;

	__ASSERT(device_is_ready(uart_dev), "UART device not ready");

	if (IS_ENABLED(CONFIG_UART_ASYNC_API)) {
		atomic_set(&uart_state, UART_STATE_ACTIVE);
		err = uart_callback_set(uart_dev, uart_callback, NULL);
		if (err < 0) {
			return err;
		}
		return uart_tx(uart_dev, init_msg, sizeof(init_msg), SYS_FOREVER_US);
	}

	uart_irq_callback_user_data_set(uart_dev, uart_isr_callback, NULL);
	for (int i = 0; i < sizeof(init_msg); i++) {
		uart_poll_out(uart_dev, init_msg[i]);
	}
	atomic_set(&uart_state, UART_STATE_IDLE);

	return 0;
}

SYS_INIT(log_frontend_uart_init, PRE_KERNEL_1, UTIL_INC(UTIL_INC(CONFIG_SERIAL_INIT_PRIORITY)));
