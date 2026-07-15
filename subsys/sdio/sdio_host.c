/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-role SDIO transport backend. Implements the CMD52/CMD53 primitives of
 * struct sdio_transport_api on top of a Zephyr SD host controller (sdhc).
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sdio/sdio_host.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sdio_core, CONFIG_SDIO_LOG_LEVEL);

#define SDIO_HOST_CMD_TIMEOUT  CONFIG_SDIO_HOST_CMD_TIMEOUT_MS
#define SDIO_HOST_DATA_TIMEOUT CONFIG_SDIO_HOST_DATA_TIMEOUT_MS

static int sdio_host_rw_direct(struct sdio_dev *dev, enum sdio_io_dir dir,
			       enum sdio_func_num func, uint32_t reg,
			       uint8_t data_in, uint8_t *data_out)
{
	const struct device *sdhc = dev->transport_ctx;
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = SDIO_RW_DIRECT;
	cmd.arg = (func << SDIO_CMD_ARG_FUNC_NUM_SHIFT) |
		  ((reg & SDIO_CMD_ARG_REG_ADDR_MASK)
		   << SDIO_CMD_ARG_REG_ADDR_SHIFT);
	if (dir == SDIO_IO_WRITE) {
		cmd.arg |= data_in & SDIO_DIRECT_CMD_DATA_MASK;
		cmd.arg |= BIT(SDIO_CMD_ARG_RW_SHIFT);
		if (data_out) {
			cmd.arg |= BIT(SDIO_DIRECT_CMD_ARG_RAW_SHIFT);
		}
	}
	cmd.response_type = (SD_RSP_TYPE_R5 | SD_SPI_RSP_TYPE_R5);
	cmd.timeout_ms = SDIO_HOST_CMD_TIMEOUT;

	ret = sdhc_request(sdhc, &cmd, NULL);
	if (ret) {
		return ret;
	}
	if (data_out) {
		if (dev->caps & SDIO_CAP_SPI) {
			*data_out = (cmd.response[0U] >> 8) &
				    SDIO_DIRECT_CMD_DATA_MASK;
		} else {
			*data_out = cmd.response[0U] & SDIO_DIRECT_CMD_DATA_MASK;
		}
	}
	return 0;
}

static int sdio_host_rw_extended(struct sdio_dev *dev, enum sdio_io_dir dir,
				 const struct sdio_xfer *xfer)
{
	const struct device *sdhc = dev->transport_ctx;
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	cmd.opcode = SDIO_RW_EXTENDED;
	cmd.arg = (xfer->func << SDIO_CMD_ARG_FUNC_NUM_SHIFT) |
		  ((xfer->reg & SDIO_CMD_ARG_REG_ADDR_MASK)
		   << SDIO_CMD_ARG_REG_ADDR_SHIFT);
	cmd.arg |= (dir == SDIO_IO_WRITE) ? BIT(SDIO_CMD_ARG_RW_SHIFT) : 0;
	cmd.arg |= xfer->increment ? BIT(SDIO_EXTEND_CMD_ARG_OP_CODE_SHIFT) : 0;
	cmd.response_type = (SD_RSP_TYPE_R5 | SD_SPI_RSP_TYPE_R5);
	cmd.timeout_ms = SDIO_HOST_CMD_TIMEOUT;
	if (xfer->blocks == 0) {
		cmd.arg |= (xfer->block_size == 512) ? 0 : xfer->block_size;
	} else {
		cmd.arg |= BIT(SDIO_EXTEND_CMD_ARG_BLK_SHIFT) | xfer->blocks;
	}

	data.block_size = xfer->block_size;
	data.blocks = xfer->blocks ? xfer->blocks : 1;
	data.data = xfer->buf;
	data.timeout_ms = SDIO_HOST_DATA_TIMEOUT;
	return sdhc_request(sdhc, &cmd, &data);
}

static const struct sdio_transport_api sdio_host_transport = {
	.rw_direct = sdio_host_rw_direct,
	.rw_extended = sdio_host_rw_extended,
};

int sdio_host_init(struct sdio_dev *dev, const struct device *sdhc,
		   uint32_t caps)
{
	if (sdhc == NULL) {
		return -EINVAL;
	}
	return sdio_dev_init(dev, SDIO_ROLE_HOST, &sdio_host_transport, sdhc,
			     caps);
}

int sdio_host_bind_card(struct sdio_dev *dev, struct sd_card *card)
{
	uint32_t caps = 0;

	if (dev == NULL || card == NULL) {
		return -EINVAL;
	}
	if ((card->type != CARD_SDIO) && (card->type != CARD_COMBO)) {
		return -ENOTSUP;
	}

	if (card->cccr_flags & SDIO_SUPPORT_MULTIBLOCK) {
		caps |= SDIO_CAP_MULTIBLOCK;
	}
	if (card->cccr_flags & SDIO_SUPPORT_HS) {
		caps |= SDIO_CAP_HS;
	}
	if (card->cccr_flags & SDIO_SUPPORT_4BIT_LS_BUS) {
		caps |= SDIO_CAP_4BIT_BUS;
	}
	if (card->host_props.is_spi) {
		caps |= SDIO_CAP_SPI;
	}

	(void)sdio_dev_init(dev, SDIO_ROLE_HOST, &sdio_host_transport,
			    card->sdhc, caps);
	dev->max_blk_size = card->func0.cis.max_blk_size;
	return 0;
}
