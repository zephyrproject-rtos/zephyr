/*
 * Copyright (C) 2023 Intel Corporation
 * Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdns_sdhc

#include "sdhc_cdns.h"

#include <string.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_sdhc, CONFIG_SDHC_LOG_LEVEL);

#define SDHC_CDNS_DESC_MAX_LEN 65536U

/**
 * @brief
 * Update the bits under `mask` of an 8-bit slot register to `value`.
 */
static inline void sdhc_cdns_update8(const struct device *dev, uint32_t reg, uint8_t mask,
				     uint8_t value)
{
	uint8_t tmp = sdhc_standard_read8(dev, reg);

	sdhc_standard_write8(dev, reg, (tmp & ~mask) | (value & mask));
}

/**
 * @brief
 * Update the bits under `mask` of a 16-bit slot register to `value`.
 */
static inline void sdhc_cdns_update16(const struct device *dev, uint32_t reg, uint16_t mask,
				      uint16_t value)
{
	uint16_t tmp = sdhc_standard_read16(dev, reg);

	sdhc_standard_write16(dev, reg, (tmp & ~mask) | (value & mask));
}

/**
 * @brief
 * Read the normal (bits 15:0) and error (bits 31:16) interrupt status
 */
static inline uint32_t sdhc_cdns_read_intr(const struct device *dev)
{
	return sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_STAT);
}

/**
 * @brief
 * Clear normal and error interrupt status bits (write-1-to-clear)
 */
static inline void sdhc_cdns_clear_intr(const struct device *dev, uint32_t bits)
{
	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, bits);
}

/**
 * @brief
 * Polled wait for any one of the given interrupt status bits.
 */
static int sdhc_cdns_wait_for_events(const struct device *dev, int32_t timeout_ms, uint32_t events)
{
	if (!WAIT_FOR((sdhc_cdns_read_intr(dev) & events) != 0U,
		      (uint32_t)timeout_ms * USEC_PER_MSEC, k_msleep(1))) {
		return -EAGAIN;
	}

	return 0;
}

/**
 * @brief
 * Check card is detected by host
 */
static int sdhc_cdns_card_detect(const struct device *dev)
{
	const struct sdhc_cdns_config *config = dev->config;
	uint32_t present_state = sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE);

	if ((present_state & SDHC_REG_PRESENT_STATE_CI_MASK) != 0U) {
		return 1;
	}

	/* In case of polling always treat card is detected */
	if (config->broken_cd == true) {
		return 1;
	}

	return 0;
}

/**
 * @brief
 * Setup ADMA2 descriptor table for data transfer
 */
static int sdhc_cdns_setup_dma(const struct device *dev, const struct sdhc_data *data)
{
	struct sdhc_cdns_data *sd_data = dev->data;
	adma2_descriptor *adma2_desc = &sd_data->adma2_desc[0];
	const uint8_t *buff = data->data;
	const uint64_t block_chunk = data->block_size * data->blocks;
	size_t desc_size;
	uint32_t table;
	uint32_t i;
	int ret = 0;

	if ((block_chunk) < SDHC_CDNS_DESC_MAX_LEN) {
		table = 1U;
	} else {
		table = ((block_chunk) / SDHC_CDNS_DESC_MAX_LEN);
		if (((block_chunk) % SDHC_CDNS_DESC_MAX_LEN) != 0U) {
			table += 1U;
		}
	}

	if (table > CONFIG_SDHC_CDNS_DESC_SIZE) {
		LOG_ERR("Descriptor size is too big");
		return -ENOTSUP;
	}

	for (i = 0U; i < (table - 1U); i++) {
		adma2_desc[i].addr = ((mem_addr_t)buff + (i * SDHC_CDNS_DESC_MAX_LEN));
		adma2_desc[i].attr = SDHC_CDNS_ADMA2_DESC_TRAN | SDHC_CDNS_ADMA2_DESC_VALID;
		adma2_desc[i].len = 0U;
	}

	adma2_desc[table - 1U].addr = ((mem_addr_t)buff + (i * SDHC_CDNS_DESC_MAX_LEN));
	adma2_desc[table - 1U].attr =
		SDHC_CDNS_ADMA2_DESC_TRAN | SDHC_CDNS_ADMA2_DESC_END | SDHC_CDNS_ADMA2_DESC_VALID;
	adma2_desc[table - 1U].len = ((block_chunk) - (i * SDHC_CDNS_DESC_MAX_LEN));

	/*
	 * The SDHC reads the ADMA2 descriptor table through DMA.
	 * If the table lives in cacheable memory, make sure all
	 * descriptor writes are visible to the controller before
	 * programming ADMA_SYS_ADDR.
	 */
	desc_size = table * sizeof(adma2_descriptor);
	ret = sys_cache_data_flush_range(adma2_desc, desc_size);
	if (ret != 0) {
		LOG_ERR("Failed to flush ADMA descriptor table: ret=%d desc=%p size=%u", ret,
			adma2_desc, (uint32_t)desc_size);
		return ret;
	}

	sdhc_standard_write64(dev, SDHC_REG_ADMA_SYS_ADDR, (uint64_t)(mem_addr_t)(&adma2_desc[0]));

	return ret;
}

/**
 * @brief
 * Frame the command into the Command register layout. Response-type ->
 * Response Type Select / index / CRC check-enable encoding follows the
 * SD Host Controller Simplified Specification table.
 */
static int sdhc_cdns_cmd_frame(const struct sdhc_command *cmd, bool data, uint16_t *frame)
{
	uint16_t command =
		(cmd->opcode << SDHC_REG_COMMAND_CIDX_SHIFT) & SDHC_REG_COMMAND_CIDX_MASK;

	switch (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		command |= SDHC_CDNS_RESP_NONE;
		break;

	case SD_RSP_TYPE_R1:
		command |= SDHC_CDNS_RESP_R1;
		break;

	case SD_RSP_TYPE_R1b:
		command |= SDHC_CDNS_RESP_R1B;
		break;

	case SD_RSP_TYPE_R2:
		command |= SDHC_CDNS_RESP_R2;
		break;

	case SD_RSP_TYPE_R3:
		command |= SDHC_CDNS_RESP_R3;
		break;

	case SD_RSP_TYPE_R4:
		command |= SDHC_CDNS_RESP_R3;
		break;

	case SD_RSP_TYPE_R5:
		command |= SDHC_CDNS_RESP_R1;
		break;

	case SD_RSP_TYPE_R6:
		command |= SDHC_CDNS_RESP_R6;
		break;

	case SD_RSP_TYPE_R7:
		command |= SDHC_CDNS_RESP_R1;
		break;

	default:
		LOG_DBG("Invalid response type");
		return -EINVAL;
	}

	if (data) {
		command |= SDHC_REG_COMMAND_DPS_MASK;
	}

	*frame = command;

	return 0;
}

/**
 * @brief
 * Check command response is success or failed also clears status register
 */
static int sdhc_cdns_cmd_response(const struct device *dev, struct sdhc_command *cmd)
{
	const struct sdhc_cdns_config *config = dev->config;
	struct sdhc_cdns_data *sd_data = dev->data;
	uint32_t events;
	uint32_t status;
	uint32_t mask;
	int ret;
	k_timeout_t timeout;

	mask = SDHC_CDNS_INT_EINT | SDHC_CDNS_INT_CC;

	if (config->irq_config_fn == NULL) {
		ret = sdhc_cdns_wait_for_events(dev, cmd->timeout_ms, mask);
		if (ret != 0) {
			LOG_ERR("No response from card");
			return ret;
		}

		status = sdhc_cdns_read_intr(dev);
		if ((status & SDHC_CDNS_INT_EINT) != 0U) {
			LOG_ERR("Error response from card");
			sdhc_cdns_clear_intr(dev, SDHC_CDNS_ERR_INT_MASK);
			return -EINVAL;
		}
		sdhc_cdns_clear_intr(dev, SDHC_CDNS_INT_CC);
	} else {
		timeout = K_MSEC(cmd->timeout_ms);

		events = k_event_wait(&sd_data->irq_event, mask, false, timeout);

		if ((events & SDHC_CDNS_INT_EINT) != 0U) {
			LOG_ERR("Error response from card");
			ret = -EINVAL;
		} else if ((events & SDHC_CDNS_INT_CC) != 0U) {
			ret = 0;
		} else {
			LOG_ERR("No response from card");
			ret = -EAGAIN;
		}
	}
	return ret;
}

/**
 * @brief
 * Update response member of command structure which is used by subsystem
 */
static void sdhc_cdns_update_response(const struct device *dev, struct sdhc_command *cmd)
{
	if (cmd->response_type == SD_RSP_TYPE_NONE) {
		return;
	}

	if (cmd->response_type == SD_RSP_TYPE_R2) {
		cmd->response[0] = sdhc_standard_read32(dev, SDHC_REG_RESPONSE_0_1);
		cmd->response[1] = sdhc_standard_read32(dev, SDHC_REG_RESPONSE_2_3);
		cmd->response[2] = sdhc_standard_read32(dev, SDHC_REG_RESPONSE_4_5);
		cmd->response[3] = sdhc_standard_read32(dev, SDHC_REG_RESPONSE_6_7);

		/* CRC is striped from the response performing shifting to update response */
		for (uint8_t i = 3; i != 0; i--) {
			cmd->response[i] <<= SDHC_CDNS_CRC_LEFT_SHIFT;
			cmd->response[i] |= cmd->response[i - 1] >> SDHC_CDNS_CRC_RIGHT_SHIFT;
		}
		cmd->response[0] <<= SDHC_CDNS_CRC_LEFT_SHIFT;
	} else {
		cmd->response[0] = sdhc_standard_read32(dev, SDHC_REG_RESPONSE_0_1);
	}
}

/**
 * @brief
 * Setup and send the command and also check for response
 */
static int sdhc_cdns_cmd(const struct device *dev, struct sdhc_command *cmd, bool data)
{
	const struct sdhc_cdns_config *config = dev->config;
	struct sdhc_cdns_data *sd_data = dev->data;
	uint32_t present_state;
	uint16_t command;
	int ret;

	sdhc_standard_write32(dev, SDHC_REG_ARGUMENT1, cmd->arg);

	sdhc_cdns_clear_intr(dev, SDHC_CDNS_TXFR_INTR_EN_MASK);

	/* Frame command */
	ret = sdhc_cdns_cmd_frame(cmd, data, &command);
	if (ret != 0) {
		return ret;
	}

	present_state = sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE);
	if (((present_state & SDHC_REG_PRESENT_STATE_CIDAT_MASK) != 0U) &&
	    ((command & SDHC_REG_COMMAND_DPS_MASK) != 0U)) {
		LOG_ERR("Card data lines busy");
		return -EBUSY;
	}

	if (config->irq_config_fn != NULL) {
		k_event_clear(&sd_data->irq_event, SDHC_CDNS_TXFR_INTR_EN_MASK);
	}

	/* Transfer Mode occupies bits 15:0 of the word, Command bits 31:16 */
	sdhc_standard_write32(dev, SDHC_REG_TRANSFER_MODE,
			      ((uint32_t)command << SDHC_CDNS_COMMAND_SHIFT) |
				      sd_data->transfermode);

	/* Check for response */
	ret = sdhc_cdns_cmd_response(dev, cmd);
	if (ret != 0) {
		return ret;
	}

	sdhc_cdns_update_response(dev, cmd);

	return 0;
}

/**
 * @brief
 * Check for data transfer completion
 */
static int sdhc_cdns_xfr(const struct device *dev, struct sdhc_data *data)
{
	const struct sdhc_cdns_config *config = dev->config;
	struct sdhc_cdns_data *sd_data = dev->data;
	k_timeout_t timeout;
	uint32_t events;
	uint32_t mask;
	uint32_t status;
	int ret;

	mask = SDHC_CDNS_ERR_INT_MASK | SDHC_CDNS_INT_TC;
	if (config->irq_config_fn == NULL) {
		ret = sdhc_cdns_wait_for_events(dev, data->timeout_ms, mask);
		if (ret != 0) {
			LOG_ERR("Data transfer timeout");
			return ret;
		}

		status = sdhc_cdns_read_intr(dev);
		if ((status & SDHC_CDNS_ERR_INT_MASK) != 0U) {
			sdhc_cdns_clear_intr(dev, SDHC_CDNS_ERR_INT_MASK);
			LOG_ERR("Error at data transfer");
			return -EINVAL;
		}

		sdhc_cdns_clear_intr(dev, SDHC_CDNS_INT_TC);
	} else {
		timeout = K_MSEC(data->timeout_ms);

		events = k_event_wait(&sd_data->irq_event, mask, false, timeout);

		if ((events & SDHC_CDNS_ERR_INT_MASK) != 0U) {
			LOG_ERR("Error at data transfer");
			ret = -EINVAL;
		} else if ((events & SDHC_CDNS_INT_TC) != 0U) {
			ret = 0;
		} else {
			LOG_ERR("Data transfer timeout");
			ret = -EAGAIN;
		}
	}

	return ret;
}

/**
 * @brief
 * Performs data and command transfer and check for transfer complete
 */
static int sdhc_cdns_transfer(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	struct sdhc_cdns_data *sd_data = dev->data;
	struct sdhc_data bounce_data;
	const struct sdhc_data *req;
	uint64_t req_len;
	uint64_t block_chunk;
	size_t cache_line_size;
	bool bounced = false;
	bool read;
	int ret;

	/* Check command line is in use */
	if ((sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE) &
	     SDHC_REG_PRESENT_STATE_CICMD_MASK) != 0U) {
		LOG_ERR("Command lines are busy");
		return -EBUSY;
	}

	if (data == NULL) {
		/* Send command and check for command complete */
		return sdhc_cdns_cmd(dev, cmd, false);
	}

	block_chunk = (uint64_t)data->block_size * data->blocks;
	sdhc_standard_write16(dev, SDHC_REG_BLOCK_SIZE, data->block_size);
	sdhc_standard_write16(dev, SDHC_REG_BLOCK_COUNT, data->blocks);

	/* Keep the caller's request: `data` may be redirected to the bounce buffer */
	req = data;
	req_len = block_chunk;

	read = (sd_data->transfermode & SDHC_REG_TRANSFER_MODE_DTDS_MASK) != 0U;
	cache_line_size = sys_cache_data_line_size_get();

	if (read && cache_line_size != 0U) {
		if (!IS_ALIGNED((mem_addr_t)data->data, cache_line_size)) {
			LOG_ERR("Read DMA buffer must be aligned to d-cache line size: "
				"buf=%p line_size=%u",
				data->data, (uint32_t)cache_line_size);
			return -EINVAL;
		}

		/*
		 * Cache invalidation requires a cache-line-sized range. Use a
		 * dedicated bounce buffer when the transfer length isn't a
		 * multiple of the cache line size.
		 */
		if (!IS_ALIGNED(block_chunk, cache_line_size)) {
			block_chunk = ROUND_UP(block_chunk, cache_line_size);
			if (block_chunk > sizeof(sd_data->read_bounce_buffer) ||
			    !IS_ALIGNED((mem_addr_t)sd_data->read_bounce_buffer, cache_line_size)) {
				LOG_ERR("Read DMA bounce buffer is incompatible: "
					"len=%u line_size=%u bounce_size=%u",
					(uint32_t)block_chunk, (uint32_t)cache_line_size,
					(uint32_t)sizeof(sd_data->read_bounce_buffer));
				return -EINVAL;
			}

			bounce_data = *data;
			bounce_data.data = sd_data->read_bounce_buffer;
			data = &bounce_data;
			bounced = true;
		}
	}

	/*
	 * For read transfers, clean and invalidate the DMA destination before
	 * starting DMA, then invalidate it again before the CPU reads data
	 * written by SDHC. For write transfers, flush CPU-written data
	 * before SDHC reads it.
	 */
	ret = sys_cache_data_flush_range(data->data, block_chunk);
	if (ret == 0 && read) {
		ret = sys_cache_data_invd_range(data->data, block_chunk);
	}
	if (ret != 0) {
		LOG_ERR("DMA buffer cache maintenance failed before transfer: "
			"ret=%d buf=%p len=%u read=%u",
			ret, data->data, (uint32_t)block_chunk, read);
		return ret;
	}

	/* Setup DMA if data is present */
	ret = sdhc_cdns_setup_dma(dev, data);
	if (ret != 0) {
		return ret;
	}

	/* Send command and check for command complete */
	ret = sdhc_cdns_cmd(dev, cmd, true);
	if (ret != 0) {
		return ret;
	}

	/* Check for data transfer complete */
	ret = sdhc_cdns_xfr(dev, data);
	if (ret != 0) {
		return ret;
	}

	if (read) {
		ret = sys_cache_data_invd_range(data->data, block_chunk);
		if (ret != 0) {
			return ret;
		}

		if (bounced) {
			memcpy(req->data, sd_data->read_bounce_buffer, req_len);
		}
	}

	return 0;
}

/**
 * @brief
 * Configure transfer mode and transfer command and data
 */
static int sdhc_cdns_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *data)
{
	struct sdhc_cdns_data *sd_data = dev->data;
	int ret;

	if (sd_data->transfermode == 0U) {
		sd_data->transfermode = SDHC_REG_TRANSFER_MODE_BCE_MASK |
					SDHC_REG_TRANSFER_MODE_DTDS_MASK |
					SDHC_REG_TRANSFER_MODE_DMAE_MASK;
	}

	switch (cmd->opcode) {
	case SD_READ_MULTIPLE_BLOCK:
		sd_data->transfermode |=
			SDHC_REG_TRANSFER_MODE_ACEN_CMD12 | SDHC_REG_TRANSFER_MODE_MSBS_MASK;
		break;

	case SD_WRITE_MULTIPLE_BLOCK:
		sd_data->transfermode |=
			SDHC_REG_TRANSFER_MODE_ACEN_CMD12 | SDHC_REG_TRANSFER_MODE_MSBS_MASK;
		sd_data->transfermode &= ~SDHC_REG_TRANSFER_MODE_DTDS_MASK;
		break;

	case SD_WRITE_SINGLE_BLOCK:
		sd_data->transfermode &= ~SDHC_REG_TRANSFER_MODE_DTDS_MASK;
		break;

	default:
		break;
	}

	ret = sdhc_cdns_transfer(dev, cmd, data);
	if (ret != 0) {
		LOG_ERR("Failed to transfer cmd/data");
	}

	sd_data->transfermode = 0;

	return ret;
}

/**
 * @brief
 * Return the host properties gathered at init
 */
static int sdhc_cdns_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct sdhc_cdns_data *sd_data = dev->data;

	*props = sd_data->standard.props;

	return 0;
}

/**
 * @brief
 * Populate the host properties from devicetree and the capability registers
 */
static void sdhc_cdns_props_init(const struct device *dev)
{
	const struct sdhc_cdns_config *config = dev->config;
	struct sdhc_cdns_data *sd_data = dev->data;
	struct sdhc_host_props *props = &sd_data->standard.props;
	struct sdhc_host_caps *host_caps = &props->host_caps;
	uint32_t current;

	sdhc_common_dt_props_init(props, &config->standard.common);
	sdhc_standard_capabilities_init(dev);

	/* 1.8 V signalling needs the supply of the bus IO lines to provide it */
	if (config->vqmmc != NULL &&
	    !regulator_is_supported_voltage(config->vqmmc, SDHC_CDNS_VQMMC_1_8_UV,
					    SDHC_CDNS_VQMMC_1_8_UV)) {
		host_caps->vol_180_support = false;
	}

	/* The PHY is only configured up to SDR25, so SDR50/SDR104/DDR50 are not offered */
	host_caps->sdr50_support = false;
	host_caps->sdr104_support = false;
	host_caps->ddr50_support = false;

	current = sdhc_standard_read32(dev, SDHC_REG_MAX_CURRENT_CAP);
	props->max_current_330 = (current & SDHC_REG_MAX_CURRENT_CAP_MC33_MASK) >>
				 SDHC_REG_MAX_CURRENT_CAP_MC33_SHIFT;
	props->max_current_300 = (current & SDHC_REG_MAX_CURRENT_CAP_MC30_MASK) >>
				 SDHC_REG_MAX_CURRENT_CAP_MC30_SHIFT;
	props->max_current_180 = (current & SDHC_REG_MAX_CURRENT_CAP_MC18_MASK) >>
				 SDHC_REG_MAX_CURRENT_CAP_MC18_SHIFT;
}

/**
 * @brief
 * Calculate clock value based on the selected speed
 */
static int sdhc_cdns_calc_clock(const struct device *dev, enum sdhc_clock_speed speed,
				uint16_t *clockval)
{
	const struct sdhc_cdns_config *config = dev->config;
	const struct sdhc_cdns_data *sd_data = dev->data;
	uint16_t divcnt, divisor = 0U;
	uint32_t pclk;
	int ret;

	pclk = sd_data->standard.props.host_caps.sd_base_clk;
	if (pclk == 0) {
		ret = clock_control_get_rate(config->clock_dev, config->clock_id, &pclk);
		if (ret != 0) {
			LOG_ERR("Failed to get clock rate");
			return ret;
		}
	} else {
		pclk = MHZ(pclk);
	}

	LOG_DBG("SDHC platform clock: %uHz, requested speed: %uHZ", pclk, speed);

	if (speed >= pclk) {
		divisor = 1U;
	} else {
		for (divcnt = 2U; divcnt <= 0x3FF; divcnt += 2U) {
			if (speed >= (pclk / divcnt)) {
				divisor = divcnt >> 1U;
				break;
			}
		}

		if (divcnt > 0x3FF) {
			LOG_ERR("SDHC platform clock %uHz is too high", pclk);
			return -EINVAL;
		}
	}

	*clockval = ((divisor << SDHC_REG_CLOCK_CTRL_SDCFSL_SHIFT) &
		     SDHC_REG_CLOCK_CTRL_SDCFSL_MASK) |
		    (((divisor >> 8U) << SDHC_REG_CLOCK_CTRL_SDCFSH_SHIFT) &
		     SDHC_REG_CLOCK_CTRL_SDCFSH_MASK);

	return 0;
}

/**
 * @brief
 * Set clock and wait for clock to be stable.
 *
 */
static int sdhc_cdns_set_clock(const struct device *dev, enum sdhc_clock_speed speed)
{
	uint16_t divisor = 0U;
	int ret;

	/* Disable clock and clear the divisor/enable bits */
	sdhc_standard_write16(dev, SDHC_REG_CLOCK_CTRL, 0U);

	if (speed == 0U) {
		return 0;
	}

	ret = sdhc_cdns_calc_clock(dev, speed, &divisor);
	if (ret < 0) {
		return -EINVAL;
	}

	/* Program divisor and enable the internal clock */
	sdhc_standard_write16(dev, SDHC_REG_CLOCK_CTRL, divisor | SDHC_REG_CLOCK_CTRL_ICE_MASK);

	/* Wait max 150ms for internal clock to be stable */
	if (!WAIT_FOR((sdhc_standard_read16(dev, SDHC_REG_CLOCK_CTRL) &
		       SDHC_REG_CLOCK_CTRL_ICS_MASK) != 0U,
		      150U * USEC_PER_MSEC, k_msleep(1))) {
		return -EAGAIN;
	}

	/* Enable div clock */
	sdhc_cdns_update16(dev, SDHC_REG_CLOCK_CTRL, SDHC_REG_CLOCK_CTRL_SDCE_MASK,
			   SDHC_REG_CLOCK_CTRL_SDCE_MASK);

	return 0;
}

/**
 * @brief
 * Set bus width on the controller (Host Control 1: DTW = 4-bit, EDTW = 8-bit)
 */
static int sdhc_cdns_set_buswidth(const struct device *dev, enum sdhc_bus_width width)
{
	const uint8_t mask = SDHC_REG_HOST_CTRL1_EDTW_MASK | SDHC_REG_HOST_CTRL1_DTW_MASK;

	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		sdhc_cdns_update8(dev, SDHC_REG_HOST_CTRL1, mask, 0U);
		break;

	case SDHC_BUS_WIDTH4BIT:
		sdhc_cdns_update8(dev, SDHC_REG_HOST_CTRL1, mask, SDHC_REG_HOST_CTRL1_DTW_MASK);
		break;

	case SDHC_BUS_WIDTH8BIT:
		sdhc_cdns_update8(dev, SDHC_REG_HOST_CTRL1, SDHC_REG_HOST_CTRL1_EDTW_MASK,
				  SDHC_REG_HOST_CTRL1_EDTW_MASK);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief
 * Enable or disable power.
 */
static void sdhc_cdns_set_power(const struct device *dev, enum sdhc_power power)
{
	sdhc_cdns_update8(dev, SDHC_REG_POWER_CTRL, SDHC_REG_POWER_CTRL_BP_MASK,
			  (power == SDHC_POWER_ON) ? SDHC_REG_POWER_CTRL_BP_MASK : 0U);
}

/**
 * @brief
 * Set voltage level and signalling voltage.
 */
static int sdhc_cdns_set_voltage(const struct device *dev, enum sd_voltage voltage)
{
	const struct sdhc_cdns_config *config = dev->config;
	uint32_t settle_ms = SDHC_CDNS_SIGNAL_SETTLE_TIME_MS;
	uint16_t signal_1v8 = 0U;
	int32_t supply_uv;
	int ret;

	switch (voltage) {
	case SD_VOL_3_3_V:
		sdhc_cdns_update8(dev, SDHC_REG_POWER_CTRL, SDHC_REG_POWER_CTRL_BVS_MASK,
				  SDHC_REG_POWER_CTRL_BVS_3_3V);
		supply_uv = SDHC_CDNS_VQMMC_3_3_UV;
		break;

	case SD_VOL_3_0_V:
		sdhc_cdns_update8(dev, SDHC_REG_POWER_CTRL, SDHC_REG_POWER_CTRL_BVS_MASK,
				  SDHC_REG_POWER_CTRL_BVS_3_0V);
		supply_uv = SDHC_CDNS_VQMMC_3_0_UV;
		break;

	case SD_VOL_1_8_V:
		/* Only the signalling level changes, the card supply stays as it is */
		signal_1v8 = SDHC_REG_HOST_CTRL2_V18SE_MASK;
		supply_uv = SDHC_CDNS_VQMMC_1_8_UV;
		break;
	default:
		return -EINVAL;
	}

	if (config->vqmmc != NULL) {
		ret = regulator_set_voltage(config->vqmmc, supply_uv, supply_uv);
		if (ret != 0) {
			LOG_ERR("Failed to set the bus IO supply to %d uV: %d", supply_uv, ret);
			return ret;
		}

		if (voltage == SD_VOL_1_8_V) {
			settle_ms = SDHC_CDNS_1_8_REG_STABLE_TIME_MS;
		}
	}

	sdhc_cdns_update16(dev, SDHC_REG_HOST_CTRL2, SDHC_REG_HOST_CTRL2_V18SE_MASK, signal_1v8);

	/* Wait for voltage switch to settle */
	k_msleep(settle_ms);

	return 0;
}

static int sdhc_cdns_set_timing(const struct device *dev, enum sdhc_timing_mode timing)
{
	struct sdhc_cdns_data *sd_data = dev->data;
	uint16_t mode = 0;
	int ret;

	switch (timing) {
	case SDHC_TIMING_LEGACY:
		sdhc_cdns_update8(dev, SDHC_REG_HOST_CTRL1, SDHC_REG_HOST_CTRL1_HSE_MASK, 0U);
		break;

	case SDHC_TIMING_HS:
		sdhc_cdns_update8(dev, SDHC_REG_HOST_CTRL1, SDHC_REG_HOST_CTRL1_HSE_MASK,
				  SDHC_REG_HOST_CTRL1_HSE_MASK);
		break;

	case SDHC_TIMING_SDR12:
		mode = SDHC_REG_HOST_CTRL2_UMS_SDR12;
		break;

	case SDHC_TIMING_SDR25:
		mode = SDHC_REG_HOST_CTRL2_UMS_SDR25;
		break;

	default:
		return -EINVAL;
	}

	/* Select one of UHS mode */
	if (timing >= SDHC_TIMING_SDR12) {
		sdhc_cdns_update16(dev, SDHC_REG_HOST_CTRL2, SDHC_REG_HOST_CTRL2_UMS_MASK, mode);
		sdhc_cdns_update8(dev, SDHC_REG_HOST_CTRL1, SDHC_REG_HOST_CTRL1_HSE_MASK,
				  SDHC_REG_HOST_CTRL1_HSE_MASK);
	}

	sd_data->timing_mode = timing;
	ret = sdhc_cdns_quirk_phy_config(dev);
	if (ret != 0) {
		LOG_ERR("Failed to configure phy %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief
 * Set voltage, power, clock, timing, bus width on host controller
 */
static int sdhc_cdns_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_cdns_data *sd_data = dev->data;
	struct sdhc_io *host_io = (struct sdhc_io *)&sd_data->host_io;
	int ret;

	LOG_DBG("SDHC I/O: bus width %d, clock %dHz, card power %s, voltage %s",
		ios->bus_width, ios->clock,
		ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V");

	/* Check given clock is valid */
	if (ios->clock != 0 &&
	    (ios->clock > sd_data->standard.props.f_max ||
	     ios->clock < sd_data->standard.props.f_min)) {
		LOG_ERR("Invalid clock value");
		return -EINVAL;
	}

	/* Set power on or off */
	if (ios->power_mode != host_io->power_mode) {
		sdhc_cdns_set_power(dev, ios->power_mode);
		host_io->power_mode = ios->power_mode;
	}

	/* Set voltage level */
	if (ios->signal_voltage != host_io->signal_voltage) {
		ret = sdhc_cdns_set_voltage(dev, ios->signal_voltage);
		if (ret != 0) {
			LOG_ERR("Failed to set voltage level");
			return ret;
		}
		host_io->signal_voltage = ios->signal_voltage;
	}

	/* Set speed mode */
	if (ios->timing != host_io->timing) {
		ret = sdhc_cdns_set_timing(dev, ios->timing);
		if (ret != 0) {
			LOG_ERR("Failed to set speed mode");
			return ret;
		}
		host_io->timing = ios->timing;
	}

	/* Set clock */
	if (ios->clock != host_io->clock) {
		ret = sdhc_cdns_set_clock(dev, ios->clock);
		if (ret != 0) {
			LOG_ERR("Failed to set clock");
			return ret;
		}
		host_io->clock = ios->clock;
	}

	/* Set bus width */
	if (ios->bus_width != host_io->bus_width) {
		ret = sdhc_cdns_set_buswidth(dev, ios->bus_width);
		if (ret != 0) {
			LOG_ERR("Failed to set bus width");
			return ret;
		}
		host_io->bus_width = ios->bus_width;
	}

	return 0;
}

/**
 * @brief
 * Perform reset and enable status registers
 */
static int sdhc_cdns_host_reset(const struct device *dev)
{
	const struct sdhc_cdns_config *config = dev->config;
	uint16_t host_ctrl2 = SDHC_CDNS_HOST_CTRL2_HV4E;

	/* Perform software reset for all; this also clears the clock config */
	sdhc_standard_write8(dev, SDHC_REG_SW_RESET, SDHC_REG_SW_RESET_SRA_MASK);
	/* Wait max 100ms for software reset to complete */
	if (!WAIT_FOR((sdhc_standard_read8(dev, SDHC_REG_SW_RESET) &
		       SDHC_REG_SW_RESET_SRA_MASK) == 0U,
		      100U * USEC_PER_MSEC, k_msleep(1))) {
		LOG_ERR("Device is busy");
		return -EBUSY;
	}

	/* Latch only the status bits the driver handles; signal them when an IRQ is wired */
	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT_EN, SDHC_CDNS_TXFR_INTR_EN_MASK);
	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_SIG_EN,
			      (config->irq_config_fn == NULL) ? 0U : SDHC_CDNS_TXFR_INTR_EN_MASK);

	/* Data line timeout interval */
	sdhc_standard_write8(dev, SDHC_REG_TIMEOUT_CTRL,
			     SDHC_CDNS_TIMEOUT_CTRL_DTCV & SDHC_REG_TIMEOUT_CTRL_DTCV_MASK);

	if (IS_ENABLED(CONFIG_64BIT)) {
		host_ctrl2 |= SDHC_CDNS_HOST_CTRL2_A64B;
	}
	sdhc_cdns_update16(dev, SDHC_REG_HOST_CTRL2,
			   SDHC_CDNS_HOST_CTRL2_HV4E | SDHC_CDNS_HOST_CTRL2_A64B, host_ctrl2);

	/* Select DMA mode */
	sdhc_cdns_update8(dev, SDHC_REG_HOST_CTRL1, SDHC_REG_HOST_CTRL1_DMASEL_MASK,
			  SDHC_REG_HOST_CTRL1_DMASEL_ADMA2_32);

	sdhc_standard_write16(dev, SDHC_REG_BLOCK_SIZE, SDHC_CDNS_BLOCK_SIZE_512);

	sdhc_cdns_clear_intr(dev, SDHC_CDNS_TXFR_INTR_EN_MASK);

	return 0;
}

/**
 * @brief
 * Check for card busy
 */
static int sdhc_cdns_card_busy(const struct device *dev)
{
	/* Wait max 2ms for card to send next command */
	if (!WAIT_FOR((sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE) &
		       SDHC_REG_PRESENT_STATE_DAT0_MASK) == 0U,
		      2U * USEC_PER_MSEC, k_msleep(1))) {
		return 0;
	}

	return 1;
}

/**
 * @brief
 * Toggle every reset line of the controller, in devicetree order
 */
static int sdhc_cdns_reset_lines(const struct device *dev)
{
	const struct sdhc_cdns_config *config = dev->config;
	int ret;

	for (size_t i = 0; i < config->num_resets; i++) {
		if (!device_is_ready(config->resets[i].dev)) {
			LOG_ERR("Reset controller %s not ready", config->resets[i].dev->name);
			return -ENODEV;
		}

		ret = reset_line_toggle_dt(&config->resets[i]);
		if (ret != 0) {
			LOG_ERR("Reset line %u failed: %d", config->resets[i].id, ret);
			return ret;
		}
	}

	return 0;
}

/**
 * @brief
 * Perform early system init for SDHC
 */
static int sdhc_cdns_init(const struct device *dev)
{
	const struct sdhc_cdns_config *config = dev->config;
	struct sdhc_cdns_data *sd_data = dev->data;
	int ret;

	/*
	 * sdhc_standard_host_init() is not used: it reads the capabilities
	 * before the reset lines are released and maps with K_MEM_DIRECT_MAP,
	 * which needs CONFIG_KERNEL_DIRECT_MAP.
	 */
	DEVICE_MMIO_NAMED_MAP(dev, sdhc_base, K_MEM_CACHE_NONE);

	if (config->has_host) {
		device_map(&sd_data->host_base, config->host_phys, config->host_size,
			   K_MEM_CACHE_NONE);
	}

	if (config->vqmmc != NULL) {
		if (!device_is_ready(config->vqmmc)) {
			LOG_ERR("Bus IO supply %s not ready", config->vqmmc->name);
			return -ENODEV;
		}

		ret = regulator_enable(config->vqmmc);
		if (ret != 0) {
			LOG_ERR("Failed to enable the bus IO supply: %d", ret);
			return ret;
		}
	}

	ret = sdhc_cdns_reset_lines(dev);
	if (ret != 0) {
		return ret;
	}

	if (sdhc_cdns_quirk_phy_init(dev)) {
		LOG_ERR("Failed to initialize phy");
		return -EIO;
	}

	if (config->irq_config_fn != NULL) {
		k_event_init(&sd_data->irq_event);
		config->irq_config_fn(dev);
	}

	ret = sdhc_cdns_host_reset(dev);
	if (ret != 0) {
		return ret;
	}

	sdhc_cdns_props_init(dev);

	return 0;
}

static DEVICE_API(sdhc, sdhc_cdns_api) = {
	.reset = sdhc_cdns_host_reset,
	.request = sdhc_cdns_request,
	.set_io = sdhc_cdns_set_io,
	.get_card_present = sdhc_cdns_card_detect,
	.card_busy = sdhc_cdns_card_busy,
	.get_host_props = sdhc_cdns_host_props,
};

/* clang-format off */
#define SDHC_CDNS_INTR_CONFIG(n)                                                                   \
	static void sdhc_cdns_irq_handler##n(const struct device *dev)                             \
	{                                                                                          \
		struct sdhc_cdns_data *sd_data = dev->data;                                        \
		uint32_t status = sdhc_cdns_read_intr(dev);                                        \
		if ((status & SDHC_CDNS_INT_CC) != 0U) {                                           \
			sdhc_cdns_clear_intr(dev, SDHC_CDNS_INT_CC);                               \
			k_event_post(&sd_data->irq_event, SDHC_CDNS_INT_CC);                       \
		}                                                                                  \
		if ((status & SDHC_CDNS_INT_TC) != 0U) {                                           \
			sdhc_cdns_clear_intr(dev, SDHC_CDNS_INT_TC);                               \
			k_event_post(&sd_data->irq_event, SDHC_CDNS_INT_TC);                       \
		}                                                                                  \
		if ((status & SDHC_CDNS_ERR_INT_MASK) != 0U) {                                     \
			sdhc_cdns_clear_intr(dev, SDHC_CDNS_ERR_INT_MASK);                         \
			k_event_post(&sd_data->irq_event, SDHC_CDNS_ERR_INT_MASK);                 \
		}                                                                                  \
	}                                                                                          \
	static void sdhc_cdns_config_intr##n(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), sdhc_cdns_irq_handler##n,   \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define SDHC_CDNS_INTR_FUNC_REG(n)                                                                 \
	.irq_config_fn = sdhc_cdns_config_intr##n,

#define SDHC_CDNS_INTR_FUNC_REG_NULL                                                               \
	.irq_config_fn = NULL,

#define SDHC_CDNS_INTR_CONFIG_API(n)                                                               \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts),                                          \
		(SDHC_CDNS_INTR_CONFIG(n)), ())

#define SDHC_CDNS_INTR_FUNC_REG_API(n)                                                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts),                                          \
		(SDHC_CDNS_INTR_FUNC_REG(n)), (SDHC_CDNS_INTR_FUNC_REG_NULL))

#define SDHC_CDNS_HOST_REG_INIT(n)                                                                 \
	IF_ENABLED(DT_INST_REG_HAS_NAME(n, host),                                                  \
		   (.host_phys = DT_INST_REG_ADDR_BY_NAME(n, host),                                \
		    .host_size = DT_INST_REG_SIZE_BY_NAME(n, host),))

#define SDHC_CDNS_CLOCK_INIT(n)                                                                    \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                        \
	IF_ENABLED(DT_INST_PHA_HAS_CELL_AT_IDX(n, clocks, 0, id),                                  \
		(.clock_id = DT_INST_CLOCKS_CELL(n, id),))

#define SDHC_CDNS_RESET_SPEC(node_id, prop, idx) RESET_DT_SPEC_GET_BY_IDX(node_id, idx)

#define SDHC_CDNS_RESET_DEFINE(n)                                                                  \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, resets),                                               \
		   (static const struct reset_dt_spec sdhc_cdns_resets##n[] = {                    \
			    DT_INST_FOREACH_PROP_ELEM_SEP(n, resets, SDHC_CDNS_RESET_SPEC, (,))};))

#define SDHC_CDNS_RESET_INIT(n)                                                                    \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, resets),                                               \
		   (.resets = sdhc_cdns_resets##n, .num_resets = ARRAY_SIZE(sdhc_cdns_resets##n),))

#define SDHC_CDNS_INIT(n)                                                                          \
	SDHC_CDNS_INTR_CONFIG_API(n)                                                               \
	SDHC_CDNS_RESET_DEFINE(n)                                                                  \
	static struct sdhc_cdns_data sdhc_cdns_data##n;                                            \
	const static struct sdhc_cdns_config sdhc_cdns_config_##n = {                              \
		.standard = SDHC_STANDARD_COMMON_CONFIG_DT_INST_INIT_BY_NAME(n, slot, NULL),       \
		SDHC_CDNS_HOST_REG_INIT(n)                                                         \
		SDHC_CDNS_CLOCK_INIT(n)                                                            \
		SDHC_CDNS_RESET_INIT(n)                                                            \
		SDHC_CDNS_INTR_FUNC_REG_API(n).broken_cd = DT_INST_PROP_OR(n, broken_cd, 0),       \
		.quirks = SDHC_CDNS_QUIRK_GET(n),                                                  \
		.quirk_config = SDHC_CDNS_QUIRK_CONFIG_GET(n),                                     \
		.vqmmc = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(n, vqmmc_supply)),                  \
		.has_host = DT_INST_PROP(n, has_host),                                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, sdhc_cdns_init, NULL, &sdhc_cdns_data##n, &sdhc_cdns_config_##n,  \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &sdhc_cdns_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_CDNS_INIT)
/* clang-format on */
