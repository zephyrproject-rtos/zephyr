/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_MSPI_NOR_QUIRKS_W35T51NW_H_
#define ZEPHYR_DRIVERS_FLASH_MSPI_NOR_QUIRKS_W35T51NW_H_

#define W35T51NW_CMD_WRITE_VCR 0x81U
#define W35T51NW_CMD_ENTER_4B  0xb7U
#define W35T51NW_VCR_IO_MODE   0x00U
#define W35T51NW_VCR_DUMMY     0x01U
#define W35T51NW_ODDR_DQS      0xe7U

static int w35t51nw_write_vcr(const struct device *dev, uint8_t address, uint8_t value)
{
	const struct flash_mspi_nor_config *cfg = dev->config;
	struct flash_mspi_nor_data *data = dev->data;
	int ret;

	ret = cmd_wren(dev);
	if (ret != 0) {
		return ret;
	}

	set_up_xfer_with_addr(dev, MSPI_TX, address, cfg->control_xfer_mode);
	data->packet.data_buf = &value;
	data->packet.num_bytes = sizeof(value);
	return perform_xfer(dev, W35T51NW_CMD_WRITE_VCR);
}

static int w35t51nw_post_switch_mode(const struct device *dev)
{
	const struct flash_mspi_nor_config *cfg = dev->config;
	struct flash_mspi_nor_data *data = dev->data;
	uint8_t dummy_cycles = cfg->rx_dummy_specified ? cfg->mspi_nor_cfg.rx_dummy
						       : data->cmd_info.read_dummy_cycles;
	int ret;

	if (cfg->mspi_nor_cfg.io_mode != MSPI_IO_MODE_OCTAL) {
		return 0;
	}

	if (cfg->mspi_nor_cfg.data_rate != MSPI_DATA_RATE_DUAL) {
		return -ENOTSUP;
	}

	if ((dummy_cycles == 0U) || (dummy_cycles > 30U)) {
		return -EINVAL;
	}

	/* ODDR register reads use eight dummy clocks without an address. */
	data->cmd_info.rdsr_dummy = 8U;
	data->cmd_info.rdid_dummy = 8U;
	data->cmd_info.rdsr_addr_4 = false;
	data->cmd_info.rdid_addr_4 = false;
	data->cmd_info.sfdp_addr_4 = true;
	data->cmd_info.sfdp_dummy_20 = false;

	/* VCR writes use the current address mode, unlike dedicated 4-byte commands. */
	set_up_xfer(dev, MSPI_TX, cfg->control_xfer_mode);
	ret = perform_xfer(dev, W35T51NW_CMD_ENTER_4B);
	if (ret != 0) {
		return ret;
	}

	ret = w35t51nw_write_vcr(dev, W35T51NW_VCR_DUMMY, dummy_cycles);
	if (ret != 0) {
		return ret;
	}

	return w35t51nw_write_vcr(dev, W35T51NW_VCR_IO_MODE, W35T51NW_ODDR_DQS);
}

static struct flash_mspi_nor_quirks flash_quirks_w35t51nw = {
	.post_switch_mode = w35t51nw_post_switch_mode,
};

#endif /* ZEPHYR_DRIVERS_FLASH_MSPI_NOR_QUIRKS_W35T51NW_H_ */
