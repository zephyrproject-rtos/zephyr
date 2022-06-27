/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdmmc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/disk.h>

#include "sd_utils.h"

LOG_MODULE_DECLARE(sd, CONFIG_SD_LOG_LEVEL);


static inline void sdmmc_decode_csd(struct sd_csd *csd,
	uint32_t *raw_csd, uint32_t *blk_cout, uint32_t *blk_size)
{
	uint32_t tmp_blk_cout, tmp_blk_size;

	csd->csd_structure = (uint8_t)((raw_csd[3U] &
		0xC0000000U) >> 30U);
	csd->read_time1 = (uint8_t)((raw_csd[3U] &
		0xFF0000U) >> 16U);
	csd->read_time2 = (uint8_t)((raw_csd[3U] &
		0xFF00U) >> 8U);
	csd->xfer_rate = (uint8_t)(raw_csd[3U] &
		0xFFU);
	csd->cmd_class = (uint16_t)((raw_csd[2U] &
		0xFFF00000U) >> 20U);
	csd->read_blk_len = (uint8_t)((raw_csd[2U] &
		0xF0000U) >> 16U);
	if (raw_csd[2U] & 0x8000U) {
		csd->flags |= SD_CSD_READ_BLK_PARTIAL_FLAG;
	}
	if (raw_csd[2U] & 0x4000U) {
		csd->flags |= SD_CSD_READ_BLK_PARTIAL_FLAG;
	}
	if (raw_csd[2U] & 0x2000U) {
		csd->flags |= SD_CSD_READ_BLK_MISALIGN_FLAG;
	}
	if (raw_csd[2U] & 0x1000U) {
		csd->flags |= SD_CSD_DSR_IMPLEMENTED_FLAG;
	}

	switch (csd->csd_structure) {
	case 0:
		csd->device_size = (uint32_t)((raw_csd[2U] &
			0x3FFU) << 2U);
		csd->device_size |= (uint32_t)((raw_csd[1U] &
			0xC0000000U) >> 30U);
		csd->read_current_min = (uint8_t)((raw_csd[1U] &
			0x38000000U) >> 27U);
		csd->read_current_max = (uint8_t)((raw_csd[1U] &
			0x7000000U) >> 24U);
		csd->write_current_min = (uint8_t)((raw_csd[1U] &
			0xE00000U) >> 20U);
		csd->write_current_max = (uint8_t)((raw_csd[1U] &
			0x1C0000U) >> 18U);
		csd->dev_size_mul = (uint8_t)((raw_csd[1U] &
			0x38000U) >> 15U);

		/* Get card total block count and block size. */
		tmp_blk_cout = ((csd->device_size + 1U) <<
			(csd->dev_size_mul + 2U));
		tmp_blk_size = (1U << (csd->read_blk_len));
		if (tmp_blk_size != SDMMC_DEFAULT_BLOCK_SIZE) {
			tmp_blk_cout = (tmp_blk_cout * tmp_blk_size);
			tmp_blk_size = SDMMC_DEFAULT_BLOCK_SIZE;
			tmp_blk_cout = (tmp_blk_cout / tmp_blk_size);
		}
		if (blk_cout) {
			*blk_cout = tmp_blk_cout;
		}
		if (blk_size) {
			*blk_size = tmp_blk_size;
		}
		break;
	case 1:
		tmp_blk_size = SDMMC_DEFAULT_BLOCK_SIZE;

		csd->device_size = (uint32_t)((raw_csd[2U] &
			0x3FU) << 16U);
		csd->device_size |= (uint32_t)((raw_csd[1U] &
			0xFFFF0000U) >> 16U);

		tmp_blk_cout = ((csd->device_size + 1U) * 1024U);
		if (blk_cout) {
			*blk_cout = tmp_blk_cout;
		}
		if (blk_size) {
			*blk_size = tmp_blk_size;
		}
		break;
	default:
		break;
	}

	if ((uint8_t)((raw_csd[1U] & 0x4000U) >> 14U)) {
		csd->flags |= SD_CSD_ERASE_BLK_EN_FLAG;
	}
	csd->erase_size = (uint8_t)((raw_csd[1U] &
		0x3F80U) >> 7U);
	csd->write_prtect_size = (uint8_t)(raw_csd[1U] &
		0x7FU);
	csd->write_speed_factor = (uint8_t)((raw_csd[0U] &
		0x1C000000U) >> 26U);
	csd->write_blk_len = (uint8_t)((raw_csd[0U] &
		0x3C00000U) >> 22U);
	if ((uint8_t)((raw_csd[0U] & 0x200000U) >> 21U)) {
		csd->flags |= SD_CSD_WRITE_BLK_PARTIAL_FLAG;
	}
	if ((uint8_t)((raw_csd[0U] & 0x8000U) >> 15U)) {
		csd->flags |= SD_CSD_FILE_FMT_GRP_FLAG;
	}
	if ((uint8_t)((raw_csd[0U] & 0x4000U) >> 14U)) {
		csd->flags |= SD_CSD_COPY_FLAG;
	}
	if ((uint8_t)((raw_csd[0U] & 0x2000U) >> 13U)) {
		csd->flags |= SD_CSD_PERMANENT_WRITE_PROTECT_FLAG;
	}
	if ((uint8_t)((raw_csd[0U] & 0x1000U) >> 12U)) {
		csd->flags |= SD_CSD_TMP_WRITE_PROTECT_FLAG;
	}
	csd->file_fmt = (uint8_t)((raw_csd[0U] & 0xC00U) >> 10U);
}

static inline void sdmmc_decode_scr(struct sd_scr *scr,
	uint32_t *raw_scr, uint32_t *version)
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

static inline void sdmmc_decode_cid(struct sd_cid *cid,
	uint32_t *raw_cid)
{
	cid->manufacturer = (uint8_t)((raw_cid[3U] & 0xFF000000U) >> 24U);
	cid->application = (uint16_t)((raw_cid[3U] & 0xFFFF00U) >> 8U);

	cid->name[0U] = (uint8_t)((raw_cid[3U] & 0xFFU));
	cid->name[1U] = (uint8_t)((raw_cid[2U] & 0xFF000000U) >> 24U);
	cid->name[2U] = (uint8_t)((raw_cid[2U] & 0xFF0000U) >> 16U);
	cid->name[3U] = (uint8_t)((raw_cid[2U] & 0xFF00U) >> 8U);
	cid->name[4U] = (uint8_t)((raw_cid[2U] & 0xFFU));

	cid->version = (uint8_t)((raw_cid[1U] & 0xFF000000U) >> 24U);

	cid->ser_num = (uint32_t)((raw_cid[1U] & 0xFFFFFFU) << 8U);
	cid->ser_num |= (uint32_t)((raw_cid[0U] & 0xFF000000U) >> 24U);

	cid->date = (uint16_t)((raw_cid[0U] & 0xFFF00U) >> 8U);
}

/* Checks SD status return codes */
static inline int sdmmc_check_response(struct sdhc_command *cmd)
{
	if (cmd->response_type == SD_RSP_TYPE_R1) {
		return (cmd->response[0U] & SD_R1_ERR_FLAGS);
	}
	return 0;
}

/* Helper to send SD app command */
static int sdmmc_app_command(struct sd_card *card, int relative_card_address)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = SD_APP_CMD;
	cmd.arg = relative_card_address << 16U;
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		/* We want to retry transmission */
		return SD_RETRY;
	}
	ret = sdmmc_check_response(&cmd);
	if (ret) {
		LOG_WRN("SD app command failed with R1 response of 0x%X",
			cmd.response[0]);
		return -EIO;
	}
	/* Check application command flag to determine if card is ready for APP CMD */
	if ((!card->host_props.is_spi) && !(cmd.response[0U] & SD_R1_APP_CMD)) {
		/* Command succeeded, but card not ready for app command. No APP CMD support */
		return -ENOTSUP;
	}
	return 0;
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

	card->ocr = cmd.response[1];
	return ret;
}

/* Sends OCR to card using ACMD41 */
static int sdmmc_send_ocr(struct sd_card *card, int ocr_arg)
{
	struct sdhc_command cmd;
	int ret;
	int retries;

	cmd.opcode = SD_APP_SEND_OP_COND;
	cmd.arg = ocr_arg;
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
		if (ocr_arg == 0) {
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

/*
 * Implements signal voltage switch procedure described in section 3.6.1 of
 * SD specification.
 */
static int sdmmc_switch_voltage(struct sd_card *card)
{
	int ret, sd_clock;
	struct sdhc_command cmd = {0};

	/* Check to make sure card supports 1.8V */
	if (!(card->flags & SD_1800MV_FLAG)) {
		/* Do not attempt to switch voltages */
		LOG_WRN("SD card reports as SDHC/SDXC, but does not support 1.8V");
		return 0;
	}
	/* Send CMD11 to request a voltage switch */
	cmd.opcode = SD_VOL_SWITCH;
	cmd.arg = 0U;
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		LOG_DBG("CMD11 failed");
		return ret;
	}
	/* Check R1 response for error */
	ret = sdmmc_check_response(&cmd);
	if (ret) {
		LOG_DBG("SD response to CMD11 indicates error");
		return ret;
	}
	/*
	 * Card should drive CMD and DAT[3:0] signals low at the next clock
	 * cycle. Some cards will only drive these
	 * lines low briefly, so we should check as soon as possible
	 */
	if (!(sdhc_card_busy(card->sdhc))) {
		/* Delay 1ms to allow card to drive lines low */
		sd_delay(1);
		if (!sdhc_card_busy(card->sdhc)) {
			/* Card did not drive CMD and DAT lines low */
			LOG_DBG("Card did not drive DAT lines low");
			return -EAGAIN;
		}
	}
	/*
	 * Per SD spec (section "Timing to Switch Signal Voltage"),
	 * host must gate clock at least 5ms.
	 */
	sd_clock = card->bus_io.clock;
	card->bus_io.clock = 0;
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		LOG_DBG("Failed to gate SD clock");
		return ret;
	}
	/* Now that clock is gated, change signal voltage */
	card->bus_io.signal_voltage = SD_VOL_1_8_V;
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		LOG_DBG("Failed to switch SD host to 1.8V");
		return ret;
	}
	sd_delay(10); /* Gate for 10ms, even though spec requires 5 */
	/* Restart the clock */
	card->bus_io.clock = sd_clock;
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		LOG_ERR("Failed to restart SD clock");
		return ret;
	}
	/*
	 * If SD does not drive at least one of
	 * DAT[3:0] high within 1ms, switch failed
	 */
	sd_delay(1);
	if (sdhc_card_busy(card->sdhc)) {
		LOG_DBG("Card failed to switch voltages");
		return -EAGAIN;
	}
	card->card_voltage = SD_VOL_1_8_V;
	LOG_INF("Card switched to 1.8V signaling");
	return 0;
}


/* Reads card id/csd register (in SPI mode) */
static int sdmmc_spi_read_cxd(struct sd_card *card,
	uint32_t opcode, uint32_t *cxd)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	int ret, i;
	/* Use internal card buffer for data transfer */
	uint32_t *cxd_be = (uint32_t *)card->card_buffer;

	cmd.opcode = opcode;
	cmd.arg = 0;
	cmd.response_type = SD_SPI_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	/* CID/CSD is 16 bytes */
	data.block_size = 16;
	data.blocks = 1U;
	data.data = cxd_be;
	data.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		LOG_DBG("CMD%d failed: %d", opcode, ret);
	}
	/* Swap endianness of CXD */
	for (i = 0; i < 4; i++) {
		cxd[3-i] = sys_be32_to_cpu(cxd_be[i]);
	}
	return 0;
}

/* Reads card id/csd register (native SD mode */
static int sdmmc_read_cxd(struct sd_card *card,
	uint32_t opcode, uint32_t rca, uint32_t *cxd)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = opcode;
	cmd.arg = (rca << 16);
	cmd.response_type = SD_RSP_TYPE_R2;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		LOG_DBG("CMD%d failed: %d", opcode, ret);
		return ret;
	}

	/* CSD/CID is 16 bytes */
	memcpy(cxd, cmd.response, 16);
	return 0;
}

/* Reads card identification register, and decodes it */
static int sdmmc_read_cid(struct sd_card *card)
{
	uint32_t cid[4];
	int ret;
	/* Keep CID on stack for reduced RAM usage */
	struct sd_cid card_cid;

	if (card->host_props.is_spi && IS_ENABLED(CONFIG_SDHC_SUPPORTS_SPI_MODE)) {
		ret = sdmmc_spi_read_cxd(card, SD_SEND_CID, cid);
	} else if (IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		ret = sdmmc_read_cxd(card, SD_ALL_SEND_CID, 0, cid);
	} else {
		/* The host controller must run in either native or SPI mode */
		return -ENOTSUP;
	}
	if (ret) {
		return ret;
	}

	/* Decode SD CID */
	sdmmc_decode_cid(&card_cid, cid);
	LOG_DBG("Card MID: 0x%x, OID: %c%c", card_cid.manufacturer,
		((char *)&card_cid.application)[0],
		((char *)&card_cid.application)[1]);
	return 0;
}

/*
 * Requests card to publish a new relative card address, and move from
 * identification to data mode
 */
static int sdmmc_request_rca(struct sd_card *card)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = SD_SEND_RELATIVE_ADDR;
	cmd.arg = 0;
	cmd.response_type = SD_RSP_TYPE_R6;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	/* Issue CMD3 until card responds with nonzero RCA */
	do {
		ret = sdhc_request(card->sdhc, &cmd, NULL);
		if (ret) {
			LOG_DBG("CMD3 failed");
			return ret;
		}
		/* Card RCA is in upper 16 bits of response */
		card->relative_addr = ((cmd.response[0U] & 0xFFFF0000) >> 16U);
	} while (card->relative_addr == 0U);
	LOG_DBG("Card relative addr: %d", card->relative_addr);
	return 0;
}

/* Read card specific data register */
static int sdmmc_read_csd(struct sd_card *card)
{
	int ret;
	uint32_t csd[4];
	/* Keep CSD on stack for reduced RAM usage */
	struct sd_csd card_csd;

	if (card->host_props.is_spi && IS_ENABLED(CONFIG_SDHC_SUPPORTS_SPI_MODE)) {
		ret = sdmmc_spi_read_cxd(card, SD_SEND_CSD, csd);
	} else if (IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		ret = sdmmc_read_cxd(card, SD_SEND_CSD, card->relative_addr, csd);
	} else {
		/* The host controller must run in either native or SPI mode */
		return -ENOTSUP;
	}
	if (ret) {
		return ret;
	}

	sdmmc_decode_csd(&card_csd, csd,
		&card->block_count, &card->block_size);
	LOG_DBG("Card block count %d, block size %d",
		card->block_count, card->block_size);
	return 0;
}

static int sdmmc_select_card(struct sd_card *card)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = SD_SELECT_CARD;
	cmd.arg = (card->relative_addr << 16U);
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		LOG_DBG("CMD7 failed");
		return ret;
	}
	ret = sdmmc_check_response(&cmd);
	if (ret) {
		LOG_DBG("CMD7 reports error");
		return ret;
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
	ret = sdmmc_check_response(&cmd);
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

/* Returns 1 if host supports UHS, zero otherwise */
static inline int sdmmc_host_uhs(struct sdhc_host_props *props)
{
	return (props->host_caps.sdr50_support |
		props->host_caps.uhs_2_support |
		props->host_caps.sdr104_support |
		props->host_caps.ddr50_support)
		& (props->host_caps.vol_180_support);
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

	if (card->host_props.is_spi && IS_ENABLED(CONFIG_SDHC_SUPPORTS_SPI_MODE)) {
		/* SD card needs CMD58 before ACMD41 to read OCR */
		ret = sdmmc_spi_send_ocr(card, 0);
		if (ret) {
			/* Card is not an SD card */
			return ret;
		}
		if (card->flags & SD_SDHC_FLAG) {
			ocr_arg |= SD_OCR_HOST_CAP_FLAG;
		}
		ret = sdmmc_send_ocr(card, ocr_arg);
		if (ret) {
			return ret;
		}
		/* Send second CMD58 to get CCS bit */
		ret = sdmmc_spi_send_ocr(card, ocr_arg);
	} else if (IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		/* Send initial probing OCR */
		ret = sdmmc_send_ocr(card, 0);
		if (ret) {
			/* Card is not an SD card */
			return ret;
		}
		if (card->flags & SD_SDHC_FLAG) {
			/* High capacity card. See if host supports 1.8V */
			if (card->host_props.host_caps.vol_180_support) {
				ocr_arg |= SD_OCR_SWITCH_18_REQ_FLAG;
			}
			/* Set host high capacity support flag */
			ocr_arg |= SD_OCR_HOST_CAP_FLAG;
		}
		/* Set voltage window */
		if (card->host_props.host_caps.vol_300_support) {
			ocr_arg |= SD_OCR_VDD29_30FLAG;
		}
		ocr_arg |= (SD_OCR_VDD32_33FLAG | SD_OCR_VDD33_34FLAG);
		/* Send SD OCR to card to initialize it */
		ret = sdmmc_send_ocr(card, ocr_arg);
	} else {
		return -ENOTSUP;
	}
	if (ret) {
		LOG_ERR("Failed to query card OCR");
		return ret;
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
	ret = sdmmc_read_cid(card);
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

/* Read card status. Return 0 if card is inactive */
static int sdmmc_read_status(struct sd_card *card)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = SD_SEND_STATUS;
	if (!card->host_props.is_spi) {
		cmd.arg = (card->relative_addr << 16U);
	}
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R2);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		return SD_RETRY;
	}
	if (card->host_props.is_spi) {
		/* Check R2 response bits */
		if ((cmd.response[0U] & SDHC_SPI_R2_CARD_LOCKED) ||
			(cmd.response[0U] & SDHC_SPI_R2_UNLOCK_FAIL)) {
			return -EACCES;
		} else if ((cmd.response[0U] & SDHC_SPI_R2_WP_VIOLATION) ||
			(cmd.response[0U] & SDHC_SPI_R2_ERASE_PARAM) ||
			(cmd.response[0U] & SDHC_SPI_R2_OUT_OF_RANGE)) {
			return -EINVAL;
		} else if ((cmd.response[0U] & SDHC_SPI_R2_ERR) ||
			(cmd.response[0U] & SDHC_SPI_R2_CC_ERR) ||
			(cmd.response[0U] & SDHC_SPI_R2_ECC_FAIL)) {
			return -EIO;
		}
		/* Otherwise, no error in R2 response */
		return 0;
	}
	/* Otherwise, check native card response */
	if ((cmd.response[0U] & SD_R1_RDY_DATA) &&
		(SD_R1_CURRENT_STATE(cmd.response[0U]) == SDMMC_R1_TRANSFER)) {
		return 0;
	}
	/* Valid response, the card is busy */
	return -EBUSY;
}

/* Waits for SD card to be ready for data. Returns 0 if card is ready */
static int sdmmc_wait_ready(struct sd_card *card)
{
	int ret, timeout = CONFIG_SD_DATA_TIMEOUT * 1000;
	bool busy = true;

	do {
		busy = sdhc_card_busy(card->sdhc);
		if (!busy) {
			/* Check card status */
			ret = sd_retry(sdmmc_read_status, card, CONFIG_SD_RETRY_COUNT);
			busy = (ret != 0);
		} else {
			/* Delay 125us before polling again */
			k_busy_wait(125);
			timeout -= 125;
		}
	} while (busy && (timeout > 0));
	return busy;
}

static int sdmmc_read(struct sd_card *card, uint8_t *rbuf,
	uint32_t start_block, uint32_t num_blocks)
{
	int ret;
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	/*
	 * Note: The SD specification allows for CMD23 to be sent before a
	 * transfer in order to set the block length (often preferable).
	 * The specification also requires that CMD12 be sent to stop a transfer.
	 * However, the host specification defines support for "Auto CMD23" and
	 * "Auto CMD12", where the host sends CMD23 and CMD12 automatically to
	 * remove the overhead of interrupts in software from sending these
	 * commands. Therefore, we will not handle CMD12 or CMD23 at this layer.
	 * The host SDHC driver is expected to recognize CMD17, CMD18, CMD24,
	 * and CMD25 as special read/write commands and handle CMD23 and
	 * CMD12 appropriately.
	 */
	cmd.opcode = (num_blocks == 1U) ? SD_READ_SINGLE_BLOCK : SD_READ_MULTIPLE_BLOCK;
	if (!(card->flags & SD_HIGH_CAPACITY_FLAG)) {
		/* SDSC cards require block size in bytes, not blocks */
		cmd.arg = start_block * card->block_size;
	} else {
		cmd.arg = start_block;
	}
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	cmd.retries = CONFIG_SD_DATA_RETRIES;

	data.block_addr = start_block;
	data.block_size = card->block_size;
	data.blocks = num_blocks;
	data.data = rbuf;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;

	LOG_DBG("READ: Sector = %u, Count = %u", start_block, num_blocks);

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		LOG_ERR("Failed to read from SDMMC %d", ret);
		return ret;
	}

	/* Verify card is back in transfer state after read */
	if (!card->host_props.is_spi) {
		ret = sdmmc_wait_ready(card);
		if (ret) {
			LOG_ERR("Card did not return to ready state");
			k_mutex_unlock(&card->lock);
			return -ETIMEDOUT;
		}
	}
	return 0;
}

/* Reads data from SD card memory card */
int sdmmc_read_blocks(struct sd_card *card, uint8_t *rbuf,
	uint32_t start_block, uint32_t num_blocks)
{
	int ret;
	uint32_t rlen;
	uint32_t sector;
	uint8_t *buf_offset;

	if ((start_block + num_blocks) > card->block_count) {
		return -EINVAL;
	}
	if (card->type == CARD_SDIO) {
		LOG_WRN("SDIO does not support MMC commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&card->lock, K_NO_WAIT);
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}

	/*
	 * If the buffer we are provided with is aligned, we can use it
	 * directly. Otherwise, we need to use the card's internal buffer
	 * and memcpy the data back out
	 */
	if ((((uintptr_t)rbuf) & (CONFIG_SDHC_BUFFER_ALIGNMENT - 1)) != 0) {
		/* lower bits of address are set, not aligned. Use internal buffer */
		LOG_DBG("Unaligned buffer access to SD card may incur performance penalty");
		if (sizeof(card->card_buffer) < card->block_size) {
			LOG_ERR("Card buffer size needs to be increased for "
				"unaligned writes to work");
			k_mutex_unlock(&card->lock);
			return -ENOBUFS;
		}
		rlen = sizeof(card->card_buffer) / card->block_size;
		sector = 0;
		buf_offset = rbuf;
		while (sector < num_blocks) {
			/* Read from disk to card buffer */
			ret = sdmmc_read(card, card->card_buffer,
				sector + start_block, rlen);
			if (ret) {
				LOG_ERR("Write failed");
				k_mutex_unlock(&card->lock);
				return ret;
			}
			/* Copy data from card buffer */
			memcpy(buf_offset, card->card_buffer, rlen * card->block_size);
			/* Increase sector count and buffer offset */
			sector += rlen;
			buf_offset += rlen * card->block_size;
		}
	} else {
		/* Aligned buffers can be used directly */
		ret = sdmmc_read(card, rbuf, start_block, num_blocks);
		if (ret) {
			LOG_ERR("Card read failed");
			k_mutex_unlock(&card->lock);
			return ret;
		}
	}
	k_mutex_unlock(&card->lock);
	return 0;
}

/*
 * Sends ACMD22 (number of written blocks) to see how many blocks were written
 * to a card
 */
static int sdmmc_query_written(struct sd_card *card, uint32_t *num_written)
{
	int ret;
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	uint32_t *blocks = (uint32_t *)card->card_buffer;

	ret = sdmmc_app_command(card, card->relative_addr);
	if (ret) {
		LOG_DBG("App CMD for ACMD22 failed");
		return ret;
	}

	cmd.opcode = SD_APP_SEND_NUM_WRITTEN_BLK;
	cmd.arg = 0;
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	data.block_size = 4U;
	data.blocks = 1U;
	data.data = blocks;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		LOG_DBG("ACMD22 failed: %d", ret);
		return ret;
	}
	ret = sdmmc_check_response(&cmd);
	if (ret) {
		LOG_DBG("ACMD22 reports error");
		return ret;
	}

	/* Decode blocks */
	*num_written = sys_be32_to_cpu(blocks[0]);
	return 0;
}

static int sdmmc_write(struct sd_card *card, const uint8_t *wbuf,
	uint32_t start_block, uint32_t num_blocks)
{
	int ret;
	uint32_t blocks;
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	/*
	 * See the note in sdmmc_read() above. We will not issue CMD23
	 * or CMD12, and expect the host to handle those details.
	 */
	cmd.opcode = (num_blocks == 1) ? SD_WRITE_SINGLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK;
	if (!(card->flags & SD_HIGH_CAPACITY_FLAG)) {
		/* SDSC cards require block size in bytes, not blocks */
		cmd.arg = start_block * card->block_size;
	} else {
		cmd.arg = start_block;
	}
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	cmd.retries = CONFIG_SD_DATA_RETRIES;

	data.block_addr = start_block;
	data.block_size = card->block_size;
	data.blocks = num_blocks;
	data.data = (uint8_t *)wbuf;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;

	LOG_DBG("WRITE: Sector = %u, Count = %u", start_block, num_blocks);

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		LOG_DBG("Write failed: %d", ret);
		if (card->host_props.is_spi) {
			/* Just check card status */
			ret = sdmmc_read_status(card);
		} else {
			/* Wait for card to be idle */
			ret = sdmmc_wait_ready(card);
		}
		if (ret) {
			return ret;
		}
		/* Query card to see how many blocks were actually written */
		ret = sdmmc_query_written(card, &blocks);
		if (ret) {
			return ret;
		}
		LOG_ERR("Only %d blocks of %d were written", blocks, num_blocks);
		return -EIO;
	}
	/* Verify card is back in transfer state after write */
	if (card->host_props.is_spi) {
		/* Just check card status */
		ret = sdmmc_read_status(card);
	} else {
		/* Wait for card to be idle */
		ret = sdmmc_wait_ready(card);
	}
	if (ret) {
		LOG_ERR("Card did not return to ready state");
		return -ETIMEDOUT;
	}
	return 0;
}

/* Writes data to SD card memory card */
int sdmmc_write_blocks(struct sd_card *card, const uint8_t *wbuf,
	uint32_t start_block, uint32_t num_blocks)
{
	int ret;
	uint32_t wlen;
	uint32_t sector;
	const uint8_t *buf_offset;

	if ((start_block + num_blocks) > card->block_count) {
		return -EINVAL;
	}
	if (card->type == CARD_SDIO) {
		LOG_WRN("SDIO does not support MMC commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&card->lock, K_NO_WAIT);
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	/*
	 * If the buffer we are provided with is aligned, we can use it
	 * directly. Otherwise, we need to use the card's internal buffer
	 * and memcpy the data back out
	 */
	if ((((uintptr_t)wbuf) & (CONFIG_SDHC_BUFFER_ALIGNMENT - 1)) != 0) {
		/* lower bits of address are set, not aligned. Use internal buffer */
		LOG_DBG("Unaligned buffer access to SD card may incur performance penalty");
		if (sizeof(card->card_buffer) < card->block_size) {
			LOG_ERR("Card buffer size needs to be increased for "
				"unaligned writes to work");
			k_mutex_unlock(&card->lock);
			return -ENOBUFS;
		}
		wlen = sizeof(card->card_buffer) / card->block_size;
		sector = 0;
		buf_offset = wbuf;
		while (sector < num_blocks) {
			/* Copy data into card buffer */
			memcpy(card->card_buffer, buf_offset, wlen * card->block_size);
			/* Write card buffer to disk */
			ret = sdmmc_write(card, card->card_buffer,
				sector + start_block, wlen);
			if (ret) {
				LOG_ERR("Write failed");
				k_mutex_unlock(&card->lock);
				return ret;
			}
			/* Increase sector count and buffer offset */
			sector += wlen;
			buf_offset += wlen * card->block_size;
		}
	} else {
		/* We can use aligned buffers directly */
		ret = sdmmc_write(card, wbuf, start_block, num_blocks);
		if (ret) {
			LOG_ERR("Write failed");
			k_mutex_unlock(&card->lock);
			return ret;
		}
	}
	k_mutex_unlock(&card->lock);
	return 0;
}


/* IO Control handler for SD MMC */
int sdmmc_ioctl(struct sd_card *card, uint8_t cmd, void *buf)
{
	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		(*(uint32_t *)buf) = card->block_count;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		(*(uint32_t *)buf) = card->block_size;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}
