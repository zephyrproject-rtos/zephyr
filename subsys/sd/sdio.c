/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdmmc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/logging/log.h>

#include "sd_ops.h"
#include "sd_utils.h"

LOG_MODULE_DECLARE(sd, CONFIG_SD_LOG_LEVEL);

uint8_t cis_tuples[] = {
	SDIO_TPL_CODE_MANIFID,
	SDIO_TPL_CODE_FUNCID,
	SDIO_TPL_CODE_FUNCE,
};

/*
 * Send SDIO OCR using CMD5
 */
static int sdio_send_ocr(struct sd_card *card, uint32_t ocr)
{
	struct sdhc_command cmd = {0};
	int ret;
	int retries;

	cmd.opcode = SDIO_SEND_OP_COND;
	cmd.arg = ocr;
	cmd.response_type = (SD_RSP_TYPE_R4 | SD_SPI_RSP_TYPE_R4);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	/* Send OCR5 to initialize card */
	for (retries = 0; retries < CONFIG_SD_OCR_RETRY_COUNT; retries++) {
		ret = sdhc_request(card->sdhc, &cmd, NULL);
		if (ret) {
			if (ocr == 0) {
				/* Just probing card, likely not SDIO */
				return SD_NOT_SDIO;
			}
			return ret;
		}
		if (ocr == 0) {
			/* We are probing card, check number of IO functions */
			card->num_io = (cmd.response[0] & SDIO_OCR_IO_NUMBER)
				>> SDIO_OCR_IO_NUMBER_SHIFT;
			if ((card->num_io == 0) ||
				((cmd.response[0] & SDIO_IO_OCR_MASK) == 0)) {
				if (cmd.response[0] & SDIO_OCR_MEM_PRESENT_FLAG) {
					/* Card is not an SDIO card */
					return SD_NOT_SDIO;
				}
				/* Card is not a supported SD device */
				return -ENOTSUP;
			}
			/* Card has IO present, return zero to
			 * indicate SDIO card
			 */
			return 0;
		}
		/* Check to see if card is busy with power up */
		if (cmd.response[0] & SD_OCR_PWR_BUSY_FLAG) {
			break;
		}
		/* Delay before retrying command */
		sd_delay(10);
	}
	if (retries >= CONFIG_SD_OCR_RETRY_COUNT) {
		/* OCR timed out */
		LOG_ERR("Card never left busy state");
		return -ETIMEDOUT;
	}
	LOG_DBG("SDIO responded to CMD5 after %d attempts", retries);
	if (!card->host_props.is_spi) {
		/* Save OCR */
		card->ocr = cmd.response[0U];
	}
	return 0;
}

static int sdio_io_rw_direct(struct sd_card *card,
			     enum sdio_io_dir direction,
			     enum sdio_func_num func,
			     uint32_t reg_addr,
			     uint8_t data_in,
			     uint8_t *data_out)
{
	int ret;
	struct sdhc_command cmd = {0};

	cmd.opcode = SDIO_RW_DIRECT;
	cmd.arg = (func << SDIO_CMD_ARG_FUNC_NUM_SHIFT) |
		((reg_addr & SDIO_CMD_ARG_REG_ADDR_MASK) << SDIO_CMD_ARG_REG_ADDR_SHIFT);
	if (direction == SDIO_IO_WRITE) {
		cmd.arg |= data_in & SDIO_DIRECT_CMD_DATA_MASK;
		cmd.arg |= BIT(SDIO_CMD_ARG_RW_SHIFT);
		if (data_out) {
			cmd.arg |= BIT(SDIO_DIRECT_CMD_ARG_RAW_SHIFT);
		}
	}
	cmd.response_type = (SD_RSP_TYPE_R5 | SD_SPI_RSP_TYPE_R5);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		return ret;
	}
	if (data_out) {
		if (card->host_props.is_spi) {
			*data_out = (cmd.response[0U] >> 8) & SDIO_DIRECT_CMD_DATA_MASK;
		} else {
			*data_out = cmd.response[0U] & SDIO_DIRECT_CMD_DATA_MASK;
		}
	}
	return ret;
}


static int sdio_io_rw_extended(struct sd_card *card,
			       enum sdio_io_dir direction,
			       enum sdio_func_num func,
			       uint32_t reg_addr,
			       bool increment,
			       uint8_t *buf,
			       uint32_t blocks,
			       uint32_t block_size)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	cmd.opcode = SDIO_RW_EXTENDED;
	cmd.arg = (func << SDIO_CMD_ARG_FUNC_NUM_SHIFT) |
		((reg_addr & SDIO_CMD_ARG_REG_ADDR_MASK) << SDIO_CMD_ARG_REG_ADDR_SHIFT);
	cmd.arg |= (direction == SDIO_IO_WRITE) ? BIT(SDIO_CMD_ARG_RW_SHIFT) : 0;
	cmd.arg |= increment ? BIT(SDIO_EXTEND_CMD_ARG_OP_CODE_SHIFT) : 0;
	cmd.response_type = (SD_RSP_TYPE_R5 | SD_SPI_RSP_TYPE_R5);
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	if (blocks == 0) {
		/* Byte mode */
		cmd.arg |= (block_size == 512) ? 0 : block_size;
	} else {
		/* Block mode */
		cmd.arg |= BIT(SDIO_EXTEND_CMD_ARG_BLK_SHIFT) | blocks;
	}

	data.block_size = block_size;
	/* Host expects blocks to be at least 1 */
	data.blocks = blocks ? blocks : 1;
	data.data = buf;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;
	return sdhc_request(card->sdhc, &cmd, &data);
}

/*
 * Helper for extended r/w. Splits the transfer into the minimum possible
 * number of block r/w, then uses byte transfers for remaining data
 */
static int sdio_io_rw_extended_helper(struct sdio_func *func,
				      enum sdio_io_dir direction,
				      uint32_t reg_addr,
				      bool increment,
				      uint8_t *buf,
				      uint32_t len)
{
	int ret;
	int remaining = len;
	uint32_t blocks, size;

	if (func->num > SDIO_MAX_IO_NUMS) {
		return -EINVAL;
	}

	if ((func->card->cccr_flags & SDIO_SUPPORT_MULTIBLOCK) &&
		((len > func->block_size))) {
		/* Use block I/O for r/w where possible */
		while (remaining >= func->block_size) {
			blocks = remaining / func->block_size;
			size = blocks * func->block_size;
			ret = sdio_io_rw_extended(func->card, direction,
				func->num, reg_addr, increment, buf, blocks,
				func->block_size);
			if (ret) {
				return ret;
			}
			/* Update remaining length and buffer pointer */
			remaining -= size;
			buf += size;
			if (increment) {
				reg_addr += size;
			}
		}
	}
	/* Remaining data must be written using byte I/O */
	while (remaining > 0) {
		size = MIN(remaining, func->cis.max_blk_size);

		ret = sdio_io_rw_extended(func->card, direction, func->num,
			reg_addr, increment, buf, 0, size);
		if (ret) {
			return ret;
		}
		remaining -= size;
		buf += size;
		if (increment) {
			reg_addr += size;
		}
	}
	return 0;
}

/*
 * Read card capability register to determine features card supports.
 */
static int sdio_read_cccr(struct sd_card *card)
{
	int ret;
	uint8_t data;
	uint32_t cccr_ver;

	ret = sdio_io_rw_direct(card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_CCCR, 0, &data);
	if (ret) {
		LOG_DBG("CCCR read failed: %d", ret);
		return ret;
	}
	cccr_ver = (data & SDIO_CCCR_CCCR_REV_MASK) >>
		SDIO_CCCR_CCCR_REV_SHIFT;
	LOG_DBG("SDIO cccr revision %u", cccr_ver);
	/* Read SD spec version */
	ret = sdio_io_rw_direct(card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_SD, 0, &data);
	if (ret) {
		return ret;
	}
	card->sd_version = (data & SDIO_CCCR_SD_SPEC_MASK) >> SDIO_CCCR_SD_SPEC_SHIFT;
	/* Read CCCR capability flags */
	ret = sdio_io_rw_direct(card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_CAPS, 0, &data);
	if (ret) {
		return ret;
	}
	card->cccr_flags = 0;
	if (data & SDIO_CCCR_CAPS_BLS) {
		card->cccr_flags |= SDIO_SUPPORT_4BIT_LS_BUS;
	}
	if (data & SDIO_CCCR_CAPS_SMB) {
		card->cccr_flags |= SDIO_SUPPORT_MULTIBLOCK;
	}
	if (cccr_ver >= SDIO_CCCR_CCCR_REV_2_00) {
		/* Read high speed properties */
		ret = sdio_io_rw_direct(card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			SDIO_CCCR_SPEED, 0, &data);
		if (ret) {
			return ret;
		}
		if (data & SDIO_CCCR_SPEED_SHS) {
			card->cccr_flags |= SDIO_SUPPORT_HS;
		}
	}
	if (cccr_ver >= SDIO_CCCR_CCCR_REV_3_00 &&
		(card->flags & SD_1800MV_FLAG)) {
		/* Read UHS properties */
		ret = sdio_io_rw_direct(card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			SDIO_CCCR_UHS, 0, &data);
		if (ret) {
			return ret;
		}
		if (sdmmc_host_uhs(&card->host_props)) {
			if (data & SDIO_CCCR_UHS_SDR50) {
				card->cccr_flags |= SDIO_SUPPORT_SDR50;
			}
			if (data & SDIO_CCCR_UHS_SDR104) {
				card->cccr_flags |= SDIO_SUPPORT_SDR104;
			}
			if (data & SDIO_CCCR_UHS_DDR50) {
				card->cccr_flags |= SDIO_SUPPORT_DDR50;
			}
		}

		ret = sdio_io_rw_direct(card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			SDIO_CCCR_DRIVE_STRENGTH, 0, &data);
		if (ret) {
			return ret;
		}
		card->switch_caps.sd_drv_type = 0;
		if (data & SDIO_CCCR_DRIVE_STRENGTH_A) {
			card->switch_caps.sd_drv_type |= SD_DRIVER_TYPE_A;
		}
		if (data & SDIO_CCCR_DRIVE_STRENGTH_C) {
			card->switch_caps.sd_drv_type |= SD_DRIVER_TYPE_C;
		}
		if (data & SDIO_CCCR_DRIVE_STRENGTH_D) {
			card->switch_caps.sd_drv_type |= SD_DRIVER_TYPE_D;
		}
	}
	return 0;
}

static void sdio_decode_cis(struct sdio_cis *cis, enum sdio_func_num func,
			    uint8_t *data, uint8_t tpl_code, uint8_t tpl_link)
{
	switch (tpl_code) {
	case SDIO_TPL_CODE_MANIFID:
		cis->manf_id = data[0] | ((uint16_t)data[1] << 8);
		cis->manf_code = data[2] | ((uint16_t)data[3] << 8);
		break;
	case SDIO_TPL_CODE_FUNCID:
		cis->func_id = data[0];
		break;
	case SDIO_TPL_CODE_FUNCE:
		if (func == 0) {
			cis->max_blk_size = data[1] | ((uint16_t)data[2] << 8);
			cis->max_speed = data[3];
		} else {
			cis->max_blk_size = data[12] | ((uint16_t)data[13] << 8);
			cis->rdy_timeout = data[28] | ((uint16_t)data[29] << 8);
		}
		break;
	default:
		LOG_WRN("Unknown CIS tuple %d", tpl_code);
		break;
	}
}

/*
 * Read CIS for a given SDIO function.
 * Tuples provides a list of tuples that should be decoded.
 */
static int sdio_read_cis(struct sdio_func *func,
			 uint8_t *tuples,
			 uint32_t tuple_count)
{
	int ret;
	char *data = func->card->card_buffer;
	uint32_t cis_ptr = 0, num = 0;
	uint8_t tpl_code, tpl_link;
	bool match_tpl = false;

	memset(&func->cis, 0, sizeof(struct sdio_cis));
	/* First find the CIS pointer for this function */
	for (int i = 0; i < 3; i++) {
		ret = sdio_io_rw_direct(func->card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			SDIO_FBR_BASE(func->num) + SDIO_FBR_CIS + i, 0, data);
		if (ret) {
			return ret;
		}
		cis_ptr |= *data << (i * 8);
	}
	/* Read CIS tuples until we have read all requested CIS tuple codes */
	do {
		/* Read tuple code */
		ret = sdio_io_rw_direct(func->card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			cis_ptr++, 0, &tpl_code);
		if (ret) {
			return ret;
		}
		if (tpl_code == SDIO_TPL_CODE_END) {
			/* End of tuple chain */
			break;
		}
		if (tpl_code == SDIO_TPL_CODE_NULL) {
			/* Skip NULL tuple */
			continue;
		}
		/* Read tuple link */
		ret = sdio_io_rw_direct(func->card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			cis_ptr++, 0, &tpl_link);
		if (ret) {
			return ret;
		}
		if (tpl_link == SDIO_TPL_CODE_END) {
			/* End of tuple chain */
			break;
		}
		/* Check to see if read tuple matches any we should look for */
		for (int i = 0; i < tuple_count; i++) {
			if (tpl_code == tuples[i]) {
				match_tpl = true;
				break;
			}
		}
		if (match_tpl) {
			/* tuple chains may be maximum of 255 bytes long */
			memset(data, 0, 255);
			for (int i = 0; i < tpl_link; i++) {
				ret = sdio_io_rw_direct(func->card, SDIO_IO_READ,
					SDIO_FUNC_NUM_0, cis_ptr++, 0, data + i);
				if (ret) {
					return ret;
				}
			}
			num++;
			match_tpl = false;
			/* Decode the CIS data we read */
			sdio_decode_cis(&func->cis, func->num, data,
				tpl_code, tpl_link);
		} else {
			/* Advance CIS pointer */
			cis_ptr += tpl_link;
		}
	} while (num < tuple_count);
	LOG_DBG("SDIO CIS max block size for func %d: %d", func->num,
		func->cis.max_blk_size);
	return ret;
}

static int sdio_set_bus_width(struct sd_card *card, enum sdhc_bus_width width)
{
	uint8_t reg_bus_interface = 0U;
	int ret;

	ret = sdio_io_rw_direct(card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_BUS_IF, 0, &reg_bus_interface);
	if (ret) {
		return ret;
	}
	reg_bus_interface &= ~SDIO_CCCR_BUS_IF_WIDTH_MASK;
	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		reg_bus_interface |= SDIO_CCCR_BUS_IF_WIDTH_1_BIT;
		break;
	case SDHC_BUS_WIDTH4BIT:
		reg_bus_interface |= SDIO_CCCR_BUS_IF_WIDTH_4_BIT;
		break;
	case SDHC_BUS_WIDTH8BIT:
		reg_bus_interface |= SDIO_CCCR_BUS_IF_WIDTH_8_BIT;
		break;
	default:
		return -ENOTSUP;
	}
	ret = sdio_io_rw_direct(card, SDIO_IO_WRITE, SDIO_FUNC_NUM_0,
		SDIO_CCCR_BUS_IF, reg_bus_interface, &reg_bus_interface);
	if (ret) {
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

static inline void sdio_select_bus_speed(struct sd_card *card)
{
	if (card->host_props.host_caps.sdr104_support &&
		(card->cccr_flags & SDIO_SUPPORT_SDR104)) {
		card->card_speed = SD_TIMING_SDR104;
		card->switch_caps.uhs_max_dtr = UHS_SDR104_MAX_DTR;
	} else if (card->host_props.host_caps.ddr50_support &&
		(card->cccr_flags & SDIO_SUPPORT_DDR50)) {
		card->card_speed = SD_TIMING_DDR50;
		card->switch_caps.uhs_max_dtr = UHS_DDR50_MAX_DTR;
	} else if (card->host_props.host_caps.sdr50_support &&
		(card->cccr_flags & SDIO_SUPPORT_SDR50)) {
		card->card_speed = SD_TIMING_SDR50;
		card->switch_caps.uhs_max_dtr = UHS_SDR50_MAX_DTR;
	} else if (card->host_props.host_caps.high_spd_support &&
		(card->cccr_flags & SDIO_SUPPORT_HS)) {
		card->card_speed = SD_TIMING_HIGH_SPEED;
		card->switch_caps.hs_max_dtr = HS_MAX_DTR;
	} else {
		card->card_speed = SD_TIMING_DEFAULT;
	}
}

/* Applies selected card bus speed to card and host */
static int sdio_set_bus_speed(struct sd_card *card)
{
	int ret, timing, retries = CONFIG_SD_RETRY_COUNT;
	uint32_t bus_clock;
	uint8_t speed_reg, target_speed;

	switch (card->card_speed) {
	/* Set bus clock speed */
	case SD_TIMING_SDR104:
		bus_clock = MIN(card->host_props.f_max, card->switch_caps.uhs_max_dtr);
		target_speed = SDIO_CCCR_SPEED_SDR104;
		timing = SDHC_TIMING_SDR104;
		break;
	case SD_TIMING_DDR50:
		bus_clock = MIN(card->host_props.f_max, card->switch_caps.uhs_max_dtr);
		target_speed = SDIO_CCCR_SPEED_DDR50;
		timing = SDHC_TIMING_DDR50;
		break;
	case SD_TIMING_SDR50:
		bus_clock = MIN(card->host_props.f_max, card->switch_caps.uhs_max_dtr);
		target_speed = SDIO_CCCR_SPEED_SDR50;
		timing = SDHC_TIMING_SDR50;
		break;
	case SD_TIMING_HIGH_SPEED:
		bus_clock = MIN(card->host_props.f_max, card->switch_caps.hs_max_dtr);
		target_speed = SDIO_CCCR_SPEED_SDR25;
		timing = SDHC_TIMING_HS;
		break;
	case SD_TIMING_DEFAULT:
		bus_clock = MIN(card->host_props.f_max, MHZ(25));
		target_speed = SDIO_CCCR_SPEED_SDR12;
		timing = SDHC_TIMING_LEGACY;
		break;
	default:
		/* No need to change bus speed */
		return 0;
	}
	/* Read the bus speed register */
	ret = sdio_io_rw_direct(card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_SPEED, 0, &speed_reg);
	if (ret) {
		return ret;
	}
	/* Attempt to set speed several times */
	do {
		/* Set new speed */
		speed_reg &= ~SDIO_CCCR_SPEED_MASK;
		speed_reg |= (target_speed << SDIO_CCCR_SPEED_SHIFT);
		ret = sdio_io_rw_direct(card, SDIO_IO_WRITE, SDIO_FUNC_NUM_0,
			SDIO_CCCR_SPEED, speed_reg, &speed_reg);
		if (ret) {
			return ret;
		}
	} while (((speed_reg & target_speed) != target_speed) && retries-- > 0);
	if (retries == 0) {
		/* Don't error out, as card can still work */
		LOG_WRN("Could not set target SDIO speed");
	} else {
		/* Set card bus clock and timing */
		card->bus_io.timing = timing;
		card->bus_io.clock = bus_clock;
		LOG_DBG("Setting bus clock to: %d", card->bus_io.clock);
		ret = sdhc_set_io(card->sdhc, &card->bus_io);
		if (ret) {
			LOG_ERR("Failed to change host bus speed");
			return ret;
		}
	}
	return ret;
}

/*
 * Initialize an SDIO card for use with subsystem
 */
int sdio_card_init(struct sd_card *card)
{
	int ret;
	uint32_t ocr_arg = 0U;

	/* Probe card with SDIO OCR CM5 */
	ret = sdio_send_ocr(card, ocr_arg);
	if (ret) {
		return ret;
	}
	/* Card responded to CMD5, type is SDIO */
	card->type = CARD_SDIO;
	/* Set voltage window */
	if (card->host_props.host_caps.vol_300_support) {
		ocr_arg |= SD_OCR_VDD29_30FLAG;
	}
	ocr_arg |= (SD_OCR_VDD32_33FLAG | SD_OCR_VDD33_34FLAG);
	if (IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE) &&
		card->host_props.host_caps.vol_180_support) {
		/* See if the card also supports 1.8V */
		ocr_arg |= SD_OCR_SWITCH_18_REQ_FLAG;
	}
	ret = sdio_send_ocr(card, ocr_arg);
	if (ret) {
		return ret;
	}
	if (card->ocr & SD_OCR_SWITCH_18_ACCEPT_FLAG) {
		LOG_DBG("Card supports 1.8V signalling");
		card->flags |= SD_1800MV_FLAG;
	}
	/* Check OCR voltage window */
	if (card->ocr & SD_OCR_VDD29_30FLAG) {
		card->flags |= SD_3000MV_FLAG;
	}
	/* Check mem present flag */
	if (card->ocr & SDIO_OCR_MEM_PRESENT_FLAG) {
		card->flags |= SD_MEM_PRESENT_FLAG;
	}
	/* Following steps are only required when using native SD mode */
	if (IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		/*
		 * If card and host support 1.8V, perform voltage switch sequence now.
		 * note that we skip this switch if the UHS protocol is not enabled.
		 */
		if (IS_ENABLED(CONFIG_SD_UHS_PROTOCOL) &&
		    (card->flags & SD_1800MV_FLAG) &&
		    (!card->host_props.is_spi) &&
		    (card->host_props.host_caps.vol_180_support)) {
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
		if ((card->flags & SD_MEM_PRESENT_FLAG) &&
			((card->flags & SD_SDHC_FLAG) == 0)) {
			/* We must send CMD2 to get card cid */
			ret = card_read_cid(card);
			if (ret) {
				return ret;
			}
		}
		/* Send CMD3 to get card relative address */
		ret = sdmmc_request_rca(card);
		if (ret) {
			return ret;
		}
		/* Move the card to transfer state (with CMD7) to run
		 * remaining commands
		 */
		ret = sdmmc_select_card(card);
		if (ret) {
			return ret;
		}
	}
	/* Read SDIO card common control register */
	ret = sdio_read_cccr(card);
	if (ret) {
		return ret;
	}
	/* Initialize internal card function 0 structure */
	card->func0.num = SDIO_FUNC_NUM_0;
	card->func0.card = card;
	ret = sdio_read_cis(&card->func0, cis_tuples,
		ARRAY_SIZE(cis_tuples));
	if (ret) {
		return ret;
	}

	/* If card and host support 4 bit bus, enable it */
	if (IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE) &&
		((card->cccr_flags & SDIO_SUPPORT_HS) ||
		(card->cccr_flags & SDIO_SUPPORT_4BIT_LS_BUS))) {
		/* Raise bus width to 4 bits */
		ret = sdio_set_bus_width(card, SDHC_BUS_WIDTH4BIT);
		if (ret) {
			return ret;
		}
		LOG_DBG("Raised card bus width to 4 bits");
	}

	/* Select and set bus speed */
	sdio_select_bus_speed(card);
	ret = sdio_set_bus_speed(card);
	if (ret) {
		return ret;
	}
	if (card->card_speed == SD_TIMING_SDR50 ||
		card->card_speed == SD_TIMING_SDR104) {
		/* SDR104, SDR50, and DDR50 mode need tuning */
		ret = sdhc_execute_tuning(card->sdhc);
		if (ret) {
			LOG_ERR("SD tuning failed: %d", ret);
		}
	}
	return ret;
}

/**
 * @brief Initialize SDIO function.
 *
 * Initializes SDIO card function. The card function will not be enabled,
 * but after this call returns the SDIO function structure can be used to read
 * and write data from the card.
 * @param func: function structure to initialize
 * @param card: SD card to enable function on
 * @param num: function number to initialize
 * @retval 0 function was initialized successfully
 * @retval -EIO: I/O error
 */
int sdio_init_func(struct sd_card *card, struct sdio_func *func,
		   enum sdio_func_num num)
{
	/* Initialize function structure */
	func->num = num;
	func->card = card;
	func->block_size = 0;
	/* Read function properties from CCCR */
	return sdio_read_cis(func, cis_tuples, ARRAY_SIZE(cis_tuples));
}



/**
 * @brief Enable SDIO function
 *
 * Enables SDIO card function. @ref sdio_init_func must be called to
 * initialized the function structure before enabling it in the card.
 * @param func: function to enable
 * @retval 0 function was enabled successfully
 * @retval -ETIMEDOUT: card I/O timed out
 * @retval -EIO: I/O error
 */
int sdio_enable_func(struct sdio_func *func)
{
	int ret;
	uint8_t reg;
	uint16_t retries = CONFIG_SD_RETRY_COUNT;

	/* Enable the I/O function */
	ret = sdio_io_rw_direct(func->card, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_IO_EN, 0, &reg);
	if (ret) {
		return ret;
	}
	reg |= BIT(func->num);
	ret = sdio_io_rw_direct(func->card, SDIO_IO_WRITE, SDIO_FUNC_NUM_0,
		SDIO_CCCR_IO_EN, reg, &reg);
	if (ret) {
		return ret;
	}
	/* Wait for I/O ready to be set */
	if (func->cis.rdy_timeout) {
		retries = 1U;
	}
	do {
		/* Timeout is in units of 10ms */
		sd_delay(((uint32_t)func->cis.rdy_timeout) * 10U);
		ret = sdio_io_rw_direct(func->card, SDIO_IO_READ,
			SDIO_FUNC_NUM_0, SDIO_CCCR_IO_RD, 0, &reg);
		if (ret) {
			return ret;
		}
		if (reg & BIT(func->num)) {
			return 0;
		}
	} while (retries-- != 0);
	return -ETIMEDOUT;
}

/**
 * @brief Set block size of SDIO function
 *
 * Set desired block size for SDIO function, used by block transfers
 * to SDIO registers.
 * @param func: function to set block size for
 * @param bsize: block size
 * @retval 0 block size was set
 * @retval -EINVAL: unsupported/invalid block size
 * @retval -EIO: I/O error
 */
int sdio_set_block_size(struct sdio_func *func, uint16_t bsize)
{
	int ret;
	uint8_t reg;

	if (func->cis.max_blk_size < bsize) {
		return -EINVAL;
	}
	for (int i = 0; i < 2; i++) {
		reg = (bsize >> (i * 8));
		ret = sdio_io_rw_direct(func->card, SDIO_IO_WRITE, SDIO_FUNC_NUM_0,
			SDIO_FBR_BASE(func->num) + SDIO_FBR_BLK_SIZE + i, reg, NULL);
		if (ret) {
			return ret;
		}
	}
	func->block_size = bsize;
	return 0;
}

/**
 * @brief Read byte from SDIO register
 *
 * Reads byte from SDIO register
 * @param func: function to read from
 * @param reg: register address to read from
 * @param val: filled with byte value read from register
 * @retval 0 read succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card read timed out
 * @retval -EIO: I/O error
 */
int sdio_read_byte(struct sdio_func *func, uint32_t reg, uint8_t *val)
{
	int ret;

	if ((func->card->type != CARD_SDIO) && (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&func->card->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	ret = sdio_io_rw_direct(func->card, SDIO_IO_READ, func->num, reg, 0, val);
	k_mutex_unlock(&func->card->lock);
	return ret;
}

/**
 * @brief Write byte to SDIO register
 *
 * Writes byte to SDIO register
 * @param func: function to write to
 * @param reg: register address to write to
 * @param write_val: value to write to register
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_write_byte(struct sdio_func *func, uint32_t reg, uint8_t write_val)
{
	int ret;

	if ((func->card->type != CARD_SDIO) && (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&func->card->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	ret = sdio_io_rw_direct(func->card, SDIO_IO_WRITE, func->num, reg,
		write_val, NULL);
	k_mutex_unlock(&func->card->lock);
	return ret;
}

/**
 * @brief Write byte to SDIO register, and read result
 *
 * Writes byte to SDIO register, and reads the register after write
 * @param func: function to write to
 * @param reg: register address to write to
 * @param write_val: value to write to register
 * @param read_val: filled with value read from register
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_rw_byte(struct sdio_func *func, uint32_t reg, uint8_t write_val,
		 uint8_t *read_val)
{
	int ret;

	if ((func->card->type != CARD_SDIO) && (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&func->card->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	ret = sdio_io_rw_direct(func->card, SDIO_IO_WRITE, func->num, reg,
		write_val, read_val);
	k_mutex_unlock(&func->card->lock);
	return ret;
}

/**
 * @brief Read bytes from SDIO fifo
 *
 * Reads bytes from SDIO register, treating it as a fifo. Reads will
 * all be done from same address.
 * @param func: function to read from
 * @param reg: register address of fifo
 * @param data: filled with data read from fifo
 * @param len: length of data to read from card
 * @retval 0 read succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card read timed out
 * @retval -EIO: I/O error
 */
int sdio_read_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
		   uint32_t len)
{
	int ret;

	if ((func->card->type != CARD_SDIO) && (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&func->card->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	ret = sdio_io_rw_extended_helper(func, SDIO_IO_READ, reg, false,
		data, len);
	k_mutex_unlock(&func->card->lock);
	return ret;
}

/**
 * @brief Write bytes to SDIO fifo
 *
 * Writes bytes to SDIO register, treating it as a fifo. Writes will
 * all be done to same address.
 * @param func: function to write to
 * @param reg: register address of fifo
 * @param data: data to write to fifo
 * @param len: length of data to write to card
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_write_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
		    uint32_t len)
{
	int ret;

	if ((func->card->type != CARD_SDIO) && (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&func->card->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	ret = sdio_io_rw_extended_helper(func, SDIO_IO_WRITE, reg, false,
		data, len);
	k_mutex_unlock(&func->card->lock);
	return ret;
}

/**
 * @brief Read blocks from SDIO fifo
 *
 * Reads blocks from SDIO register, treating it as a fifo. Reads will
 * all be done from same address.
 * @param func: function to read from
 * @param reg: register address of fifo
 * @param data: filled with data read from fifo
 * @param blocks: number of blocks to read from fifo
 * @retval 0 read succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card read timed out
 * @retval -EIO: I/O error
 */
int sdio_read_blocks_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
			  uint32_t blocks)
{
	int ret;

	if ((func->card->type != CARD_SDIO) && (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&func->card->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	ret = sdio_io_rw_extended(func->card, SDIO_IO_READ, func->num, reg,
		false, data, blocks, func->block_size);
	k_mutex_unlock(&func->card->lock);
	return ret;
}

/**
 * @brief Write blocks to SDIO fifo
 *
 * Writes blocks from SDIO register, treating it as a fifo. Writes will
 * all be done to same address.
 * @param func: function to write to
 * @param reg: register address of fifo
 * @param data: data to write to fifo
 * @param blocks: number of blocks to write to fifo
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_write_blocks_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
			   uint32_t blocks)
{
	int ret;

	if ((func->card->type != CARD_SDIO) && (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&func->card->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	ret = sdio_io_rw_extended(func->card, SDIO_IO_WRITE, func->num, reg,
		false, data, blocks, func->block_size);
	k_mutex_unlock(&func->card->lock);
	return ret;
}

/**
 * @brief Copy bytes from an SDIO card
 *
 * Copies bytes from an SDIO card, starting from provided address.
 * @param func: function to read from
 * @param reg: register address to start copy at
 * @param data: buffer to copy data into
 * @param len: length of data to read
 * @retval 0 read succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card read timed out
 * @retval -EIO: I/O error
 */
int sdio_read_addr(struct sdio_func *func, uint32_t reg, uint8_t *data,
		   uint32_t len)
{
	int ret;

	if ((func->card->type != CARD_SDIO) && (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&func->card->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	ret = sdio_io_rw_extended_helper(func, SDIO_IO_READ, reg, true,
		data, len);
	k_mutex_unlock(&func->card->lock);
	return ret;
}

/**
 * @brief Copy bytes to an SDIO card
 *
 * Copies bytes to an SDIO card, starting from provided address.
 *
 * @param func: function to write to
 * @param reg: register address to start copy at
 * @param data: buffer to copy data from
 * @param len: length of data to write
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_write_addr(struct sdio_func *func, uint32_t reg, uint8_t *data,
		    uint32_t len)
{
	int ret;

	if ((func->card->type != CARD_SDIO) && (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&func->card->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		LOG_WRN("Could not get SD card mutex");
		return -EBUSY;
	}
	ret = sdio_io_rw_extended_helper(func, SDIO_IO_WRITE, reg, true,
		data, len);
	k_mutex_unlock(&func->card->lock);
	return ret;
}
