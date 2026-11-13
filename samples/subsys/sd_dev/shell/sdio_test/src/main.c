/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SDIO Device Application Sample
 *
 * This sample demonstrates SDIO device-side functionality including
 * command processing, data transfer, and throughput testing.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/sd_dev/sd_dev.h>
#include <zephyr/sd_dev/sdio_dev.h>

LOG_MODULE_REGISTER(sdio_device_app, LOG_LEVEL_INF);

/**
 * @name Protocol Definitions
 *
 * Protocol definitions matching the Linux host-side driver.
 * @{
 */

/**
 * @brief Application data type
 */
typedef enum {
	/** Command packet type */
	MSDIO_APP_CMD_TYPE = 0,
	/** Data packet type */
	MSDIO_APP_DATA_TYPE
} app_data_type;

/**
 * @brief SDIO packet header structure
 */
typedef struct {
	/** Packet type (command or data) */
	uint32_t type;
	/** Payload length in bytes */
	uint32_t length;
	/** Variable-length payload data */
	uint8_t data[0];
} __packed msdio_packet_t;

/**
 * @brief Test command types
 */
typedef enum {
	/** Loopback test command */
	TEST_CMD_LOOPBACK = 1,
	/** Data transmit test command */
	TEST_CMD_DATA_TX  = 2,
	/** Data receive test command */
	TEST_CMD_DATA_RX  = 3,
	/** Stop test command */
	TEST_CMD_STOP     = 99
} test_command_t;

/**
 * @brief Test command packet structure
 */
typedef struct {
	/** Command identifier */
	uint32_t cmd;
	/** Packet size for data transfer tests */
	uint32_t packet_size;
	/** Number of packets for data transfer tests */
	uint32_t packet_count;
} __packed test_cmd_packet_t;

/**
 * @}
 */

/**
 * @name Buffer Management
 * @{
 */

/** Maximum packet size in bytes */
#define MAX_PACKET_SIZE 4096

/** Receive buffer size */
#define RX_BUFFER_SIZE  (sizeof(msdio_packet_t) + MAX_PACKET_SIZE)

/** Transmit buffer size */
#define TX_BUFFER_SIZE  (sizeof(msdio_packet_t) + MAX_PACKET_SIZE)

/** Transmit buffer (aligned for DMA) */
static uint8_t tx_buffer[TX_BUFFER_SIZE] __aligned(4);

/**
 * @}
 */

/**
 * @name Global State
 * @{
 */

/** SDIO function handle */
static struct sdio_dev_func *sdio_func;

/** Received packet counter */
static uint32_t rx_packet_count;

/** Received byte counter */
static uint32_t rx_byte_count;

/**
 * @}
 */

/**
 * @name Packet Send/Receive Functions
 * @{
 */

/**
 * @brief Send a packet to the host
 *
 * @param type Packet type (command or data)
 * @param data Pointer to payload data
 * @param data_len Length of payload in bytes
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
static int send_packet(app_data_type type, const void *data,
		       uint32_t data_len)
{
	msdio_packet_t *pkt = (msdio_packet_t *)tx_buffer;
	uint32_t total_len;
	int ret;

	if (data_len > MAX_PACKET_SIZE) {
		LOG_ERR("Data too large: %u", data_len);
		return -1;
	}

	pkt->type   = type;
	pkt->length = data_len;

	if (data && data_len > 0) {
		memcpy(pkt->data, data, data_len);
	}

	total_len = sizeof(msdio_packet_t) + data_len;

	LOG_DBG("Sending packet: type=%d, len=%u, total=%u",
		type, data_len, total_len);

	ret = sdio_write(sdio_func, tx_buffer, total_len);
	if (ret < 0) {
		LOG_ERR("sd_dev_write failed: %d", ret);
		return ret;
	}

	LOG_DBG("Packet sent successfully");
	return 0;
}

/**
 * @brief Receive a packet from the host
 *
 * @return Pointer to received packet on success, NULL on failure
 */
static sd_dev_pkt_t *recv_packet(void)
{
	sd_dev_pkt_t *sd_dev_pkt = sdio_read_pkt(sdio_func);

	if (!sd_dev_pkt) {
		LOG_ERR("sd_dev_read failed");
		return NULL;
	}

	if (sd_dev_pkt->len < sizeof(msdio_packet_t)) {
		LOG_ERR("Packet too small");
		return NULL;
	}

	return sd_dev_pkt;
}

/**
 * @}
 */

/**
 * @name Command Handlers
 * @{
 */

/**
 * @brief Handle loopback command
 *
 * Echoes the command back to the host.
 *
 * @param cmd Pointer to command packet
 */
static void handle_cmd_loopback(test_cmd_packet_t *cmd)
{
	test_cmd_packet_t response;

	LOG_INF("CMD_LOOPBACK received");

	response.cmd          = cmd->cmd;
	response.packet_size  = cmd->packet_size;
	response.packet_count = cmd->packet_count;

	send_packet(MSDIO_APP_CMD_TYPE, &response, sizeof(response));

	LOG_INF("Loopback response sent");
}

/**
 * @brief Handle data transmit command
 *
 * Receives data packets from the host and measures throughput.
 *
 * @param cmd Pointer to command packet
 */
static void handle_cmd_data_tx(test_cmd_packet_t *cmd)
{
	msdio_packet_t *pkt;
	int64_t start_time, end_time, elapsed_ms;
	uint32_t throughput_bps;
	uint32_t expect_packet_count = cmd->packet_count;

	LOG_INF("CMD_DATA_TX: size=%u, count=%u",
		cmd->packet_size, cmd->packet_count);

	rx_packet_count = 0;
	rx_byte_count   = 0;

	start_time = k_uptime_get();

	for (uint32_t i = 0; i < expect_packet_count; i++) {
		sd_dev_pkt_t *sd_dev_pkt = recv_packet();

		if (!sd_dev_pkt) {
			LOG_ERR("Failed to receive packet %u", i);
			break;
		}

		pkt = (msdio_packet_t *)sd_dev_pkt->data;

		if (pkt->type != MSDIO_APP_DATA_TYPE) {
			LOG_WRN("Wrong packet type: %d", pkt->type);
			continue;
		}

		sd_dev_pkt_free(sd_dev_pkt);

		rx_packet_count++;
		rx_byte_count += pkt->length;

		if ((i + 1) % 1000 == 0) {
			LOG_INF("Received %u packets", i + 1);
		}
	}

	end_time   = k_uptime_get();
	elapsed_ms = end_time - start_time;

	LOG_INF("Data RX completed: %u packets, %u bytes",
		rx_packet_count, rx_byte_count);

	if (elapsed_ms > 0) {
		throughput_bps =
			(uint32_t)((uint64_t)rx_byte_count * 8 * 1000 /
				   elapsed_ms);
		LOG_INF("Elapsed time: %lld ms", elapsed_ms);
		LOG_INF("Throughput: %u bps (%u.%03u Kbps, %u.%03u Mbps)",
			throughput_bps,
			throughput_bps / 1000,
			throughput_bps % 1000,
			throughput_bps / 1000000,
			(throughput_bps % 1000000) / 1000);
	} else {
		LOG_WRN("Elapsed time is 0, cannot calculate throughput");
	}
}

/**
 * @brief Handle data receive command
 *
 * Sends data packets to the host.
 *
 * @param cmd Pointer to command packet
 */
static void handle_cmd_data_rx(test_cmd_packet_t *cmd)
{
	test_cmd_packet_t response;
	uint8_t *data_buf;
	uint32_t i, j;
	uint32_t expect_packet_count = cmd->packet_count;

	LOG_INF("CMD_DATA_RX: size=%u, count=%u",
		cmd->packet_size, cmd->packet_count);

	response.cmd          = cmd->cmd;
	response.packet_size  = cmd->packet_size;
	response.packet_count = cmd->packet_count;

	send_packet(MSDIO_APP_CMD_TYPE, &response, sizeof(response));

	k_msleep(100);

	data_buf = k_malloc(cmd->packet_size);
	if (!data_buf) {
		LOG_ERR("Failed to allocate buffer");
		return;
	}

	for (j = 0; j < cmd->packet_size; j++) {
		data_buf[j] = j & 0xFF;
	}

	LOG_INF("Starting to send %u packets...", cmd->packet_count);

	for (i = 0; i < expect_packet_count; i++) {
		if (send_packet(MSDIO_APP_DATA_TYPE, data_buf,
				cmd->packet_size) < 0) {
			LOG_ERR("Failed to send packet %u", i);
			break;
		}

		if ((i + 1) % 1000 == 0) {
			LOG_INF("Sent %u packets", i + 1);
		}

		k_yield();
	}

	k_free(data_buf);

	LOG_INF("Data TX completed: %u packets", i);
}

/**
 * @brief Handle stop command
 *
 * Sends statistics back to the host.
 *
 * @param cmd Pointer to command packet
 */
static void handle_cmd_stop(test_cmd_packet_t *cmd)
{
	test_cmd_packet_t response;

	LOG_INF("CMD_STOP received");

	response.cmd          = cmd->cmd;
	response.packet_count = rx_packet_count;
	response.packet_size  = rx_byte_count;

	send_packet(MSDIO_APP_CMD_TYPE, &response, sizeof(response));

	LOG_INF("Statistics sent: packets=%u, bytes=%u",
		rx_packet_count, rx_byte_count);
}

/**
 * @}
 */

/**
 * @name Main Command Processing
 * @{
 */

/**
 * @brief Process received command packet
 *
 * @param data Pointer to packet data
 */
static void process_command(void *data)
{
	msdio_packet_t *pkt = (msdio_packet_t *)data;
	test_cmd_packet_t *cmd;

	if (pkt->type != MSDIO_APP_CMD_TYPE) {
		LOG_WRN("Not a command packet: type=%d", pkt->type);
		return;
	}

	cmd = (test_cmd_packet_t *)pkt->data;

	LOG_INF("Processing command: %d", cmd->cmd);

	switch (cmd->cmd) {
	case TEST_CMD_LOOPBACK:
		handle_cmd_loopback(cmd);
		break;
	case TEST_CMD_DATA_TX:
		handle_cmd_data_tx(cmd);
		break;
	case TEST_CMD_DATA_RX:
		handle_cmd_data_rx(cmd);
		break;
	case TEST_CMD_STOP:
		handle_cmd_stop(cmd);
		break;
	default:
		LOG_WRN("Unknown command: %d", cmd->cmd);
		break;
	}
}

/**
 * @}
 */

/**
 * @name Command Processing Thread
 * @{
 */

/** Command thread stack size */
#define CMD_THREAD_STACK_SIZE 4096

/** Command thread priority */
#define CMD_THREAD_PRIORITY   3

K_THREAD_STACK_DEFINE(cmd_thread_stack, CMD_THREAD_STACK_SIZE);
static struct k_thread cmd_thread_data;

/**
 * @brief Command processing thread entry point
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
static void cmd_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Command processing thread started");

	while (1) {
		sd_dev_pkt_t *sd_dev_pkt = recv_packet();

		if (sd_dev_pkt) {
			process_command(sd_dev_pkt->data);
			sd_dev_pkt_free(sd_dev_pkt);
		}
	}
}

/**
 * @}
 */

/**
 * @brief Application entry point
 *
 * Initializes the SDIO device and starts the command processing thread.
 *
 * @return 0 on success, negative errno on failure
 */
int main(void)
{
	int ret = 0;
	struct sd_dev_card *card = 0;
	const struct device *dev = 0;
	k_tid_t tid;
	struct sdio_dev *sdio = 0;

#if DT_NODE_EXISTS(DT_NODELABEL(sdio_device0))
#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdio_device0), okay)
#define SDIO_DEVICE_NODE DT_NODELABEL(sdio_device0)
#endif
#endif

#ifdef SDIO_DEVICE_NODE
	dev = DEVICE_DT_GET(SDIO_DEVICE_NODE);
	if (!device_is_ready(dev)) {
		LOG_ERR("SDIO device not ready");
		return -ENODEV;
	}
#else
	LOG_ERR("SDIO device node not found in devicetree");
	goto fail;
#endif

	ret = sd_dev_init(dev, &card);
	if (ret < 0) {
		LOG_ERR("sd_dev_init failed");
		goto fail;
	}

	sdio = card->sdio;
	sdio_func = sdio->funcs[1];
	LOG_INF("RW6xx SDIO device ready");

	tid = k_thread_create(&cmd_thread_data, cmd_thread_stack,
			      CMD_THREAD_STACK_SIZE,
			      cmd_thread_entry,
			      NULL, NULL, NULL,
			      CMD_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(tid, "cmd_thread");

	LOG_INF("SDIO Application Initialized");
	LOG_INF("Command thread created successfully");

	while (1) {
		if (sd_dev_card_is_enumed(card)) {
			if (card->is_enum == false) {
				card->is_enum = true;
				LOG_INF("SD CARD unenum ---> enumed");
				sd_dev_set_state(card->dev, SDEV_DEVICE_READY);
				sdio_func->block_size =
					sdio_dev_get_block_size(sdio_func);
				sdio_func->func_code =
					sdio_dev_get_fn_code(sdio_func);
				sdio->cfg->bus_speed =
					sdio_dev_get_bus_speed(sdio);
				sdio->cfg->bus_width =
					sdio_dev_get_bus_width(sdio);

				LOG_INF("sdio func%d enumed value: block_size=%d func_code=%d",
					sdio_func->fn,
					sdio_func->block_size,
					sdio_func->func_code);
				LOG_INF("sdio enumed value: speed=%d width=%d",
					sdio->cfg->bus_speed,
					sdio->cfg->bus_width);
			}
		} else if (card->is_enum == true) {
			card->is_enum = false;
			LOG_INF("SD CARD change enumed ---> unenum");
		} else {
			/* No state change */
		}

		k_sleep(K_MSEC(100));
	}

fail:
	LOG_ERR("Application failed with error: %d", ret);

	return ret;
}
