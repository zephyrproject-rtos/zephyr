/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sdhc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/mmc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sd_spec.h>

#include "sd_ops.h"
#include "sd_utils.h"

/* These are used as specific arguments to commands */
/* For switch commands, the argument structure is:
 * [31:26] : Set to 0
 * [25:24] : Access
 * [23:16] : Index (byte address in EXT_CSD)
 * [15:8]  : Value (data)
 * [7:3]   : Set to 0
 * [2:0]   : Cmd Set
 */
#define MMC_SWITCH_8_BIT_DDR_BUS_ARG                                                               \
	(0xFC000000 & (0U << 26)) + (0x03000000 & (0b11 << 24)) + (0x00FF0000 & (183U << 16)) +    \
		(0x0000FF00 & (6U << 8)) + (0x000000F7 & (0U << 3)) + (0x00000000 & (3U << 0))
#define MMC_SWITCH_8_BIT_BUS_ARG                                                                   \
	(0xFC000000 & (0U << 26)) + (0x03000000 & (0b11 << 24)) + (0x00FF0000 & (183U << 16)) +    \
		(0x0000FF00 & (2U << 8)) + (0x000000F7 & (0U << 3)) + (0x00000000 & (3U << 0))
#define MMC_SWITCH_4_BIT_BUS_ARG                                                                   \
	(0xFC000000 & (0U << 26)) + (0x03000000 & (0b11 << 24)) + (0x00FF0000 & (183U << 16)) +    \
		(0x0000FF00 & (1U << 8)) + (0x000000F7 & (0U << 3)) + (0x00000000 & (3U << 0))
#define MMC_SWITCH_HS_TIMING_ARG                                                                   \
	(0xFC000000 & (0U << 26)) + (0x03000000 & (0b11 << 24)) + (0x00FF0000 & (185U << 16)) +    \
		(0x0000FF00 & (1U << 8)) + (0x000000F7 & (0U << 3)) + (0x00000000 & (3U << 0))
#define MMC_SWITCH_HS400_TIMING_ARG                                                                \
	(0xFC000000 & (0U << 26)) + (0x03000000 & (0b11 << 24)) + (0x00FF0000 & (185U << 16)) +    \
		(0x0000FF00 & (3U << 8)) + (0x000000F7 & (0U << 3)) + (0x00000000 & (3U << 0))
#define MMC_SWITCH_HS200_TIMING_ARG                                                                \
	(0xFC000000 & (0U << 26)) + (0x03000000 & (0b11 << 24)) + (0x00FF0000 & (185U << 16)) +    \
		(0x0000FF00 & (2U << 8)) + (0x000000F7 & (0U << 3)) + (0x00000000 & (3U << 0))
#define MMC_RCA_ARG	(CONFIG_MMC_RCA << 16U)
#define MMC_REL_ADR_ARG (card->relative_addr << 16U)
#define MMC_SWITCH_PWR_CLASS_ARG                                                                   \
	(0xFC000000 & (0U << 26)) + (0x03000000 & (0b11 << 24)) + (0x00FF0000 & (187U << 16)) +    \
		(0x0000FF00 & (0U << 8)) + (0x000000F7 & (0U << 3)) + (0x00000000 & (3U << 0))
#define MMC_SWITCH_CACHE_ON_ARG                                                                    \
	(0xFC000000 & (0U << 26)) + (0x03000000 & (0b11 << 24)) + (0x00FF0000 & (33U << 16)) +     \
		(0x0000FF00 & (1U << 8)) + (0x000000F7 & (0U << 3)) + (0x00000000 & (3U << 0))

LOG_MODULE_DECLARE(sd, CONFIG_SD_LOG_LEVEL);

inline int mmc_write_blocks(struct sd_card *card, const uint8_t *wbuf, uint32_t start_block,
			    uint32_t num_blocks)
{
	return card_write_blocks(card, wbuf, start_block, num_blocks);
}

inline int mmc_read_blocks(struct sd_card *card, uint8_t *rbuf, uint32_t start_block,
			   uint32_t num_blocks)
{
	return card_read_blocks(card, rbuf, start_block, num_blocks);
}

inline int mmc_ioctl(struct sd_card *card, uint8_t cmd, void *buf)
{
	return card_ioctl(card, cmd, buf);
}

/* Sends CMD1 */
static int mmc_send_op_cond(struct sd_card *card, int ocr);

/* Sends CMD3 */
static int mmc_set_rca(struct sd_card *card);

/* Sends CMD9 */
static int mmc_read_csd(struct sd_card *card, struct sd_csd *card_csd);
static inline void mmc_decode_csd(struct sd_csd *csd, uint32_t *raw_csd);

/* Sends CMD8 */
static int mmc_read_ext_csd(struct sd_card *card, struct mmc_ext_csd *card_ext_csd);
static inline void mmc_decode_ext_csd(struct mmc_ext_csd *ext_csd, uint8_t *raw);

/* Sets SDHC max frequency in legacy timing */
static inline int mmc_set_max_freq(struct sd_card *card, struct sd_csd *card_csd);

/* Sends CMD6 to switch bus width*/
static int mmc_set_bus_width(struct sd_card *card);

/* Sets card to the fastest timing mode (using CMD6) and SDHC to max frequency */
static int mmc_set_timing(struct sd_card *card, struct mmc_ext_csd *card_ext_csd);

/* Enable cache for emmc if applicable */
static int mmc_set_cache(struct sd_card *card, struct mmc_ext_csd *card_ext_csd);

/*
 * Initialize MMC card for use with subsystem
 */
int mmc_card_init(struct sd_card *card)
{
	int ret = 0;
	uint32_t ocr_arg = 0U;
	/* Keep CSDs on stack for reduced RAM usage */
	struct sd_csd card_csd = {0};
	struct mmc_ext_csd card_ext_csd = {0};

	/* SPI is not supported for MMC */
	if (card->host_props.is_spi) {
		return -EINVAL;
	}

	/* Set OCR Arguments */
	if (card->host_props.host_caps.vol_180_support) {
		ocr_arg |= MMC_OCR_VDD170_195FLAG;
	}
	if (card->host_props.host_caps.vol_330_support ||
	    card->host_props.host_caps.vol_300_support) {
		ocr_arg |= MMC_OCR_VDD27_36FLAG;
	}
	/* Modern SDHC always at least supports 512 byte block sizes,
	 * which is enough to support sectors
	 */
	ocr_arg |= MMC_OCR_SECTOR_MODE;

	/* CMD1 */
	ret = mmc_send_op_cond(card, ocr_arg);
	if (ret) {
		LOG_DBG("Failed to query card OCR");
		return ret;
	}

	/* CMD2 */
	ret = card_read_cid(card);
	if (ret) {
		return ret;
	}

	/* CMD3 */
	ret = mmc_set_rca(card);
	if (ret) {
		LOG_ERR("Failed on sending RCA to card");
		return ret;
	}

	/* CMD9 */
	ret = mmc_read_csd(card, &card_csd);
	if (ret) {
		return ret;
	}

	/* Set max bus clock in legacy timing to speed up initialization
	 * Currently only eMMC is supported for this command
	 * Legacy MMC cards will initialize slowly
	 */
	ret = mmc_set_max_freq(card, &card_csd);
	if (ret) {
		return ret;
	}

	/* CMD7 */
	ret = sdmmc_select_card(card);
	if (ret) {
		return ret;
	}

	/* CMD6: Set bus width to max supported*/
	ret = mmc_set_bus_width(card);
	if (ret) {
		return ret;
	}

	/* CMD8 */
	ret = mmc_read_ext_csd(card, &card_ext_csd);
	if (ret) {
		return ret;
	}

	/* Set timing to fastest supported */
	ret = mmc_set_timing(card, &card_ext_csd);
	if (ret) {
		return ret;
	}

	/* Turn on cache if it exists */
	ret = mmc_set_cache(card, &card_ext_csd);
	if (ret) {
		return ret;
	}

	return 0;
}

static int mmc_send_op_cond(struct sd_card *card, int ocr)
{
	struct sdhc_command cmd = {0};
	int ret = 0;
	int retries;

	cmd.opcode = MMC_SEND_OP_COND;
	cmd.arg = ocr;
	cmd.response_type = SD_RSP_TYPE_R3;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	for (retries = 0;
	     retries < CONFIG_SD_OCR_RETRY_COUNT && !(cmd.response[0] & SD_OCR_PWR_BUSY_FLAG);
	     retries++) {
		ret = sdhc_request(card->sdhc, &cmd, NULL);
		if (ret) {
			/* OCR failed */
			return ret;
		}
		if (retries == 0) {
			/* Card is MMC card if no error
			 * (Only MMC protocol supports CMD1)
			 */
			card->type = CARD_MMC;
		}
		sd_delay(10);
	}
	if (retries >= CONFIG_SD_OCR_RETRY_COUNT) {
		/* OCR timed out */
		LOG_ERR("Card never left busy state");
		return -ETIMEDOUT;
	}
	LOG_DBG("MMC responded to CMD1 after %d attempts", retries);

	if (cmd.response[0] & MMC_OCR_VDD170_195FLAG) {
		card->flags |= SD_1800MV_FLAG;
	}
	if (cmd.response[0] & MMC_OCR_VDD27_36FLAG) {
		card->flags |= SD_3000MV_FLAG;
	}

	/* Switch to 1.8V */
	if (card->host_props.host_caps.vol_180_support && (card->flags & SD_1800MV_FLAG)) {
		card->bus_io.signal_voltage = SD_VOL_1_8_V;
		ret = sdhc_set_io(card->sdhc, &card->bus_io);
		if (ret) {
			LOG_DBG("Failed to switch MMC host to 1.8V");
			return ret;
		}
		sd_delay(10);

		card->card_voltage = SD_VOL_1_8_V;
		LOG_INF("Card switched to 1.8V signaling");
	}

	/* SD high Capacity is >2GB, the same as sector supporting MMC cards. */
	if (cmd.response[0] & MMC_OCR_SECTOR_MODE) {
		card->flags |= SD_HIGH_CAPACITY_FLAG;
	}

	return 0;
}

static int mmc_set_rca(struct sd_card *card)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = MMC_SEND_RELATIVE_ADDR;
	cmd.arg = MMC_RCA_ARG;
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		return ret;
	}
	ret = sd_check_response(&cmd);
	if (ret) {
		return ret;
	}

	card->relative_addr = CONFIG_MMC_RCA;

	return 0;
}

static int mmc_read_csd(struct sd_card *card, struct sd_csd *card_csd)
{
	int ret;
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_SEND_CSD;
	cmd.arg = MMC_REL_ADR_ARG;
	cmd.response_type = SD_RSP_TYPE_R2;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		LOG_DBG("CMD9 failed: %d", ret);
		return ret;
	}

	mmc_decode_csd(card_csd, cmd.response);
	if (card_csd->csd_structure < 2) {
		LOG_ERR("Legacy MMC cards are not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static inline void mmc_decode_csd(struct sd_csd *csd, uint32_t *raw_csd)
{
	csd->csd_structure = (uint8_t)((raw_csd[3U] & 0xC0000000U) >> 30U);
	csd->read_time1 = (uint8_t)((raw_csd[3U] & 0xFF0000U) >> 16U);
	csd->read_time2 = (uint8_t)((raw_csd[3U] & 0xFF00U) >> 8U);
	csd->xfer_rate = (uint8_t)(raw_csd[3U] & 0xFFU);
	csd->cmd_class = (uint16_t)((raw_csd[2U] & 0xFFF00000U) >> 20U);
	csd->read_blk_len = (uint8_t)((raw_csd[2U] & 0xF0000U) >> 16U);
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
	csd->device_size =
		(uint16_t)(((raw_csd[2U] & 0x3FFU) << 2U) + ((raw_csd[1U] & 0xC0000000U) >> 30U));
	csd->read_current_min = (uint8_t)((raw_csd[1U] & 0x38000000U) >> 27U);
	csd->read_current_max = (uint8_t)((raw_csd[1U] & 0x07000000U) >> 24U);
	csd->write_current_min = (uint8_t)((raw_csd[1U] & 0x00E00000U) >> 21U);
	csd->write_current_max = (uint8_t)((raw_csd[1U] & 0x001C0000U) >> 18U);
	csd->dev_size_mul = (uint8_t)((raw_csd[1U] & 0x00038000U) >> 15U);
	csd->erase_size = (uint8_t)((raw_csd[1U] & 0x00007C00U) >> 10U);
	csd->write_prtect_size = (uint8_t)(raw_csd[1U] & 0x0000001FU);
	csd->write_speed_factor = (uint8_t)((raw_csd[0U] & 0x1C000000U) >> 26U);
	csd->write_blk_len = (uint8_t)((raw_csd[0U] & 0x03C00000U) >> 22U);
	csd->file_fmt = (uint8_t)((raw_csd[0U] & 0x00000C00U) >> 10U);
}

static inline int mmc_set_max_freq(struct sd_card *card, struct sd_csd *card_csd)
{
	int ret;
	enum mmc_csd_freq frequency_code = (card_csd->xfer_rate & 0x7);
	enum mmc_csd_freq multiplier_code = (card_csd->xfer_rate & 0x78);

	/* 4.3 - 5.1 emmc spec says 26 MHz */
	if (frequency_code == MMC_MAXFREQ_10MHZ && multiplier_code == MMC_MAXFREQ_MULT_26) {
		card->bus_io.clock = 26000000U;
		card->bus_io.timing = SDHC_TIMING_LEGACY;
	}
	/* 4.0 - 4.2 emmc spec says 20 MHz */
	else if (frequency_code == MMC_MAXFREQ_10MHZ && multiplier_code == MMC_MAXFREQ_MULT_20) {
		card->bus_io.clock = 20000000U;
		card->bus_io.timing = SDHC_TIMING_LEGACY;
	} else {
		LOG_INF("Using Legacy MMC will have slow initialization");
		return 0;
	}

	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		LOG_ERR("Error seetting initial clock frequency");
		return ret;
	}

	return 0;
}

static int mmc_set_bus_width(struct sd_card *card)
{
	int ret;
	struct sdhc_command cmd = {0};

	if (card->host_props.host_caps.bus_8_bit_support && card->bus_width == 8) {
		cmd.arg = MMC_SWITCH_8_BIT_BUS_ARG;
		card->bus_io.bus_width = SDHC_BUS_WIDTH8BIT;
	} else if (card->host_props.host_caps.bus_4_bit_support && card->bus_width >= 4) {
		cmd.arg = MMC_SWITCH_4_BIT_BUS_ARG;
		card->bus_io.bus_width = SDHC_BUS_WIDTH4BIT;
	} else {
		/* If only 1 bit bus is supported, nothing to be done */
		return 0;
	}

	/* Set Card Bus Width */
	cmd.opcode = SD_SWITCH;
	cmd.response_type = SD_RSP_TYPE_R1b;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	sdmmc_wait_ready(card);
	if (ret) {
		LOG_ERR("Setting card data bus width failed: %d", ret);
		return ret;
	}

	/* Set host controller bus width */
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		LOG_ERR("Setting SDHC data bus width failed: %d", ret);
		return ret;
	}

	return ret;
}

static int mmc_set_hs_timing(struct sd_card *card)
{
	int ret = 0;
	struct sdhc_command cmd = {0};

	/* Change Card Timing Mode */
	cmd.opcode = SD_SWITCH;
	cmd.arg = MMC_SWITCH_HS_TIMING_ARG;
	cmd.response_type = SD_RSP_TYPE_R1b;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		LOG_DBG("Error setting bus timing: %d", ret);
		return ret;
	}
	sdmmc_wait_ready(card);

	/* Max frequency in HS mode is 52 MHz */
	card->bus_io.clock = MMC_CLOCK_52MHZ;
	card->bus_io.timing = SDHC_TIMING_HS;
	/* Change SDHC bus timing */
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		return ret;
	}

	return ret;
}

static int mmc_set_power_class_HS200(struct sd_card *card, struct mmc_ext_csd *ext)
{
	int ret = 0;
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_SWITCH;
	cmd.arg = ((MMC_SWITCH_PWR_CLASS_ARG) | (ext->pwr_class_200MHZ_VCCQ195 << 8));
	cmd.response_type = SD_RSP_TYPE_R1b;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	sdmmc_wait_ready(card);
	return ret;
}

static int mmc_set_timing(struct sd_card *card, struct mmc_ext_csd *ext)
{
	int ret = 0;
	struct sdhc_command cmd = {0};

	/* Timing depends on EXT_CSD register information */
	if ((ext->device_type.MMC_HS200_SDR_1200MV || ext->device_type.MMC_HS200_SDR_1800MV) &&
	    (card->host_props.host_caps.hs200_support) &&
	    (card->bus_io.signal_voltage == SD_VOL_1_8_V) &&
	    (card->bus_io.bus_width >= SDHC_BUS_WIDTH4BIT)) {
		ret = mmc_set_hs_timing(card);
		if (ret) {
			return ret;
		}
		cmd.arg = MMC_SWITCH_HS200_TIMING_ARG;
		card->bus_io.clock = MMC_CLOCK_HS200;
		card->bus_io.timing = SDHC_TIMING_HS200;
	} else if (ext->device_type.MMC_HS_52_DV) {
		return mmc_set_hs_timing(card);
	} else if (ext->device_type.MMC_HS_26_DV) {
		/* Nothing to do, card is already configured for this */
		return 0;
	} else {
		return -ENOTSUP;
	}

	/* Set card timing mode */
	cmd.opcode = SD_SWITCH;
	cmd.response_type = SD_RSP_TYPE_R1b;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		LOG_DBG("Error setting bus timing: %d", ret);
		return ret;
	}
	ret = sdmmc_wait_ready(card);
	if (ret) {
		return ret;
	}
	card->card_speed = ((cmd.arg & 0xFF << 8) >> 8);

	/* Set power class to match timing mode */
	if (card->card_speed == MMC_HS200_TIMING) {
		ret = mmc_set_power_class_HS200(card, ext);
		if (ret) {
			return ret;
		}
	}

	/* Set SDHC bus io parameters */
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		return ret;
	}

	/* Execute Tuning for HS200 */
	if (card->card_speed == MMC_HS200_TIMING) {
		ret = sdhc_execute_tuning(card->sdhc);
		if (ret) {
			LOG_ERR("MMC Tuning failed: %d", ret);
			return ret;
		}
	}

	/* Switch to HS400 if applicable */
	if ((ext->device_type.MMC_HS400_DDR_1200MV || ext->device_type.MMC_HS400_DDR_1800MV) &&
	    (card->host_props.host_caps.hs400_support) &&
	    (card->bus_io.bus_width == SDHC_BUS_WIDTH8BIT)) {
		/* Switch back to regular HS timing */
		ret = mmc_set_hs_timing(card);
		if (ret) {
			LOG_ERR("Switching MMC back to HS from HS200 during HS400 init failed.");
			return ret;
		}
		/* Set bus width to DDR 8 bit */
		cmd.opcode = SD_SWITCH;
		cmd.arg = MMC_SWITCH_8_BIT_DDR_BUS_ARG;
		cmd.response_type = SD_RSP_TYPE_R1b;
		cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
		ret = sdhc_request(card->sdhc, &cmd, NULL);
		sdmmc_wait_ready(card);
		if (ret) {
			LOG_ERR("Setting DDR data bus width failed during HS400 init: %d", ret);
			return ret;
		}
		/* Set card timing mode to HS400 */
		cmd.opcode = SD_SWITCH;
		cmd.arg = MMC_SWITCH_HS400_TIMING_ARG;
		cmd.response_type = SD_RSP_TYPE_R1b;
		cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
		ret = sdhc_request(card->sdhc, &cmd, NULL);
		if (ret) {
			LOG_DBG("Error setting card to HS400 bus timing: %d", ret);
			return ret;
		}
		ret = sdmmc_wait_ready(card);
		if (ret) {
			return ret;
		}
		/* Set SDHC bus io parameters */
		card->bus_io.clock = MMC_CLOCK_HS400;
		card->bus_io.timing = SDHC_TIMING_HS400;
		ret = sdhc_set_io(card->sdhc, &card->bus_io);
		if (ret) {
			return ret;
		}
		card->card_speed = ((cmd.arg & 0xFF << 8) >> 8);
	}
	return ret;
}

static int mmc_read_ext_csd(struct sd_card *card, struct mmc_ext_csd *card_ext_csd)
{
	int ret;
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	cmd.opcode = MMC_SEND_EXT_CSD;
	cmd.arg = 0;
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	data.block_size = MMC_EXT_CSD_BYTES;
	data.blocks = 1;
	data.data = card->card_buffer;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		LOG_ERR("CMD8 (send_ext_csd) failed: %d", ret);
		return ret;
	}

	mmc_decode_ext_csd(card_ext_csd, card->card_buffer);
	card->block_count = card_ext_csd->sec_count;
	card->block_size = SDMMC_DEFAULT_BLOCK_SIZE;

	LOG_INF("Card block count is %d, block size is %d", card->block_count, card->block_size);

	return 0;
}

static inline void mmc_decode_ext_csd(struct mmc_ext_csd *ext, uint8_t *raw)
{
	ext->sec_count =
		(raw[215U] << 24U) + (raw[214U] << 16U) + (raw[213U] << 8U) + (raw[212U] << 0U);
	ext->bus_width = raw[183U];
	ext->hs_timing = raw[185U];
	ext->device_type.MMC_HS400_DDR_1200MV = ((1 << 7U) & raw[196U]);
	ext->device_type.MMC_HS400_DDR_1800MV = ((1 << 6U) & raw[196U]);
	ext->device_type.MMC_HS200_SDR_1200MV = ((1 << 5U) & raw[196U]);
	ext->device_type.MMC_HS200_SDR_1800MV = ((1 << 4U) & raw[196U]);
	ext->device_type.MMC_HS_DDR_1200MV = ((1 << 3U) & raw[196U]);
	ext->device_type.MMC_HS_DDR_1800MV = ((1 << 2U) & raw[196U]);
	ext->device_type.MMC_HS_52_DV = ((1 << 1U) & raw[196U]);
	ext->device_type.MMC_HS_26_DV = ((1 << 0U) & raw[196U]);
	ext->rev = raw[192U];
	ext->power_class = (raw[187] & 0x0F);
	ext->mmc_driver_strengths = raw[197U];
	ext->pwr_class_200MHZ_VCCQ195 = raw[237U];
	ext->cache_size =
		(raw[252] << 24U) + (raw[251] << 16U) + (raw[250] << 8U) + (raw[249] << 0U);
}

static int mmc_set_cache(struct sd_card *card, struct mmc_ext_csd *card_ext_csd)
{
	int ret = 0;
	struct sdhc_command cmd = {0};

	/* If there is no cache, don't use cache */
	if (card_ext_csd->cache_size == 0) {
		return 0;
	}
	/* CMD6 to write to EXT CSD to turn on cache */
	cmd.opcode = SD_SWITCH;
	cmd.arg = MMC_SWITCH_CACHE_ON_ARG;
	cmd.response_type = SD_RSP_TYPE_R1b;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		LOG_DBG("Error turning on card cache: %d", ret);
		return ret;
	}
	ret = sdmmc_wait_ready(card);
	return ret;
}
