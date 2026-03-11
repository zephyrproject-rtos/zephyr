/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/ztest.h>
#include <string.h>

#define MSPI_CONTROLLER_NODE DT_NODELABEL(controller)
#define MSPI_PERIPHERAL_NODE DT_NODELABEL(peripheral)

#define SCK_FREQUENCY MHZ(8)

#define CMD_LEN_MAX 2
#define ADDR_LEN_MAX 4
#define DATA_LEN_MAX 500
#define NUM_PACKETS_MAX 5

#define PRINT_RAW_DATA 0

static uint8_t tx_buff[DATA_LEN_MAX] __aligned(4);
static uint8_t rx_buff[NUM_PACKETS_MAX][DATA_LEN_MAX + CMD_LEN_MAX + ADDR_LEN_MAX] __aligned(4);

static const struct device *mspi_controller = DEVICE_DT_GET(MSPI_CONTROLLER_NODE);
static const struct device *mspi_peripheral = DEVICE_DT_GET(MSPI_PERIPHERAL_NODE);

static const struct mspi_dev_id tx_id = {
	.dev_idx = 0,
};
static const struct mspi_dev_id rx_id = {
	.dev_idx = 0,
};

struct k_sem async_sem;
static struct mspi_callback_context cb_ctx;

static void mspi_peripheral_callback(struct mspi_callback_context *mspi_cb_ctx)
{
	ARG_UNUSED(mspi_cb_ctx);
	k_sem_give(&async_sem);
}

static void configure_devices(enum mspi_io_mode io_mode)
{
	struct mspi_dev_cfg controller_cfg = {
		.ce_num = 1,
		.freq = SCK_FREQUENCY,
		.io_mode = io_mode,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.cpp = MSPI_CPP_MODE_0,
		.endian = MSPI_XFER_BIG_ENDIAN,
		.ce_polarity = MSPI_CE_ACTIVE_LOW,
	};

	struct mspi_dev_cfg peripheral_cfg = {
		.ce_num = 1,
		.freq = SCK_FREQUENCY,
		.io_mode = io_mode,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.cpp = MSPI_CPP_MODE_0,
		.endian = MSPI_XFER_BIG_ENDIAN,
		.ce_polarity = MSPI_CE_ACTIVE_LOW,
	};

	int rc;

	rc = mspi_dev_config(mspi_controller, &tx_id,
			     MSPI_DEVICE_CONFIG_ALL, &controller_cfg);
	zassert_false(rc < 0, "mspi_dev_config() controller failed: %d", rc);

	rc = mspi_dev_config(mspi_peripheral, &rx_id,
			     MSPI_DEVICE_CONFIG_ALL, &peripheral_cfg);
	zassert_false(rc < 0, "mspi_dev_config() peripheral failed: %d", rc);

	rc = mspi_register_callback(mspi_peripheral, &rx_id, MSPI_BUS_XFER_COMPLETE,
				    (mspi_callback_handler_t)mspi_peripheral_callback, &cb_ctx);
	zassert_false(rc < 0, "mspi_register_callback() failed: %d", rc);
}

static void test_tx_transfer_multi_packet(struct mspi_xfer *tx_xfer, struct mspi_xfer *rx_xfer,
					  struct mspi_xfer_packet *tx_packets,
					  struct mspi_xfer_packet *rx_packets,
					  uint32_t transfer_length)
{
	uint32_t rx_idx;
	int rc;

	rx_xfer->num_packet = tx_xfer->num_packet;
	rx_xfer->cmd_length = tx_xfer->cmd_length;
	rx_xfer->addr_length = tx_xfer->addr_length;

	/* Set packet sizes */
	for (int i = 0; i < tx_xfer->num_packet; i++) {
		tx_packets[i].num_bytes = transfer_length;
		rx_packets[i].num_bytes = tx_xfer->cmd_length + tx_xfer->addr_length +
					  tx_packets[i].num_bytes;
	}

	/* Clean up RX buffers */
	for (int p = 0; p < rx_xfer->num_packet; ++p) {
		memset(rx_xfer->packets[p].data_buf, 0xAA,
		       rx_xfer->packets[p].num_bytes);
	}

	k_sem_reset(&async_sem);

	/* Start peripheral transfer in async mode */
	rc = mspi_transceive(mspi_peripheral, &rx_id, rx_xfer);
	zassert_false(rc < 0, "mspi_transceive() peripheral failed: %d", rc);

	k_msleep(10);

	/* Start controller transfer */
	rc = mspi_transceive(mspi_controller, &tx_id, tx_xfer);
	zassert_false(rc < 0, "mspi_transceive() controller failed: %d", rc);

	/* Wait for peripheral transfer to complete */
	rc = k_sem_take(&async_sem, K_MSEC(500));
	zassert_false(rc < 0, "peripheral transfer timeout");

	/* Verify each packet */
	for (int p = 0; p < tx_xfer->num_packet && p < rx_xfer->num_packet; ++p) {
		const struct mspi_xfer_packet *tx_packet = &tx_xfer->packets[p];
		const struct mspi_xfer_packet *rx_packet = &rx_xfer->packets[p];

		rx_idx = 0;

		/* Verify command */
		for (int i = 0; i < tx_xfer->cmd_length; ++i) {
			uint8_t shift = (tx_xfer->cmd_length - 1 - i) * 8;
			uint8_t expected = (tx_packet->cmd >> shift) & 0xff;
			uint8_t actual = rx_packet->data_buf[rx_idx++];

			if (PRINT_RAW_DATA) {
				TC_PRINT("packet %d command at index %d: 0x%02X : 0x%02X\n",
				       p, i, actual, expected);
			}

			zassert_equal(actual, expected,
				      "packet %d command mismatch at index %d: 0x%02X != 0x%02X",
				      p, i, actual, expected);
		}

		/* Verify address */
		for (int i = 0; i < tx_xfer->addr_length; ++i) {
			uint8_t shift = (tx_xfer->addr_length - 1 - i) * 8;
			uint8_t expected = (tx_packet->address >> shift) & 0xff;
			uint8_t actual = rx_packet->data_buf[rx_idx++];

			if (PRINT_RAW_DATA) {
				TC_PRINT("packet %d address at index %d: 0x%02X : 0x%02X\n",
				       p, i, actual, expected);
			}

			zassert_equal(actual, expected,
				      "packet %d address mismatch at index %d: 0x%02X != 0x%02X",
				      p, i, actual, expected);
		}

		/* Verify data */
		for (int i = 0; i < tx_packet->num_bytes; ++i) {
			uint8_t expected = tx_packet->data_buf[i];
			uint8_t actual = rx_packet->data_buf[rx_idx++];

			if (PRINT_RAW_DATA) {
				TC_PRINT("packet %d data at index %d: 0x%02X : 0x%02X\n",
				       p, i, actual, expected);
			}

			zassert_equal(actual, expected,
				      "packet %d data mismatch at index %d: 0x%02X != 0x%02X",
				      p, i, actual, expected);
		}
	}
}

static void test_tx_transfers(uint32_t transfer_length, uint8_t num_packets)
{
	TC_PRINT("Testing with transfer length of %d and %d packets\r\n",
		transfer_length, num_packets);

	struct mspi_xfer_packet tx_packets[NUM_PACKETS_MAX];
	struct mspi_xfer_packet rx_packets[NUM_PACKETS_MAX];

	for (int i = 0; i < num_packets; i++) {
		/* TX packet setup */
		tx_packets[i].dir = MSPI_TX;
		tx_packets[i].cmd = 0x1234;
		tx_packets[i].address = 0x98765432;
		tx_packets[i].data_buf = tx_buff;

		/* RX packet setup */
		rx_packets[i].dir = MSPI_RX;
		rx_packets[i].data_buf = rx_buff[i];
		rx_packets[i].cb_mask = MSPI_BUS_XFER_COMPLETE_CB;
		rx_packets[i].cmd = 0;
		rx_packets[i].address = 0;
	}

	struct mspi_xfer tx_xfer = {
#if defined(CONFIG_MSPI_DMA)
		.xfer_mode = MSPI_DMA,
#else
		.xfer_mode = MSPI_PIO,
#endif
		.packets = tx_packets,
		.num_packet = num_packets,
		.timeout = 1000,
	};

	struct mspi_xfer rx_xfer = {
#if defined(CONFIG_MSPI_DMA)
		.xfer_mode = MSPI_DMA,
#else
		.xfer_mode = MSPI_PIO,
#endif
		.packets = rx_packets,
		.timeout = 1000,
		.async = true,
	};

	if (transfer_length == 0) {
		TC_PRINT("- 8-bit command only\n");
		tx_xfer.cmd_length = 1;
		tx_xfer.addr_length = 0;
		test_tx_transfer_multi_packet(&tx_xfer, &rx_xfer, tx_packets, rx_packets,
					      transfer_length);

		TC_PRINT("- 16-bit command only\n");
		tx_xfer.cmd_length = 2;
		tx_xfer.addr_length = 0;
		test_tx_transfer_multi_packet(&tx_xfer, &rx_xfer, tx_packets, rx_packets,
					      transfer_length);

		TC_PRINT("- 8-bit command and 24-bit address only\n");
		tx_xfer.cmd_length = 1;
		tx_xfer.addr_length = 3;
		test_tx_transfer_multi_packet(&tx_xfer, &rx_xfer, tx_packets, rx_packets,
					      transfer_length);

	} else {
		TC_PRINT("- 8-bit command, 24-bit address\n");
		tx_xfer.cmd_length = 1;
		tx_xfer.addr_length = 3;
		test_tx_transfer_multi_packet(&tx_xfer, &rx_xfer, tx_packets, rx_packets,
					      transfer_length);

		TC_PRINT("- 8-bit command, 32-bit address\n");
		tx_xfer.cmd_length = 1;
		tx_xfer.addr_length = 4;
		test_tx_transfer_multi_packet(&tx_xfer, &rx_xfer, tx_packets, rx_packets,
					      transfer_length);

		TC_PRINT("- 16-bit command, 24-bit address\n");
		tx_xfer.cmd_length = 2;
		tx_xfer.addr_length = 3;
		test_tx_transfer_multi_packet(&tx_xfer, &rx_xfer, tx_packets, rx_packets,
					      transfer_length);

		TC_PRINT("- 16-bit command, 32-bit address\n");
		tx_xfer.cmd_length = 2;
		tx_xfer.addr_length = 4;
		test_tx_transfer_multi_packet(&tx_xfer, &rx_xfer, tx_packets, rx_packets,
					      transfer_length);

		TC_PRINT("- Just data\n");
		tx_xfer.cmd_length = 0;
		tx_xfer.addr_length = 0;
		test_tx_transfer_multi_packet(&tx_xfer, &rx_xfer, tx_packets, rx_packets,
					      transfer_length);
	}
}

static void test_tx_transfers_io_mode(enum mspi_io_mode io_mode)
{
	int rc;

	configure_devices(io_mode);

	/* 32-bit data frame size alignment */
	test_tx_transfers(DATA_LEN_MAX, NUM_PACKETS_MAX);

	/* 16-bit data frame size alignment */
	test_tx_transfers(DATA_LEN_MAX-2, NUM_PACKETS_MAX);

	/* 8-bit data frame size alignment */
	test_tx_transfers(DATA_LEN_MAX-1, NUM_PACKETS_MAX);

	/* Transmitting a small buffer */
	test_tx_transfers(4, NUM_PACKETS_MAX);

	/* Just command/address */
	test_tx_transfers(0, NUM_PACKETS_MAX);

	/* Single packet transfer */
	test_tx_transfers(DATA_LEN_MAX, 1);

	/* Release controller channel */
	rc = mspi_get_channel_status(mspi_controller, 0);
	zassert_equal(rc, 0, "mspi_get_channel_status() controller failed: %d", rc);

	/* Release peripheral channel */
	rc = mspi_get_channel_status(mspi_peripheral, 0);
	zassert_equal(rc, 0, "mspi_get_channel_status() peripheral failed: %d", rc);
}

/* Only Single, Dual and Quad modes tested as the peripheral cannot interpret a
 * command/address without an extra software layer
 */
ZTEST(mspi_tx_rx_loopback, test_single_io_mode)
{
	test_tx_transfers_io_mode(MSPI_IO_MODE_SINGLE);
}

ZTEST(mspi_tx_rx_loopback, test_dual_io_mode)
{
	test_tx_transfers_io_mode(MSPI_IO_MODE_DUAL);
}

ZTEST(mspi_tx_rx_loopback, test_quad_io_mode)
{
	test_tx_transfers_io_mode(MSPI_IO_MODE_QUAD);
}

static void *setup(void)
{
	k_sem_init(&async_sem, 0, 1);

	#if defined(CONFIG_MSPI_DMA)
		TC_PRINT("Using MSPI peripheral in DMA mode\n");
	#else
		TC_PRINT("Using MSPI peripheral in PIO (FIFO) mode\n");
	#endif

	for (int i = 0; i < DATA_LEN_MAX; ++i) {
		tx_buff[i] = (uint8_t)i;
	}

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_true(device_is_ready(mspi_controller),
		"MSPI controller device %s is not ready", mspi_controller->name);

	zassert_true(device_is_ready(mspi_peripheral),
		"MSPI peripheral device %s is not ready", mspi_peripheral->name);
}

ZTEST_SUITE(mspi_tx_rx_loopback, NULL, setup, before, NULL, NULL);
