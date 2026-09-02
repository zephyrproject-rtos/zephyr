/** @file
 *  @brief Custom logging over UART
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart_pipe.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk-hooks.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "monitor.h"
#include "monitor_buffer.h"

/* This is the same default priority as for other console handlers,
 * except that we're not exporting it as a Kconfig variable until a
 * clear need arises.
 */
#define MONITOR_INIT_PRIORITY 60

/* These defines follow the values used by syslog(2) */
#define BT_LOG_ERR      3
#define BT_LOG_WARN     4
#define BT_LOG_INFO     6
#define BT_LOG_DBG      7

/* TS resolution is 1/10th of a millisecond */
#define MONITOR_TS_FREQ 10000

/* Maximum (string) length of a log message */
#define MONITOR_MSG_MAX 128

enum {
	BT_LOG_BUSY,
	BT_CONSOLE_BUSY,
};

static atomic_t flags;

static struct {
	atomic_t cmd;
	atomic_t evt;
	atomic_t acl_tx;
	atomic_t acl_rx;
#if defined(CONFIG_BT_CLASSIC)
	atomic_t sco_tx;
	atomic_t sco_rx;
#endif
	atomic_t other;
} drops;

static void drop_add(uint16_t opcode)
{
	switch (opcode) {
	case BT_MONITOR_COMMAND_PKT:
		atomic_inc(&drops.cmd);
		break;
	case BT_MONITOR_EVENT_PKT:
		atomic_inc(&drops.evt);
		break;
	case BT_MONITOR_ACL_TX_PKT:
		atomic_inc(&drops.acl_tx);
		break;
	case BT_MONITOR_ACL_RX_PKT:
		atomic_inc(&drops.acl_rx);
		break;
#if defined(CONFIG_BT_CLASSIC)
	case BT_MONITOR_SCO_TX_PKT:
		atomic_inc(&drops.sco_tx);
		break;
	case BT_MONITOR_SCO_RX_PKT:
		atomic_inc(&drops.sco_rx);
		break;
#endif
	default:
		atomic_inc(&drops.other);
		break;
	}
}

#if defined(CONFIG_BT_DEBUG_MONITOR_RTT)
#include <SEGGER_RTT.h>

static bool panic_mode;

#define RTT_BUFFER_NAME CONFIG_BT_DEBUG_MONITOR_RTT_BUFFER_NAME
#define RTT_BUF_SIZE CONFIG_BT_DEBUG_MONITOR_RTT_BUFFER_SIZE

static bool monitor_send(const struct bt_monitor_data *frags, size_t count)
{
	static uint8_t rtt_buf[RTT_BUF_SIZE];
	size_t total = 0;
	unsigned int cnt = 0;

	for (size_t i = 0; i < count; i++) {
		/* Zero-length fragments may carry a NULL data pointer */
		if (frags[i].len == 0) {
			continue;
		}

		/* total only grows after passing this check, so it never
		 * exceeds sizeof(rtt_buf) and the subtraction cannot underflow.
		 */
		if (frags[i].len > sizeof(rtt_buf) - total) {
			return false;
		}

		(void)memcpy(rtt_buf + total, frags[i].data, frags[i].len);
		total += frags[i].len;
	}

	if (panic_mode) {
		cnt = SEGGER_RTT_WriteNoLock(CONFIG_BT_DEBUG_MONITOR_RTT_BUFFER, rtt_buf, total);
	} else {
		cnt = SEGGER_RTT_Write(CONFIG_BT_DEBUG_MONITOR_RTT_BUFFER, rtt_buf, total);
	}

	return cnt != 0;
}
#elif defined(CONFIG_BT_DEBUG_MONITOR_UART)
static const struct device *const monitor_dev =
#if DT_HAS_CHOSEN(zephyr_bt_mon_uart)
	DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_mon_uart));
#elif !defined(CONFIG_UART_CONSOLE) && DT_HAS_CHOSEN(zephyr_console)
	/* Fall back to console UART if it's available */
	DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
#else
	NULL;
#error "BT_DEBUG_MONITOR_UART enabled but no UART specified"
#endif

static void monitor_poll_send(const struct bt_monitor_data *frags, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		const uint8_t *buf = frags[i].data;

		for (size_t j = 0; j < frags[i].len; j++) {
			uart_poll_out(monitor_dev, buf[j]);
		}
	}
}

#if defined(CONFIG_BT_DEBUG_MONITOR_UART_INTERRUPT_DRIVEN)
/* Single producer (serialized by BT_LOG_BUSY) and single consumer (the UART
 * ISR), matching the ring buffer's lock-free SPSC contract. The memory
 * ordering that contract additionally requires on SMP systems is provided by
 * the fences in the bt_monitor_ring_buf_* helpers. On panic the flush in
 * monitor_log_panic() becomes a second consumer; monitor_tx_lock serializes
 * it with the ISR, while producers stay lock-free.
 */
static uint8_t monitor_tx_data[CONFIG_BT_DEBUG_MONITOR_UART_BUFFER_SIZE];
static struct ring_buf monitor_tx_buf = RING_BUF_INIT(monitor_tx_data, sizeof(monitor_tx_data));
static atomic_t monitor_tx_busy;
static struct k_spinlock monitor_tx_lock;
/* Set on panic or when the UART driver lacks interrupt support */
static bool poll_mode;

static void monitor_uart_tx(void)
{
	uint8_t *data;
	uint32_t len;
	int sent;

	len = bt_monitor_ring_buf_get_ptr(&monitor_tx_buf, &data);
	if (len > 0) {
		sent = uart_fifo_fill(monitor_dev, data, len);
		if (sent > 0) {
			bt_monitor_ring_buf_consume(&monitor_tx_buf, sent);
			return;
		}

		/* TX ready but nothing accepted: disable TX so a persistent
		 * driver error cannot cause an interrupt storm. The next
		 * committed record re-enables TX and retries. Unlike the
		 * empty-buffer path below, deliberately no re-check here:
		 * the buffer is non-empty at this point, so re-enabling
		 * would defeat the backoff.
		 */
		uart_irq_tx_disable(monitor_dev);
		atomic_set(&monitor_tx_busy, 0);
		return;
	}

	uart_irq_tx_disable(monitor_dev);
	atomic_set(&monitor_tx_busy, 0);

	/* Close the race with a producer that committed while TX was disabled. */
	if (!ring_buf_is_empty(&monitor_tx_buf) && atomic_cas(&monitor_tx_busy, 0, 1)) {
		uart_irq_tx_enable(monitor_dev);
	}
}

static void monitor_uart_isr(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	k_spinlock_key_t key = k_spin_lock(&monitor_tx_lock);

	uart_irq_update(dev);
	if (uart_irq_tx_ready(dev) > 0) {
		monitor_uart_tx();
	}

	k_spin_unlock(&monitor_tx_lock, key);
}

static bool monitor_send(const struct bt_monitor_data *frags, size_t count)
{
	if (poll_mode) {
		monitor_poll_send(frags, count);
		return true;
	}

	if (!bt_monitor_ring_buf_put(&monitor_tx_buf, frags, count)) {
		return false;
	}

	if (atomic_cas(&monitor_tx_busy, 0, 1)) {
		uart_irq_tx_enable(monitor_dev);
	}

	return true;
}
#else
static bool monitor_send(const struct bt_monitor_data *frags, size_t count)
{
	monitor_poll_send(frags, count);

	return true;
}
#endif /* CONFIG_BT_DEBUG_MONITOR_UART_INTERRUPT_DRIVEN */
#endif /* CONFIG_BT_DEBUG_MONITOR_UART */

static void encode_drops(struct bt_monitor_hdr *hdr, uint8_t type,
			 atomic_t *val)
{
	atomic_val_t count;

	count = atomic_set(val, 0);
	if (count) {
		hdr->ext[hdr->hdr_len++] = type;
		hdr->ext[hdr->hdr_len++] = MIN(count, 255);
		if (count > 255) {
			/* Keep the surplus for the next record */
			atomic_add(val, count - 255);
		}
	}
}

static atomic_t *drop_counter(uint8_t type)
{
	switch (type) {
	case BT_MONITOR_COMMAND_DROPS:
		return &drops.cmd;
	case BT_MONITOR_EVENT_DROPS:
		return &drops.evt;
	case BT_MONITOR_ACL_TX_DROPS:
		return &drops.acl_tx;
	case BT_MONITOR_ACL_RX_DROPS:
		return &drops.acl_rx;
#if defined(CONFIG_BT_CLASSIC)
	case BT_MONITOR_SCO_TX_DROPS:
		return &drops.sco_tx;
	case BT_MONITOR_SCO_RX_DROPS:
		return &drops.sco_rx;
#endif
	case BT_MONITOR_OTHER_DROPS:
		return &drops.other;
	default:
		return NULL;
	}
}

/* Restore drop counts that encode_hdr() consumed into a record which then
 * failed to be sent: without this, every record dropped back-to-back would
 * destroy the counts accumulated by its predecessors, understating drops
 * under sustained overload.
 */
static void restore_drops(const struct bt_monitor_hdr *hdr)
{
	/* The timestamp is always encoded first, drop pairs follow */
	for (uint8_t i = sizeof(struct bt_monitor_ts32); i + 1U < hdr->hdr_len; i += 2U) {
		atomic_t *counter = drop_counter(hdr->ext[i]);

		if (counter != NULL) {
			atomic_add(counter, hdr->ext[i + 1]);
		}
	}
}

static log_timestamp_t monitor_ts_get(void)
{
	uint64_t cycle;

	if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		cycle = k_cycle_get_64();
	} else {
		cycle = k_cycle_get_32();
	}

	/* Convert to 1/10th of a millisecond via microseconds, rather than
	 * dividing by hw_cycles / MONITOR_TS_FREQ: the latter truncates badly
	 * for cycle rates that are not a multiple of MONITOR_TS_FREQ (e.g.
	 * 32768 Hz yields a divisor of 3 instead of 3.2768, making the
	 * timestamps run 9.2% fast).
	 */
	return (log_timestamp_t)(k_cyc_to_us_floor64(cycle) / (USEC_PER_MSEC / 10U));
}

static inline void encode_hdr(struct bt_monitor_hdr *hdr, log_timestamp_t timestamp,
			      uint16_t opcode, uint16_t len)
{
	struct bt_monitor_ts32 *ts;

	hdr->opcode   = sys_cpu_to_le16(opcode);
	hdr->flags    = 0U;

	ts = (void *)hdr->ext;
	ts->type = BT_MONITOR_TS32;
	/* The btsnoop protocol only supports 32-bit timestamps (in 1/10th ms).
	 * This overflows after 4.97 days, which is acceptable as it only affects
	 * rendering in btmon/wireshark. Recordings are usually not that long
	 * anyway, and packet ordering is still preserved.
	 */
	ts->ts32 = (uint32_t)timestamp;
	hdr->hdr_len = sizeof(*ts);

	encode_drops(hdr, BT_MONITOR_COMMAND_DROPS, &drops.cmd);
	encode_drops(hdr, BT_MONITOR_EVENT_DROPS, &drops.evt);
	encode_drops(hdr, BT_MONITOR_ACL_TX_DROPS, &drops.acl_tx);
	encode_drops(hdr, BT_MONITOR_ACL_RX_DROPS, &drops.acl_rx);
#if defined(CONFIG_BT_CLASSIC)
	encode_drops(hdr, BT_MONITOR_SCO_TX_DROPS, &drops.sco_tx);
	encode_drops(hdr, BT_MONITOR_SCO_RX_DROPS, &drops.sco_rx);
#endif
	encode_drops(hdr, BT_MONITOR_OTHER_DROPS, &drops.other);

	hdr->data_len = sys_cpu_to_le16(4 + hdr->hdr_len + len);
}

void bt_monitor_send(uint16_t opcode, const void *data, size_t len)
{
	struct bt_monitor_hdr hdr;
	struct bt_monitor_data frags[] = {
		{ &hdr, 0 }, /* Length known only after encode_hdr() */
		{ data, len },
	};

	if (atomic_test_and_set_bit(&flags, BT_LOG_BUSY)) {
		drop_add(opcode);
		return;
	}

	encode_hdr(&hdr, monitor_ts_get(), opcode, len);
	frags[0].len = BT_MONITOR_BASE_HDR_LEN + hdr.hdr_len;

	if (!monitor_send(frags, ARRAY_SIZE(frags))) {
		restore_drops(&hdr);
		drop_add(opcode);
	}

	atomic_clear_bit(&flags, BT_LOG_BUSY);
}

void bt_monitor_new_index(uint8_t type, uint8_t bus, const bt_addr_t *addr,
			  const char *name)
{
	struct bt_monitor_new_index pkt;

	pkt.type = type;
	pkt.bus = bus;
	memcpy(pkt.bdaddr, addr, 6);
	strncpy(pkt.name, name, sizeof(pkt.name) - 1);
	pkt.name[sizeof(pkt.name) - 1] = '\0';

	bt_monitor_send(BT_MONITOR_NEW_INDEX, &pkt, sizeof(pkt));
}

#if !defined(CONFIG_UART_CONSOLE) && !defined(CONFIG_RTT_CONSOLE) && !defined(CONFIG_LOG_PRINTK)
static int monitor_console_out(int c)
{
	static char buf[MONITOR_MSG_MAX];
	static size_t len;

	if (atomic_test_and_set_bit(&flags, BT_CONSOLE_BUSY)) {
		return c;
	}

	if (c != '\n' && len < sizeof(buf) - 1) {
		buf[len++] = c;
		atomic_clear_bit(&flags, BT_CONSOLE_BUSY);
		return c;
	}

	buf[len++] = '\0';

	bt_monitor_send(BT_MONITOR_SYSTEM_NOTE, buf, len);
	len = 0;

	atomic_clear_bit(&flags, BT_CONSOLE_BUSY);

	return c;
}
#endif /* !CONFIG_UART_CONSOLE && !CONFIG_RTT_CONSOLE && !CONFIG_LOG_PRINTK */

#ifndef CONFIG_LOG_MODE_MINIMAL
struct monitor_log_ctx {
	size_t total_len;
	char msg[MONITOR_MSG_MAX];
};

static int monitor_log_out(uint8_t *data, size_t length, void *user_data)
{
	struct monitor_log_ctx *ctx = user_data;
	size_t i;

	for (i = 0; i < length && ctx->total_len < sizeof(ctx->msg); i++) {
		/* With CONFIG_LOG_PRINTK the line terminator will come as
		 * as part of messages.
		 */
		if (IS_ENABLED(CONFIG_LOG_PRINTK) &&
		    (data[i] == '\r' || data[i] == '\n')) {
			break;
		}

		ctx->msg[ctx->total_len++] = data[i];
	}

	return length;
}

static uint8_t buf;

LOG_OUTPUT_DEFINE(monitor_log_output, monitor_log_out, &buf, 1);

static inline uint8_t monitor_priority_get(uint8_t log_level)
{
	static const uint8_t prios[] = {
		[LOG_LEVEL_NONE]  = 0,
		[LOG_LEVEL_ERR]   = BT_LOG_ERR,
		[LOG_LEVEL_WRN]   = BT_LOG_WARN,
		[LOG_LEVEL_INF]   = BT_LOG_INFO,
		[LOG_LEVEL_DBG]   = BT_LOG_DBG,
	};

	if (log_level < ARRAY_SIZE(prios)) {
		return prios[log_level];
	}

	return BT_LOG_DBG;
}

static void monitor_log_process(const struct log_backend *const backend,
				union log_msg_generic *msg)
{
	struct bt_monitor_user_logging user_log;
	struct monitor_log_ctx ctx;
	struct bt_monitor_hdr hdr;
	static const char id[] = "bt";
	struct bt_monitor_data frags[] = {
		{ &hdr, 0 },    /* Length known only after encode_hdr() */
		{ &user_log, sizeof(user_log) },
		{ id, sizeof(id) },
		{ ctx.msg, 0 }, /* Length known only after log processing */
		{ "", 1 }, /* Terminating NUL for the message string */
	};

	log_output_ctx_set(&monitor_log_output, &ctx);

	ctx.total_len = 0;
	log_output_msg_process(&monitor_log_output, &msg->log,
			       LOG_OUTPUT_FLAG_CRLF_NONE);

	if (atomic_test_and_set_bit(&flags, BT_LOG_BUSY)) {
		drop_add(BT_MONITOR_USER_LOGGING);
		return;
	}

	encode_hdr(&hdr, log_msg_get_timestamp(&msg->log),
		   BT_MONITOR_USER_LOGGING,
		   sizeof(user_log) + sizeof(id) + ctx.total_len + 1);

	user_log.priority = monitor_priority_get(log_msg_get_level(&msg->log));
	user_log.ident_len = sizeof(id);
	frags[0].len = BT_MONITOR_BASE_HDR_LEN + hdr.hdr_len;
	frags[3].len = ctx.total_len;

	if (!monitor_send(frags, ARRAY_SIZE(frags))) {
		restore_drops(&hdr);
		drop_add(BT_MONITOR_USER_LOGGING);
	}

	atomic_clear_bit(&flags, BT_LOG_BUSY);
}

static void monitor_log_panic(const struct log_backend *const backend)
{
#if defined(CONFIG_BT_DEBUG_MONITOR_RTT)
	panic_mode = true;
#elif defined(CONFIG_BT_DEBUG_MONITOR_UART_INTERRUPT_DRIVEN)
	k_spinlock_key_t key;
	bool locked = false;
	uint8_t *data;
	uint32_t len;

	poll_mode = true;

	/* Setting monitor_tx_busy prevents a producer that read poll_mode
	 * before it was set above from re-enabling TX interrupts: from here
	 * on the buffer is drained only by the flush below. A record such a
	 * producer still commits stays buffered and untransmitted, which is
	 * accepted in panic context.
	 */
	atomic_set(&monitor_tx_busy, 1);
	uart_irq_tx_disable(monitor_dev);

	/* Serialize with a UART ISR that may be consuming on another CPU.
	 * Bounded, because if the panic originated inside that ISR the lock
	 * would never be released: then flush unserialized rather than hang
	 * the panic path.
	 */
	for (int i = 0; i < 100; i++) {
		if (k_spin_trylock(&monitor_tx_lock, &key) == 0) {
			locked = true;
			break;
		}

		k_busy_wait(10);
	}

	len = bt_monitor_ring_buf_get_ptr(&monitor_tx_buf, &data);
	while (len > 0) {
		struct bt_monitor_data frag = { data, len };

		monitor_poll_send(&frag, 1);
		bt_monitor_ring_buf_consume(&monitor_tx_buf, len);
		len = bt_monitor_ring_buf_get_ptr(&monitor_tx_buf, &data);
	}

	if (locked) {
		k_spin_unlock(&monitor_tx_lock, key);
	}
#endif
}

static void monitor_log_init(const struct log_backend *const backend)
{
	log_set_timestamp_func(monitor_ts_get, MONITOR_TS_FREQ);
}

static const struct log_backend_api monitor_log_api = {
	.process = monitor_log_process,
	.panic = monitor_log_panic,
	.init = monitor_log_init,
};

LOG_BACKEND_DEFINE(bt_monitor, monitor_log_api, true);
#endif /* CONFIG_LOG_MODE_MINIMAL */

static int bt_monitor_init(void)
{

#if defined(CONFIG_BT_DEBUG_MONITOR_RTT)
	static uint8_t rtt_up_buf[RTT_BUF_SIZE];

	SEGGER_RTT_ConfigUpBuffer(CONFIG_BT_DEBUG_MONITOR_RTT_BUFFER,
				  RTT_BUFFER_NAME, rtt_up_buf, RTT_BUF_SIZE,
				  SEGGER_RTT_MODE_NO_BLOCK_SKIP);
#elif defined(CONFIG_BT_DEBUG_MONITOR_UART)
	__ASSERT_NO_MSG(device_is_ready(monitor_dev));

#if defined(CONFIG_BT_DEBUG_MONITOR_UART_INTERRUPT_DRIVEN)
	/* SERIAL_SUPPORT_INTERRUPT only guarantees that some UART driver
	 * supports the interrupt API, not necessarily the monitor UART's.
	 * Fall back to polling instead of failing, so that a misconfigured
	 * board still produces monitor output.
	 */
	if (uart_irq_callback_user_data_set(monitor_dev, monitor_uart_isr, NULL) != 0) {
		poll_mode = true;
	}
#endif /* CONFIG_BT_DEBUG_MONITOR_UART_INTERRUPT_DRIVEN */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_rx_disable(monitor_dev);
	uart_irq_tx_disable(monitor_dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#endif /* CONFIG_BT_DEBUG_MONITOR_UART */

#if !defined(CONFIG_UART_CONSOLE) && !defined(CONFIG_RTT_CONSOLE) && !defined(CONFIG_LOG_PRINTK)
	__printk_hook_install(monitor_console_out);
	__stdout_hook_install(monitor_console_out);
#endif /* !CONFIG_UART_CONSOLE && !CONFIG_RTT_CONSOLE && !CONFIG_LOG_PRINTK */

	return 0;
}

SYS_INIT(bt_monitor_init, PRE_KERNEL_1, MONITOR_INIT_PRIORITY);
