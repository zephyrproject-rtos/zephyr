/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/printk.h>

#define RC_NODE DT_ALIAS(rc0)

static const char *proto_name(uint16_t proto)
{
	switch (proto) {
	case RC_PROTO_NEC:
		return "nec";
	case RC_PROTO_NECX:
		return "necx";
	case RC_PROTO_NEC32:
		return "nec32";
	default:
		return "unknown";
	}
}

static const char *key_state(uint16_t flags)
{
	if ((flags & RC_EVENT_FLAG_RELEASE) != 0U) {
		return "release";
	}
	if ((flags & RC_EVENT_FLAG_REPEAT) != 0U) {
		return "repeat";
	}

	return "press";
}

static void rc_event_cb(const struct device *dev, const struct rc_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	printk("%s: %s scancode 0x%04x %s\n", dev->name, proto_name(evt->proto), evt->scancode,
	       key_state(evt->flags));
}

RC_CALLBACK_DEFINE(DEVICE_DT_GET(RC_NODE), rc_event_cb, NULL);

#if defined(CONFIG_SAMPLE_RC_RAW)
RC_DT_RAW_IODEV(raw_iodev, RC_NODE);
RTIO_DEFINE_WITH_MEMPOOL(raw_ctx, 4, 4, 8, 64, sizeof(uint32_t));

static void print_word(uint32_t word)
{
	if ((word & RC_RAW_OVERFLOW) != 0U) {
		printk("raw: overflow\n");
	} else if ((word & RC_RAW_TIMEOUT) != 0U) {
		printk("raw: timeout %u\n", rc_raw_duration_us(word));
	} else {
		printk("raw: %s %u\n", rc_raw_is_pulse(word) ? "pulse" : "space",
		       rc_raw_duration_us(word));
	}
}

static void print_raw(void)
{
	while (true) {
		struct rtio_cqe *cqe = rtio_cqe_consume_block(&raw_ctx);
		uint8_t *buf = NULL;
		uint32_t len = 0U;

		if ((cqe->result > 0) &&
		    (rtio_cqe_get_mempool_buffer(&raw_ctx, cqe, &buf, &len) == 0)) {
			for (int i = 0; i < cqe->result; i++) {
				uint32_t word;

				memcpy(&word, &buf[i * sizeof(word)], sizeof(word));
				print_word(word);
			}
			rtio_release_buffer(&raw_ctx, buf, len);
		}
		rtio_cqe_release(&raw_ctx, cqe);
	}
}
#endif

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(RC_NODE);
	int ret;

	if (!device_is_ready(dev)) {
		printk("%s not ready\n", dev->name);
		return 0;
	}

	ret = rc_protocol_enable(dev, RC_PROTO_NEC);
	if (ret != 0) {
		printk("enable NEC failed: %d\n", ret);
		return 0;
	}

#if defined(CONFIG_SAMPLE_RC_RAW)
	ret = rc_stream(&raw_iodev, &raw_ctx, NULL, NULL);
	if (ret != 0) {
		printk("raw stream failed: %d\n", ret);
		return 0;
	}
#endif

	ret = rc_rx_enable(dev, true);
	if (ret != 0) {
		printk("rx enable failed: %d\n", ret);
		return 0;
	}

	printk("rc receive sample on %s\n", dev->name);

#ifdef CONFIG_SAMPLE_RC_SELFTEST
	const struct rc_tx_req req = {
		.proto = RC_PROTO_NEC,
		.scancode = 0x0102U,
	};

	ret = rc_send_sync(dev, &req, K_SECONDS(1));
	if (ret != 0) {
		printk("selftest send failed: %d\n", ret);
	}
#endif

#if defined(CONFIG_SAMPLE_RC_RAW)
	print_raw();
#endif

	return 0;
}
