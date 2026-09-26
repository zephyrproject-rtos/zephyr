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
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_pkt.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include "sniffer.h"

#define DEFAULT_CHANNEL 20
#define SNIFFER_STREAM_Q_DEPTH 16
#define SNIFFER_STREAM_STACK_SIZE 2048
#define SNIFFER_STREAM_PRIO 7

/* "received: " + 2 hex digits per PSDU byte + " power: -32768 lqi: 255 time: 4294967295\r\n" */
#define SNIFFER_LINE_PREFIX "received: "
#define SNIFFER_LINE_TAIL_MAX 48

struct sniffer_capture_msg {
	uint16_t len;
	uint8_t lqi;
	int16_t rssi_dbm;
	uint32_t timestamp_us;
	uint8_t payload[IEEE802154_MAX_PHY_PACKET_SIZE];
};

static struct ieee802154_radio_api *radio_api;
static const struct device *const radio = DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));
static enum ieee802154_hw_caps caps;
static uint16_t current_channel;

/* Guards the capture state machine against concurrent shell commands. */
static K_MUTEX_DEFINE(sniffer_lock);
static bool sniffer_running;

#if DT_HAS_CHOSEN(zephyr_console)
static const struct device *const stream_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
#else
static const struct device *const stream_uart;
#endif

K_MSGQ_DEFINE(sniffer_capture_q, sizeof(struct sniffer_capture_msg), SNIFFER_STREAM_Q_DEPTH, 4);

static atomic_t stat_rx_enqueued;
static atomic_t stat_rx_dropped;
static atomic_t stat_tx_records;
static atomic_t stat_tx_bytes;
static atomic_t stat_q_high_wm;

static void sniffer_stream_write(const uint8_t *buf, size_t len)
{
	if (stream_uart == NULL || !device_is_ready(stream_uart)) {
		return;
	}

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(stream_uart, buf[i]);
	}
}

/*
 * One capture line, as consumed by the extcap plugin:
 *
 *   received: <psdu-hex> power: <rssi-dbm> lqi: <lqi> time: <us>
 *
 * The hex digits are streamed out byte by byte rather than formatted into a
 * buffer: a maximum-length PSDU is 254 characters on its own.
 */
static void sniffer_emit_frame(const struct sniffer_capture_msg *msg)
{
	static const char hex[] = "0123456789abcdef";
	char tail[SNIFFER_LINE_TAIL_MAX];
	int len;

	sniffer_stream_write((const uint8_t *)SNIFFER_LINE_PREFIX,
			     sizeof(SNIFFER_LINE_PREFIX) - 1U);

	for (size_t i = 0; i < msg->len; i++) {
		char nibbles[2];

		nibbles[0] = hex[(msg->payload[i] >> 4) & 0x0FU];
		nibbles[1] = hex[msg->payload[i] & 0x0FU];
		sniffer_stream_write((const uint8_t *)nibbles, sizeof(nibbles));
	}

	/*
	 * An unavailable RSSI is reported as IEEE802154_MAC_RSSI_DBM_UNDEFINED
	 * (INT16_MIN) rather than omitted, so every line has the same shape.
	 */
	len = snprintk(tail, sizeof(tail), " power: %d lqi: %u time: %u\r\n", msg->rssi_dbm,
		       msg->lqi, msg->timestamp_us);
	if (len <= 0) {
		return;
	}

	if ((size_t)len >= sizeof(tail)) {
		len = sizeof(tail) - 1;
	}

	sniffer_stream_write((const uint8_t *)tail, (size_t)len);

	atomic_inc(&stat_tx_records);
	atomic_add(&stat_tx_bytes, sizeof(SNIFFER_LINE_PREFIX) - 1U + 2U * msg->len + (size_t)len);
}

static void sniffer_stream_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct sniffer_capture_msg msg;

	while (1) {
		k_msgq_get(&sniffer_capture_q, &msg, K_FOREVER);
		sniffer_emit_frame(&msg);
	}
}

K_THREAD_DEFINE(sniffer_stream_tid, SNIFFER_STREAM_STACK_SIZE, sniffer_stream_thread, NULL, NULL,
		NULL, K_PRIO_PREEMPT(SNIFFER_STREAM_PRIO), 0, 0);

/*
 * Raw mode (CONFIG_IEEE802154_RAW_MODE) leaves the net stack out of the build,
 * so the driver's receive path lands here directly. Keep this function short:
 * it runs in the driver's RX thread, and serializing a line takes milliseconds.
 */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	struct sniffer_capture_msg msg = { 0 };
	size_t len = net_pkt_get_len(pkt);
	uint32_t q_used;
	uint32_t q_hwm;
	int ret;

	if (len > IEEE802154_MAX_PHY_PACKET_SIZE) {
		atomic_inc(&stat_rx_dropped);
		net_pkt_unref(pkt);
		return 0;
	}

	msg.len = (uint16_t)len;
	msg.lqi = net_pkt_ieee802154_lqi(pkt);
	msg.rssi_dbm = net_pkt_ieee802154_rssi_dbm(pkt);
	msg.timestamp_us = (uint32_t)(net_pkt_timestamp_ns(pkt) / NSEC_PER_USEC);
	net_pkt_cursor_init(pkt);
	ret = net_pkt_read(pkt, msg.payload, len);
	net_pkt_unref(pkt);

	if (ret < 0) {
		atomic_inc(&stat_rx_dropped);
		return 0;
	}

	ret = k_msgq_put(&sniffer_capture_q, &msg, K_NO_WAIT);
	if (ret != 0) {
		atomic_inc(&stat_rx_dropped);
		return 0;
	}

	atomic_inc(&stat_rx_enqueued);

	q_used = k_msgq_num_used_get(&sniffer_capture_q);
	q_hwm = atomic_get(&stat_q_high_wm);
	while (q_used > q_hwm &&
	       !atomic_cas(&stat_q_high_wm, (atomic_val_t)q_hwm, (atomic_val_t)q_used)) {
		q_hwm = atomic_get(&stat_q_high_wm);
	}

	return 0;
}

uint16_t sniffer_get_channel(void)
{
	return current_channel;
}

int sniffer_set_channel(uint16_t channel)
{
	int err;

	if (!radio_api || !radio_api->set_channel) {
		return -ENOTSUP;
	}

	k_mutex_lock(&sniffer_lock, K_FOREVER);

	/* The host tags the capture with the channel it asked for, so only retune while stopped. */
	if (sniffer_running) {
		k_mutex_unlock(&sniffer_lock);
		return -EBUSY;
	}

	err = radio_api->set_channel(radio, channel);
	if (err == 0) {
		current_channel = channel;
	}

	k_mutex_unlock(&sniffer_lock);

	return err;
}

int sniffer_start(void)
{
	int err;

	if (!radio_api || !radio_api->start) {
		return -ENOTSUP;
	}

	k_mutex_lock(&sniffer_lock, K_FOREVER);

	if (sniffer_running) {
		k_mutex_unlock(&sniffer_lock);
		return -EALREADY;
	}

	err = radio_api->start(radio);
	if (err == 0 || err == -EALREADY) {
		sniffer_running = true;
		err = 0;
	}

	k_mutex_unlock(&sniffer_lock);

	return err;
}

int sniffer_stop(void)
{
	int err;

	if (!radio_api || !radio_api->stop) {
		return -ENOTSUP;
	}

	k_mutex_lock(&sniffer_lock, K_FOREVER);

	if (!sniffer_running) {
		k_mutex_unlock(&sniffer_lock);
		return -EALREADY;
	}

	err = radio_api->stop(radio);
	if (err == 0 || err == -EALREADY) {
		sniffer_running = false;
		err = 0;
	}

	k_mutex_unlock(&sniffer_lock);

	return err;
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
		LOG_WRN("Stream UART unavailable; capture output disabled");
	}

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
