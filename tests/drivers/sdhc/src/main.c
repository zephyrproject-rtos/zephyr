/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

static const struct device *const sdhc_dev = DEVICE_DT_GET(DT_ALIAS(sdhc0));
static struct sdhc_host_props props;
static struct sdhc_io io;

#define SDHC_FREQUENCY_SLIP 10000000

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

#define SDHC_TEST_BLOCK_SIZE      512
#define SDHC_TEST_BLOCK_COUNT     4
#define SDHC_TEST_START_BLOCK     0x1000
#define SDHC_TEST_CMD_TIMEOUT_MS  1000
#define SDHC_TEST_OCR_RETRIES     100
#define SDHC_TEST_READY_RETRIES   1000
#define SDHC_TEST_HANG_TIMEOUT_MS 5000

static uint8_t test_buf[SDHC_TEST_BLOCK_SIZE * SDHC_TEST_BLOCK_COUNT]
	__aligned(MAX(4, CONFIG_SDHC_BUFFER_ALIGNMENT));
static uint32_t written_blocks __aligned(MAX(4, CONFIG_SDHC_BUFFER_ALIGNMENT));

K_THREAD_STACK_DEFINE(request_stack, CONFIG_ZTEST_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread request_thread;

struct card_request {
	struct sdhc_command cmd;
	struct sdhc_data *data;
	int ret;
};

static void card_cmd_init(struct sdhc_command *cmd, uint32_t opcode, uint32_t arg,
			  uint32_t response_type)
{
	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = opcode;
	cmd->arg = arg;
	cmd->response_type = response_type;
	cmd->timeout_ms = SDHC_TEST_CMD_TIMEOUT_MS;
	cmd->retries = 3;
}

static int card_cmd(struct sdhc_command *cmd, uint32_t opcode, uint32_t arg, uint32_t response_type,
		    struct sdhc_data *data)
{
	card_cmd_init(cmd, opcode, arg, response_type);

	return sdhc_request(sdhc_dev, cmd, data);
}

static void card_request_entry(void *p1, void *p2, void *p3)
{
	struct card_request *req = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	req->ret = sdhc_request(sdhc_dev, &req->cmd, req->data);
}

/*
 * Sends the request from a separate thread, so a driver that hangs fails
 * this test instead of stalling the rest of the suite. The thread is
 * preemptible and has lower priority than the test thread, so it can be
 * aborted even if the driver busy-waits. Returns 0 if the driver completed the
 * request, with its result in req->ret.
 */
static int card_request_timeout(struct card_request *req)
{
	k_tid_t tid;
	int ret;

	tid = k_thread_create(&request_thread, request_stack, K_THREAD_STACK_SIZEOF(request_stack),
			      card_request_entry, req, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO,
			      0, K_NO_WAIT);
	ret = k_thread_join(tid, K_MSEC(SDHC_TEST_HANG_TIMEOUT_MS));
	if (ret != 0) {
		k_thread_abort(tid);
	}

	return ret;
}

/* Waits for the card to finish programming and return to transfer state */
static void card_wait_ready(uint32_t rca)
{
	struct sdhc_command cmd;
	int ret;

	for (int i = 0; i < SDHC_TEST_READY_RETRIES; i++) {
		ret = card_cmd(&cmd, SD_SEND_STATUS, rca << 16, SD_RSP_TYPE_R1, NULL);
		zassert_equal(ret, 0, "Send status command failed");
		if ((cmd.response[0] & SD_R1_RDY_DATA) &&
		    (SD_R1_CURRENT_STATE(cmd.response[0]) == SDMMC_R1_TRANSFER)) {
			return;
		}
		k_msleep(1);
	}
	zassert_unreachable("Card did not return to transfer state");
}

static void card_transfer(uint32_t opcode, uint32_t arg, uint32_t rca)
{
	struct sdhc_command cmd;
	struct sdhc_data data = {
		.block_addr = SDHC_TEST_START_BLOCK,
		.block_size = SDHC_TEST_BLOCK_SIZE,
		.blocks = SDHC_TEST_BLOCK_COUNT,
		.data = test_buf,
		.timeout_ms = SDHC_TEST_CMD_TIMEOUT_MS,
	};
	int ret;

	ret = card_cmd(&cmd, opcode, arg, SD_RSP_TYPE_R1, &data);
	zassert_equal(ret, 0, "Data transfer with CMD%u failed", opcode);
	zassert_equal(cmd.response[0] & SD_R1_ERR_FLAGS, 0, "CMD%u reports error", opcode);
	card_wait_ready(rca);
}

/*
 * Verify that the driver can read a short data block, by asking the card how
 * many blocks the last write programmed (ACMD22). The SD stack sends this
 * command after a failed write. The blocks are read and written back
 * unchanged, so the card contents are preserved.
 */
ZTEST(sdhc, test_card_written_blocks)
{
	struct sdhc_command cmd;
	struct sdhc_data data = {
		.block_size = sizeof(written_blocks),
		.blocks = 1,
		.data = &written_blocks,
		.timeout_ms = SDHC_TEST_CMD_TIMEOUT_MS,
	};
	struct card_request req = {
		.data = &data,
	};
	uint32_t ocr = SD_OCR_VDD32_33FLAG | SD_OCR_VDD33_34FLAG;
	uint32_t rca, addr;
	int ret, i;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	if (props.is_spi) {
		/* The card initialization below uses native mode commands */
		ztest_test_skip();
	}

	/* Toggle power to card, to clear state */
	io.clock = props.f_min;
	io.bus_width = SDHC_BUS_WIDTH1BIT;
	io.power_mode = SDHC_POWER_OFF;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);
	io.power_mode = SDHC_POWER_ON;
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");
	k_msleep(props.power_delay);

	ret = card_cmd(&cmd, SD_GO_IDLE_STATE, 0, SD_RSP_TYPE_NONE, NULL);
	zassert_equal(ret, 0, "Card reset command failed");

	/* Only cards that respond to CMD8 may be offered high capacity support */
	ret = card_cmd(&cmd, SD_SEND_IF_COND, SD_IF_COND_VHS_3V3 | SD_IF_COND_CHECK, SD_RSP_TYPE_R7,
		       NULL);
	if (ret == 0 && (cmd.response[0] & 0xFF) == SD_IF_COND_CHECK) {
		ocr |= SD_OCR_HOST_CAP_FLAG;
	}

	for (i = 0; i < SDHC_TEST_OCR_RETRIES; i++) {
		ret = card_cmd(&cmd, SD_APP_CMD, 0, SD_RSP_TYPE_R1, NULL);
		zassert_equal(ret, 0, "App command failed");
		ret = card_cmd(&cmd, SD_APP_SEND_OP_COND, ocr, SD_RSP_TYPE_R3, NULL);
		zassert_equal(ret, 0, "Send operating condition failed");
		if (cmd.response[0] & SD_OCR_PWR_BUSY_FLAG) {
			break;
		}
		k_msleep(10);
	}
	zassert_true(i < SDHC_TEST_OCR_RETRIES, "Card never left busy state");

	/* SDSC cards are addressed in bytes, high capacity cards in blocks */
	addr = SDHC_TEST_START_BLOCK;
	if (!(cmd.response[0] & SD_OCR_CARD_CAP_FLAG)) {
		addr *= SDHC_TEST_BLOCK_SIZE;
	}

	ret = card_cmd(&cmd, SD_ALL_SEND_CID, 0, SD_RSP_TYPE_R2, NULL);
	zassert_equal(ret, 0, "Reading card identification failed");
	do {
		ret = card_cmd(&cmd, SD_SEND_RELATIVE_ADDR, 0, SD_RSP_TYPE_R6, NULL);
		zassert_equal(ret, 0, "Requesting relative card address failed");
		rca = cmd.response[0] >> 16;
	} while (rca == 0);
	ret = card_cmd(&cmd, SD_SELECT_CARD, rca << 16, SD_RSP_TYPE_R1, NULL);
	zassert_equal(ret, 0, "Selecting card failed");
	card_wait_ready(rca);

	/* Use the bus the SD stack would use, as the driver may handle it differently */
	if (props.bus_4_bit_support) {
		ret = card_cmd(&cmd, SD_APP_CMD, rca << 16, SD_RSP_TYPE_R1, NULL);
		zassert_equal(ret, 0, "App command failed");
		ret = card_cmd(&cmd, SD_APP_SET_BUS_WIDTH, 2, SD_RSP_TYPE_R1, NULL);
		zassert_equal(ret, 0, "Setting card bus width failed");
		io.bus_width = SDHC_BUS_WIDTH4BIT;
	}
	/* Leave the identification clock for default speed data transfers */
	io.clock = MIN(props.f_max, SD_CLOCK_25MHZ);
	ret = sdhc_set_io(sdhc_dev, &io);
	zassert_equal(ret, 0, "Setting io configuration failed");

	card_transfer(SD_READ_MULTIPLE_BLOCK, addr, rca);
	card_transfer(SD_WRITE_MULTIPLE_BLOCK, addr, rca);

	ret = card_cmd(&cmd, SD_APP_CMD, rca << 16, SD_RSP_TYPE_R1, NULL);
	zassert_equal(ret, 0, "App command failed");
	zassert_true(cmd.response[0] & SD_R1_APP_CMD, "Card did not accept app command");
	written_blocks = 0;
	card_cmd_init(&req.cmd, SD_APP_SEND_NUM_WRITTEN_BLK, 0, SD_RSP_TYPE_R1);
	/* Keep the driver's own timeouts well below the hang timeout */
	req.cmd.retries = 0;
	ret = card_request_timeout(&req);
	zassert_equal(ret, 0, "Driver did not complete ACMD22 request, aborted it");
	zassert_equal(req.ret, 0, "Reading number of written blocks failed");
	zassert_equal(req.cmd.response[0] & SD_R1_ERR_FLAGS, 0, "ACMD22 reports error");
	zassert_equal(sys_be32_to_cpu(written_blocks), SDHC_TEST_BLOCK_COUNT,
		      "Card reports wrong number of written blocks");
}

ZTEST_SUITE(sdhc, NULL, sdhc_power_on, NULL, NULL, NULL);
