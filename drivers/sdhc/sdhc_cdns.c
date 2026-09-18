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
LOG_MODULE_REGISTER(cdns_sdhc, CONFIG_SD_LOG_LEVEL);

#define SDHC_CDNS_SLOT_TYPE(dev)                                                                   \
	((((struct sdhc_cdns_data *)(dev)->data)->props.host_caps.slot_type != 0)                  \
		 ? SDHC_CDNS_EMMC_SLOT                                                             \
		 : SDHC_CDNS_SD_SLOT)

#define SDHC_CDNS_GET_HOST_PROP_BIT(cap, mask) ((uint8_t)(((cap) & (mask)) != 0U))

/* Sentinel for an invalid command frame; distinct name from any 16-bit
 * legacy sentinel in sdhc_cdns.h since the command frame is now 32-bit.
 */
#define SDHC_CDNS_CMD_FRAME_INVALID UINT32_MAX

#define SDHC_CDNS_DESC_MAX_LEN 65536U

/**
 * @brief
 * Read a 32-bit register at the given SDHC_CDNS_* offset.
 */
static inline uint32_t sdhc_cdns_read(const struct device *dev, uint32_t offset)
{
	return sys_read32(DEVICE_MMIO_NAMED_GET(dev, slot) + offset);
}

/**
 * @brief
 * Write a 32-bit register at the given SDHC_CDNS_* offset.
 */
static inline void sdhc_cdns_write(const struct device *dev, uint32_t offset, uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_NAMED_GET(dev, slot) + offset);
}

/**
 * @brief
 * Set (OR in) bits in a 32-bit register at the given SDHC_CDNS_* offset.
 */
static inline void sdhc_cdns_set_bits(const struct device *dev, uint32_t offset, uint32_t bits)
{
	sys_set_bits(DEVICE_MMIO_NAMED_GET(dev, slot) + offset, bits);
}

/**
 * @brief
 * Clear bits in a 32-bit register at the given SDHC_CDNS_* offset.
 */
static inline void sdhc_cdns_clear_bits(const struct device *dev, uint32_t offset, uint32_t bits)
{
	sys_clear_bits(DEVICE_MMIO_NAMED_GET(dev, slot) + offset, bits);
}

/**
 * @brief
 * Clear the controller status register (write-1-to-clear, normal + error bits)
 */
static inline void sdhc_cdns_clear_intr(const struct device *dev)
{
	sdhc_cdns_write(dev, SDHC_CDNS_INT_STATUS,
			SDHC_CDNS_NORMAL_INT_MASK | SDHC_CDNS_ERR_INT_MASK);
}

/**
 * @brief
 * Polled wait for a 32-bit register (by offset) to reach `value` under `mask`.
 */
static int sdhc_cdns_wait_reg_mask(const struct device *dev, uint32_t offset, int32_t timeout_ms,
				   uint32_t mask, uint32_t value)
{
	for (uint32_t retry = 0; retry < (uint32_t)timeout_ms; retry++) {
		if ((sdhc_cdns_read(dev, offset) & mask) == value) {
			return 0;
		}
		k_msleep(1);
	}

	return -EAGAIN;
}

/**
 * @brief
 * Polled wait for any one of the given events in a 32-bit register (by offset).
 */
static int sdhc_cdns_wait_for_events(const struct device *dev, uint32_t offset, int32_t timeout_ms,
				     uint32_t events)
{
	for (uint32_t retry = 0; retry < (uint32_t)timeout_ms; retry++) {
		if ((sdhc_cdns_read(dev, offset) & events) != 0U) {
			return 0;
		}
		k_msleep(1);
	}

	return -EAGAIN;
}

/**
 * @brief
 * Check card is detected by host
 */
static int sdhc_cdns_card_detect(const struct device *dev)
{
	const struct sdhc_cdns_config *config = dev->config;
	uint32_t present_state = sdhc_cdns_read(dev, SDHC_CDNS_PRESENT_STATE);

	if ((present_state & SDHC_CDNS_PRESENT_STATE_CI) != 0U) {
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
	uint64_t desc_addr;
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

	desc_addr = (uint64_t)(mem_addr_t)(&adma2_desc[0]);
	sdhc_cdns_write(dev, SDHC_CDNS_ADMA_SYS_ADDR1, (uint32_t)(desc_addr & UINT32_MAX));
	sdhc_cdns_write(dev, SDHC_CDNS_ADMA_SYS_ADDR2, (uint32_t)(desc_addr >> 32));

	return ret;
}

/**
 * @brief
 * Frame the command into the 32-bit SDHC_CDNS_XFER_MODE upper half (Command
 * register bits). Response-type -> Response Type Select / index / CRC
 * check-enable encoding follows the standard SDHC_CDNS Simplified Spec table.
 */
static uint32_t sdhc_cdns_cmd_frame(struct sdhc_command *cmd, bool data, uint8_t slottype)
{
	uint32_t command =
		(cmd->opcode << SDHC_CDNS_XFER_MODE_CIDX_POS) & SDHC_CDNS_XFER_MODE_CIDX_MASK;

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
		/* As per spec, EMMC does not support R7 */
		if (slottype == SDHC_CDNS_EMMC_SLOT) {
			return SDHC_CDNS_CMD_FRAME_INVALID;
		}
		command |= SDHC_CDNS_RESP_R1;
		break;

	default:
		LOG_DBG("Invalid response type");
		return SDHC_CDNS_CMD_FRAME_INVALID;
	}

	/* EMMC does not support APP command */
	if (cmd->opcode == SD_APP_CMD && slottype == SDHC_CDNS_EMMC_SLOT) {
		LOG_DBG("Invalid response type");
		return SDHC_CDNS_CMD_FRAME_INVALID;
	}

	if (data) {
		command |= SDHC_CDNS_XFER_MODE_DPS;
	}

	return command;
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

	mask = SDHC_CDNS_INT_STATUS_EINT | SDHC_CDNS_INT_STATUS_CC;
	if (cmd->opcode == SD_SEND_TUNING_BLOCK || cmd->opcode == MMC_SEND_TUNING_BLOCK) {
		mask |= SDHC_CDNS_INT_STATUS_BRR;
	}

	if (config->irq_config_fn == NULL) {
		ret = sdhc_cdns_wait_for_events(dev, SDHC_CDNS_INT_STATUS, cmd->timeout_ms, mask);
		if (ret != 0) {
			LOG_ERR("No response from card");
			return ret;
		}

		status = sdhc_cdns_read(dev, SDHC_CDNS_INT_STATUS);
		if ((status & SDHC_CDNS_INT_STATUS_EINT) != 0U) {
			LOG_ERR("Error response from card");
			sdhc_cdns_write(dev, SDHC_CDNS_INT_STATUS, SDHC_CDNS_ERR_INT_MASK);
			return -EINVAL;
		}
		sdhc_cdns_write(dev, SDHC_CDNS_INT_STATUS, SDHC_CDNS_INT_STATUS_CC);
	} else {
		timeout = K_MSEC(cmd->timeout_ms);

		events = k_event_wait(&sd_data->irq_event, mask, false, timeout);

		if ((events & SDHC_CDNS_INT_STATUS_EINT) != 0U) {
			LOG_ERR("Error response from card");
			ret = -EINVAL;
		} else if ((events & (SDHC_CDNS_INT_STATUS_CC | SDHC_CDNS_INT_STATUS_BRR)) != 0U) {
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
		cmd->response[0] = sdhc_cdns_read(dev, SDHC_CDNS_RESPONSE0);
		cmd->response[1] = sdhc_cdns_read(dev, SDHC_CDNS_RESPONSE1);
		cmd->response[2] = sdhc_cdns_read(dev, SDHC_CDNS_RESPONSE2);
		cmd->response[3] = sdhc_cdns_read(dev, SDHC_CDNS_RESPONSE3);

		/* CRC is striped from the response performing shifting to update response */
		for (uint8_t i = 3; i != 0; i--) {
			cmd->response[i] <<= SDHC_CDNS_CRC_LEFT_SHIFT;
			cmd->response[i] |= cmd->response[i - 1] >> SDHC_CDNS_CRC_RIGHT_SHIFT;
		}
		cmd->response[0] <<= SDHC_CDNS_CRC_LEFT_SHIFT;
	} else {
		cmd->response[0] = sdhc_cdns_read(dev, SDHC_CDNS_RESPONSE0);
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
	uint8_t slottype = SDHC_CDNS_SLOT_TYPE(dev);
	uint32_t command;
	int ret;

	sdhc_cdns_write(dev, SDHC_CDNS_ARGUMENT, cmd->arg);

	sdhc_cdns_clear_intr(dev);

	/* Frame command */
	command = sdhc_cdns_cmd_frame(cmd, data, slottype);
	if (command == SDHC_CDNS_CMD_FRAME_INVALID) {
		return -EINVAL;
	}

	if ((cmd->opcode != SD_SEND_TUNING_BLOCK) && (cmd->opcode != MMC_SEND_TUNING_BLOCK)) {
		uint32_t present_state = sdhc_cdns_read(dev, SDHC_CDNS_PRESENT_STATE);

		if (((present_state & SDHC_CDNS_PRESENT_STATE_CIDAT) != 0U) &&
		    ((command & SDHC_CDNS_XFER_MODE_DPS) != 0U)) {
			LOG_ERR("Card data lines busy");
			return -EBUSY;
		}
	}

	if (config->irq_config_fn != NULL) {
		k_event_clear(&sd_data->irq_event, SDHC_CDNS_TXFR_INTR_EN_MASK);
	}

	/* SRS03: Transfer Mode occupies bits 15:0, Command occupies bits 31:16 */
	sdhc_cdns_write(dev, SDHC_CDNS_XFER_MODE, command | sd_data->transfermode);

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

	mask = SDHC_CDNS_ERR_INT_MASK | SDHC_CDNS_INT_STATUS_TC;
	if (config->irq_config_fn == NULL) {
		ret = sdhc_cdns_wait_for_events(dev, SDHC_CDNS_INT_STATUS, data->timeout_ms, mask);
		if (ret != 0) {
			LOG_ERR("Data transfer timeout");
			return ret;
		}

		status = sdhc_cdns_read(dev, SDHC_CDNS_INT_STATUS);
		if ((status & SDHC_CDNS_ERR_INT_MASK) != 0U) {
			sdhc_cdns_write(dev, SDHC_CDNS_INT_STATUS, SDHC_CDNS_ERR_INT_MASK);
			LOG_ERR("Error at data transfer");
			return -EINVAL;
		}

		sdhc_cdns_write(dev, SDHC_CDNS_INT_STATUS, SDHC_CDNS_INT_STATUS_TC);
	} else {
		timeout = K_MSEC(data->timeout_ms);

		events = k_event_wait(&sd_data->irq_event, mask, false, timeout);

		if ((events & SDHC_CDNS_ERR_INT_MASK) != 0U) {
			LOG_ERR("Error at data transfer");
			ret = -EINVAL;
		} else if ((events & SDHC_CDNS_INT_STATUS_TC) != 0U) {
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
	uint32_t block_reg;
	size_t cache_line_size;
	bool bounced = false;
	bool read;
	int ret;

	/* Check command line is in use */
	if ((sdhc_cdns_read(dev, SDHC_CDNS_PRESENT_STATE) & SDHC_CDNS_PRESENT_STATE_CICMD) != 0U) {
		LOG_ERR("Command lines are busy");
		return -EBUSY;
	}

	if (data == NULL) {
		/* Send command and check for command complete */
		return sdhc_cdns_cmd(dev, cmd, false);
	}

	block_chunk = (uint64_t)data->block_size * data->blocks;
	block_reg =
		((data->blocks << SDHC_CDNS_BLOCK_SIZE_BCCT_POS) & SDHC_CDNS_BLOCK_SIZE_BCCT_MASK) |
		data->block_size;
	sdhc_cdns_write(dev, SDHC_CDNS_BLOCK_SIZE, block_reg);

	/* Keep the caller's request: `data` may be redirected to the bounce buffer */
	req = data;
	req_len = block_chunk;

	read = (sd_data->transfermode & SDHC_CDNS_XFER_MODE_DTDS) != 0U;
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
		sd_data->transfermode = SDHC_CDNS_XFER_MODE_BCE | SDHC_CDNS_XFER_MODE_DTDS |
					SDHC_CDNS_XFER_MODE_DMAE;
	}

	switch (cmd->opcode) {
	case SD_READ_MULTIPLE_BLOCK:
		sd_data->transfermode |= SDHC_CDNS_XFER_MODE_CMD12_EN | SDHC_CDNS_XFER_MODE_MSBS;
		break;

	case SD_WRITE_MULTIPLE_BLOCK:
		sd_data->transfermode |= SDHC_CDNS_XFER_MODE_CMD12_EN | SDHC_CDNS_XFER_MODE_MSBS;
		sd_data->transfermode &= ~SDHC_CDNS_XFER_MODE_DTDS;
		break;

	case SD_WRITE_SINGLE_BLOCK:
		sd_data->transfermode &= ~SDHC_CDNS_XFER_MODE_DTDS;
		break;

	case SDIO_RW_EXTENDED:
		if (IS_BIT_SET(cmd->arg, SDIO_CMD_ARG_RW_SHIFT)) {
			sd_data->transfermode &= ~SDHC_CDNS_XFER_MODE_DTDS;
		}
		if (data->blocks > 1) {
			sd_data->transfermode |= SDHC_CDNS_XFER_MODE_MSBS;
		}
		break;

	case SDIO_RW_DIRECT:
		if (IS_BIT_SET(cmd->arg, SDIO_CMD_ARG_RW_SHIFT)) {
			sd_data->transfermode &= ~SDHC_CDNS_XFER_MODE_DTDS;
		}
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
 * Populate sdhc_host_props structure with all sd host controller property
 */
static int sdhc_cdns_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct sdhc_cdns_config *config = dev->config;
	struct sdhc_cdns_data *sd_data = dev->data;
	struct sdhc_host_caps *host_caps = &props->host_caps;
	uint32_t caps1 = sdhc_cdns_read(dev, SDHC_CDNS_CAPS1);
	uint32_t caps2 = sdhc_cdns_read(dev, SDHC_CDNS_CAPS2);
	uint32_t current1 = sdhc_cdns_read(dev, SDHC_CDNS_MAX_CURRENT1);

	memset(props, 0, sizeof(struct sdhc_host_props));
	props->f_max = SD_CLOCK_208MHZ;
	props->f_min = SDMMC_CLOCK_400KHZ;
	props->power_delay = config->powerdelay;

	/* Single-bit capability flags */
	host_caps->vol_180_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps1, SDHC_CDNS_CAPS1_VS18);
	host_caps->vol_300_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps1, SDHC_CDNS_CAPS1_VS30);
	host_caps->vol_330_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps1, SDHC_CDNS_CAPS1_VS33);
	host_caps->sdma_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps1, SDHC_CDNS_CAPS1_DMAS);
	host_caps->high_spd_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps1, SDHC_CDNS_CAPS1_HSS);
	host_caps->adma_2_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps1, SDHC_CDNS_CAPS1_ADMA2S);
	host_caps->bus_8_bit_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps1, SDHC_CDNS_CAPS1_EDS8);
	host_caps->ddr50_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps2, SDHC_CDNS_CAPS2_DDR50);
	host_caps->sdr104_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps2, SDHC_CDNS_CAPS2_SDR104);
	host_caps->sdr50_support = SDHC_CDNS_GET_HOST_PROP_BIT(caps2, SDHC_CDNS_CAPS2_SDR50);

	/* Multi-bit masked fields — use MASK/POS, not SDHC_CDNS_GET_HOST_PROP_BIT */
	props->max_current_330 = (uint8_t)((current1 & SDHC_CDNS_MAX_CURRENT1_MC33_MASK) >>
					   SDHC_CDNS_MAX_CURRENT1_MC33_POS);
	props->max_current_300 = (uint8_t)((current1 & SDHC_CDNS_MAX_CURRENT1_MC30_MASK) >>
					   SDHC_CDNS_MAX_CURRENT1_MC30_POS);
	props->max_current_180 = (uint8_t)((current1 & SDHC_CDNS_MAX_CURRENT1_MC18_MASK) >>
					   SDHC_CDNS_MAX_CURRENT1_MC18_POS);
	host_caps->max_blk_len =
		(uint8_t)((caps1 & SDHC_CDNS_CAPS1_MBL_MASK) >> SDHC_CDNS_CAPS1_MBL_POS);
	host_caps->slot_type =
		(uint8_t)((caps1 & SDHC_CDNS_CAPS1_SLT_MASK) >> SDHC_CDNS_CAPS1_SLT_POS);

	props->hs200_support = config->mmc_hs200_1_8v;
	props->bus_4_bit_support = true;

	sd_data->props = *props;

	return 0;
}


/**
 * @brief
 * Calculate clock value based on the selected speed
 */
static int sdhc_cdns_calc_clock(const struct device *dev, enum sdhc_clock_speed speed,
				uint32_t *clockval)
{
	const struct sdhc_cdns_config *config = dev->config;
	uint16_t divcnt, divisor = 0U;
	uint32_t pclk;
	int ret;

	pclk = (sdhc_cdns_read(dev, SDHC_CDNS_CAPS1) & SDHC_CDNS_CAPS1_BCSDCLK_MASK) >>
	       SDHC_CDNS_CAPS1_BCSDCLK_POS;
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

	*clockval |= (divisor & UINT8_MAX) << SDHC_CDNS_CLOCK_CTRL_SDCFSL_POS;
	*clockval |= ((divisor >> SDHC_CDNS_CLOCK_CTRL_SDCFSL_POS) & 3U)
		     << SDHC_CDNS_CLOCK_CTRL_SDCFSH_POS;

	return 0;
}

/**
 * @brief
 * Set clock and wait for clock to be stable.
 *
 */
static int sdhc_cdns_set_clock(const struct device *dev, enum sdhc_clock_speed speed)
{
	uint32_t divisor = 0U;
	int ret;

	/* Disable clock and clear the divisor/enable bits, keep DTCV */
	sdhc_cdns_clear_bits(dev, SDHC_CDNS_CLOCK_CTRL, (uint32_t)~SDHC_CDNS_CLOCK_CTRL_DTCV_MASK);

	if (speed == 0U) {
		return 0;
	}

	/* Calculate clock (already bit-positioned for the low 16 bits) */
	ret = sdhc_cdns_calc_clock(dev, speed, &divisor);
	if (ret < 0) {
		return -EINVAL;
	}

	/* Program divisor and enable the internal clock */
	sdhc_cdns_set_bits(dev, SDHC_CDNS_CLOCK_CTRL, divisor | SDHC_CDNS_CLOCK_CTRL_ICE);

	/* Wait max 150ms for internal clock to be stable */
	ret = sdhc_cdns_wait_reg_mask(dev, SDHC_CDNS_CLOCK_CTRL, 150, SDHC_CDNS_CLOCK_CTRL_ICS,
				      SDHC_CDNS_CLOCK_CTRL_ICS);
	if (ret != 0) {
		return ret;
	}

	/* Enable div clock */
	sdhc_cdns_set_bits(dev, SDHC_CDNS_CLOCK_CTRL, SDHC_CDNS_CLOCK_CTRL_SDCE);

	return ret;
}

/**
 * @brief
 * Set bus width on the controller (SDHC_CDNS_HOST_CTRL: DTW = 4-bit, EDTW = 8-bit)
 */
static int sdhc_cdns_set_buswidth(const struct device *dev, enum sdhc_bus_width width)
{
	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_HOST_CTRL,
				     SDHC_CDNS_HOST_CTRL_EDTW | SDHC_CDNS_HOST_CTRL_DTW);
		break;

	case SDHC_BUS_WIDTH4BIT:
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_EDTW);
		sdhc_cdns_set_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_DTW);
		break;

	case SDHC_BUS_WIDTH8BIT:
		sdhc_cdns_set_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_EDTW);
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
	if (power == SDHC_POWER_ON) {
		sdhc_cdns_set_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_BP);
	} else {
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_BP);
	}
}

/**
 * @brief
 * Set voltage level and signalling voltage.
 *
 * The 1.8V signalling enable bit maps to SDHC_CDNS_HOST_CTRL2_V18SE in the
 * combined host-control-2 register.
 */
static int sdhc_cdns_set_voltage(const struct device *dev, enum sd_voltage voltage)
{
	switch (voltage) {
	case SD_VOL_3_3_V:
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_BVS_MASK);
		sdhc_cdns_set_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_VOL_3_3_V_SELECT);
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_AUTO_CMD_HOST_CTRL2,
				     SDHC_CDNS_HOST_CTRL2_V18SE);
		break;

	case SD_VOL_3_0_V:
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_BVS_MASK);
		sdhc_cdns_set_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_VOL_3_0_V_SELECT);
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_AUTO_CMD_HOST_CTRL2,
				     SDHC_CDNS_HOST_CTRL2_V18SE);
		break;

	case SD_VOL_1_8_V:
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_BVS_MASK);
		sdhc_cdns_set_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_VOL_1_8_V_SELECT);
		sdhc_cdns_set_bits(dev, SDHC_CDNS_AUTO_CMD_HOST_CTRL2, SDHC_CDNS_HOST_CTRL2_V18SE);
		break;
	default:
		return -EINVAL;
	}

	/* Wait for voltage switch to settle */
	k_msleep(5);

	return 0;
}

static int sdhc_cdns_set_timing(const struct device *dev, enum sdhc_timing_mode timing)
{
	struct sdhc_cdns_data *sd_data = dev->data;
	uint32_t mode = 0;
	int ret;

	switch (timing) {
	case SDHC_TIMING_LEGACY:
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_HSE);
		break;

	case SDHC_TIMING_HS:
		sdhc_cdns_set_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_HSE);
		break;

	case SDHC_TIMING_SDR12:
		mode = SDHC_CDNS_UHS_SPEED_MODE_SDR12;
		break;

	case SDHC_TIMING_SDR25:
		mode = SDHC_CDNS_UHS_SPEED_MODE_SDR25;
		break;

	case SDHC_TIMING_SDR50:
		mode = SDHC_CDNS_UHS_SPEED_MODE_SDR50;
		break;

	case SDHC_TIMING_HS200:
	case SDHC_TIMING_SDR104:
		mode = SDHC_CDNS_UHS_SPEED_MODE_SDR104;
		break;

	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_DDR52:
		mode = SDHC_CDNS_UHS_SPEED_MODE_DDR50;
		break;

	case SDHC_TIMING_HS400:
		mode = SDHC_CDNS_UHS_SPEED_MODE_DDR200;
		break;

	default:
		return -EINVAL;
	}

	/* Select one of UHS mode */
	if (timing >= SDHC_TIMING_SDR12) {
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_AUTO_CMD_HOST_CTRL2,
				     SDHC_CDNS_HOST_CTRL2_UMS_MASK);
		sdhc_cdns_set_bits(dev, SDHC_CDNS_AUTO_CMD_HOST_CTRL2,
				   mode & SDHC_CDNS_HOST_CTRL2_UMS_MASK);
		sdhc_cdns_set_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_HSE);
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

	LOG_DBG("SDHC I/O: slot: %d, bus width %d, clock %dHz, card power %s, voltage %s",
		SDHC_CDNS_SLOT_TYPE(dev), ios->bus_width, ios->clock,
		ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V");

	/* Check given clock is valid */
	if (ios->clock != 0 &&
	    (ios->clock > sd_data->props.f_max || ios->clock < sd_data->props.f_min)) {
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
	int ret;

	/* Perform software reset (SRFA); this also clears the clock config
	 * that shares this 32-bit register.
	 */
	sdhc_cdns_write(dev, SDHC_CDNS_CLOCK_CTRL, SDHC_CDNS_CLOCK_CTRL_SRFA);
	/* Wait max 100ms for software reset to complete */
	ret = sdhc_cdns_wait_reg_mask(dev, SDHC_CDNS_CLOCK_CTRL, 100, SDHC_CDNS_CLOCK_CTRL_SRFA, 0);
	if (ret != 0) {
		LOG_ERR("Device is busy");
		return -EBUSY;
	}

	/* Enable status reg and configure interrupt (normal + error together) */
	sdhc_cdns_write(dev, SDHC_CDNS_INT_ENABLE,
			SDHC_CDNS_NORMAL_INT_MASK | SDHC_CDNS_ERR_INT_MASK);

	if (config->irq_config_fn == NULL) {
		sdhc_cdns_write(dev, SDHC_CDNS_INT_SIGNAL_ENABLE, 0);
	} else {
		/*
		 * Enable command complete, transfer complete, read buffer ready and
		 * error status interrupt
		 */
		sdhc_cdns_write(dev, SDHC_CDNS_INT_SIGNAL_ENABLE, SDHC_CDNS_TXFR_INTR_EN_MASK);
	}

	/* Data line timeout interval (DTCV field, shares SDHC_CDNS_CLOCK_CTRL) */
	sdhc_cdns_write(dev, SDHC_CDNS_CLOCK_CTRL,
			(SDHC_CDNS_CLOCK_CTRL_DTCV << SDHC_CDNS_CLOCK_CTRL_DTCV_POS) &
				SDHC_CDNS_CLOCK_CTRL_DTCV_MASK);

	sdhc_cdns_set_bits(dev, SDHC_CDNS_AUTO_CMD_HOST_CTRL2, SDHC_CDNS_HOST_CTRL2_HV4E);
	if (IS_ENABLED(CONFIG_64BIT)) {
		sdhc_cdns_set_bits(dev, SDHC_CDNS_AUTO_CMD_HOST_CTRL2, SDHC_CDNS_HOST_CTRL2_A64B);
	} else {
		sdhc_cdns_clear_bits(dev, SDHC_CDNS_AUTO_CMD_HOST_CTRL2,
				     SDHC_CDNS_HOST_CTRL2_A64B);
	}

	/* Select DMA mode (DMA Select field, shares SDHC_CDNS_HOST_CTRL) */
	sdhc_cdns_clear_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_DMASEL_MASK);
	sdhc_cdns_set_bits(dev, SDHC_CDNS_HOST_CTRL, SDHC_CDNS_HOST_CTRL_DMASEL_ADMA2_32);

	sdhc_cdns_write(dev, SDHC_CDNS_BLOCK_SIZE, SDHC_CDNS_BLOCK_SIZE_512);

	sdhc_cdns_clear_intr(dev);

	return 0;
}

/**
 * @brief
 * Check for card busy
 */
static int sdhc_cdns_card_busy(const struct device *dev)
{
	int ret;

	/* Wait max 2ms for card to send next command */
	ret = sdhc_cdns_wait_reg_mask(dev, SDHC_CDNS_PRESENT_STATE, 2,
				      SDHC_CDNS_PRESENT_STATE_DATSL1_D0, 0);
	if (ret != 0) {
		return 0;
	}

	return 1;
}

/**
 * @brief
 * Enable tuning clock
 */
static int sdhc_cdns_card_tuning(const struct device *dev)
{
	struct sdhc_cdns_data *sd_data = dev->data;
	const struct sdhc_io *io = &sd_data->host_io;
	struct sdhc_command cmd = {0};
	uint32_t block_reg;
	uint8_t blksize;
	int ret;

	if (io->timing == SDHC_TIMING_HS200 || io->timing == SDHC_TIMING_HS400) {
		cmd.opcode = MMC_SEND_TUNING_BLOCK;
	} else {
		cmd.opcode = SD_SEND_TUNING_BLOCK;
	}

	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	sd_data->transfermode = SDHC_CDNS_XFER_MODE_DTDS;

	blksize = SDHC_CDNS_TUNING_CMD_BLKSIZE;
	if (io->bus_width == SDHC_BUS_WIDTH8BIT) {
		blksize = SDHC_CDNS_TUNING_CMD_BLKSIZE * 2;
	}

	block_reg = ((SDHC_CDNS_TUNING_CMD_BLKCOUNT << SDHC_CDNS_BLOCK_SIZE_BCCT_POS) &
		    SDHC_CDNS_BLOCK_SIZE_BCCT_MASK) | blksize;

	/* Execute tuning */
	for (uint8_t count = 0; count < SDHC_CDNS_MAX_TUNING_COUNT; count++) {
		sdhc_cdns_write(dev, SDHC_CDNS_BLOCK_SIZE, block_reg);
		ret = sdhc_cdns_cmd(dev, &cmd, true);
		if (ret != 0) {
			return ret;
		}
	}

	sd_data->transfermode = 0;

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

	DEVICE_MMIO_NAMED_MAP(dev, slot, K_MEM_CACHE_NONE);

	if (config->has_host) {
		DEVICE_MMIO_NAMED_MAP(dev, host, K_MEM_CACHE_NONE);
	}

	if (sdhc_cdns_quirk_phy_init(dev)) {
		LOG_ERR("Failed to initialize phy");
		return -EIO;
	}

	if (config->irq_config_fn != NULL) {
		k_event_init(&sd_data->irq_event);
		config->irq_config_fn(dev);
	}

	return sdhc_cdns_host_reset(dev);
}

static DEVICE_API(sdhc, sdhc_cdns_api) = {
	.reset = sdhc_cdns_host_reset,
	.request = sdhc_cdns_request,
	.set_io = sdhc_cdns_set_io,
	.get_card_present = sdhc_cdns_card_detect,
	.execute_tuning = sdhc_cdns_card_tuning,
	.card_busy = sdhc_cdns_card_busy,
	.get_host_props = sdhc_cdns_host_props,
};

/* clang-format off */
#define SDHC_CDNS_INTR_CONFIG(n)                                                                   \
	static void sdhc_cdns_irq_handler##n(const struct device *dev)                             \
	{                                                                                          \
		struct sdhc_cdns_data *sd_data = dev->data;                                        \
		uint32_t status = sdhc_cdns_read(dev, SDHC_CDNS_INT_STATUS);                       \
		if ((status & SDHC_CDNS_INT_STATUS_CC) != 0U) {                                    \
			sdhc_cdns_write(dev, SDHC_CDNS_INT_STATUS, SDHC_CDNS_INT_STATUS_CC);       \
			k_event_post(&sd_data->irq_event, SDHC_CDNS_INT_STATUS_CC);                \
		}                                                                                  \
		if ((status & SDHC_CDNS_INT_STATUS_BRR) != 0U) {                                   \
			sdhc_cdns_write(dev, SDHC_CDNS_INT_STATUS, SDHC_CDNS_INT_STATUS_BRR);      \
			k_event_post(&sd_data->irq_event, SDHC_CDNS_INT_STATUS_BRR);               \
		}                                                                                  \
		if ((status & SDHC_CDNS_INT_STATUS_TC) != 0U) {                                    \
			sdhc_cdns_write(dev, SDHC_CDNS_INT_STATUS, SDHC_CDNS_INT_STATUS_TC);       \
			k_event_post(&sd_data->irq_event, SDHC_CDNS_INT_STATUS_TC);                \
		}                                                                                  \
		if ((status & SDHC_CDNS_ERR_INT_MASK) != 0U) {                                     \
			sdhc_cdns_write(dev, SDHC_CDNS_INT_STATUS, SDHC_CDNS_ERR_INT_MASK);        \
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

#define SDHC_CDNS_MMIO_ROM_INIT(n)                                                                 \
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(slot, DT_DRV_INST(n)),                                  \
		COND_CODE_1(DT_INST_REG_HAS_NAME(n, host),                                         \
		    (DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(host, DT_DRV_INST(n)),), ())

#define SDHC_CDNS_CLOCK_INIT(n)                                                                    \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                        \
	IF_ENABLED(DT_INST_PHA_HAS_CELL_AT_IDX(n, clocks, 0, id),                                  \
		(.clock_id = DT_INST_CLOCKS_CELL(n, id),))

#define SDHC_CDNS_INIT(n)                                                                          \
	SDHC_CDNS_INTR_CONFIG_API(n)                                                               \
	static struct sdhc_cdns_data sdhc_cdns_data##n;                                            \
	const static struct sdhc_cdns_config sdhc_cdns_config_##n = {                              \
		SDHC_CDNS_MMIO_ROM_INIT(n)                                                         \
		SDHC_CDNS_CLOCK_INIT(n)                                                            \
		SDHC_CDNS_INTR_FUNC_REG_API(n).broken_cd = DT_INST_PROP_OR(n, broken_cd, 0),       \
		.quirks = SDHC_CDNS_QUIRK_GET(n),                                                  \
		.quirk_data = SDHC_CDNS_QUIRK_DATA_GET(n),                                         \
		.quirk_config = SDHC_CDNS_QUIRK_CONFIG_GET(n),                                     \
		.powerdelay = DT_INST_PROP_OR(n, power_delay_ms, 0),                               \
		.mmc_hs200_1_8v = DT_INST_PROP(n, mmc_hs200_1_8v),                                 \
		.has_host = DT_INST_PROP(n, has_host),                                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, sdhc_cdns_init, NULL, &sdhc_cdns_data##n, &sdhc_cdns_config_##n,  \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &sdhc_cdns_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_CDNS_INIT)
/* clang-format on */
