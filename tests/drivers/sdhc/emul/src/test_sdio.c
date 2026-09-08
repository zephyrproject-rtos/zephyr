/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/sdhc/sdhc_emul.h>
#include <zephyr/sd/sd_spec.h>

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(sdhc_sdio));

static void sdio_irq_cb(const struct device *cb_dev, int reason, const void *user_data)
{
	struct k_sem *sem = (struct k_sem *)user_data;

	if (reason == SDHC_INT_SDIO) {
		k_sem_give(sem);
	}
}

/* Verify CMD5 returns correct SDIO function count from devicetree. */
ZTEST(sdhc_sdio, test_io_send_op_cond)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SDIO_SEND_OP_COND;
	cmd.arg = 0;

	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_ok(ret, "CMD5 failed");

	/* Verify function count in response */
	uint32_t func_count = (cmd.response[0] >> 28) & 0x7;

	zassert_equal(func_count, 2, "Function count mismatch from DT config");
}

/* Verify CMD52 can read CCCR revision register. */
ZTEST(sdhc_sdio, test_cccr_revision)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SDIO_RW_DIRECT;
	cmd.arg = (0 << 31) | (0 << 28) | (0x00 << 9); /* Read, Func 0, Addr 0x00 */

	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_ok(ret, "CMD52 failed");

	uint8_t data = cmd.response[0] & 0xFF;

	zassert_equal(data, 0x32, "CCCR Revision mismatch");
}

/* Verify CMD52 can read FBR function 1 standard interface code. */
ZTEST(sdhc_sdio, test_fbr_function1)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SDIO_RW_DIRECT;
	cmd.arg = (0 << 31) | (1 << 28) | (0x100 << 9); /* Read, Func 1, Addr 0x100 */

	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_ok(ret, "CMD52 failed");

	uint8_t data = cmd.response[0] & 0xFF;

	zassert_equal(data, 0x01, "FBR standard interface code mismatch");
}

/* Verify CMD53 byte-mode write and read round-trip with bytes_xfered. */
ZTEST(sdhc_sdio, test_cmd53_byte_mode_rw)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data sdhc_data = {0};
	uint8_t wbuf[64];
	uint8_t rbuf[64];

	for (int i = 0; i < 64; i++) {
		wbuf[i] = (uint8_t)i;
	}

	cmd.opcode = SDIO_RW_EXTENDED;
	cmd.arg = (1 << 31) | (1 << 28) | (0 << 27) | (0x00 << 9) | 64;
	sdhc_data.data = wbuf;
	zassert_ok(sdhc_request(dev, &cmd, &sdhc_data), "CMD53 write failed");
	zassert_equal(sdhc_data.bytes_xfered, 64, "bytes_xfered mismatch on write");

	memset(rbuf, 0, 64);
	cmd.arg = (0 << 31) | (1 << 28) | (0 << 27) | (0x00 << 9) | 64;
	sdhc_data.data = rbuf;
	zassert_ok(sdhc_request(dev, &cmd, &sdhc_data), "CMD53 read failed");
	zassert_equal(sdhc_data.bytes_xfered, 64, "bytes_xfered mismatch on read");
	zassert_mem_equal(wbuf, rbuf, 64, "CMD53 byte mode data mismatch");
}

/* Verify CMD53 block-mode write and read round-trip with bytes_xfered. */
ZTEST(sdhc_sdio, test_cmd53_block_mode_rw)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data sdhc_data = {0};
	uint8_t wbuf[1024];
	uint8_t rbuf[1024];

	for (int i = 0; i < 1024; i++) {
		wbuf[i] = (uint8_t)(i ^ 0xAA);
	}

	cmd.opcode = SDIO_RW_EXTENDED;
	cmd.arg = (1 << 31) | (1 << 28) | (1 << 27) | (0x00 << 9) | 2;
	sdhc_data.data = wbuf;
	zassert_ok(sdhc_request(dev, &cmd, &sdhc_data), "CMD53 write failed");
	zassert_equal(sdhc_data.bytes_xfered, 1024, "bytes_xfered mismatch on write");

	memset(rbuf, 0, 1024);
	cmd.arg = (0 << 31) | (1 << 28) | (1 << 27) | (0x00 << 9) | 2;
	sdhc_data.data = rbuf;
	zassert_ok(sdhc_request(dev, &cmd, &sdhc_data), "CMD53 read failed");
	zassert_equal(sdhc_data.bytes_xfered, 1024, "bytes_xfered mismatch on read");
	zassert_mem_equal(wbuf, rbuf, 1024, "CMD53 block mode data mismatch");
}

/* Verify CMD52 is rejected when the function number exceeds devicetree count. */
ZTEST(sdhc_sdio, test_sdio_invalid_function_boundary)
{
	struct sdhc_command cmd = {0};

	/* CMD52 with function count + 1 should fail */
	cmd.opcode = SDIO_RW_DIRECT;
	cmd.arg = (0 << 31) | (3 << 28) | (0x00 << 9); /* Read, Func 3 (DT has 2 functions) */
	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_not_equal(ret, 0, "CMD52 with invalid func should fail");
}

/* Verify SDIO net loopback writes and reads back payload with bytes_xfered. */
#ifdef CONFIG_SDHC_EMUL_SDIO_NET
ZTEST(sdhc_sdio, test_net_loopback)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data sdhc_data = {0};
	uint8_t wbuf[64] = "mock_udp_payload";
	uint8_t rbuf[64] = {0};

	cmd.opcode = SDIO_RW_EXTENDED;
	cmd.arg = (1 << 31) | (1 << 28) | (0 << 27) | (0x00 << 9) | 64;
	sdhc_data.data = wbuf;
	zassert_ok(sdhc_request(dev, &cmd, &sdhc_data), "Net TX write failed");
	zassert_equal(sdhc_data.bytes_xfered, 64, "bytes_xfered mismatch on TX");

	cmd.arg = (0 << 31) | (1 << 28) | (0 << 27) | (0x00 << 9) | 64;
	sdhc_data.data = rbuf;
	zassert_ok(sdhc_request(dev, &cmd, &sdhc_data), "Net RX read failed");
	zassert_equal(sdhc_data.bytes_xfered, 64, "bytes_xfered mismatch on RX");
	zassert_mem_equal(wbuf, rbuf, 64, "Net loopback payload mismatch");
}
#endif

/* Verify SDIO interrupt callback fires after emulator trigger. */
ZTEST(sdhc_sdio, test_interrupt_pending)
{
	struct k_sem irq_sem;

	k_sem_init(&irq_sem, 0, 1);

	sdhc_enable_interrupt(dev, sdio_irq_cb, SDHC_INT_SDIO, &irq_sem);

	/* Trigger IRQ via emulator test accessor */
	sdhc_emul_trigger_sdio_irq(dev, 1);

	/* Wait for the IRQ callback to fire */
	int ret = k_sem_take(&irq_sem, K_MSEC(100));

	zassert_ok(ret, "SDIO IRQ callback did not fire in time");
}
