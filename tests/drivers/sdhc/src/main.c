/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

static const struct device *const sdhc_dev = DEVICE_DT_GET(DT_ALIAS(sdhc0));
static struct sdhc_host_props props;
static struct sdhc_io io;

#define SDHC_FREQUENCY_SLIP 10000000

#define SDHC_EMMC_RCA 0x2U

K_SEM_DEFINE(card_sem, 0, 1);

/* Prepare IO settings for card */
static void *sdhc_power_on(void)
{
	int ret;

	ret = sdhc_get_host_props(sdhc_dev, &props);
	zassert_equal(ret, 0, "SDHC host props api call failed");

	io.clock = props.f_min;
	io.bus_mode = SDHC_BUSMODE_PUSHPULL;
	io.power_mode = SDHC_POWER_ON;
	io.bus_width = SDHC_BUS_WIDTH1BIT;
	io.timing = SDHC_TIMING_LEGACY;
	io.signal_voltage = SD_VOL_3_3_V;

	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);
	return NULL;
}

/* Resets SD host controller, verifies API */
ZTEST(sdhc, test_reset)
{
	int ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	ret = sdhc_hw_reset(sdhc_dev);
	zassert_equal(ret, 0, "SDHC HW reset failed");
}

/* Gets host properties, verifies all properties are set */
ZTEST(sdhc, test_host_props)
{
	int ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	/* Set all host properties to 0xFF */
	props.f_max = 0xFF;
	props.f_min = 0xFF;
	props.power_delay = 0xFF;
	props.max_current_330 = 0xFF;
	props.max_current_300 = 0xFF;
	props.max_current_180 = 0xFF;
	ret = sdhc_get_host_props(sdhc_dev, &props);
	zassert_equal(ret, 0, "SDHC host props api call failed");

	zassert_not_equal(props.f_max, 0xFF, "props structure not initialized");
	zassert_not_equal(props.f_min, 0xFF, "props structure not initialized");
	zassert_not_equal(props.power_delay, 0xFF, "props structure not initialized");
	zassert_not_equal(props.max_current_330, 0xFF, "props structure not initialized");
	zassert_not_equal(props.max_current_300, 0xFF, "props structure not initialized");
	zassert_not_equal(props.max_current_180, 0xFF, "props structure not initialized");
}

/* Verify that driver rejects frequencies outside of claimed range */
ZTEST(sdhc, test_set_io)
{
	int ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	io.clock = props.f_min;
	io.bus_mode = SDHC_BUSMODE_PUSHPULL;
	io.power_mode = SDHC_POWER_ON;
	io.bus_width = SDHC_BUS_WIDTH1BIT;
	io.timing = SDHC_TIMING_LEGACY;
	io.signal_voltage = SD_VOL_3_3_V;

	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "IO configuration failed");

	/* Verify that IO configuration fails with high frequency */
	/* Since SDHC may select nearby frequency, increase frequency
	 * by a large margin over the max.
	 */
	io.clock = props.f_max + SDHC_FREQUENCY_SLIP;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_not_equal(ret, 0, "Invalid io configuration should not succeed");
}

void sdhc_interrupt_cb(const struct device *dev, int source, const void *data)
{
	ARG_UNUSED(data);

	/* Check that the device pointer is correct */
	zassert_equal(dev, sdhc_dev, "Incorrect device pointer in interrupt callback");
	zassert_equal(source, SDHC_INT_INSERTED, "Got unexpected SDHC interrupt");
	k_sem_give(&card_sem);
}


/*
 * Verify that the driver can detect a present SD card
 */
ZTEST(sdhc, test_card_presence)
{
	int ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	ret = sdhc_card_present(sdhc_dev);
	if (ret == 0) {
		/* Card not in slot, test card insertion interrupt */
		TC_PRINT("Waiting for card to be present in slot\n");
		ret = sdhc_enable_interrupt(sdhc_dev, sdhc_interrupt_cb,
					SDHC_INT_INSERTED, NULL);
		zassert_equal(ret, 0, "Could not install card insertion interrupt");
		/* Wait for card insertion */
		ret = k_sem_take(&card_sem, K_FOREVER);
		/* Delay now that card is in slot */
		k_msleep(props.power_delay);
		zassert_equal(ret, 0, "Card insertion interrupt did not fire");
		ret = sdhc_card_present(sdhc_dev);
	}
	zassert_equal(ret, 1, "Card is not reported as present, is one connected?");
}

/* Verify that the driver can send commands to SD card, by reading interface
 * condition. This follows the first part of the SD initialization defined in
 * the SD specification.
 */
ZTEST(sdhc, test_card_if_cond)
{
	struct sdhc_command cmd;
	int ret, resp;
	int check_pattern = SD_IF_COND_CHECK;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	/* Toggle power to card, to clear state */
	io.power_mode = SDHC_POWER_OFF;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);
	io.power_mode = SDHC_POWER_ON;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);


	memset(&cmd, 0, sizeof(struct sdhc_command));
	cmd.opcode = SD_GO_IDLE_STATE;
	cmd.response_type = (SD_RSP_TYPE_NONE | SD_SPI_RSP_TYPE_R1);
	cmd.timeout_ms = 200;

	ret = sdhc_request(sdhc_dev, &cmd, NULL);
	zassert_equal(ret, 0, "Card reset command failed");

	cmd.opcode = SD_SEND_IF_COND;
	/* Indicate 3.3V support, plus check pattern */
	cmd.arg = SD_IF_COND_VHS_3V3 | check_pattern;
	cmd.response_type = (SD_RSP_TYPE_R7 | SD_SPI_RSP_TYPE_R7);
	cmd.timeout_ms = 500;
	cmd.retries = 3;

	ret = sdhc_request(sdhc_dev, &cmd, NULL);
	zassert_equal(ret, 0, "Read Interface condition failed");
	if (props.is_spi) {
		resp = cmd.response[1];
	} else {
		resp = cmd.response[0];
	}
	if ((resp & 0xFF) == check_pattern) {
		/* Although both responses are valid in SD spec, most
		 * modern cards are SDHC or better, and should respond as such.
		 */
		TC_PRINT("Found SDHC/SDXC card\n");
	} else if (resp & 0x4) {
		/* An illegal command response indicates an SDSC card */
		TC_PRINT("Found SDSC card\n");
	} else {
		zassert_unreachable("Invalid response to SD interface condition");
	}
}

/* CMD8 with no voltage window must time out. A valid 3.3V CMD8 is test_card_if_cond. */
ZTEST(sdhc, test_wrong_voltage_cmd8)
{
	struct sdhc_command cmd = {0};
	int ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");
	if (props.is_spi || props.host_caps.bus_8_bit_support) {
		ztest_test_skip();
	}

	io.power_mode = SDHC_POWER_OFF;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);
	io.power_mode = SDHC_POWER_ON;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);

	cmd.opcode = SD_GO_IDLE_STATE;
	cmd.response_type = SD_RSP_TYPE_NONE;
	cmd.timeout_ms = 200;
	ret = sdhc_request(sdhc_dev, &cmd, NULL);
	zassert_equal(ret, 0, "Card reset command failed");

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SD_SEND_IF_COND;
	cmd.arg = 0;
	cmd.response_type = SD_RSP_TYPE_R7;
	cmd.timeout_ms = 200;
	cmd.retries = 0;

	ret = sdhc_request(sdhc_dev, &cmd, NULL);
	zassert_equal(ret, -ETIMEDOUT, "CMD8 with VHS 0 must time out");
}

static int sdhc_send(const struct device *dev, uint32_t opcode, uint32_t arg,
		     uint32_t response_type, struct sdhc_data *data)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = opcode;
	cmd.arg = arg;
	cmd.response_type = response_type;
	cmd.timeout_ms = 500;
	cmd.retries = 3;

	ret = sdhc_request(dev, &cmd, data);
	if (ret == 0 && data == NULL) {
		return (int)cmd.response[0];
	}
	return ret;
}

/*
 * eMMC reliable write: CMD23 bit 31, then CMD25. The MMC stack never sends
 * CMD23, so this stays an SDHC command test. SD cards do not implement it.
 */
ZTEST(sdhc, test_reliable_write)
{
	static uint8_t wbuf[512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	static uint8_t rbuf[512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	struct sdhc_data data = {0};
	uint32_t ocr;
	int ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");
	if (props.is_spi) {
		ztest_test_skip();
	}

	io.power_mode = SDHC_POWER_OFF;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);
	io.power_mode = SDHC_POWER_ON;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);

	ret = sdhc_send(sdhc_dev, SD_GO_IDLE_STATE, 0, SD_RSP_TYPE_NONE, NULL);
	zassert_equal(ret, 0, "CMD0 failed");

	ocr = 0;
	for (int i = 0; i < 100; i++) {
		struct sdhc_command cmd = {0};

		cmd.opcode = MMC_SEND_OP_COND;
		cmd.arg = MMC_OCR_VDD27_36FLAG | MMC_OCR_SECTOR_MODE;
		cmd.response_type = SD_RSP_TYPE_R3;
		cmd.timeout_ms = 500;
		cmd.retries = 1;
		ret = sdhc_request(sdhc_dev, &cmd, NULL);
		if (ret != 0) {
			/* SD card: reliable write does not apply. */
			ztest_test_skip();
		}
		ocr = cmd.response[0];
		if (ocr & SD_OCR_PWR_BUSY_FLAG) {
			break;
		}
		k_msleep(10);
	}
	if ((ocr & SD_OCR_PWR_BUSY_FLAG) == 0) {
		ztest_test_skip();
	}

	ret = sdhc_send(sdhc_dev, SD_ALL_SEND_CID, 0, SD_RSP_TYPE_R2, NULL);
	zassert_ok(ret, "CMD2 failed");

	/* eMMC CMD3: the host assigns the RCA. The emulator does not check it. */
	ret = sdhc_send(sdhc_dev, MMC_SEND_RELATIVE_ADDR, SDHC_EMMC_RCA << 16,
			SD_RSP_TYPE_R1, NULL);
	zassert_ok(ret, "CMD3 failed");

	ret = sdhc_send(sdhc_dev, SD_SELECT_CARD, SDHC_EMMC_RCA << 16,
			SD_RSP_TYPE_R1b, NULL);
	zassert_ok(ret, "CMD7 failed");

	ret = sdhc_send(sdhc_dev, SD_SET_BLOCK_SIZE, 512, SD_RSP_TYPE_R1, NULL);
	zassert_ok(ret, "CMD16 failed");

	ret = sdhc_send(sdhc_dev, SD_SET_BLOCK_COUNT, BIT(31) | 1U,
			SD_RSP_TYPE_R1, NULL);
	zassert_ok(ret, "CMD23 failed");

	for (int i = 0; i < 512; i++) {
		wbuf[i] = (uint8_t)(i + 1);
	}
	data.block_addr = 0;
	data.block_size = 512;
	data.blocks = 1;
	data.data = wbuf;
	data.timeout_ms = 1000;
	ret = sdhc_send(sdhc_dev, SD_WRITE_MULTIPLE_BLOCK, 0, SD_RSP_TYPE_R1, &data);
	zassert_ok(ret, "CMD25 failed");
	zassert_equal(data.bytes_xfered, 512, "write bytes_xfered mismatch");

	memset(&data, 0, sizeof(data));
	memset(rbuf, 0, sizeof(rbuf));
	data.block_addr = 0;
	data.block_size = 512;
	data.blocks = 1;
	data.data = rbuf;
	data.timeout_ms = 1000;
	ret = sdhc_send(sdhc_dev, SD_READ_SINGLE_BLOCK, 0, SD_RSP_TYPE_R1, &data);
	zassert_ok(ret, "CMD17 failed");
	zassert_equal(data.bytes_xfered, 512, "read bytes_xfered mismatch");
	zassert_mem_equal(wbuf, rbuf, 512, "reliable write data mismatch");
}

/* Tuning is optional. Success, or "not supported", is a valid result. */
ZTEST(sdhc, test_execute_tuning)
{
	int ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	ret = sdhc_execute_tuning(sdhc_dev);
	zassert_true(ret == 0 || ret == -ENOTSUP || ret == -ENOSYS,
		     "tuning returned %d", ret);
}

/* CMD17 is legal only in the transfer state. CMD0 leaves the card idle. */
/* CMD17 is legal only in the transfer state. CMD0 leaves the card idle. */
ZTEST(sdhc, test_illegal_state_cmd17_in_idle)
{
	static uint8_t buf[512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	int ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	cmd.opcode = SD_GO_IDLE_STATE;
	cmd.response_type = SD_RSP_TYPE_NONE;
	cmd.timeout_ms = 200;
	ret = sdhc_request(sdhc_dev, &cmd, NULL);
	zassert_equal(ret, 0, "Card reset command failed");

	cmd.opcode = SD_READ_SINGLE_BLOCK;
	cmd.arg = 0;
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = 200;
	cmd.retries = 0;

	data.block_addr = 0;
	data.block_size = 512;
	data.blocks = 1;
	data.data = buf;
	data.timeout_ms = 200;

	ret = sdhc_request(sdhc_dev, &cmd, &data);
	zassert_not_equal(ret, 0, "CMD17 should fail in IDLE state");
}

/* eMMC CMD5: bit 15 sleeps, RCA is in bits [31:16]. SD cards skip at CMD1. */
ZTEST(sdhc, test_sleep_awake)
{
	static uint8_t buf[512] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	uint32_t ocr = 0;
	int ret;
	int sleep_ret;
	int read_ret;
	int awake_ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");
	if (props.is_spi) {
		ztest_test_skip();
	}

	io.power_mode = SDHC_POWER_OFF;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);
	io.power_mode = SDHC_POWER_ON;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);

	cmd.opcode = SD_GO_IDLE_STATE;
	cmd.response_type = SD_RSP_TYPE_NONE;
	cmd.timeout_ms = 200;
	zassert_equal(sdhc_request(sdhc_dev, &cmd, NULL), 0, "CMD0 failed");

	for (int i = 0; i < 100; i++) {
		memset(&cmd, 0, sizeof(cmd));
		cmd.opcode = MMC_SEND_OP_COND;
		cmd.arg = MMC_OCR_VDD27_36FLAG | MMC_OCR_SECTOR_MODE;
		cmd.response_type = SD_RSP_TYPE_R3;
		cmd.timeout_ms = 500;
		cmd.retries = 1;
		ret = sdhc_request(sdhc_dev, &cmd, NULL);
		if (ret != 0) {
			ztest_test_skip();
		}
		ocr = cmd.response[0];
		if (ocr & SD_OCR_PWR_BUSY_FLAG) {
			break;
		}
		k_msleep(10);
	}
	if ((ocr & SD_OCR_PWR_BUSY_FLAG) == 0) {
		ztest_test_skip();
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SD_ALL_SEND_CID;
	cmd.response_type = SD_RSP_TYPE_R2;
	cmd.timeout_ms = 500;
	zassert_equal(sdhc_request(sdhc_dev, &cmd, NULL), 0, "CMD2 failed");

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = MMC_SEND_RELATIVE_ADDR;
	cmd.arg = SDHC_EMMC_RCA << 16;
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = 500;
	zassert_equal(sdhc_request(sdhc_dev, &cmd, NULL), 0, "CMD3 failed");

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = MMC_SLEEP_AWAKE;
	cmd.arg = (SDHC_EMMC_RCA << 16) | BIT(15);
	cmd.response_type = SD_RSP_TYPE_R1b;
	cmd.timeout_ms = 1000;
	sleep_ret = sdhc_request(sdhc_dev, &cmd, NULL);

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SD_READ_SINGLE_BLOCK;
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = 200;
	data.block_size = 512;
	data.blocks = 1;
	data.data = buf;
	data.timeout_ms = 200;
	read_ret = sdhc_request(sdhc_dev, &cmd, &data);

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = MMC_SLEEP_AWAKE;
	cmd.arg = SDHC_EMMC_RCA << 16;
	cmd.response_type = SD_RSP_TYPE_R1b;
	cmd.timeout_ms = 1000;
	awake_ret = sdhc_request(sdhc_dev, &cmd, NULL);

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SD_GO_IDLE_STATE;
	cmd.response_type = SD_RSP_TYPE_NONE;
	cmd.timeout_ms = 200;
	sdhc_request(sdhc_dev, &cmd, NULL);

	zassert_equal(sleep_ret, 0, "CMD5 SLEEP failed");
	zassert_not_equal(read_ret, 0, "CMD17 succeeded while the card was asleep");
	zassert_equal(awake_ret, 0, "CMD5 AWAKE failed");
}

ZTEST_SUITE(sdhc, NULL, sdhc_power_on, NULL, NULL, NULL);
