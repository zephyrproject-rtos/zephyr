/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdmmc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/disk.h>

#include "sd_utils.h"
#include "sd_ops.h"

LOG_MODULE_DECLARE(sd, CONFIG_SD_LOG_LEVEL);

static inline void sdmmc_decode_scr(struct sd_scr *scr,
	uint32_t *raw_scr, uint8_t *version)
{
	uint32_t tmp_version = 0;

	scr->flags = 0U;
	scr->scr_structure = (uint8_t)((raw_scr[0U] & 0xF0000000U) >> 28U);
	scr->sd_spec = (uint8_t)((raw_scr[0U] & 0xF000000U) >> 24U);
	if ((uint8_t)((raw_scr[0U] & 0x800000U) >> 23U)) {
		scr->flags |= SD_SCR_DATA_STATUS_AFTER_ERASE;
	}
	scr->sd_sec = (uint8_t)((raw_scr[0U] & 0x700000U) >> 20U);
	scr->sd_width = (uint8_t)((raw_scr[0U] & 0xF0000U) >> 16U);
	if ((uint8_t)((raw_scr[0U] & 0x8000U) >> 15U)) {
		scr->flags |= SD_SCR_SPEC3;
	}
	scr->sd_ext_sec = (uint8_t)((raw_scr[0U] & 0x7800U) >> 10U);
	scr->cmd_support = (uint8_t)(raw_scr[0U] & 0x3U);
	scr->rsvd = raw_scr[1U];
	/* Get specification version. */
	switch (scr->sd_spec) {
	case 0U:
		tmp_version = SD_SPEC_VER1_0;
		break;
	case 1U:
		tmp_version = SD_SPEC_VER1_1;
		break;
	case 2U:
		tmp_version = SD_SPEC_VER2_0;
		if (scr->flags & SD_SCR_SPEC3) {
			tmp_version = SD_SPEC_VER3_0;
		}
		break;
	default:
		break;
	}

	if (version && tmp_version) {
		*version = tmp_version;
	}
}

/* Helper to send SD app command */
static int sdmmc_app_command(struct sd_card *card, int relative_card_address)
{
	return card_app_command(card, relative_card_address);
}

/* Reads OCR from SPI mode card using CMD58 */
static int sdmmc_spi_send_ocr(struct sd_card *card, uint32_t arg)
{
	struct sdhc_command cmd;
	int ret;

	cmd.opcode = SD_SPI_READ_OCR;
	cmd.arg = arg;
	cmd.response_type = SD_SPI_RSP_TYPE_R3;

	ret = sdhc_request(card->sdhc, &cmd, NULL);

	if (ret) {
		LOG_DBG("CMD58 failed: %d", ret);
		return ret;
	}

	card->ocr = cmd.response[1];
	if (card->ocr == 0) {
		LOG_DBG("No OCR detected");
		return -ENOTSUP;
	}

	return ret;
}

/* Sends OCR to card using ACMD41 */
static int sdmmc_send_ocr(struct sd_card *card, int ocr)
{
	struct sdhc_command cmd;
	int ret;
	int retries;

	cmd.opcode = SD_APP_SEND_OP_COND;
	cmd.arg = ocr;
	cmd.response_type = (SD_RSP_TYPE_R3 | SD_SPI_RSP_TYPE_R1);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	/* Send initialization ACMD41 */
	for (retries = 0; retries < CONFIG_SD_OCR_RETRY_COUNT; retries++) {
		ret = sdmmc_app_command(card, 0U);
		if (ret == SD_RETRY) {
			/* Retry */
			continue;
		} else if (ret) {
			return ret;
		}
		ret = sdhc_request(card->sdhc, &cmd, NULL);
		if (ret) {
			/* OCR failed */
			return ret;
		}
		if (ocr == 0) {
			/* Just probing, don't wait for card to exit busy state */
			return 0;
		}
		/*
		 * Check to see if card is busy with power up. PWR_BUSY
		 * flag will be cleared when card finishes power up sequence
		 */
		if (card->host_props.is_spi) {
			if (!(cmd.response[0] & SD_SPI_R1IDLE_STATE)) {
				break;
			}
		} else {
			if ((cmd.response[0U] & SD_OCR_PWR_BUSY_FLAG)) {
				break;
			}
		}
		sd_delay(10);
	}
	if (retries >= CONFIG_SD_OCR_RETRY_COUNT) {
		/* OCR timed out */
		LOG_ERR("Card never left busy state");
		return -ETIMEDOUT;
	}
	LOG_DBG("SDMMC responded to ACMD41 after %d attempts", retries);
	if (!card->host_props.is_spi) {
		/* Save OCR */
		card->ocr = cmd.response[0U];
	}
	return 0;
}

/* Reads SD configuration register */
static int sdmmc_read_scr(struct sd_card *card)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	/* Place SCR struct on stack to reduce flash usage */
	struct sd_scr card_scr;
	int ret;
	/* DMA onto stack is unsafe, so we use an internal card buffer */
	uint32_t *scr = (uint32_t *)card->card_buffer;
	uint32_t raw_scr[2];

	ret = sdmmc_app_command(card, card->relative_addr);
	if (ret) {
		LOG_DBG("SD app command failed for SD SCR");
		return ret;
	}

	cmd.opcode = SD_APP_SEND_SCR;
	cmd.arg = 0;
	cmd.response_type =  (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	data.block_size = 8U;
	data.blocks = 1U;
	data.data = scr;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		LOG_DBG("ACMD51 failed: %d", ret);
		return ret;
	}
	/* Decode SCR */
	raw_scr[0] = sys_be32_to_cpu(scr[0]);
	raw_scr[1] = sys_be32_to_cpu(scr[1]);
	sdmmc_decode_scr(&card_scr, raw_scr, &card->sd_version);
	LOG_DBG("SD reports specification version %d", card->sd_version);
	/* Check card supported bus width */
	if (card_scr.sd_width & 0x4U) {
		card->flags |= SD_4BITS_WIDTH;
	}
	/* Check if card supports speed class command (CMD20) */
	if (card_scr.cmd_support & 0x1U) {
		card->flags |= SD_SPEED_CLASS_CONTROL_FLAG;
	}
	/* Check for set block count (CMD 23) support */
	if (card_scr.cmd_support & 0x2U) {
		card->flags |= SD_CMD23_FLAG;
	}
	return 0;
}

/* Sets block length of SD card */
static int sdmmc_set_blocklen(struct sd_card *card, uint32_t block_len)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_SET_BLOCK_SIZE;
	cmd.arg = block_len;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	cmd.response_type =  (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);

	return sdhc_request(card->sdhc, &cmd, NULL);
}

/*
 * Sets bus width of host and card, following section 3.4 of
 * SD host controller specification
 */
static int sdmmc_set_bus_width(struct sd_card *card, enum sdhc_bus_width width)
{
	struct sdhc_command cmd = {0};
	int ret;

	/*
	 * The specification strictly requires card interrupts to be masked, but
	 * Linux does not do so, so we won't either.
	 */
	/* Send ACMD6 to change bus width */
	ret = sdmmc_app_command(card, card->relative_addr);
	if (ret) {
		LOG_DBG("SD app command failed for ACMD6");
		return ret;
	}
	cmd.opcode = SD_APP_SET_BUS_WIDTH;
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		cmd.arg = 0U;
		break;
	case SDHC_BUS_WIDTH4BIT:
		cmd.arg = 2U;
		break;
	default:
		return -ENOTSUP;
	}
	/* Send app command */
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		LOG_DBG("Error on ACMD6: %d", ret);
		return ret;
	}
	ret = sd_check_response(&cmd);
	if (ret) {
		LOG_DBG("ACMD6 reports error, response 0x%x", cmd.response[0U]);
		return ret;
	}
	/* Card now has changed bus width. Change host bus width */
	card->bus_io.bus_width = width;
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		LOG_DBG("Could not change host bus width");
	}
	return ret;
}

/*
 * Sends SD switch function CMD6.
 * See table 4-32 in SD physical specification for argument details.
 * When setting a function, we should set the 4 bit block of the command
 * argument corresponding to that function to "value", and all other 4 bit
 * blocks should be left as 0xF (no effect on current function)
 */
static int sdmmc_switch(struct sd_card *card, enum sd_switch_arg mode,
	enum sd_group_num group, uint8_t value, uint8_t *response)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	cmd.opcode = SD_SWITCH;
	cmd.arg = ((mode & 0x1) << 31) | 0x00FFFFFF;
	cmd.arg &= ~(0xFU << (group * 4));
	cmd.arg |= (value & 0xF) << (group * 4);
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	data.block_size = 64U;
	data.blocks = 1;
	data.data = response;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;

	return sdhc_request(card->sdhc, &cmd, &data);
}

static int sdmmc_read_switch(struct sd_card *card)
{
	uint8_t *status;
	int ret;

	if (card->sd_version < SD_SPEC_VER1_1) {
		/* Switch not supported */
		LOG_INF("SD spec 1.01 does not support CMD6");
		return 0;
	}
	/* Use card internal buffer to read 64 byte switch data */
	status = card->card_buffer;
	/*
	 * Setting switch to zero will read card's support values,
	 * otherwise known as SD "check function"
	 */
	ret = sdmmc_switch(card, SD_SWITCH_CHECK, 0, 0, status);
	if (ret) {
		LOG_DBG("CMD6 failed %d", ret);
		return ret;
	}
	/*
	 * See table 4-11 and 4.3.10.4 of physical layer specification for
	 * bit definitions. Note that response is big endian, so index 13 will
	 * read bits 400-408.
	 * Bit n being set in support bit field indicates support for function
	 * number n on the card. (So 0x3 indicates support for functions 0 and 1)
	 */
	if (status[13] & HIGH_SPEED_BUS_SPEED) {
		card->switch_caps.hs_max_dtr = HS_MAX_DTR;
	}
	if (card->sd_version >= SD_SPEC_VER3_0) {
		card->switch_caps.bus_speed = status[13];
		card->switch_caps.sd_drv_type = status[9];
		card->switch_caps.sd_current_limit = status[7];
	}
	return 0;
}

static inline void sdmmc_select_bus_speed(struct sd_card *card)
{
	/*
	 * Note that function support is defined using bitfields, but function
	 * selection is defined using values 0x0-0xF.
	 */
	if (card->host_props.host_caps.sdr104_support &&
		(card->switch_caps.bus_speed & UHS_SDR104_BUS_SPEED) &&
		(card->host_props.f_max >= SD_CLOCK_208MHZ)) {
		card->card_speed = SD_TIMING_SDR104;
	} else if (card->host_props.host_caps.ddr50_support &&
		(card->switch_caps.bus_speed & UHS_DDR50_BUS_SPEED) &&
		(card->host_props.f_max >= SD_CLOCK_50MHZ)) {
		card->card_speed = SD_TIMING_DDR50;
	} else if (card->host_props.host_caps.sdr50_support &&
		(card->switch_caps.bus_speed & UHS_SDR50_BUS_SPEED) &&
		(card->host_props.f_max >= SD_CLOCK_100MHZ)) {
		card->card_speed = SD_TIMING_SDR50;
	} else if (card->host_props.host_caps.high_spd_support &&
		(card->switch_caps.bus_speed & UHS_SDR12_BUS_SPEED) &&
		(card->host_props.f_max >= SD_CLOCK_25MHZ)) {
		card->card_speed = SD_TIMING_SDR12;
	}
}

/* Selects driver type for SD card */
static int sdmmc_select_driver_type(struct sd_card *card)
{
	int ret = 0;
	uint8_t *status = card->card_buffer;

	/*
	 * We will only attempt to use driver type C over the default of type B,
	 * since it should result in lower current consumption if supported.
	 */
	if (card->host_props.host_caps.drv_type_c_support &&
		(card->switch_caps.sd_drv_type & SD_DRIVER_TYPE_C)) {
		card->bus_io.driver_type = SD_DRIVER_TYPE_C;
		/* Change drive strength */
		ret = sdmmc_switch(card, SD_SWITCH_SET,
			SD_GRP_DRIVER_STRENGTH_MODE,
			(find_msb_set(SD_DRIVER_TYPE_C) - 1), status);
	}
	return ret;
}

/* Sets current limit for SD card */
static int sdmmc_set_current_limit(struct sd_card *card)
{
	int ret;
	int max_current = -1;
	uint8_t *status = card->card_buffer;

	if ((card->card_speed != SD_TIMING_SDR50) &&
		(card->card_speed != SD_TIMING_SDR104) &&
		(card->card_speed != SD_TIMING_DDR50)) {
		return 0; /* Cannot set current limit */
	} else if (card->host_props.max_current_180 >= 800 &&
		(card->switch_caps.sd_current_limit & SD_MAX_CURRENT_800MA)) {
		max_current = SD_SET_CURRENT_800MA;
	} else if (card->host_props.max_current_180 >= 600 &&
		(card->switch_caps.sd_current_limit & SD_MAX_CURRENT_600MA)) {
		max_current = SD_SET_CURRENT_600MA;
	} else if (card->host_props.max_current_180 >= 400 &&
		(card->switch_caps.sd_current_limit & SD_MAX_CURRENT_400MA)) {
		max_current = SD_SET_CURRENT_400MA;
	} else if (card->host_props.max_current_180 >= 200 &&
		(card->switch_caps.sd_current_limit & SD_MAX_CURRENT_200MA)) {
		max_current = SD_SET_CURRENT_200MA;
	}
	if (max_current != -1) {
		LOG_DBG("Changing SD current limit: %d", max_current);
		/* Switch SD current */
		ret = sdmmc_switch(card, SD_SWITCH_SET, SD_GRP_CURRENT_LIMIT_MODE,
			max_current, status);
		if (ret) {
			LOG_DBG("Failed to set SD current limit");
			return ret;
		}
		if (((status[15] >> 4) & 0x0F) != max_current) {
			/* Status response indicates card did not select request limit */
			LOG_WRN("Card did not accept current limit");
		}
	}
	return 0;
}

/* Applies selected card bus speed to card and host */
static int sdmmc_set_bus_speed(struct sd_card *card)
{
	int ret;
	int timing = 0;
	uint8_t *status = card->card_buffer;

	switch (card->card_speed) {
	/* Set bus clock speed */
	case SD_TIMING_SDR104:
		card->switch_caps.uhs_max_dtr = SD_CLOCK_208MHZ;
		timing = SDHC_TIMING_SDR104;
		break;
	case SD_TIMING_DDR50:
		card->switch_caps.uhs_max_dtr = SD_CLOCK_50MHZ;
		timing = SDHC_TIMING_DDR50;
		break;
	case SD_TIMING_SDR50:
		card->switch_caps.uhs_max_dtr = SD_CLOCK_100MHZ;
		timing = SDHC_TIMING_SDR50;
		break;
	case SD_TIMING_SDR25:
		card->switch_caps.uhs_max_dtr = SD_CLOCK_50MHZ;
		timing = SDHC_TIMING_SDR25;
		break;
	case SD_TIMING_SDR12:
		card->switch_caps.uhs_max_dtr = SD_CLOCK_25MHZ;
		timing = SDHC_TIMING_SDR12;
		break;
	default:
		/* No need to change bus speed */
		return 0;
	}

	/* Switch bus speed */
	ret = sdmmc_switch(card, SD_SWITCH_SET, SD_GRP_TIMING_MODE,
		card->card_speed, status);
	if (ret) {
		LOG_DBG("Failed to switch SD card speed");
		return ret;
	}
	if ((status[16] & 0xF) != card->card_speed) {
		LOG_WRN("Card did not accept new speed");
	} else {
		/* Change host bus speed */
		card->bus_io.timing = timing;
		card->bus_io.clock = card->switch_caps.uhs_max_dtr;
		LOG_DBG("Setting bus clock to: %d", card->bus_io.clock);
		ret = sdhc_set_io(card->sdhc, &card->bus_io);
		if (ret) {
			LOG_ERR("Failed to change host bus speed");
			return ret;
		}
	}
	return 0;
}

/*
 * Init UHS capable SD card. Follows figure 3-16 in physical layer specification.
 */
static int sdmmc_init_uhs(struct sd_card *card)
{
	int ret;

	/* Raise bus width to 4 bits */
	ret = sdmmc_set_bus_width(card, SDHC_BUS_WIDTH4BIT);
	if (ret) {
		LOG_ERR("Failed to change card bus width to 4 bits");
		return ret;
	}

	/* Select bus speed for card depending on host and card capability*/
	sdmmc_select_bus_speed(card);
	/* Now, set the driver strength for the card */
	ret = sdmmc_select_driver_type(card);
	if (ret) {
		LOG_DBG("Failed to select new driver type");
		return ret;
	}
	ret = sdmmc_set_current_limit(card);
	if (ret) {
		LOG_DBG("Failed to set card current limit");
		return ret;
	}
	/* Apply the bus speed selected earlier */
	ret = sdmmc_set_bus_speed(card);
	if (ret) {
		LOG_DBG("Failed to set card bus speed");
		return ret;
	}
	if (card->card_speed == SD_TIMING_SDR50 ||
		card->card_speed == SD_TIMING_SDR104 ||
		card->card_speed == SD_TIMING_DDR50) {
		/* SDR104, SDR50, and DDR50 mode need tuning */
		ret = sdhc_execute_tuning(card->sdhc);
		if (ret) {
			LOG_ERR("SD tuning failed: %d", ret);
		}
	}
	return ret;
}

/* Performs initialization for SD high speed cards */
static int sdmmc_init_hs(struct sd_card *card)
{
	int ret;

	if ((!card->host_props.host_caps.high_spd_support) ||
		(card->sd_version < SD_SPEC_VER1_1) ||
		(card->switch_caps.hs_max_dtr == 0)) {
		/* No high speed support. Leave card untouched */
		return 0;
	}
	card->card_speed = SD_TIMING_SDR25;
	ret = sdmmc_set_bus_speed(card);
	if (ret) {
		LOG_ERR("Failed to switch card to HS mode");
		return ret;
	}
	if (card->flags & SD_4BITS_WIDTH) {
		/* Raise bus width to 4 bits */
		ret = sdmmc_set_bus_width(card, SDHC_BUS_WIDTH4BIT);
		if (ret) {
			LOG_ERR("Failed to change card bus width to 4 bits");
			return ret;
		}
	}
	return 0;
}

/*
 * Initializes SDMMC card. Note that the common SD function has already
 * sent CMD0 and CMD8 to the card at function entry.
 */
int sdmmc_card_init(struct sd_card *card)
{
	int ret;
	uint32_t ocr_arg = 0U;

	/* First send a probing OCR */
	if (card->host_props.is_spi && IS_ENABLED(CONFIG_SDHC_SUPPORTS_SPI_MODE)) {
		/* Probe SPI card with CMD58*/
		ret = sdmmc_spi_send_ocr(card, ocr_arg);
	} else if (IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		/* Probe Native card with ACMD41 */
		ret = sdmmc_send_ocr(card, ocr_arg);
	} else {
		return -ENOTSUP;
	}
	if (ret) {
		return ret;
	}
	/* Card responded to ACMD41, type is SDMMC */
	card->type = CARD_SDMMC;

	if (card->flags & SD_SDHC_FLAG) {
		if (IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
			/* High capacity card. See if host supports 1.8V */
			if (card->host_props.host_caps.vol_180_support) {
				ocr_arg |= SD_OCR_SWITCH_18_REQ_FLAG;
			}
		}
		/* Set host high capacity support flag */
		ocr_arg |= SD_OCR_HOST_CAP_FLAG;
	}
	if (IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		/* Set voltage window */
		if (card->host_props.host_caps.vol_300_support) {
			ocr_arg |= SD_OCR_VDD29_30FLAG;
		}
		ocr_arg |= (SD_OCR_VDD32_33FLAG | SD_OCR_VDD33_34FLAG);
	}
	/* Momentary delay before initialization OCR. Some cards will
	 * never leave busy state if init OCR is sent too soon after
	 * probing OCR
	 */
	k_busy_wait(100);
	/* Send SD OCR to card to initialize it */
	ret = sdmmc_send_ocr(card, ocr_arg);
	if (ret) {
		LOG_ERR("Failed to query card OCR");
		return ret;
	}
	if (card->host_props.is_spi && IS_ENABLED(CONFIG_SDHC_SUPPORTS_SPI_MODE)) {
		/* Send second CMD58 to get CCS bit */
		ret = sdmmc_spi_send_ocr(card, ocr_arg);
		if (ret) {
			return ret;
		}
	}
	/* Check SD high capacity and 1.8V support flags */
	if (card->ocr & SD_OCR_CARD_CAP_FLAG) {
		card->flags |= SD_HIGH_CAPACITY_FLAG;
	}
	if (card->ocr & SD_OCR_SWITCH_18_ACCEPT_FLAG) {
		LOG_DBG("Card supports 1.8V signaling");
		card->flags |= SD_1800MV_FLAG;
	}
	/* Check OCR voltage window */
	if (card->ocr & SD_OCR_VDD29_30FLAG) {
		card->flags |= SD_3000MV_FLAG;
	}
	/*
	 * If card is high capacity (SDXC or SDHC), and supports 1.8V signaling,
	 * switch to new signal voltage using "signal voltage switch procedure"
	 * described in SD specification
	 */
	if ((card->flags & SD_1800MV_FLAG) &&
		(card->host_props.host_caps.vol_180_support) &&
		(!card->host_props.is_spi) &&
		IS_ENABLED(CONFIG_SD_UHS_PROTOCOL)) {
		ret = sdmmc_switch_voltage(card);
		if (ret) {
			/* Disable host support for 1.8 V */
			card->host_props.host_caps.vol_180_support = false;
			/*
			 * The host or SD card may have already switched to
			 * 1.8V. Return SD_RESTART to indicate
			 * negotiation should be restarted.
			 */
			card->status = CARD_ERROR;
			return SD_RESTART;
		}
	}
	/* Read the card's CID (card identification register) */
	ret = card_read_cid(card);
	if (ret) {
		return ret;
	}
	if (!card->host_props.is_spi && IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		/*
		 * Request new relative card address. This moves the card from
		 * identification mode to data transfer mode
		 */
		ret = sdmmc_request_rca(card);
		if (ret) {
			return ret;
		}
	}
	/* Card has entered data transfer mode. Get card specific data register */
	ret = sdmmc_read_csd(card);
	if (ret) {
		return ret;
	}
	if (!card->host_props.is_spi && IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		/* Move the card to transfer state (with CMD7) to run remaining commands */
		ret = sdmmc_select_card(card);
		if (ret) {
			return ret;
		}
	}
	/*
	 * With card in data transfer state, we can set SD clock to maximum
	 * frequency for non high speed mode (25Mhz)
	 */
	if (card->host_props.f_max < SD_CLOCK_25MHZ) {
		LOG_INF("Maximum SD clock is under 25MHz, using clock of %dHz",
			card->host_props.f_max);
		card->bus_io.clock = card->host_props.f_max;
	} else {
		card->bus_io.clock = SD_CLOCK_25MHZ;
	}
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		LOG_ERR("Failed to raise bus frequency to 25MHz");
		return ret;
	}
	/* Read SD SCR (SD configuration register),
	 * to get supported bus width
	 */
	ret = sdmmc_read_scr(card);
	if (ret) {
		return ret;
	}
	/* Read switch capabilities to determine what speeds card supports */
	if (!card->host_props.is_spi && IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		ret = sdmmc_read_switch(card);
		if (ret) {
			LOG_ERR("Failed to read card functions");
			return ret;
		}
	}
	if ((card->flags & SD_1800MV_FLAG) &&
		sdmmc_host_uhs(&card->host_props) &&
		!(card->host_props.is_spi) &&
		IS_ENABLED(CONFIG_SD_UHS_PROTOCOL)) {
		ret = sdmmc_init_uhs(card);
		if (ret) {
			LOG_ERR("UHS card init failed");
		}
	} else {
		if ((card->flags & SD_HIGH_CAPACITY_FLAG) == 0) {
			/* Standard capacity SDSC card. set block length to 512 */
			ret = sdmmc_set_blocklen(card, SDMMC_DEFAULT_BLOCK_SIZE);
			if (ret) {
				LOG_ERR("Could not set SD blocklen to 512");
				return ret;
			}
			card->block_size = 512;
		}
		/* Card is not UHS. Try to use high speed mode */
		ret = sdmmc_init_hs(card);
		if (ret) {
			LOG_ERR("HS card init failed");
		}
	}
	return ret;
}

int sdmmc_ioctl(struct sd_card *card, uint8_t cmd, void *buf)
{
	return card_ioctl(card, cmd, buf);
}

int sdmmc_read_blocks(struct sd_card *card, uint8_t *rbuf,
	uint32_t start_block, uint32_t num_blocks)
{
	return card_read_blocks(card, rbuf, start_block, num_blocks);
}

int sdmmc_write_blocks(struct sd_card *card, const uint8_t *wbuf,
	uint32_t start_block, uint32_t num_blocks)
{
	return card_write_blocks(card, wbuf, start_block, num_blocks);
}
