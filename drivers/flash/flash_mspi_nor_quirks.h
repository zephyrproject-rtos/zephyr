/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLASH_MSPI_NOR_QUIRKS_H__
#define __FLASH_MSPI_NOR_QUIRKS_H__

/* Flash chip specific quirks */
struct flash_mspi_nor_quirks {
	/* Called after switching to default IO mode. */
	int (*post_switch_mode)(const struct device *dev);
};

/* Extend this macro when adding new flash chip with quirks */
#define FLASH_MSPI_QUIRKS_GET(node)						\
	COND_CODE_1(DT_NODE_HAS_COMPAT_STATUS(node, mxicy_mx25r, okay),		\
		    (&flash_quirks_mxicy_mx25r),				\
	(COND_CODE_1(DT_NODE_HAS_COMPAT_STATUS(node, mxicy_mx25u, okay),	\
		    (&flash_quirks_mxicy_mx25u),				\
		    (NULL))))

#if DT_HAS_COMPAT_STATUS_OKAY(mxicy_mx25r)

#define MXICY_MX25R_LH_MASK BIT(1)
#define MXICY_MX25R_QE_MASK BIT(6)
#define MXICY_MX25R_REGS_LEN 3

static uint8_t mxicy_mx25r_hp_payload[MXICY_MX25R_REGS_LEN] = {
	MXICY_MX25R_QE_MASK, 0x0, MXICY_MX25R_LH_MASK
};

/* For quad io mode above 8 MHz and single io mode above 33 MHz,
 * high performance mode needs to be enabled.
 */
static inline bool needs_hp(enum mspi_io_mode io_mode, uint32_t freq)
{
	if ((io_mode == MSPI_IO_MODE_QUAD_1_1_4) || (io_mode == MSPI_IO_MODE_QUAD_1_4_4)) {
		if (freq > MHZ(8)) {
			return true;
		}
	} else if (io_mode == MSPI_IO_MODE_SINGLE) {
		if (freq > MHZ(33)) {
			return true;
		}
	}

	return false;
}

static inline int mxicy_mx25r_post_switch_mode(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	enum mspi_io_mode io_mode = dev_config->mspi_nor_cfg.io_mode;
	uint32_t freq = dev_config->mspi_nor_cfg.freq;
	int rc;
	uint8_t status;
	uint8_t config[MXICY_MX25R_REGS_LEN - 1];

	if (!needs_hp(io_mode, freq)) {
		return 0;
	}

	/* Wait for previous write to finish */
	do {
		flash_mspi_command_set(dev, &dev_config->jedec_cmds->status);
		dev_data->packet.data_buf  = &status;
		dev_data->packet.num_bytes = sizeof(status);
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
		if (rc < 0) {
			return rc;
		}
	} while (status & SPI_NOR_WIP_BIT);

	/* Write enable */
	flash_mspi_command_set(dev, &commands[MSPI_IO_MODE_SINGLE].write_en);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		return rc;
	}

	/* Write status and config registers */
	const struct flash_mspi_nor_cmd cmd_status = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_WRSR,
		.cmd_length = 1,
	};

	flash_mspi_command_set(dev, &cmd_status);
	dev_data->packet.data_buf  = mxicy_mx25r_hp_payload;
	dev_data->packet.num_bytes = sizeof(mxicy_mx25r_hp_payload);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
	if (rc < 0) {
		return rc;
	}

	/* Wait for write to end and verify status register */
	do {
		flash_mspi_command_set(dev, &dev_config->jedec_cmds->status);
		dev_data->packet.data_buf  = &status;
		dev_data->packet.num_bytes = sizeof(status);
		rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
		if (rc < 0) {
			return rc;
		}
	} while (status & SPI_NOR_WIP_BIT);

	if (status != mxicy_mx25r_hp_payload[0]) {
		return -EIO;
	}

	/* Verify configuration registers */
	flash_mspi_command_set(dev, &dev_config->jedec_cmds->config);
	dev_data->packet.data_buf  = config;
	dev_data->packet.num_bytes = sizeof(config);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
	if (rc < 0) {
		return rc;
	}

	for (uint8_t i = 0; i < MXICY_MX25R_REGS_LEN - 1; i++) {
		if (config[i] != mxicy_mx25r_hp_payload[i + 1]) {
			return -EIO;
		}
	}

	return 0;
}

struct flash_mspi_nor_quirks flash_quirks_mxicy_mx25r = {
	.post_switch_mode = mxicy_mx25r_post_switch_mode,
};

#endif /* DT_HAS_COMPAT_STATUS_OKAY(mxicy_mx25r) */

#if DT_HAS_COMPAT_STATUS_OKAY(mxicy_mx25u)

#define MXICY_MX25R_OE_MASK BIT(0)

static uint8_t mxicy_mx25u_oe_payload = MXICY_MX25R_OE_MASK;

static inline int mxicy_mx25u_post_switch_mode(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	enum mspi_io_mode io_mode = dev_config->mspi_nor_cfg.io_mode;
	int rc;

	if (io_mode != MSPI_IO_MODE_OCTAL) {
		return 0;
	}

	/* Write enable */
	flash_mspi_command_set(dev, &commands[MSPI_IO_MODE_SINGLE].write_en);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		return rc;
	}

	/* Write config register 2 */
	const struct flash_mspi_nor_cmd cmd_status = {
		.dir = MSPI_TX,
		.cmd = SPI_NOR_CMD_WR_CFGREG2,
		.cmd_length = 1,
		.addr_length = 4,
	};

	flash_mspi_command_set(dev, &cmd_status);
	dev_data->packet.data_buf  = &mxicy_mx25u_oe_payload;
	dev_data->packet.num_bytes = sizeof(mxicy_mx25u_oe_payload);
	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id, &dev_data->xfer);
	return rc;
}

struct flash_mspi_nor_quirks flash_quirks_mxicy_mx25u = {
	.post_switch_mode = mxicy_mx25u_post_switch_mode,
};

#endif /* DT_HAS_COMPAT_STATUS_OKAY(mxicy_mx25u) */

#endif /*__FLASH_MSPI_NOR_QUIRKS_H__*/
