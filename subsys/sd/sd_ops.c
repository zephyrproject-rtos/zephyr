/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "sd_utils.h"

LOG_MODULE_DECLARE(sd, CONFIG_SD_LOG_LEVEL);

static inline void sdmmc_decode_csd(struct sd_csd *csd,
	uint32_t *raw_csd, uint32_t *blk_count, uint32_t *blk_size)
{
	uint32_t tmp_blk_count, tmp_blk_size;

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
		tmp_blk_count = ((csd->device_size + 1U) <<
			(csd->dev_size_mul + 2U));
		tmp_blk_size = (1U << (csd->read_blk_len));
		if (tmp_blk_size != SDMMC_DEFAULT_BLOCK_SIZE) {
			tmp_blk_count = (tmp_blk_count * tmp_blk_size);
			tmp_blk_size = SDMMC_DEFAULT_BLOCK_SIZE;
			tmp_blk_count = (tmp_blk_count / tmp_blk_size);
		}
		if (blk_count) {
			*blk_count = tmp_blk_count;
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

		tmp_blk_count = ((csd->device_size + 1U) * 1024U);
		if (blk_count) {
			*blk_count = tmp_blk_count;
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


/* Read card specific data register */
int sdmmc_read_csd(struct sd_card *card)
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

/* Reads card identification register, and decodes it */
int sdmmc_read_cid(struct sd_card *card)
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
 * Implements signal voltage switch procedure described in section 3.6.1 of
 * SD specification.
 */
int sdmmc_switch_voltage(struct sd_card *card)
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
	ret = sd_check_response(&cmd);
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

/*
 * Requests card to publish a new relative card address, and move from
 * identification to data mode
 */
int sdmmc_request_rca(struct sd_card *card)
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

/*
 * Selects card, moving it into data transfer mode
 */
int sdmmc_select_card(struct sd_card *card)
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
	ret = sd_check_response(&cmd);
	if (ret) {
		LOG_DBG("CMD7 reports error");
		return ret;
	}
	return 0;
}
