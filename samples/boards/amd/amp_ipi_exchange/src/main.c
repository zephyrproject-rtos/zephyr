/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AMD RPU AMP IPI Exchange Demo
 *
 * Demonstrates two independent Zephyr instances running on separate
 * Cortex-R cores communicating via IPI (Inter-Processor Interrupt)
 * mailbox — the hardware data-exchange mechanism on AMD Versal and
 * VersalNet platforms.
 *
 * This sample addresses the AMP (Asymmetric Multi-Processing) use case
 * where workload isolation and freedom from interference (FFI) are
 * required, while still allowing structured data to flow between the
 * isolated applications.
 *
 * Architecture:
 *   Core 0 — Producer: generates simulated sensor data and sends it
 *            to Core 1 via IPI mailbox.  Measures round-trip latency
 *            for each exchange.
 *   Core 1 — Consumer: receives sensor data, processes/logs it, and
 *            sends an ACK back to Core 0 via IPI mailbox.
 *
 * Communication protocol:
 *   1. Handshake: Core 0 sends PING, Core 1 replies PONG
 *   2. Data exchange: Core 0 sends DATA messages, Core 1 replies ACK
 *   3. Completion: Core 0 sends DONE to signal end of test
 *
 * The same source code runs on both cores.  The physical core ID
 * (read from MPIDR) determines the role at runtime.
 *
 * Supported platforms:
 *   - Versal RPU    (Cortex-R5,  GICv1/v2) — versal_rpu/versal_rpu/amp/{core0,core1}
 *   - VersalNet RPU (Cortex-R52, GICv3)    — versalnet_rpu/amd_versalnet_rpu/amp/{core0,core1}
 *
 * Build (Versal):
 *   west build -b versal_rpu/versal_rpu/amp/core0 -d build_core0 \
 *     samples/boards/amd/amp_ipi_exchange
 *   west build -b versal_rpu/versal_rpu/amp/core1 -d build_core1 \
 *     samples/boards/amd/amp_ipi_exchange
 *
 * Build (VersalNet):
 *   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core0 -d build_core0 \
 *     samples/boards/amd/amp_ipi_exchange
 *   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core1 -d build_core1 \
 *     samples/boards/amd/amp_ipi_exchange
 *
 * Flash (dual-core via XSDB):
 *   cd build_core0
 *   west flash --runner xsdb \
 *     --pdi <path-to-pdi> \
 *     --second-elf ../build_core1/zephyr/zephyr.elf
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(amp_ipi, LOG_LEVEL_INF);

/* ------------------------------------------------------------------ */
/* IPI message protocol                                               */
/* ------------------------------------------------------------------ */

/** Message types */
#define MSG_TYPE_PING 0x00U /**< Handshake request (producer → consumer) */
#define MSG_TYPE_PONG 0x01U /**< Handshake reply   (consumer → producer) */
#define MSG_TYPE_DATA 0x02U /**< Sensor data       (producer → consumer) */
#define MSG_TYPE_ACK  0x03U /**< Acknowledgment   (consumer → producer) */
#define MSG_TYPE_DONE 0xFFU /**< End of test       (producer → consumer) */

/**
 * IPI message structure — exactly 32 bytes to fill the IPI mailbox
 * hardware buffer (8 × 32-bit words).
 */
struct ipi_msg {
	uint32_t seq;       /**< Sequence number */
	uint32_t type;      /**< Message type (MSG_TYPE_*) */
	uint32_t timestamp; /**< k_cycle_get_32() at send time */
	uint32_t data[5];   /**< Payload (sensor data or zeros for control msgs) */
};

BUILD_ASSERT(sizeof(struct ipi_msg) == 32, "ipi_msg must be exactly 32 bytes (IPI MTU)");

/** Number of data exchange rounds */
#define NUM_EXCHANGES 20

/** Maximum handshake attempts before giving up */
#define HANDSHAKE_RETRIES 40

/* ------------------------------------------------------------------ */
/* MBOX channel specs from devicetree                                 */
/* ------------------------------------------------------------------ */

static const struct mbox_dt_spec tx_chan = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
static const struct mbox_dt_spec rx_chan = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);

/* ------------------------------------------------------------------ */
/* RX handling                                                        */
/* ------------------------------------------------------------------ */

static K_SEM_DEFINE(rx_sem, 0, 1);
static struct ipi_msg rx_buf;

/**
 * Mailbox RX callback — copies received data and signals the
 * waiting thread.  Runs in ISR context.
 */
static void rx_callback(const struct device *dev, mbox_channel_id_t channel_id, void *user_data,
			struct mbox_msg *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(user_data);

	if (data != NULL && data->data != NULL) {
		size_t copy_len = MIN(data->size, sizeof(struct ipi_msg));

		memcpy(&rx_buf, data->data, copy_len);
	}
	k_sem_give(&rx_sem);
}

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/**
 * Read physical core ID from MPIDR (Aff0 field).
 * Works on both Cortex-R5 (ARMv7-R) and Cortex-R52 (ARMv8-R AArch32).
 */
static inline uint32_t get_core_id(void)
{
	uint32_t mpidr;

	__asm__ volatile("mrc p15, 0, %0, c0, c0, 5" : "=r"(mpidr));
	return mpidr & 0xFFU;
}

/**
 * Send an IPI message with automatic retry on -EBUSY.
 * The IPI hardware reports busy when the remote hasn't acknowledged
 * the previous interrupt yet.
 */
static int send_ipi_msg(const struct ipi_msg *msg)
{
	struct mbox_msg mbox_msg = {
		.data = (const void *)msg,
		.size = sizeof(struct ipi_msg),
	};
	int ret;

	for (int retry = 0; retry < 100; retry++) {
		ret = mbox_send_dt(&tx_chan, &mbox_msg);
		if (ret == 0) {
			return 0;
		}
		if (ret != -EBUSY) {
			return ret;
		}
		k_usleep(100);
	}
	return -EBUSY;
}

/* ------------------------------------------------------------------ */
/* Producer (Core 0)                                                  */
/* ------------------------------------------------------------------ */

/**
 * Producer role: generates simulated sensor readings, sends them to
 * the consumer core, waits for ACK, and measures round-trip latency.
 */
static void run_producer(void)
{
	struct ipi_msg msg;
	uint32_t latencies[NUM_EXCHANGES];
	int ret;
	bool connected = false;

	/*
	 * Phase 1 — Handshake
	 *
	 * Send PING repeatedly until the consumer replies with PONG.
	 * This handles the startup race where Core 1 may not have
	 * enabled its RX callback yet.
	 *
	 * On Versal RPU, the IPI driver init clears ISR, which would lose
	 * any messages sent before Core 1's mbox_set_enabled() runs.
	 * Add a delay to let Core 1 fully initialize before we start.
	 */
	/* Brief delay to let Core 1 initialize its IPI driver */
	k_msleep(500);

	printk("[Producer] Starting handshake...\n");
	for (int attempt = 0; attempt < HANDSHAKE_RETRIES && !connected; attempt++) {
		memset(&msg, 0, sizeof(msg));
		msg.type = MSG_TYPE_PING;
		msg.seq = 0;
		msg.timestamp = k_cycle_get_32();

		ret = send_ipi_msg(&msg);
		if (ret < 0 && attempt < 3) {
			printk("[Producer] PING %d failed: %d\n", attempt, ret);
		}

		ret = k_sem_take(&rx_sem, K_MSEC(500));
		if (ret == 0 && rx_buf.type == MSG_TYPE_PONG) {
			connected = true;
		}
	}

	if (!connected) {
		printk("[Producer] FAIL: Consumer did not respond to PING\n");
		printk("Overall: FAIL\n");
		return;
	}
	printk("[Producer] Consumer connected!\n");
	k_msleep(100);

	/*
	 * Phase 2 — Data Exchange
	 *
	 * Send simulated sensor data and measure the round-trip time
	 * (send DATA → receive ACK).
	 */
	printk("\n--- Data Exchange Phase (%d rounds) ---\n\n", NUM_EXCHANGES);

	int success_count = 0;

	for (int i = 0; i < NUM_EXCHANGES; i++) {
		/* Build sensor-data message */
		memset(&msg, 0, sizeof(msg));
		msg.seq = i;
		msg.type = MSG_TYPE_DATA;
		msg.data[0] = 2500U + (i * 10U);         /* temperature  (0.01 °C units) */
		msg.data[1] = 101325U + (i * 100U);      /* pressure     (Pa) */
		msg.data[2] = 4500U + (i * 5U);          /* humidity     (0.01 % units) */
		msg.data[3] = k_uptime_get_32();         /* sender uptime (ms) */
		msg.data[4] = 0xA5A50000U | (uint32_t)i; /* marker */

		uint32_t t_start = k_cycle_get_32();

		msg.timestamp = t_start;

		ret = send_ipi_msg(&msg);
		if (ret < 0) {
			LOG_ERR("Send failed (round %d): %d", i, ret);
			latencies[i] = 0;
			continue;
		}

		/* Wait for ACK */
		ret = k_sem_take(&rx_sem, K_MSEC(5000));
		if (ret < 0) {
			LOG_ERR("ACK timeout (round %d)", i);
			latencies[i] = 0;
			continue;
		}

		uint32_t t_end = k_cycle_get_32();

		latencies[i] = t_end - t_start;

		if (rx_buf.type == MSG_TYPE_ACK && rx_buf.seq == (uint32_t)i) {
			success_count++;
		} else {
			LOG_WRN("Unexpected reply: type=0x%02x seq=%d (expected ACK seq=%d)",
				rx_buf.type, rx_buf.seq, i);
		}

		/* Only print progress every 5 rounds to reduce UART contention */
		if ((i + 1) % 5 == 0 || i == 0) {
			printk("[Producer] Progress: %d/%d rounds\n", i + 1, NUM_EXCHANGES);
		}

		k_msleep(50);
	}

	/*
	 * Phase 3 — Signal completion
	 */
	memset(&msg, 0, sizeof(msg));
	msg.type = MSG_TYPE_DONE;
	msg.seq = NUM_EXCHANGES;
	msg.timestamp = k_cycle_get_32();
	send_ipi_msg(&msg);

	/*
	 * Phase 4 — Performance summary
	 */
	uint32_t total = 0;
	uint32_t min_lat = UINT32_MAX;
	uint32_t max_lat = 0;
	int valid = 0;

	for (int i = 0; i < NUM_EXCHANGES; i++) {
		if (latencies[i] > 0) {
			total += latencies[i];
			if (latencies[i] < min_lat) {
				min_lat = latencies[i];
			}
			if (latencies[i] > max_lat) {
				max_lat = latencies[i];
			}
			valid++;
		}
	}

	printk("\n=============================================\n");
	printk(" AMP IPI Exchange — Producer Summary\n");
	printk("=============================================\n");
	printk("  Total rounds:    %d\n", NUM_EXCHANGES);
	printk("  Successful:      %d\n", success_count);
	if (valid > 0) {
		printk("  Min latency:     %u cycles\n", min_lat);
		printk("  Max latency:     %u cycles\n", max_lat);
		printk("  Avg latency:     %u cycles\n", total / valid);
	}
	printk("  Overall: %s\n", (success_count == NUM_EXCHANGES) ? "ALL PASS" : "FAIL");
	printk("=============================================\n");
}

/* ------------------------------------------------------------------ */
/* Consumer (Core 1)                                                  */
/* ------------------------------------------------------------------ */

/**
 * Consumer role: receives sensor data from the producer, logs it,
 * and sends an ACK for each message.
 */
static void run_consumer(void)
{
	struct ipi_msg ack;
	int count = 0;
	int ret;

	printk("[Consumer] Waiting for messages...\n");

	while (1) {
		ret = k_sem_take(&rx_sem, K_MSEC(30000));
		if (ret < 0) {
			LOG_ERR("RX timeout after %d messages", count);
			break;
		}

		switch (rx_buf.type) {
		case MSG_TYPE_PING:
			/* Handshake — reply with PONG */
			memset(&ack, 0, sizeof(ack));
			ack.type = MSG_TYPE_PONG;
			ack.seq = rx_buf.seq;
			ack.timestamp = k_cycle_get_32();
			send_ipi_msg(&ack);
			printk("[Consumer] Connected!\n");
			break;

		case MSG_TYPE_DATA:
			count++;
			/* Only print every 5th message to reduce UART contention */
			if (count % 5 == 0 || count == 1) {
				printk("[Consumer] Received %d messages...\n", count);
			}

			/* Send ACK */
			memset(&ack, 0, sizeof(ack));
			ack.type = MSG_TYPE_ACK;
			ack.seq = rx_buf.seq;
			ack.timestamp = k_cycle_get_32();
			ret = send_ipi_msg(&ack);
			if (ret < 0) {
				LOG_ERR("ACK send failed: %d", ret);
			}
			break;

		case MSG_TYPE_DONE:
			printk("[Consumer] DONE received after %d messages\n", count);
			goto done;

		default:
			LOG_WRN("Unknown message type: 0x%02x", rx_buf.type);
			break;
		}
	}

done:
	printk("\n=============================================\n");
	printk(" AMP IPI Exchange — Consumer Summary\n");
	printk("=============================================\n");
	printk("  Messages received: %d\n", count);
	printk("  Expected:          %d\n", NUM_EXCHANGES);
	printk("  Overall: %s\n", (count == NUM_EXCHANGES) ? "ALL PASS" : "FAIL");
	printk("=============================================\n");
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */

int main(void)
{
	uint32_t core_id = get_core_id();
	int ret;

	printk("\n=============================================\n");
	printk(" AMD RPU AMP IPI Exchange Demo\n");
	printk(" Core %d (%s)\n", core_id, core_id == 0 ? "Producer" : "Consumer");
	printk("=============================================\n\n");

	/* Report mailbox configuration */
	int mtu = mbox_mtu_get_dt(&tx_chan);

	printk("[Core%d] IPI mailbox MTU: %d bytes\n", core_id, mtu);

	/* Register RX callback and enable the RX channel */
	ret = mbox_register_callback_dt(&rx_chan, rx_callback, NULL);
	if (ret < 0) {
		printk("[Core%d] FAIL: mbox_register_callback: %d\n", core_id, ret);
		return -1;
	}

	ret = mbox_set_enabled_dt(&rx_chan, true);
	if (ret < 0) {
		printk("[Core%d] FAIL: mbox_set_enabled: %d\n", core_id, ret);
		return -1;
	}

	printk("[Core%d] IPI mailbox initialized\n\n", core_id);

	/* Dispatch based on physical core ID */
	if (core_id == 0) {
		run_producer();
	} else {
		run_consumer();
	}

	printk("\n[Core%d] Test complete.\n", core_id);
	return 0;
}
