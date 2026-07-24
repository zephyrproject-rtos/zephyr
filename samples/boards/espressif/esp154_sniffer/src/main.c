/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <sys/errno.h>
LOG_MODULE_REGISTER(sniffer, LOG_LEVEL_INF);

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_pkt.h>
#include "../../subsys/net/ip/net_private.h"
#include <zephyr/net/ieee802154_pkt.h>
#include <zephyr/net/ieee802154_radio.h>
#include "../../subsys/net/l2/ieee802154/ieee802154_utils.h"
#include <zephyr/sys/printk.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <stdarg.h>
#include <string.h>
#include "sniffer.h"

#define DEFAULT_CHANNEL 12
#define SNIFFER_STREAM_MAGIC 0xA154
#define SNIFFER_STREAM_VERSION 1
#define SNIFFER_STREAM_Q_DEPTH 16
#define SNIFFER_STREAM_STACK_SIZE 2048
#define SNIFFER_STREAM_PRIO 7

#if defined(IEEE802154_MAX_PHY_PACKET_SIZE)
#define SNIFFER_MAX_FRAME_LEN IEEE802154_MAX_PHY_PACKET_SIZE
#else
#define SNIFFER_MAX_FRAME_LEN 127
#endif

struct sniffer_capture_msg {
	uint16_t len;
	uint8_t lqi;
	int16_t rssi_dbm;
	uint8_t channel;
	uint8_t flags;
	uint64_t timestamp_ns;
	uint8_t payload[SNIFFER_MAX_FRAME_LEN];
};

struct __packed sniffer_stream_hdr {
	uint16_t magic;
	uint8_t version;
	uint8_t hdr_len;
	uint16_t payload_len;
	uint8_t lqi;
	int16_t rssi_dbm;
	uint8_t channel;
	uint8_t flags;
	uint64_t timestamp_ns;
};

static struct ieee802154_radio_api *radio_api;
static const struct device *const radio = DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));
static enum ieee802154_hw_caps caps;
static uint16_t current_channel;

#if DT_HAS_CHOSEN(zephyr_console)
static const struct device *const stream_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
#else
static const struct device *const stream_uart;
#endif

K_MSGQ_DEFINE(sniffer_capture_q, sizeof(struct sniffer_capture_msg), SNIFFER_STREAM_Q_DEPTH, 4);

enum sniffer_output_mode {
	SNIFFER_OUTPUT_OFF = 0,
	SNIFFER_OUTPUT_ASCII,
	SNIFFER_OUTPUT_BINARY,
};

static atomic_t output_mode = ATOMIC_INIT(SNIFFER_OUTPUT_BINARY);
static atomic_t stat_rx_enqueued;
static atomic_t stat_rx_dropped;
static atomic_t stat_tx_records;
static atomic_t stat_tx_bytes;
static atomic_t stat_q_high_wm;

static bool sniffer_binary_enabled(void)
{
	return (enum sniffer_output_mode)atomic_get(&output_mode) == SNIFFER_OUTPUT_BINARY;
}

static bool sniffer_ascii_enabled(void)
{
	return (enum sniffer_output_mode)atomic_get(&output_mode) == SNIFFER_OUTPUT_ASCII;
}

static void sniffer_set_output_mode(enum sniffer_output_mode mode)
{
	atomic_set(&output_mode, (atomic_val_t)mode);
}

static uint16_t sniffer_checksum16(const uint8_t *data, size_t len)
{
	uint32_t sum = 0U;

	for (size_t i = 0; i < len; i++) {
		sum += data[i];
	}

	return (uint16_t)sum;
}

static void sniffer_stream_write(const uint8_t *buf, size_t len)
{
	if (stream_uart == NULL || !device_is_ready(stream_uart)) {
		return;
	}

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(stream_uart, buf[i]);
	}
}

static void sniffer_stream_puts(const char *s)
{
	sniffer_stream_write((const uint8_t *)s, strlen(s));
}

static void sniffer_ascii_printf(const char *fmt, ...)
{
	char line[160];
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsnprintk(line, sizeof(line), fmt, ap);
	va_end(ap);

	if (len <= 0) {
		return;
	}

	if ((size_t)len > sizeof(line)) {
		len = sizeof(line);
	}

	sniffer_stream_write((const uint8_t *)line, (size_t)len);
}

static void sniffer_ascii_hexdump(const uint8_t *data, size_t len)
{
	static const char hex[] = "0123456789abcdef";

	for (size_t i = 0; i < len; i++) {
		char b[3];

		b[0] = hex[(data[i] >> 4) & 0x0F];
		b[1] = hex[data[i] & 0x0F];
		b[2] = ' ';
		//b[2] = (i + 1U) < len ? ' ' : '\n';
		sniffer_stream_write((const uint8_t *)b, sizeof(b));
	}

	if (len == 0U) {
		sniffer_stream_puts("\r\n");
	}
}

static void sniffer_export_one(const struct sniffer_capture_msg *msg)
{
	struct sniffer_stream_hdr hdr = {
		.magic = sys_cpu_to_le16(SNIFFER_STREAM_MAGIC),
		.version = SNIFFER_STREAM_VERSION,
		.hdr_len = sizeof(struct sniffer_stream_hdr),
		.payload_len = sys_cpu_to_le16(msg->len),
		.lqi = msg->lqi,
		.rssi_dbm = sys_cpu_to_le16((uint16_t)msg->rssi_dbm),
		.channel = msg->channel,
		.flags = msg->flags,
		.timestamp_ns = sys_cpu_to_le64(msg->timestamp_ns),
	};
	uint16_t checksum;
	uint16_t checksum_le;

	sniffer_stream_write((const uint8_t *)&hdr, sizeof(hdr));
	sniffer_stream_write(msg->payload, msg->len);

	checksum = sniffer_checksum16((const uint8_t *)&hdr, sizeof(hdr));
	checksum += sniffer_checksum16(msg->payload, msg->len);
	checksum_le = sys_cpu_to_le16(checksum);
	sniffer_stream_write((const uint8_t *)&checksum_le, sizeof(checksum_le));

	atomic_inc(&stat_tx_records);
	atomic_add(&stat_tx_bytes, sizeof(hdr) + msg->len + sizeof(checksum_le));
}

static void sniffer_stream_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct sniffer_capture_msg msg;

	while (1) {
		k_msgq_get(&sniffer_capture_q, &msg, K_FOREVER);
		sniffer_export_one(&msg);
	}
}

K_THREAD_DEFINE(sniffer_stream_tid, SNIFFER_STREAM_STACK_SIZE, sniffer_stream_thread, NULL, NULL,
		NULL, K_PRIO_PREEMPT(SNIFFER_STREAM_PRIO), 0, 0);

int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	struct sniffer_capture_msg msg = { 0 };
	size_t len = net_pkt_get_len(pkt);
	int ret;

	if (len > SNIFFER_MAX_FRAME_LEN) {
		atomic_inc(&stat_rx_dropped);
		net_pkt_unref(pkt);
		return 0;
	}

	msg.flags = 0;
	msg.len = (uint16_t)len;
	msg.lqi = net_pkt_ieee802154_lqi(pkt);
	msg.rssi_dbm = net_pkt_ieee802154_rssi_dbm(pkt);
	msg.channel = (uint8_t)current_channel;
	msg.timestamp_ns = net_pkt_timestamp_ns(pkt);
	net_pkt_cursor_init(pkt);
	ret = net_pkt_read(pkt, msg.payload, len);
	if (ret < 0) {
		atomic_inc(&stat_rx_dropped);
		net_pkt_unref(pkt);
		return 0;
	}

	if (sniffer_ascii_enabled()) {
		if (msg.rssi_dbm == IEEE802154_MAC_RSSI_DBM_UNDEFINED) {
			sniffer_ascii_printf("RX len=%u ch=%u lqi=%u rssi=undef ts=%llu ns ",
					     msg.len, msg.channel, msg.lqi,
					     (unsigned long long)msg.timestamp_ns);
		} else {
			sniffer_ascii_printf("RX len=%u ch=%u lqi=%u rssi=%d dBm ts=%llu ns ",
					     msg.len, msg.channel, msg.lqi, msg.rssi_dbm,
					     (unsigned long long)msg.timestamp_ns);
		}
		sniffer_stream_puts("PSDU: ");
		sniffer_ascii_hexdump(msg.payload, msg.len);
	}

	if (sniffer_binary_enabled()) {
		ret = k_msgq_put(&sniffer_capture_q, &msg, K_NO_WAIT);
		if (ret != 0) {
			atomic_inc(&stat_rx_dropped);
		} else {
			uint32_t q_used = k_msgq_num_used_get(&sniffer_capture_q);
			uint32_t q_hwm = atomic_get(&stat_q_high_wm);

			atomic_inc(&stat_rx_enqueued);
			while (q_used > q_hwm &&
			       !atomic_cas(&stat_q_high_wm, (atomic_val_t)q_hwm,
					   (atomic_val_t)q_used)) {
				q_hwm = atomic_get(&stat_q_high_wm);
			}
		}
	}

	net_pkt_unref(pkt);
	return 0;
}

uint16_t sniffer_get_channel(void)
{
	return current_channel;
}

int sniffer_set_channel(uint16_t channel)
{
	if (!radio_api || !radio_api->set_channel) {
		return -ENOTSUP;
	}

	int err = radio_api->set_channel(radio, channel);
	if (err == 0) {
		current_channel = channel;
	}

	return err;
}

int sniffer_start(void)
{
	if (!radio_api || !radio_api->start) {
		return -ENOTSUP;
	}

	return radio_api->start(radio);
}

int sniffer_stop(void)
{
	if (!radio_api) {
		return -ENOTSUP;
	}

	if (radio_api->stop) {
		return radio_api->stop(radio);
	}
#if 0
	/* Some drivers expose sleep() instead of stop(). */
	if (radio_api->sleep) {
		return radio_api->sleep(radio);
	}
#endif
	return -ENOTSUP;
}

void sniffer_stream_set_enabled(bool enabled)
{
	ARG_UNUSED(enabled);
}

bool sniffer_stream_is_enabled(void)
{
	return false;
}

void sniffer_stream_get_stats(struct sniffer_stream_stats *stats)
{
	if (stats == NULL) {
		return;
	}

	stats->rx_enqueued = (uint32_t)atomic_get(&stat_rx_enqueued);
	stats->rx_dropped = (uint32_t)atomic_get(&stat_rx_dropped);
	stats->tx_records = (uint32_t)atomic_get(&stat_tx_records);
	stats->tx_bytes = (uint32_t)atomic_get(&stat_tx_bytes);
	stats->q_high_wm = (uint32_t)atomic_get(&stat_q_high_wm);
}

void sniffer_stream_reset_stats(void)
{
	atomic_clear(&stat_rx_enqueued);
	atomic_clear(&stat_rx_dropped);
	atomic_clear(&stat_tx_records);
	atomic_clear(&stat_tx_bytes);
	atomic_clear(&stat_q_high_wm);
}

uint32_t sniffer_stream_queue_used(void)
{
	return k_msgq_num_used_get(&sniffer_capture_q);
}

uint32_t sniffer_stream_queue_capacity(void)
{
	return SNIFFER_STREAM_Q_DEPTH;
}

int main(void)
{
	int ret;

	LOG_INF("IEEE 802.15.4 sniffer for ESP SoCs");

	/* Radio setup start */
	LOG_INF("Initialize radio...");

	if (!device_is_ready(radio)) {
		LOG_ERR("Radio not ready");
		return -1;
	}

	radio_api = (struct ieee802154_radio_api *)radio->api;
	if (!radio_api || !radio_api->start || !radio_api->set_channel) {
		LOG_ERR("IEEE 802.15.4 driver API incomplete");
		return -1;
	}

	if (radio_api->configure && radio_api->get_capabilities) {
		caps = radio_api->get_capabilities(radio);

		LOG_INF("Radio caps: 0x%x", (unsigned int)caps);

		if (caps & IEEE802154_HW_PROMISC) {
			struct ieee802154_config cfg = { 0 };

			cfg.promiscuous = true;
			if (radio_api->configure(radio, IEEE802154_CONFIG_PROMISCUOUS, &cfg)) {
				LOG_WRN("Promiscuous mode enable failed");
			} else {
				LOG_INF("Promiscuous mode enabled");
			}
		} else {
			LOG_WRN("Driver does not allow IEEE802154_HW_PROMISC");
		}
	}

#if defined(CONFIG_NET_CONFIG_IEEE802154_CHANNEL)
	current_channel = CONFIG_NET_CONFIG_IEEE802154_CHANNEL;
#else
	current_channel = DEFAULT_CHANNEL;
#endif

	if (sniffer_set_channel(current_channel)) {
		LOG_WRN("Failed to set channel %d", current_channel);
	} else {
		LOG_INF("Channel set to %d", current_channel);
	}

	if (stream_uart == NULL || !device_is_ready(stream_uart)) {
		LOG_WRN("Stream UART unavailable; capture stream output disabled");
	}

#if defined(CONFIG_ESP154_SNIFFER_OUTPUT_MODE_ASCII)
	sniffer_set_output_mode(SNIFFER_OUTPUT_ASCII);
#elif defined(CONFIG_ESP154_SNIFFER_OUTPUT_MODE_BINARY)
	if (stream_uart == NULL || !device_is_ready(stream_uart)) {
		sniffer_set_output_mode(SNIFFER_OUTPUT_OFF);
	} else {
		sniffer_set_output_mode(SNIFFER_OUTPUT_BINARY);
	}
#else
	sniffer_set_output_mode(SNIFFER_OUTPUT_OFF);
#endif

	LOG_INF("Output mode: binary=%s ascii=%s", sniffer_binary_enabled() ? "on" : "off",
		sniffer_ascii_enabled() ? "on" : "off");

	net_pkt_init();

	ret = sniffer_start();
	if (ret && ret != -EALREADY) {
		LOG_ERR("Failed to start radio (%d)", ret);
		return -1;
	}

	LOG_INF("IEEE 802.15.4 sniffer started");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
