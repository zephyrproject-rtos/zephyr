/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/peci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define PECI_HOST_ADDR 0x30U
#define RX_FILL        0xa5U

struct fake_peci_data {
	/* The message the last transfer call was given */
	struct peci_msg msg;
	unsigned int calls;
};

/* Placed in the ztest memory partition so user mode tests can inspect the fake */
static ZTEST_DMEM struct fake_peci_data fake_data;

/* Kernel memory a user mode thread must not be able to pass as a transfer buffer */
static uint8_t kernel_only_buf[PECI_RD_PKG_CMD_LEN];

/*
 * Stands in for a controller: records the message and fills the rx buffer the way a driver
 * does, data bytes followed by the frame check sequence byte.
 */
static int fake_peci_transfer(const struct device *dev, struct peci_msg *msg)
{
	struct fake_peci_data *data = dev->data;

	data->calls++;
	data->msg = *msg;

	if (msg->rx_buffer.buf != NULL) {
		memset(msg->rx_buffer.buf, RX_FILL, msg->rx_buffer.len + 1U);
	}

	return 0;
}

static DEVICE_API(peci, fake_peci_api) = {
	.transfer = fake_peci_transfer,
};

DEVICE_DEFINE(fake_peci, "fake_peci", NULL, NULL, &fake_data, NULL, POST_KERNEL,
	      CONFIG_PECI_INIT_PRIORITY, &fake_peci_api);

static const struct device *const dev = DEVICE_GET(fake_peci);

/*
 * A NULL buffer is valid only when it carries no data: a tx_buffer.len of 0 or 1 (the command
 * byte alone), or an rx_buffer.len of 0. With a larger length the driver would dereference it.
 */
ZTEST_USER(peci_userspace, test_null_tx_with_data_rejected)
{
	uint8_t rx[PECI_RD_PKG_LEN_DWORD + 1U];
	struct peci_msg msg = {
		.addr = PECI_HOST_ADDR,
		.cmd_code = PECI_CMD_RD_PKG_CFG0,
		.tx_buffer = {.buf = NULL, .len = PECI_RD_PKG_WR_LEN},
		.rx_buffer = {.buf = rx, .len = PECI_RD_PKG_LEN_DWORD},
	};

	ztest_set_fault_valid(true);
	(void)peci_transfer(dev, &msg);

	zassert_unreachable("NULL tx buffer with data was accepted");
}

ZTEST_USER(peci_userspace, test_null_rx_with_data_rejected)
{
	struct peci_msg msg = {
		.addr = PECI_HOST_ADDR,
		.cmd_code = PECI_CMD_GET_TEMP0,
		.tx_buffer = {.buf = NULL, .len = PECI_GET_TEMP_WR_LEN},
		.rx_buffer = {.buf = NULL, .len = PECI_GET_TEMP_RD_LEN},
	};

	ztest_set_fault_valid(true);
	(void)peci_transfer(dev, &msg);

	zassert_unreachable("NULL rx buffer with data was accepted");
}

ZTEST_USER(peci_userspace, test_kernel_tx_rejected)
{
	uint8_t rx[PECI_RD_PKG_LEN_DWORD + 1U];
	struct peci_msg msg = {
		.addr = PECI_HOST_ADDR,
		.cmd_code = PECI_CMD_RD_PKG_CFG0,
		.tx_buffer = {.buf = kernel_only_buf, .len = PECI_RD_PKG_WR_LEN},
		.rx_buffer = {.buf = rx, .len = PECI_RD_PKG_LEN_DWORD},
	};

	ztest_set_fault_valid(true);
	(void)peci_transfer(dev, &msg);

	zassert_unreachable("kernel-only tx buffer was accepted");
}

ZTEST_USER(peci_userspace, test_kernel_rx_rejected)
{
	struct peci_msg msg = {
		.addr = PECI_HOST_ADDR,
		.cmd_code = PECI_CMD_GET_TEMP0,
		.tx_buffer = {.buf = NULL, .len = PECI_GET_TEMP_WR_LEN},
		.rx_buffer = {.buf = kernel_only_buf, .len = PECI_GET_TEMP_RD_LEN},
	};

	ztest_set_fault_valid(true);
	(void)peci_transfer(dev, &msg);

	zassert_unreachable("kernel-only rx buffer was accepted");
}

/* Ping carries no data in either direction, so both buffers may be NULL. */
ZTEST_USER(peci_userspace, test_ping_null_buffers_allowed)
{
	struct peci_msg msg = {
		.addr = PECI_HOST_ADDR,
		.cmd_code = PECI_CMD_PING,
		.tx_buffer = {.buf = NULL, .len = 0},
		.rx_buffer = {.buf = NULL, .len = 0},
	};

	zassert_ok(peci_transfer(dev, &msg));

	zassert_equal(fake_data.calls, 1U);
	zassert_equal(fake_data.msg.cmd_code, PECI_CMD_PING);
	zassert_is_null(fake_data.msg.tx_buffer.buf);
	zassert_equal(fake_data.msg.tx_buffer.len, 0U);
	zassert_is_null(fake_data.msg.rx_buffer.buf);
	zassert_equal(fake_data.msg.rx_buffer.len, 0U);
}

/* GetTemp sends only the command byte, so the tx buffer may be NULL. */
ZTEST_USER(peci_userspace, test_get_temp_null_tx_allowed)
{
	uint8_t rx[PECI_GET_TEMP_RD_LEN + 1U] = {0};
	struct peci_msg msg = {
		.addr = PECI_HOST_ADDR,
		.cmd_code = PECI_CMD_GET_TEMP0,
		.tx_buffer = {.buf = NULL, .len = PECI_GET_TEMP_WR_LEN},
		.rx_buffer = {.buf = rx, .len = PECI_GET_TEMP_RD_LEN},
	};

	zassert_ok(peci_transfer(dev, &msg));

	zassert_equal(fake_data.calls, 1U);
	zassert_equal(fake_data.msg.cmd_code, PECI_CMD_GET_TEMP0);
	zassert_is_null(fake_data.msg.tx_buffer.buf);
	zassert_equal(fake_data.msg.tx_buffer.len, PECI_GET_TEMP_WR_LEN);
	zassert_equal_ptr(fake_data.msg.rx_buffer.buf, rx);
	zassert_equal(fake_data.msg.rx_buffer.len, PECI_GET_TEMP_RD_LEN);
	/* The last byte is the frame check sequence slot */
	zassert_equal(rx[PECI_GET_TEMP_RD_LEN], RX_FILL);
}

ZTEST_USER(peci_userspace, test_rd_pkg_cfg_allowed)
{
	uint8_t tx[PECI_RD_PKG_WR_LEN - 1U] = {0x00, 0x00, 0x00, 0x00};
	uint8_t rx[PECI_RD_PKG_LEN_DWORD + 1U] = {0};
	struct peci_msg msg = {
		.addr = PECI_HOST_ADDR,
		.cmd_code = PECI_CMD_RD_PKG_CFG0,
		.tx_buffer = {.buf = tx, .len = PECI_RD_PKG_WR_LEN},
		.rx_buffer = {.buf = rx, .len = PECI_RD_PKG_LEN_DWORD},
	};

	zassert_ok(peci_transfer(dev, &msg));

	zassert_equal(fake_data.calls, 1U);
	zassert_equal(fake_data.msg.cmd_code, PECI_CMD_RD_PKG_CFG0);
	zassert_equal_ptr(fake_data.msg.tx_buffer.buf, tx);
	zassert_equal(fake_data.msg.tx_buffer.len, PECI_RD_PKG_WR_LEN);
	zassert_equal_ptr(fake_data.msg.rx_buffer.buf, rx);
	zassert_equal(fake_data.msg.rx_buffer.len, PECI_RD_PKG_LEN_DWORD);
	zassert_equal(rx[PECI_RD_PKG_LEN_DWORD], RX_FILL);
}

static void *setup(void)
{
	zassert_true(device_is_ready(dev));
	k_object_access_grant(dev, k_current_get());

	return NULL;
}

static void before(void *f)
{
	ARG_UNUSED(f);

	memset(&fake_data, 0, sizeof(fake_data));
}

ZTEST_SUITE(peci_userspace, NULL, setup, before, NULL, NULL);
