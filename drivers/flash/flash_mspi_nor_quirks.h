/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLASH_MSPI_NOR_QUIRKS_H__
#define __FLASH_MSPI_NOR_QUIRKS_H__

/* Flash chip specific quirks */
struct flash_mspi_nor_quirks {
	/* Called at the beginning of the flash chip initialization,
	 * right after reset if any is performed. Can be used to alter
	 * structures that define communication with the chip, like
	 * `cmd_info`, `switch_info`, and `erase_types`, which are set
	 * to default values at this point.
	 */
	int (*pre_init)(const struct device *dev);
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

	/* Write enable */
	rc = cmd_wren(dev);
	if (rc < 0) {
		return rc;
	}

	/* Write status and config registers */
	set_up_xfer(dev, MSPI_TX);
	dev_data->packet.data_buf  = mxicy_mx25r_hp_payload;
	dev_data->packet.num_bytes = sizeof(mxicy_mx25r_hp_payload);
	rc = perform_xfer(dev, SPI_NOR_CMD_WRSR, false);
	if (rc < 0) {
		return rc;
	}

	/* Wait for write to end and verify status register */
	do {
		rc = cmd_rdsr(dev, SPI_NOR_CMD_RDSR, &status);
		if (rc < 0) {
			return rc;
		}
	} while (status & SPI_NOR_WIP_BIT);

	if (status != mxicy_mx25r_hp_payload[0]) {
		return -EIO;
	}

	/* Verify configuration registers */
	set_up_xfer(dev, MSPI_RX);
	dev_data->packet.num_bytes = sizeof(config);
	dev_data->packet.data_buf  = config;
	rc = perform_xfer(dev, SPI_NOR_CMD_RDCR, false);
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

static inline int mxicy_mx25u_post_switch_mode(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	enum mspi_io_mode io_mode = dev_config->mspi_nor_cfg.io_mode;
	uint8_t opi_enable;
	int rc;

	if (io_mode != MSPI_IO_MODE_OCTAL) {
		return 0;
	}

	/*
	 * TODO - replace this with a generic routine that uses information
	 *        from SFDP header FF87 (Status, Control and Configuration
	 *        Register Map)
	 */

	if (dev_config->mspi_nor_cfg.data_rate == MSPI_DATA_RATE_DUAL) {
		opi_enable = BIT(1);
	} else {
		opi_enable = BIT(0);
	}

	/* Write enable */
	rc = cmd_wren(dev);
	if (rc < 0) {
		return rc;
	}

	/* Write config register 2 */
	set_up_xfer(dev, MSPI_TX);
	dev_data->xfer.addr_length = 4;
	dev_data->packet.address   = 0;
	dev_data->packet.data_buf  = &opi_enable;
	dev_data->packet.num_bytes = sizeof(opi_enable);
	return perform_xfer(dev, SPI_NOR_CMD_WR_CFGREG2, false);
}

static int mxicy_mx25u_pre_init(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	static const uint8_t dummy_cycles[8] = {
		20, 18, 16, 14, 12, 10, 8, 6
	};
	uint8_t cfg_reg;
	int rc;

	if (dev_config->mspi_nor_cfg.io_mode != MSPI_IO_MODE_OCTAL) {
		return 0;
	}

	if (dev_config->mspi_nor_cfg.data_rate == MSPI_DATA_RATE_SINGLE) {
		dev_data->cmd_info.cmd_extension = CMD_EXTENSION_INVERSE;
	}

	/*
	 * TODO - replace this with a generic routine that uses information
	 *        from SFDP header FF87 (Status, Control and Configuration
	 *        Register Map)
	 */

	/* Read configured number of dummy cycles for memory reading commands. */
	set_up_xfer(dev, MSPI_RX);
	dev_data->xfer.addr_length = 4;
	dev_data->packet.address   = 0x300;
	dev_data->packet.data_buf  = &cfg_reg;
	dev_data->packet.num_bytes = sizeof(cfg_reg);
	rc = perform_xfer(dev, SPI_NOR_CMD_RD_CFGREG2, false);
	if (rc < 0) {
		LOG_ERR("Failed to read Dummy Cycle from CFGREG2");
		return rc;
	}

	dev_data->cmd_info.read_mode_bit_cycles = 0;
	dev_data->cmd_info.read_dummy_cycles = dummy_cycles[cfg_reg & 0x7];

	return 0;
}

struct flash_mspi_nor_quirks flash_quirks_mxicy_mx25u = {
	.pre_init = mxicy_mx25u_pre_init,
	.post_switch_mode = mxicy_mx25u_post_switch_mode,
};

#endif /* DT_HAS_COMPAT_STATUS_OKAY(mxicy_mx25u) */

#endif /*__FLASH_MSPI_NOR_QUIRKS_H__*/
