/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DMX512-A input sample.
 *
 * Receives DMX frames on a DMX interface and dispatches by start code.
 * Handles NULL SC (dimmer data), test (0x55), ASCII text (0x17), UTF-8 text (0x90), manufacturer
 * (0x91), and SIP (0xCF) packets. Unknown start codes are logged with a raw hex dump.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dmx/dmx.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dmx_sample, LOG_LEVEL_INF);

static const struct device *const dmx_dev = DEVICE_DT_GET(DT_NODELABEL(dmx0));

#if DT_NODE_EXISTS(DT_ALIAS(led0))
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#endif

static uint8_t prev_null_data[DMX_MAX_SLOTS];
static uint16_t prev_null_slot_count;

static void handle_null_packet(const struct dmx_null_packet *pkt)
{
	bool changed = (pkt->slot_count != prev_null_slot_count) ||
		       memcmp(pkt->data, prev_null_data, pkt->slot_count) != 0;

	LOG_INF("NULL SC @ %u ms, slots 1-%u%s",
		pkt->hdr.timestamp,
		pkt->slot_count,
		changed ? " [changed]" : "");

	if (changed) {
		LOG_HEXDUMP_INF(pkt->data, pkt->slot_count, "slot data");
		memcpy(prev_null_data, pkt->data, pkt->slot_count);
		prev_null_slot_count = pkt->slot_count;
	}
}

static void handle_test_packet(const struct dmx_test_packet *pkt)
{
	if (pkt->pass) {
		LOG_INF("Test packet @ %u ms: PASS", pkt->hdr.timestamp);
	} else if (pkt->slot_count != DMX_MAX_SLOTS) {
		LOG_WRN("Test packet @ %u ms: FAIL (incomplete, %u/%u slots)",
			pkt->hdr.timestamp, pkt->slot_count, DMX_MAX_SLOTS);
	} else {
		LOG_WRN("Test packet @ %u ms: FAIL (%d/%u slot errors)",
			pkt->hdr.timestamp, pkt->slot_errors, pkt->slot_count);
	}
}

static void print_dmx_text(const uint8_t *text, uint16_t len, uint8_t chars_per_line)
{
	if (len == 0) {
		return;
	}

	/* Find null terminator — text may be shorter than len. */
	uint16_t text_len = 0;

	while (text_len < len && text[text_len] != '\0') {
		text_len++;
	}

	if (chars_per_line == 0 || chars_per_line >= text_len) {
		/* No line wrapping. */
		LOG_INF("  %.*s", text_len, text);
		return;
	}

	for (uint16_t pos = 0; pos < text_len; pos += chars_per_line) {
		uint16_t remaining = text_len - pos;
		uint16_t line_len = MIN(remaining, chars_per_line);

		LOG_INF("  %.*s", line_len, &text[pos]);
	}
}

static void handle_text_packet(const struct dmx_text_packet *pkt)
{
	uint16_t text_slots = (pkt->slot_count >= DMX_TEXT_MIN_SLOTS)
		? (pkt->slot_count - (DMX_TEXT_MIN_SLOTS - 1)) : 0;

	LOG_INF("ASCII text @ %u ms: page=%u, chars/line=%u, %u slots",
		pkt->hdr.timestamp, pkt->page, pkt->chars_per_line, pkt->slot_count);
	print_dmx_text(pkt->text, text_slots, pkt->chars_per_line);
}

static void handle_utf8_packet(const struct dmx_utf8_packet *pkt)
{
	uint16_t text_slots = (pkt->slot_count >= DMX_TEXT_MIN_SLOTS)
		? (pkt->slot_count - (DMX_TEXT_MIN_SLOTS - 1)) : 0;

	LOG_INF("UTF-8 text @ %u ms: page=%u, chars/line=%u, %u slots",
		pkt->hdr.timestamp, pkt->page, pkt->chars_per_line, pkt->slot_count);
	print_dmx_text(pkt->text, text_slots, pkt->chars_per_line);
}

static void handle_manufacturer_packet(const struct dmx_manufacturer_packet *pkt)
{
	uint16_t data_len = (pkt->slot_count > DMX_MANUFACTURER_MIN_SLOTS)
		? (pkt->slot_count - DMX_MANUFACTURER_MIN_SLOTS) : 0;

	LOG_INF("Manufacturer packet @ %u ms: ID=0x%04x, %u data bytes",
		pkt->hdr.timestamp, pkt->manufacturer_id, data_len);
	if (data_len > 0) {
		LOG_HEXDUMP_INF(pkt->data, data_len, "manufacturer data");
	}
}

static void handle_sip_packet(const struct dmx_sip_packet *pkt)
{
	LOG_INF("SIP @ %u ms: checksum %s",
		pkt->hdr.timestamp,
		pkt->checksum_valid ? "OK" : "FAIL");
	LOG_INF("  seq=%u, universe=%u, processing_level=%u, sw_version=%u",
		pkt->sequence, pkt->universe, pkt->processing_level, pkt->sw_version);
	LOG_INF("  sip_length=%u, control=0x%02x, prev_checksum=0x%04x",
		pkt->sip_length, pkt->control, pkt->prev_checksum);
	LOG_INF("  std_pkt_len=%u, num_packets=%u",
		pkt->std_packet_len, pkt->num_packets);
	LOG_INF("  mfr_ids: 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x",
		pkt->manufacturer_ids[0], pkt->manufacturer_ids[1],
		pkt->manufacturer_ids[2], pkt->manufacturer_ids[3],
		pkt->manufacturer_ids[4]);
}

static void handle_raw_packet(const struct dmx_raw_packet *pkt)
{
	LOG_INF("Unknown SC=0x%02x @ %u ms, %u slots",
		pkt->hdr.start_code, pkt->hdr.timestamp, pkt->slot_count);
	LOG_HEXDUMP_DBG(pkt->data, pkt->slot_count, "raw data");
}

int main(void)
{
	int ret;
	const struct dmx_rx_header *hdr;
	struct dmx_filter filter = {
		.sc_null = true,
		.sc_text = true,
		.sc_test = true,
		.sc_utf8 = true,
		.sc_manufacturer = true,
		.sc_sip = true,
		.sc_other = true,
	};

	if (!device_is_ready(dmx_dev)) {
		LOG_ERR("DMX device not ready");
		return -ENODEV;
	}

#if DT_NODE_EXISTS(DT_ALIAS(led0))
	if (gpio_is_ready_dt(&led)) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	}
#endif

	ret = dmx_set_mode(dmx_dev, DMX_MODE_INPUT);
	if (ret != 0) {
		LOG_ERR("dmx_set_mode failed: %d", ret);
		return ret;
	}

	ret = dmx_set_filter(dmx_dev, &filter);
	if (ret != 0) {
		LOG_ERR("dmx_set_filter failed: %d", ret);
		return ret;
	}

	ret = dmx_enable(dmx_dev);
	if (ret != 0) {
		LOG_ERR("dmx_enable failed: %d", ret);
		return ret;
	}

	LOG_INF("DMX input started, capturing full universe");

	while (true) {
		/* 100ms timeout so the LED turns off promptly when no packets are
		 * received. Use K_FOREVER if the LED indicator is not needed.
		 */
		ret = dmx_rx_claim(dmx_dev, &hdr, K_MSEC(100));
		if (ret == -EAGAIN) {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
			if (gpio_is_ready_dt(&led)) {
				gpio_pin_set_dt(&led, 0);
			}
#endif
			continue;
		}
		if (ret != 0) {
			LOG_ERR("dmx_rx_claim error: %d", ret);
			break;
		}

		switch (hdr->start_code) {
		case DMX_SC_NULL:
			handle_null_packet((const struct dmx_null_packet *)hdr);
			break;
		case DMX_SC_TEST:
			handle_test_packet((const struct dmx_test_packet *)hdr);
			break;
		case DMX_SC_TEXT:
			handle_text_packet((const struct dmx_text_packet *)hdr);
			break;
		case DMX_SC_UTF8:
			handle_utf8_packet((const struct dmx_utf8_packet *)hdr);
			break;
		case DMX_SC_MANUFACTURER:
			handle_manufacturer_packet((const struct dmx_manufacturer_packet *)hdr);
			break;
		case DMX_SC_SIP:
			handle_sip_packet((const struct dmx_sip_packet *)hdr);
			break;
		default:
			handle_raw_packet((const struct dmx_raw_packet *)hdr);
			break;
		}

#if DT_NODE_EXISTS(DT_ALIAS(led0))
		if (gpio_is_ready_dt(&led)) {
			gpio_pin_toggle_dt(&led);
		}
#endif
		dmx_rx_free(dmx_dev, hdr);
	}

	return 0;
}
