/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/toolchain.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include <irq_ctrl.h>
#if defined(CONFIG_BOARD_NATIVE_SIM)
#include <nsi_cpu_if.h>
#include <nsi_main_semipublic.h>
#else
#error "Platform not supported"
#endif

/* ---------------------------------------------------------------------------
 * Fuzz transport context
 *
 * The custom MQTT transport feeds bytes directly from the libFuzzer-provided
 * buffer instead of a real socket.  Two logical sub-buffers are maintained:
 *
 *   setup_buf  - injected before the fuzz data (used to deliver a synthetic
 *                CONNACK when operating in "connected" mode)
 *   fuzz_buf   - the actual libFuzzer-provided bytes
 *
 * Reads drain setup_buf first, then fuzz_buf.
 * ---------------------------------------------------------------------------
 */
struct fuzz_buf_ctx {
	/* Setup (synthetic) data injected before the fuzz payload */
	const uint8_t *setup_data;
	size_t         setup_remaining;

	/* Actual fuzz data */
	const uint8_t *fuzz_data;
	size_t         fuzz_remaining;
};

/* ---------------------------------------------------------------------------
 * A minimal valid MQTT 5.0 CONNACK: success, no session present, no props.
 *
 *   Byte 0: 0x20  - packet type CONNACK
 *   Byte 1: 0x03  - remaining length = 3
 *   Byte 2: 0x00  - connect acknowledge flags (no session present)
 *   Byte 3: 0x00  - connect reason code 0x00 = Success
 *   Byte 4: 0x00  - properties length = 0
 * ---------------------------------------------------------------------------
 */
static const uint8_t connack_success[] = { 0x20, 0x03, 0x00, 0x00, 0x00 };

K_SEM_DEFINE(fuzz_sem, 0, 1);

static struct mqtt_client g_client_ctx;
/* The custom transport ignores the broker address entirely; this is kept as a
 * zero-initialised placeholder to satisfy the mqtt_client broker pointer field.
 */
static struct net_sockaddr_storage broker;
static struct fuzz_buf_ctx g_fuzz_ctx;

#define MQTT_BUFFER_SIZE 4096

static uint8_t rx_buffer[MQTT_BUFFER_SIZE];
static uint8_t tx_buffer[MQTT_BUFFER_SIZE];

/* ---------------------------------------------------------------------------
 * MQTT event handler
 *
 * When a PUBLISH is decoded the library sets remaining_payload > 0 and
 * expects the application to drain it via mqtt_read_publish_payload().
 * Failing to do so causes every subsequent mqtt_input() call to return
 * -EBUSY, blocking all further packet parsing.  We drain it here so the
 * fuzzer can keep processing packets within the same fuzz iteration.
 * ---------------------------------------------------------------------------
 */
static uint8_t payload_drain_buf[MQTT_BUFFER_SIZE];

static void mqtt_evt_handler(struct mqtt_client *client, const struct mqtt_evt *evt)
{
	if (evt->type == MQTT_EVT_PUBLISH) {
		uint32_t remaining = evt->param.publish.message.payload.len;

		/* Drain the payload so mqtt_input() is not blocked on next call */
		while (remaining > 0) {
			uint32_t chunk = MIN(remaining,
					     (uint32_t)sizeof(payload_drain_buf));
			int ret = mqtt_read_publish_payload(
				client, payload_drain_buf, chunk);

			if (ret <= 0) {
				break;
			}
			remaining -= (uint32_t)ret;
		}
	}
}

/* ---------------------------------------------------------------------------
 * Custom transport callbacks
 *
 * All five symbols are mandatory: the MQTT library resolves them by name at
 * link time with no weak defaults and no NULL guards in the dispatch table
 * (mqtt_transport.c).  Omitting any one causes a linker error.
 *
 * connect / write / write_msg are intentional no-ops: the "connection" is
 * virtual (no real socket) and outbound packets are discarded because the
 * fuzzer targets inbound packet parsing only.
 * ---------------------------------------------------------------------------
 */
int mqtt_client_custom_transport_connect(struct mqtt_client *client)
{
	ARG_UNUSED(client);
	return 0;
}

int mqtt_client_custom_transport_write(struct mqtt_client *client,
				       const uint8_t *data, uint32_t datalen)
{
	ARG_UNUSED(client);
	ARG_UNUSED(data);
	ARG_UNUSED(datalen);
	return 0;
}

int mqtt_client_custom_transport_write_msg(struct mqtt_client *client,
					   const struct msghdr *message)
{
	ARG_UNUSED(client);
	ARG_UNUSED(message);
	return 0;
}

int mqtt_client_custom_transport_read(struct mqtt_client *client, uint8_t *data,
				      uint32_t buflen, bool shall_block)
{
	ARG_UNUSED(client);
	ARG_UNUSED(shall_block);

	uint32_t copied = 0;

	/* Drain setup (synthetic) data first */
	if (g_fuzz_ctx.setup_remaining > 0) {
		uint32_t n = (uint32_t)MIN(g_fuzz_ctx.setup_remaining,
					   (size_t)buflen);

		memcpy(data, g_fuzz_ctx.setup_data, n);
		g_fuzz_ctx.setup_data      += n;
		g_fuzz_ctx.setup_remaining -= n;
		copied += n;
		buflen -= n;
		data   += n;
	}

	/* Then drain fuzz data */
	if (buflen > 0 && g_fuzz_ctx.fuzz_remaining > 0) {
		uint32_t n = (uint32_t)MIN(g_fuzz_ctx.fuzz_remaining,
					   (size_t)buflen);

		memcpy(data, g_fuzz_ctx.fuzz_data, n);
		g_fuzz_ctx.fuzz_data      += n;
		g_fuzz_ctx.fuzz_remaining -= n;
		copied += n;
	}

	return (int)copied;
}

int mqtt_client_custom_transport_disconnect(struct mqtt_client *client)
{
	ARG_UNUSED(client);
	g_fuzz_ctx.setup_data      = NULL;
	g_fuzz_ctx.setup_remaining = 0;
	g_fuzz_ctx.fuzz_data       = NULL;
	g_fuzz_ctx.fuzz_remaining  = 0;
	return 0;
}

/* ---------------------------------------------------------------------------
 * Client (re-)initialisation helper
 *
 * Called at the start of every fuzz iteration to guarantee a clean slate.
 * ---------------------------------------------------------------------------
 */
static void client_setup(void)
{
	memset(&g_client_ctx, 0, sizeof(g_client_ctx));
	mqtt_client_init(&g_client_ctx);

	g_client_ctx.broker           = &broker;
	g_client_ctx.evt_cb           = mqtt_evt_handler;
	g_client_ctx.client_id.utf8   = (uint8_t *)"zephyr_mqtt_fuzz";
	g_client_ctx.client_id.size   = sizeof("zephyr_mqtt_fuzz") - 1;
	g_client_ctx.protocol_version = MQTT_VERSION_5_0;
	g_client_ctx.transport.type   = MQTT_TRANSPORT_CUSTOM;
	g_client_ctx.rx_buf           = rx_buffer;
	g_client_ctx.rx_buf_size      = sizeof(rx_buffer);
	g_client_ctx.tx_buf           = tx_buffer;
	g_client_ctx.tx_buf_size      = sizeof(tx_buffer);
}

/* ---------------------------------------------------------------------------
 * Fuzz ISR
 *
 * Executed inside the simulated Zephyr OS context.
 *
 * Mode byte (data[0]) bit layout:
 *
 *   bits [1:0]  connection mode
 *     0x00  pre-connected, MQTT 5.0
 *           Client is in MQTT_STATE_TCP_CONNECTED after mqtt_connect().
 *           Exercises CONNACK parsing, AUTH pre-connection, and all
 *           malformed pre-connection packets.
 *
 *     0x01  connected, MQTT 5.0
 *           Synthetic CONNACK injected first → MQTT_STATE_CONNECTED.
 *           Exercises PUBLISH (QoS 0/1/2, topic aliases, all properties),
 *           PUBACK/PUBREC/PUBREL/PUBCOMP, SUBACK, UNSUBACK, PINGRSP,
 *           DISCONNECT, AUTH re-auth, and the payload-drain path.
 *           mqtt_input() is called in a loop until the fuzz buffer is
 *           drained, so multi-packet sequences (e.g. set topic alias then
 *           use it) are fully exercised within one fuzz iteration.
 *
 *     0x02  pre-connected, MQTT 3.1.0
 *           Same as 0x00 but with protocol_version = MQTT_VERSION_3_1_0.
 *           Exercises the connect_ack_decode() MQTT_VERSION_3_1_0 branch
 *           (goto out, skipping property parsing) and the -ENOTSUP paths
 *           in disconnect_decode() and auth_decode().
 *
 *     0x03  connected, MQTT 3.1.0
 *           Synthetic CONNACK (3.1.0 format: 2-byte body, no props) injected
 *           first, then fuzz data fed in a loop.  Exercises all post-
 *           connection decoders under the 3.1.0 code paths.
 *
 *   bits [7:2]  reserved / ignored (gives libFuzzer mutation room)
 * ---------------------------------------------------------------------------
 */
/*
 * Cross-context data handoff: written by LLVMFuzzerTestOneInput() and read by
 * fuzz_isr(), which runs in the simulated Zephyr OS context triggered by the
 * synthetic hardware interrupt.  File scope is required for this handoff.
 */
static const uint8_t *g_fuzz_data;
static size_t         g_fuzz_sz;

/*
 * Minimal valid MQTT 3.1.0 CONNACK: remaining=2, flags=0x00, return_code=0x00.
 * No properties field (3.1.0 does not have one).
 */
static const uint8_t connack_v310[] = { 0x20, 0x02, 0x00, 0x00 };

/*
 * Drive mqtt_input() repeatedly until the fuzz buffer is exhausted or the
 * client disconnects.  This allows multi-packet sequences (e.g. register a
 * topic alias then use it) to be exercised within a single fuzz iteration.
 */
static void drain_input(void)
{
	int rc;
	size_t prev_remaining;

	while (g_fuzz_ctx.fuzz_remaining > 0) {
		prev_remaining = g_fuzz_ctx.fuzz_remaining;
		rc = mqtt_input(&g_client_ctx);
		/* Stop if the client disconnected or a fatal error occurred */
		if (rc < 0 && rc != -EAGAIN) {
			break;
		}
		/* Stop if no bytes were consumed to avoid an infinite loop */
		if (g_fuzz_ctx.fuzz_remaining == prev_remaining) {
			break;
		}
	}
}

static void fuzz_isr(const void *arg)
{
	ARG_UNUSED(arg);

	const uint8_t *data = g_fuzz_data;
	size_t         sz   = g_fuzz_sz;

	/* Need at least the mode byte plus one data byte to do anything
	 * meaningful.
	 */
	if (sz < 2) {
		k_sem_give(&fuzz_sem);
		return;
	}

	uint8_t mode = data[0] & 0x03;

	/* Advance past the mode byte */
	data++;
	sz--;

	/* Re-initialise the client for a clean slate on every iteration */
	client_setup();

	/* Override protocol version for 3.1.0 modes */
	if (mode >= 0x02) {
		g_client_ctx.protocol_version = MQTT_VERSION_3_1_0;
	}

	if (mode == 0x00 || mode == 0x02) {
		/*
		 * Pre-connected mode (MQTT 5.0 or 3.1.0):
		 * Feed fuzz data directly into a TCP-connected-but-not-yet-
		 * MQTT-connected client.  Exercises CONNACK parsing and all
		 * pre-connection packet handling including the 3.1.0 goto-out
		 * branch and the -ENOTSUP paths in disconnect/auth decoders.
		 */
		g_fuzz_ctx.setup_data      = NULL;
		g_fuzz_ctx.setup_remaining = 0;
		g_fuzz_ctx.fuzz_data       = data;
		g_fuzz_ctx.fuzz_remaining  = sz;

		int rc = mqtt_connect(&g_client_ctx);

		if (rc != 0) {
			k_sem_give(&fuzz_sem);
			return;
		}

		drain_input();

	} else {
		/*
		 * Connected mode (MQTT 5.0 or 3.1.0):
		 * Inject a synthetic CONNACK so the state machine reaches
		 * MQTT_STATE_CONNECTED, then drain the fuzz data in a loop.
		 * The loop allows multi-packet sequences such as:
		 *   PUBLISH (register topic alias=1) → PUBLISH (use alias=1)
		 * to be exercised within a single fuzz iteration, covering the
		 * topic-alias lookup path (mqtt_decoder.c lines 780-783).
		 */
		const uint8_t *synth    = (mode == 0x03) ? connack_v310
							  : connack_success;
		size_t         synth_sz = (mode == 0x03) ? sizeof(connack_v310)
							  : sizeof(connack_success);

		g_fuzz_ctx.setup_data      = synth;
		g_fuzz_ctx.setup_remaining = synth_sz;
		g_fuzz_ctx.fuzz_data       = data;
		g_fuzz_ctx.fuzz_remaining  = sz;

		int rc = mqtt_connect(&g_client_ctx);

		if (rc != 0) {
			k_sem_give(&fuzz_sem);
			return;
		}

		/* Consume the synthetic CONNACK → state becomes CONNECTED */
		rc = mqtt_input(&g_client_ctx);
		if (rc != 0) {
			k_sem_give(&fuzz_sem);
			return;
		}

		/* Drain all fuzz packets in a loop */
		drain_input();
	}

	k_sem_give(&fuzz_sem);
}

/* ---------------------------------------------------------------------------
 * main()
 * ---------------------------------------------------------------------------
 */
int main(void)
{
	printk("MQTT fuzzer starting on %s\n", CONFIG_BOARD);

	IRQ_CONNECT(CONFIG_ARCH_POSIX_FUZZ_IRQ, 0, fuzz_isr, NULL, 0);
	irq_enable(CONFIG_ARCH_POSIX_FUZZ_IRQ);

	while (true) {
		k_sem_take(&fuzz_sem, K_FOREVER);
	}
}

/* ---------------------------------------------------------------------------
 * LLVMFuzzerTestOneInput - libFuzzer entry point
 *
 * Delivers the fuzz buffer to the embedded OS via a simulated hardware
 * interrupt and then runs the simulator for enough ticks to reach idle.
 *
 * Input format:
 *   Byte 0:    mode selector (bits [1:0])
 *                0x00  pre-connected, MQTT 5.0
 *                0x01  connected,     MQTT 5.0  (multi-packet loop)
 *                0x02  pre-connected, MQTT 3.1.0
 *                0x03  connected,     MQTT 3.1.0 (multi-packet loop)
 *   Bytes 1..: raw MQTT packet bytes fed into mqtt_input() (looped for
 *              connected modes until the buffer is exhausted)
 *
 * Seed corpus and dictionary are in:
 *   samples/subsys/debug/fuzz_mqtt/corpus/   (binary seed files)
 *   samples/subsys/debug/fuzz_mqtt/mqtt5.dict
 * ---------------------------------------------------------------------------
 */
#if defined(CONFIG_BOARD_NATIVE_SIM)
NATIVE_SIMULATOR_IF /* expose to the final runner link stage */
#endif
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t sz)
{
	static bool runner_initialized;

	if (!runner_initialized) {
		nsi_init(0, NULL);
		runner_initialized = true;
	}

	g_fuzz_data = data;
	g_fuzz_sz   = sz;

	hw_irq_ctrl_set_irq(CONFIG_ARCH_POSIX_FUZZ_IRQ);

	nsi_exec_for(k_ticks_to_us_ceil64(CONFIG_ARCH_POSIX_FUZZ_TICKS));

	return 0;
}
