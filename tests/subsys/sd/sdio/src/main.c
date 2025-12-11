/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sd/sdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/ztest.h>


static const struct device *sdhc_dev = DEVICE_DT_GET(DT_ALIAS(sdhc0));
static struct sd_card card;

/*
 * Verify that SD stack can initialize an SDIO card.
 * This test must run first, to ensure the card is initialized.
 */
ZTEST(sd_stack, test_0_init)
{
	int ret;

	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	ret = sd_is_card_present(sdhc_dev);
	zassert_equal(ret, 1, "SD card not present in slot");

	ret = sd_init(sdhc_dev, &card);
	zassert_equal(ret, 0, "Card initialization failed");
}

/*
 * Verify that a register read works. Given the custom nature of SDIO devices,
 * we just read from the card common I/O area.
 */
ZTEST(sd_stack, test_read)
{
	int ret;
	uint8_t reg = 0xFF;

	/* Read from card common I/O area. */
	ret = sdio_read_byte(&card.func0, SDIO_CCCR_CCCR, &reg);
	zassert_equal(ret, 0, "SD card read failed");
	/* Check to make sure CCCR read actually returned valid data */
	zassert_not_equal(reg, 0xFF, "CCCR read returned invalid data");
}

/*
 * Verify that a register write works. Given the custom nature of SDIO devices,
 * we just write to the card common I/O area.
 */
ZTEST(sd_stack, test_write)
{
	int ret;
	uint8_t data = 0x01;
	uint8_t reg = 0xFF;
	uint8_t new_reg_value = 0xFF;

	/* Read from card common I/O area. */
	ret = sdio_read_byte(&card.func0, SDIO_CCCR_BUS_IF, &reg);
	zassert_equal(ret, 0, "SD card read failed");

	/* Write to card common I/O area. */
	ret = sdio_write_byte(&card.func0, SDIO_CCCR_BUS_IF, data);
	zassert_equal(ret, 0, "SD card write failed");

	/* Read after write to verify that data was written */
	ret = sdio_read_byte(&card.func0, SDIO_CCCR_BUS_IF, &new_reg_value);
	zassert_equal(ret, 0, "SD card read failed");
	new_reg_value = new_reg_value & SDIO_CCCR_BUS_IF_WIDTH_MASK;
	zassert_equal(new_reg_value, data, "read data was not as expected");
}

/* Simply dump the card configuration. */
ZTEST(sd_stack, test_card_config)
{
	switch (card.card_voltage) {
	case SD_VOL_1_2_V:
		TC_PRINT("Card voltage: 1.2V\n");
		break;
	case SD_VOL_1_8_V:
		TC_PRINT("Card voltage: 1.8V\n");
		break;
	case SD_VOL_3_0_V:
		TC_PRINT("Card voltage: 3.0V\n");
		break;
	case SD_VOL_3_3_V:
		TC_PRINT("Card voltage: 3.3V\n");
		break;
	default:
		zassert_unreachable("Card voltage is not known value");
	}
	zassert_equal(card.status, CARD_INITIALIZED, "Card status is not OK");
	switch (card.card_speed) {
	case SD_TIMING_SDR12:
		TC_PRINT("Card timing: SDR12\n");
		break;
	case SD_TIMING_SDR25:
		TC_PRINT("Card timing: SDR25\n");
		break;
	case SD_TIMING_SDR50:
		TC_PRINT("Card timing: SDR50\n");
		break;
	case SD_TIMING_SDR104:
		TC_PRINT("Card timing: SDR104\n");
		break;
	case SD_TIMING_DDR50:
		TC_PRINT("Card timing: DDR50\n");
		break;
	default:
		zassert_unreachable("Card timing is not known value");
	}
	switch (card.type) {
	case CARD_SDIO:
		TC_PRINT("Card type: SDIO\n");
		break;
	case CARD_SDMMC:
		TC_PRINT("Card type: SDMMC\n");
		break;
	case CARD_COMBO:
		TC_PRINT("Card type: combo card\n");
		break;
	default:
		zassert_unreachable("Card type is not known value");
	}
	if (card.sd_version >= SD_SPEC_VER3_0) {
		TC_PRINT("Card spec: 3.0\n");
	} else if (card.sd_version >= SD_SPEC_VER2_0) {
		TC_PRINT("Card spec: 2.0\n");
	} else if (card.sd_version >= SD_SPEC_VER1_1) {
		TC_PRINT("Card spec: 1.1\n");
	} else if (card.sd_version >= SD_SPEC_VER1_0) {
		TC_PRINT("Card spec: 1.0\n");
	} else {
		zassert_unreachable("Card spec is unknown value");
	}
}

ZTEST_SUITE(sd_stack, NULL, NULL, NULL, NULL, NULL);
