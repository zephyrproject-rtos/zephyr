/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * libFuzzer harness for the MQTT client's receive path.
 *
 * A custom transport hands the fuzz case to the client as bytes from the
 * broker, and mqtt_input() is driven the way an application's poll loop
 * would. This exercises the real length parsing in mqtt_rx.c (remaining
 * length, UTF-8 string lengths, 5.0 property length) instead of feeding a
 * decoder a buffer of the harness's own choosing.
 *
 * A CONNACK is served before the fuzz case so the stream reaches the
 * packet types worth fuzzing; see mqtt_client_custom_transport_read().
 *
 * See README.rst for how to build and run it.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_BOARD_NATIVE_SIM)
#include <nsi_cpu_if.h>
#include <nsi_main_semipublic.h>
#else
#error "Platform not supported"
#endif

/* Bounds a single packet: a message the client cannot buffer is refused
 * rather than parsed. In-tree buffers range from 64 (MQTT shell) to 1024
 * (Azure sample); 256 is what a finding has to fit in here.
 */
#define FUZZ_RX_BUF_SIZE 256
#define FUZZ_TX_BUF_SIZE 256

/* Beyond the buffer the stream is only more packets, already reached by
 * shorter cases, so the case length just follows the buffer size.
 */
#define FUZZ_MAX_INPUT FUZZ_RX_BUF_SIZE

/* The peer running out of data ends every case (transport returns 0,
 * mqtt_rx.c turns it into -ENOTCONN) well before this cap; it only
 * guards against a future transport change turning the loop into a hang.
 */
#define FUZZ_MAX_INPUT_ITERATIONS FUZZ_MAX_INPUT

/* Read back in the event callback the way an application does. */
#define FUZZ_PAYLOAD_CHUNK 64

/* The stream the peer sends: a CONNACK, then the fuzz case. */
static const uint8_t *stream_prologue;
static size_t stream_prologue_len;
static size_t stream_prologue_pos;
static const uint8_t *stream_case;
static size_t stream_case_len;
static size_t stream_case_pos;

/* Connection accepted, no session present; 5.0 form has an empty property list. */
static const uint8_t connack_3_1_1[] = {0x20, 0x02, 0x00, 0x00};
static const uint8_t connack_5_0[] = {0x20, 0x03, 0x00, 0x00, 0x00};

int main(void)
{
	/* Fuzz cases run from LLVMFuzzerTestOneInput(), outside the OS. */
	return 0;
}

static size_t stream_read(uint8_t *data, size_t buflen)
{
	size_t taken = 0;

	while (taken < buflen) {
		const uint8_t *src;
		size_t avail;

		if (stream_prologue_pos < stream_prologue_len) {
			src = stream_prologue + stream_prologue_pos;
			avail = stream_prologue_len - stream_prologue_pos;
		} else if (stream_case_pos < stream_case_len) {
			src = stream_case + stream_case_pos;
			avail = stream_case_len - stream_case_pos;
		} else {
			break;
		}

		if (avail > buflen - taken) {
			avail = buflen - taken;
		}

		memcpy(data + taken, src, avail);
		taken += avail;

		if (stream_prologue_pos < stream_prologue_len) {
			stream_prologue_pos += avail;
		} else {
			stream_case_pos += avail;
		}
	}

	return taken;
}

int mqtt_client_custom_transport_connect(struct mqtt_client *c)
{
	ARG_UNUSED(c);

	return 0;
}

int mqtt_client_custom_transport_write(struct mqtt_client *c, const uint8_t *data, uint32_t datalen)
{
	ARG_UNUSED(c);
	ARG_UNUSED(data);

	/* Not part of the fuzz surface: the harness controls what is sent. */
	return (int)datalen;
}

int mqtt_client_custom_transport_write_msg(struct mqtt_client *c, const struct net_msghdr *message)
{
	ARG_UNUSED(c);
	ARG_UNUSED(message);

	return 0;
}

int mqtt_client_custom_transport_read(struct mqtt_client *c, uint8_t *data, uint32_t buflen,
				      bool shall_block)
{
	size_t taken;

	ARG_UNUSED(c);
	ARG_UNUSED(shall_block);

	taken = stream_read(data, buflen);
	if (taken == 0U) {
		/* Peer sent everything and closed: stream ended. */
		return 0;
	}

	/* Short reads are served as-is, the way a stream socket would. */
	return (int)taken;
}

int mqtt_client_custom_transport_disconnect(struct mqtt_client *c)
{
	ARG_UNUSED(c);

	return 0;
}

static void consume_topic(const struct mqtt_utf8 *topic)
{
	if (topic->utf8 == NULL || topic->size == 0U) {
		return;
	}

	/* Touch both ends: an overrun (or a bad 5.0 topic-alias
	 * resolution) is reported here, not in the copying application.
	 */
	volatile uint8_t sink = topic->utf8[0];

	sink = topic->utf8[topic->size - 1U];
	(void)sink;
}

static void consume_publish(struct mqtt_client *c, const struct mqtt_publish_param *param)
{
	uint8_t payload[FUZZ_PAYLOAD_CHUNK];
	uint32_t left = param->message.payload.len;

	consume_topic(&param->message.topic.topic);

	/* Reading from the event callback is what advances the client's
	 * remaining-payload accounting, as a real application would.
	 */
	while (left > 0U) {
		size_t chunk = left > sizeof(payload) ? sizeof(payload) : left;
		int ret = mqtt_read_publish_payload(c, payload, chunk);

		if (ret <= 0) {
			break;
		}

		left -= (uint32_t)ret;
	}
}

static void fuzz_evt_cb(struct mqtt_client *c, const struct mqtt_evt *evt)
{
	if (evt->result != 0) {
		return;
	}

	if (evt->type == MQTT_EVT_PUBLISH) {
		consume_publish(c, &evt->param.publish);
	}
}

static void run_one(uint8_t protocol_version, const uint8_t *connack, size_t connack_len)
{
	/* Allocated per run, not kept as globals: an overrun lands in
	 * guarded memory and is reported instead of the linker's next object.
	 */
	struct mqtt_client *client;
	uint8_t *rx_buf;
	uint8_t *tx_buf;

	stream_prologue = connack;
	stream_prologue_len = connack_len;
	stream_prologue_pos = 0U;
	stream_case_pos = 0U;

	client = calloc(1, sizeof(*client));
	rx_buf = calloc(1, FUZZ_RX_BUF_SIZE);
	tx_buf = calloc(1, FUZZ_TX_BUF_SIZE);

	if (client == NULL || rx_buf == NULL || tx_buf == NULL) {
		goto out;
	}

	mqtt_client_init(client);

	client->broker = NULL;
	client->evt_cb = fuzz_evt_cb;
	client->client_id.utf8 = (uint8_t *)"fuzz";
	client->client_id.size = 4U;
	client->protocol_version = protocol_version;
	client->transport.type = MQTT_TRANSPORT_CUSTOM;
	client->rx_buf = rx_buf;
	client->rx_buf_size = FUZZ_RX_BUF_SIZE;
	client->tx_buf = tx_buf;
	client->tx_buf_size = FUZZ_TX_BUF_SIZE;

	if (mqtt_connect(client) < 0) {
		goto out;
	}

	for (int i = 0; i < FUZZ_MAX_INPUT_ITERATIONS; i++) {
		if (mqtt_input(client) < 0) {
			/* Client refused the stream and closed. */
			goto out;
		}
	}

out:
	free(tx_buf);
	free(rx_buf);
	free(client);
}

/**
 * Entry point for fuzzing. The native simulator boots once; each case
 * then runs directly since the receive path needs no thread of its own.
 */
#if defined(CONFIG_BOARD_NATIVE_SIM)
NATIVE_SIMULATOR_IF /* We expose this function to the final runner link stage */
#endif
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static bool runner_initialized;

	if (!runner_initialized) {
		nsi_init(0, NULL);
		runner_initialized = true;
	}

	if (size > FUZZ_MAX_INPUT) {
		/* Skip rather than truncate, so a kept case matches what
		 * ran. Pass -max_len=256 to keep the mutator in range.
		 */
		return 0;
	}

	stream_case = data;
	stream_case_len = size;

	/* Protocol version is fixed at connect and the decoders diverge
	 * from the CONNACK onward, so offer the same stream to both.
	 */
	run_one(MQTT_VERSION_3_1_1, connack_3_1_1, sizeof(connack_3_1_1));
	run_one(MQTT_VERSION_5_0, connack_5_0, sizeof(connack_5_0));

	return 0;
}
