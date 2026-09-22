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

ZTEST_SUITE(sdhc, NULL, sdhc_power_on, NULL, NULL, NULL);
