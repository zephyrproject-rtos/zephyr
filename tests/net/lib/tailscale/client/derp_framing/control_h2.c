/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/ztest.h>

#include "control_internal.h"

static const uint8_t map_body[] = "{\"KeepAlive\":true}";
static const uint8_t data_header[] = {
	0, 0, sizeof(map_body) - 1U, 0, 1, 0, 0, 0, 5,
};
static const char key_hex[] =
	"0101010101010101010101010101010101010101010101010101010101010101";
static struct control_h2 http2;
static uint8_t output[768];
static size_t written;
static bool publish;

int control_noise_read(struct control_noise *noise, void *data, size_t len)
{
	ARG_UNUSED(noise);

	if (len == sizeof(data_header)) {
		memcpy(data, data_header, len);
		return 0;
	}
	if (len != sizeof(map_body) - 1U || !publish) {
		return -EIO;
	}

	/* A normal endpoint update runs while the remaining map bytes are pending. */
	publish = false;
	size_t first = len / 2U;
	const struct control_h2_map_request request = {
		.node_hex = key_hex,
		.disco_hex = key_hex,
		.hostname = "test-node",
		.local_ipv4 = "192.0.2.1:41641",
		.local_ipv6 = "[2001:db8::1]:41641",
	};
	char scratch[512];

	memcpy(data, map_body, first);
	zassert_ok(control_h2_publish_endpoint(&http2, 7, &request, 1, scratch, sizeof(scratch)));
	memcpy((uint8_t *)data + first, map_body + first, len - first);
	return 0;
}

int control_noise_write(struct control_noise *noise, const void *data, size_t len)
{
	ARG_UNUSED(noise);

	if (len > sizeof(output) - written) {
		return -ENOSPC;
	}
	memcpy(output + written, data, len);
	written += len;
	return 0;
}

ZTEST(derp_framing, test_control_map_read_with_endpoint_publication)
{
	static struct control_noise noise;
	struct control_h2_body body = {.stream = 5};
	const uint8_t *data;
	size_t len;

	http2 = (struct control_h2){.noise = &noise};
	written = 0;
	publish = true;

	zassert_ok(control_h2_read_body(&http2, &body, &data, &len));
	zassert_equal(len, sizeof(map_body) - 1U);
	zassert_mem_equal(data, map_body, len);
	zassert_false(publish);
	zassert_true(written > sizeof(data_header));
	zassert_equal(output[3], 1U); /* HEADERS */
	zassert_equal(output[8], 7U);
}
