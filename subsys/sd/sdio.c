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
#include <zephyr/sd/sdio.h>
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

static int sdio_io_rw_direct(struct sdio_dev *dev,
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

	ret = sdhc_request(dev->sdhc, &cmd, NULL);
	if (ret) {
		return ret;
	}
	if (data_out) {
		if (dev->caps & SDIO_CAP_SPI) {
			*data_out = (cmd.response[0U] >> 8) & SDIO_DIRECT_CMD_DATA_MASK;
		} else {
			*data_out = cmd.response[0U] & SDIO_DIRECT_CMD_DATA_MASK;
		}
	}
	return ret;
}


static int sdio_io_rw_extended(struct sdio_dev *dev,
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
		/* Byte mode: 9-bit byte count, 0 = 512 per spec. */
		cmd.arg |= (block_size == 512) ? 0 : block_size;
	} else {
		/* Block mode: 9-bit block count, valid range 1..511. Per the
		 * SDIO spec, count = 0 in block mode means "transfer until
		 * I/O-abort", not 512 blocks; 512 itself has no valid
		 * encoding. Reject blocks > 511 so the caller gets a clear
		 * error rather than either silent register-address corruption
		 * (bit 9 leaking from blocks=0x200) or a silent unbounded
		 * transfer (a naive 9-bit mask of 512 yielding 0). Callers
		 * needing more than 511 blocks must issue multiple CMD53s.
		 */
		if (blocks > 511) {
			return -EINVAL;
		}
		cmd.arg |= BIT(SDIO_EXTEND_CMD_ARG_BLK_SHIFT) | blocks;
	}

	data.block_size = block_size;
	/* Host expects blocks to be at least 1 */
	data.blocks = blocks ? blocks : 1;
	data.data = buf;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;
	return sdhc_request(dev->sdhc, &cmd, &data);
}

/* Sizing context for a split extended transfer. */
struct sdio_ext_xfer {
	struct sdio_dev *dev;
	enum sdio_func_num func;
	enum sdio_io_dir dir;
	uint32_t reg;
	bool increment;
	uint16_t block_size; /*!< Negotiated block size (0 if unset) */
	uint16_t max_byte; /*!< Byte-mode transfer limit */
};

/*
 * Helper for extended r/w. Splits the transfer into the minimum possible
 * number of block r/w, then uses byte transfers for remaining data
 */
static int sdio_io_rw_extended_helper(const struct sdio_ext_xfer *x,
				      uint8_t *buf, uint32_t len)
{
	int ret;
	int remaining = len;
	uint32_t blocks, size;
	uint32_t reg_addr = x->reg;

	if (x->func > SDIO_MAX_IO_NUMS) {
		return -EINVAL;
	}

	if ((x->dev->caps & SDIO_CAP_MULTIBLOCK) && (x->block_size != 0U) &&
		(len > x->block_size)) {
		/* Use block I/O for r/w where possible */
		while (remaining >= x->block_size) {
			blocks = remaining / x->block_size;
			size = blocks * x->block_size;
			ret = sdio_io_rw_extended(x->dev, x->dir, x->func,
				reg_addr, x->increment, buf, blocks,
				x->block_size);
			if (ret) {
				return ret;
			}
			/* Update remaining length and buffer pointer */
			remaining -= size;
			buf += size;
			if (x->increment) {
				reg_addr += size;
			}
		}
	}
	/* Remaining data must be transferred using byte I/O */
	if (remaining > 0 && x->max_byte == 0U) {
		/* No known byte-mode limit: a zero size would spin forever. */
		return -EIO;
	}
	while (remaining > 0) {
		size = MIN(remaining, x->max_byte);

		ret = sdio_io_rw_extended(x->dev, x->dir, x->func, reg_addr,
			x->increment, buf, 0, size);
		if (ret) {
			return ret;
		}
		remaining -= size;
		buf += size;
		if (x->increment) {
			reg_addr += size;
		}
	}
	return 0;
}

/*
 * Read card capability register to determine features card supports.
 */
int sdio_read_cccr(struct sdio_dev *dev, struct sdio_cccr *cccr, bool probe_uhs)
{
	int ret;
	uint8_t data;
	uint32_t cccr_ver;

	memset(cccr, 0, sizeof(*cccr));

	ret = sdio_io_rw_direct(dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_CCCR, 0, &data);
	if (ret) {
		LOG_DBG("CCCR read failed: %d", ret);
		return ret;
	}
	cccr_ver = (data & SDIO_CCCR_CCCR_REV_MASK) >>
		SDIO_CCCR_CCCR_REV_SHIFT;
	cccr->sdio_revision = cccr_ver;
	LOG_DBG("SDIO cccr revision %u", cccr_ver);
	/* Read SD spec version */
	ret = sdio_io_rw_direct(dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_SD, 0, &data);
	if (ret) {
		return ret;
	}
	cccr->sd_spec = (data & SDIO_CCCR_SD_SPEC_MASK) >> SDIO_CCCR_SD_SPEC_SHIFT;
	/* Read CCCR capability flags */
	ret = sdio_io_rw_direct(dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_CAPS, 0, &data);
	if (ret) {
		return ret;
	}
	cccr->support_4bit_ls = (data & SDIO_CCCR_CAPS_BLS) != 0;
	cccr->support_multiblock = (data & SDIO_CCCR_CAPS_SMB) != 0;
	if (cccr_ver >= SDIO_CCCR_CCCR_REV_2_00) {
		/* Read high speed properties */
		ret = sdio_io_rw_direct(dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			SDIO_CCCR_SPEED, 0, &data);
		if (ret) {
			return ret;
		}
		cccr->support_hs = (data & SDIO_CCCR_SPEED_SHS) != 0;
	}
	if (cccr_ver >= SDIO_CCCR_CCCR_REV_3_00 && probe_uhs) {
		/* Read UHS properties */
		ret = sdio_io_rw_direct(dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			SDIO_CCCR_UHS, 0, &data);
		if (ret) {
			return ret;
		}
		cccr->support_sdr50 = (data & SDIO_CCCR_UHS_SDR50) != 0;
		cccr->support_sdr104 = (data & SDIO_CCCR_UHS_SDR104) != 0;
		cccr->support_ddr50 = (data & SDIO_CCCR_UHS_DDR50) != 0;

		ret = sdio_io_rw_direct(dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			SDIO_CCCR_DRIVE_STRENGTH, 0, &data);
		if (ret) {
			return ret;
		}
		cccr->drv_type_a = (data & SDIO_CCCR_DRIVE_STRENGTH_A) != 0;
		cccr->drv_type_c = (data & SDIO_CCCR_DRIVE_STRENGTH_C) != 0;
		cccr->drv_type_d = (data & SDIO_CCCR_DRIVE_STRENGTH_D) != 0;
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
	/* CIS tuple chains may be at most 255 bytes long. */
	uint8_t data[255];
	uint32_t cis_ptr = 0, num = 0;
	uint8_t tpl_code, tpl_link;
	bool match_tpl = false;

	memset(&func->cis, 0, sizeof(struct sdio_cis));
	/* First find the CIS pointer for this function */
	for (int i = 0; i < 3; i++) {
		ret = sdio_io_rw_direct(func->dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
			SDIO_FBR_BASE(func->num) + SDIO_FBR_CIS + i, 0, data);
		if (ret) {
			return ret;
		}
		cis_ptr |= *data << (i * 8);
	}
	/* Read CIS tuples until we have read all requested CIS tuple codes */
	do {
		/* Read tuple code */
		ret = sdio_io_rw_direct(func->dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
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
		ret = sdio_io_rw_direct(func->dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
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
				ret = sdio_io_rw_direct(func->dev, SDIO_IO_READ,
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

	ret = sdio_io_rw_direct(&card->sdio_bus, SDIO_IO_READ, SDIO_FUNC_NUM_0,
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
	ret = sdio_io_rw_direct(&card->sdio_bus, SDIO_IO_WRITE, SDIO_FUNC_NUM_0,
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
	ret = sdio_io_rw_direct(&card->sdio_bus, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_SPEED, 0, &speed_reg);
	if (ret) {
		return ret;
	}
	/* Attempt to set speed several times */
	do {
		/* Set new speed */
		speed_reg &= ~SDIO_CCCR_SPEED_MASK;
		speed_reg |= (target_speed << SDIO_CCCR_SPEED_SHIFT);
		ret = sdio_io_rw_direct(&card->sdio_bus, SDIO_IO_WRITE, SDIO_FUNC_NUM_0,
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
	uint32_t cid[4] = {0};

	/* Probe card with SDIO OCR CM5 */
	ret = sdio_send_ocr(card, ocr_arg);
	if (ret) {
		return ret;
	}
	/* Card responded to CMD5, type is SDIO */
	card->type = CARD_SDIO;
	/* Bind the SDIO host endpoint used for all function I/O */
	sdio_dev_init(&card->sdio_bus, card->sdhc,
		      card->host_props.is_spi ? SDIO_CAP_SPI : 0);
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
			ret = card_read_cid(card, cid);
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
	struct sdio_cccr cccr;

	ret = sdio_read_cccr(&card->sdio_bus, &cccr,
			     (card->flags & SD_1800MV_FLAG) != 0);
	if (ret) {
		return ret;
	}
	/* Map the parsed CCCR onto card bookkeeping */
	card->sd_version = cccr.sd_spec;
	card->cccr_flags = 0;
	if (cccr.support_4bit_ls) {
		card->cccr_flags |= SDIO_SUPPORT_4BIT_LS_BUS;
	}
	if (cccr.support_multiblock) {
		card->cccr_flags |= SDIO_SUPPORT_MULTIBLOCK;
	}
	if (cccr.support_hs) {
		card->cccr_flags |= SDIO_SUPPORT_HS;
	}
	if (sdmmc_host_uhs(&card->host_props)) {
		if (cccr.support_sdr50) {
			card->cccr_flags |= SDIO_SUPPORT_SDR50;
		}
		if (cccr.support_sdr104) {
			card->cccr_flags |= SDIO_SUPPORT_SDR104;
		}
		if (cccr.support_ddr50) {
			card->cccr_flags |= SDIO_SUPPORT_DDR50;
		}
	}
	card->switch_caps.sd_drv_type = 0;
	if (cccr.drv_type_a) {
		card->switch_caps.sd_drv_type |= SD_DRIVER_TYPE_A;
	}
	if (cccr.drv_type_c) {
		card->switch_caps.sd_drv_type |= SD_DRIVER_TYPE_C;
	}
	if (cccr.drv_type_d) {
		card->switch_caps.sd_drv_type |= SD_DRIVER_TYPE_D;
	}
	/* Reflect the negotiated CCCR capabilities on the host endpoint */
	if (card->cccr_flags & SDIO_SUPPORT_MULTIBLOCK) {
		card->sdio_bus.caps |= SDIO_CAP_MULTIBLOCK;
	}
	if (card->cccr_flags & SDIO_SUPPORT_HS) {
		card->sdio_bus.caps |= SDIO_CAP_HS;
	}
	if (card->cccr_flags & SDIO_SUPPORT_4BIT_LS_BUS) {
		card->sdio_bus.caps |= SDIO_CAP_4BIT_BUS;
	}
	/* Initialize internal card function 0 structure */
	card->func0.num = SDIO_FUNC_NUM_0;
	card->func0.card = card;
	card->func0.dev = &card->sdio_bus;
	ret = sdio_read_cis(&card->func0, cis_tuples,
		ARRAY_SIZE(cis_tuples));
	if (ret) {
		return ret;
	}
	card->sdio_bus.max_blk_size = card->func0.cis.max_blk_size;

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

int sdio_dev_init(struct sdio_dev *dev, const struct device *sdhc,
		  uint32_t caps)
{
	if (dev == NULL || sdhc == NULL) {
		return -EINVAL;
	}
	dev->sdhc = sdhc;
	dev->caps = caps;
	dev->max_blk_size = 0;
	k_mutex_init(&dev->lock);
	return 0;
}

int sdio_func_bind(struct sdio_dev *dev, struct sdio_func *func,
		   enum sdio_func_num num)
{
	if (dev == NULL || func == NULL) {
		return -EINVAL;
	}
	func->num = num;
	func->dev = dev;
	func->card = NULL;
	func->block_size = 0;
	memset(&func->cis, 0, sizeof(func->cis));
	return 0;
}

int sdio_init_func(struct sd_card *card, struct sdio_func *func,
		   enum sdio_func_num num)
{
	/* Initialize function structure */
	func->num = num;
	func->card = card;
	func->dev = &card->sdio_bus;
	func->block_size = 0;
	/* Read function properties from CCCR */
	return sdio_read_cis(func, cis_tuples, ARRAY_SIZE(cis_tuples));
}

/*
 * Endpoint-level API. Role-neutral: keyed on a host endpoint and function
 * number, no dependency on struct sdio_func. The struct sdio_func based calls
 * further down are thin wrappers over these.
 */

static int sdio_bus_lock(struct sdio_dev *dev)
{
	int ret = k_mutex_lock(&dev->lock, K_MSEC(CONFIG_SD_DATA_TIMEOUT));

	if (ret) {
		LOG_WRN("Could not get SDIO bus mutex");
		return -EBUSY;
	}
	return 0;
}

int sdio_dev_enable_func(struct sdio_dev *dev, enum sdio_func_num func,
			 uint16_t rdy_timeout)
{
	int ret;
	uint8_t reg;
	uint16_t retries = CONFIG_SD_RETRY_COUNT;

	/* Enable the I/O function */
	ret = sdio_io_rw_direct(dev, SDIO_IO_READ, SDIO_FUNC_NUM_0,
		SDIO_CCCR_IO_EN, 0, &reg);
	if (ret) {
		return ret;
	}
	reg |= BIT(func);
	ret = sdio_io_rw_direct(dev, SDIO_IO_WRITE, SDIO_FUNC_NUM_0,
		SDIO_CCCR_IO_EN, reg, &reg);
	if (ret) {
		return ret;
	}
	/* Wait for I/O ready to be set */
	if (rdy_timeout) {
		retries = 1U;
	}
	do {
		/* Timeout is in units of 10ms */
		sd_delay(((uint32_t)rdy_timeout) * 10U);
		ret = sdio_io_rw_direct(dev, SDIO_IO_READ,
			SDIO_FUNC_NUM_0, SDIO_CCCR_IO_RD, 0, &reg);
		if (ret) {
			return ret;
		}
		if (reg & BIT(func)) {
			return 0;
		}
	} while (retries-- != 0);
	return -ETIMEDOUT;
}

int sdio_dev_set_block_size(struct sdio_dev *dev, enum sdio_func_num func,
			    uint16_t bsize)
{
	int ret;
	uint8_t reg;

	for (int i = 0; i < 2; i++) {
		reg = (bsize >> (i * 8));
		ret = sdio_io_rw_direct(dev, SDIO_IO_WRITE, SDIO_FUNC_NUM_0,
			SDIO_FBR_BASE(func) + SDIO_FBR_BLK_SIZE + i, reg, NULL);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

int sdio_dev_read_byte(struct sdio_dev *dev, enum sdio_func_num func,
		       uint32_t reg, uint8_t *val)
{
	int ret = sdio_bus_lock(dev);

	if (ret) {
		return ret;
	}
	ret = sdio_io_rw_direct(dev, SDIO_IO_READ, func, reg, 0, val);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_dev_write_byte(struct sdio_dev *dev, enum sdio_func_num func,
			uint32_t reg, uint8_t write_val)
{
	int ret = sdio_bus_lock(dev);

	if (ret) {
		return ret;
	}
	ret = sdio_io_rw_direct(dev, SDIO_IO_WRITE, func, reg, write_val, NULL);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_dev_rw_byte(struct sdio_dev *dev, enum sdio_func_num func,
		     uint32_t reg, uint8_t write_val, uint8_t *read_val)
{
	int ret = sdio_bus_lock(dev);

	if (ret) {
		return ret;
	}
	ret = sdio_io_rw_direct(dev, SDIO_IO_WRITE, func, reg, write_val,
		read_val);
	k_mutex_unlock(&dev->lock);
	return ret;
}

static int sdio_dev_rw_split(struct sdio_dev *dev, enum sdio_func_num func,
			     enum sdio_io_dir dir, uint32_t reg, bool increment,
			     uint8_t *data, uint32_t len, uint16_t block_size,
			     uint16_t max_byte)
{
	struct sdio_ext_xfer x = {
		.dev = dev,
		.func = func,
		.dir = dir,
		.reg = reg,
		.increment = increment,
		.block_size = block_size,
		.max_byte = max_byte,
	};
	int ret = sdio_bus_lock(dev);

	if (ret) {
		return ret;
	}
	ret = sdio_io_rw_extended_helper(&x, data, len);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_dev_read_fifo(struct sdio_dev *dev, enum sdio_func_num func,
		       uint32_t reg, uint8_t *data, uint32_t len,
		       uint16_t block_size, uint16_t max_byte)
{
	return sdio_dev_rw_split(dev, func, SDIO_IO_READ, reg, false, data,
		len, block_size, max_byte);
}

int sdio_dev_write_fifo(struct sdio_dev *dev, enum sdio_func_num func,
			uint32_t reg, uint8_t *data, uint32_t len,
			uint16_t block_size, uint16_t max_byte)
{
	return sdio_dev_rw_split(dev, func, SDIO_IO_WRITE, reg, false, data,
		len, block_size, max_byte);
}

int sdio_dev_read_addr(struct sdio_dev *dev, enum sdio_func_num func,
		       uint32_t reg, uint8_t *data, uint32_t len,
		       uint16_t block_size, uint16_t max_byte)
{
	return sdio_dev_rw_split(dev, func, SDIO_IO_READ, reg, true, data,
		len, block_size, max_byte);
}

int sdio_dev_write_addr(struct sdio_dev *dev, enum sdio_func_num func,
			uint32_t reg, uint8_t *data, uint32_t len,
			uint16_t block_size, uint16_t max_byte)
{
	return sdio_dev_rw_split(dev, func, SDIO_IO_WRITE, reg, true, data,
		len, block_size, max_byte);
}

int sdio_dev_read_blocks_fifo(struct sdio_dev *dev, enum sdio_func_num func,
			      uint32_t reg, uint8_t *data, uint32_t blocks,
			      uint16_t block_size)
{
	int ret = sdio_bus_lock(dev);

	if (ret) {
		return ret;
	}
	ret = sdio_io_rw_extended(dev, SDIO_IO_READ, func, reg, false, data,
		blocks, block_size);
	k_mutex_unlock(&dev->lock);
	return ret;
}

int sdio_dev_write_blocks_fifo(struct sdio_dev *dev, enum sdio_func_num func,
			       uint32_t reg, uint8_t *data, uint32_t blocks,
			       uint16_t block_size)
{
	int ret = sdio_bus_lock(dev);

	if (ret) {
		return ret;
	}
	ret = sdio_io_rw_extended(dev, SDIO_IO_WRITE, func, reg, false, data,
		blocks, block_size);
	k_mutex_unlock(&dev->lock);
	return ret;
}

/*
 * struct sdio_func based API. Thin wrappers that supply per-function state
 * (block size, CIS limits, SD-card type) to the endpoint-level calls above.
 */

static int sdio_func_check(struct sdio_func *func)
{
	if (func->card && (func->card->type != CARD_SDIO) &&
	    (func->card->type != CARD_COMBO)) {
		LOG_WRN("Card does not support SDIO commands");
		return -ENOTSUP;
	}
	return 0;
}

static uint16_t sdio_func_max_byte(struct sdio_func *func)
{
	return func->cis.max_blk_size ? func->cis.max_blk_size
	     : (func->block_size ? func->block_size : func->dev->max_blk_size);
}

int sdio_enable_func(struct sdio_func *func)
{
	return sdio_dev_enable_func(func->dev, func->num, func->cis.rdy_timeout);
}

int sdio_set_block_size(struct sdio_func *func, uint16_t bsize)
{
	int ret;

	if (func->cis.max_blk_size < bsize) {
		return -EINVAL;
	}
	ret = sdio_dev_set_block_size(func->dev, func->num, bsize);
	if (ret == 0) {
		func->block_size = bsize;
	}
	return ret;
}

int sdio_read_byte(struct sdio_func *func, uint32_t reg, uint8_t *val)
{
	int ret = sdio_func_check(func);

	if (ret) {
		return ret;
	}
	return sdio_dev_read_byte(func->dev, func->num, reg, val);
}

int sdio_write_byte(struct sdio_func *func, uint32_t reg, uint8_t write_val)
{
	int ret = sdio_func_check(func);

	if (ret) {
		return ret;
	}
	return sdio_dev_write_byte(func->dev, func->num, reg, write_val);
}

int sdio_rw_byte(struct sdio_func *func, uint32_t reg, uint8_t write_val,
		 uint8_t *read_val)
{
	int ret = sdio_func_check(func);

	if (ret) {
		return ret;
	}
	return sdio_dev_rw_byte(func->dev, func->num, reg, write_val, read_val);
}

int sdio_read_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
		   uint32_t len)
{
	int ret = sdio_func_check(func);

	if (ret) {
		return ret;
	}
	return sdio_dev_read_fifo(func->dev, func->num, reg, data, len,
		func->block_size, sdio_func_max_byte(func));
}

int sdio_write_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
		    uint32_t len)
{
	int ret = sdio_func_check(func);

	if (ret) {
		return ret;
	}
	return sdio_dev_write_fifo(func->dev, func->num, reg, data, len,
		func->block_size, sdio_func_max_byte(func));
}

int sdio_read_blocks_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
			  uint32_t blocks)
{
	int ret = sdio_func_check(func);

	if (ret) {
		return ret;
	}
	return sdio_dev_read_blocks_fifo(func->dev, func->num, reg, data,
		blocks, func->block_size);
}

int sdio_write_blocks_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
			   uint32_t blocks)
{
	int ret = sdio_func_check(func);

	if (ret) {
		return ret;
	}
	return sdio_dev_write_blocks_fifo(func->dev, func->num, reg, data,
		blocks, func->block_size);
}

int sdio_read_addr(struct sdio_func *func, uint32_t reg, uint8_t *data,
		   uint32_t len)
{
	int ret = sdio_func_check(func);

	if (ret) {
		return ret;
	}
	return sdio_dev_read_addr(func->dev, func->num, reg, data, len,
		func->block_size, sdio_func_max_byte(func));
}

int sdio_write_addr(struct sdio_func *func, uint32_t reg, uint8_t *data,
		    uint32_t len)
{
	int ret = sdio_func_check(func);

	if (ret) {
		return ret;
	}
	return sdio_dev_write_addr(func->dev, func->num, reg, data, len,
		func->block_size, sdio_func_max_byte(func));
}
