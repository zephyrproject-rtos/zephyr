/*
 * SPDX-FileCopyrightText: Copyright 2026 EXALT Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_MSPI_NOR_QUIRKS_M95P32_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_MSPI_NOR_QUIRKS_M95P32_H_

#include <zephyr/drivers/flash/m95p32_flash_api_extensions.h>

#define M95P32_CMD_RDID_PAGE 0x83
#define M95P32_CMD_WRID_PAGE 0x82
#define M95P32_CMD_RDVR      0x85
#define M95P32_CMD_WRVR      0x81
#define M95P32_CMD_PGPR      0x0A

#define M95P32_VOLATILE_BUFEN BIT(1)
#define M95P32_VOLATILE_BUFLD BIT(0)

#define M95P32_PAGE_SIZE 512U

static void m95p32_set_up_xfer_with_addr(const struct device *dev, enum mspi_xfer_direction dir,
					 uint32_t addr, enum mspi_xfer_mode xfer_mode)
{
	struct flash_mspi_nor_data *dev_data = dev->data;

	set_up_xfer(dev, dir, xfer_mode);
	dev_data->xfer.addr_length = 3;
	dev_data->packet.address = addr;
}

static int m95p32_ex_op_read_id_page(const struct device *dev,
				     const struct flash_m95p32_ex_op_read_id_page_in *op_in,
				     void *out)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	uint32_t addr;
	uint8_t *data = out;
	size_t remaining;
	int rc;

	if (op_in == NULL) {
		return -EINVAL;
	}
	if (op_in->length == 0U) {
		return 0;
	}
	if (out == NULL || op_in->offset < 0 || op_in->offset >= M95P32_ID_AREA_SIZE ||
	    op_in->length > M95P32_ID_AREA_SIZE - (size_t)op_in->offset) {
		return -EINVAL;
	}

	addr = (uint32_t)op_in->offset;
	remaining = op_in->length;

	while (remaining > 0U) {
		size_t to_read = remaining;

		if (dev_config->packet_data_limit != 0U) {
			to_read = MIN(to_read, dev_config->packet_data_limit);
		}

		m95p32_set_up_xfer_with_addr(dev, MSPI_RX, addr, dev_config->control_xfer_mode);
		dev_data->packet.data_buf = data;
		dev_data->packet.num_bytes = to_read;
		rc = perform_xfer(dev, M95P32_CMD_RDID_PAGE);
		if (rc < 0) {
			return rc;
		}

		addr += to_read;
		data += to_read;
		remaining -= to_read;
	}

	return 0;
}

static int m95p32_ex_op_write_id_page(const struct device *dev,
				      const struct flash_m95p32_ex_op_write_id_page_in *op_in)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	const uint8_t *data;
	uint32_t addr;
	size_t remaining;
	int rc;

	if (op_in == NULL) {
		return -EINVAL;
	}
	if (op_in->length == 0U) {
		return 0;
	}
	if (op_in->data == NULL || op_in->offset < M95P32_CUSTOMER_ID_PAGE_OFFSET ||
	    op_in->offset >= M95P32_ID_AREA_SIZE ||
	    op_in->length > M95P32_ID_AREA_SIZE - (size_t)op_in->offset) {
		return -EINVAL;
	}

	addr = (uint32_t)op_in->offset;
	data = op_in->data;
	remaining = op_in->length;

	while (remaining > 0U) {
		size_t to_write = remaining;

		if (dev_config->packet_data_limit != 0U) {
			to_write = MIN(to_write, dev_config->packet_data_limit);
		}

		rc = cmd_wren(dev);
		if (rc < 0) {
			return rc;
		}

		m95p32_set_up_xfer_with_addr(dev, MSPI_TX, addr, dev_config->control_xfer_mode);
		dev_data->packet.data_buf = (uint8_t *)data;
		dev_data->packet.num_bytes = to_write;
		rc = perform_xfer(dev, M95P32_CMD_WRID_PAGE);
		if (rc < 0) {
			return rc;
		}

		rc = wait_until_ready(dev, K_MSEC(1));
		if (rc < 0) {
			return rc;
		}

		addr += to_write;
		data += to_write;
		remaining -= to_write;
	}

	return 0;
}

static int m95p32_read_volatile_reg(const struct device *dev, uint8_t *value)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;

	if (value == NULL) {
		return -EINVAL;
	}

	set_up_xfer(dev, MSPI_RX, dev_config->control_xfer_mode);
	dev_data->packet.data_buf = value;
	dev_data->packet.num_bytes = sizeof(*value);

	return perform_xfer(dev, M95P32_CMD_RDVR);
}

static int m95p32_write_volatile_reg(const struct device *dev, uint8_t value)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	if ((value & ~M95P32_VOLATILE_BUFEN) != 0U) {
		return -EINVAL;
	}

	rc = cmd_wren(dev);
	if (rc < 0) {
		return rc;
	}

	set_up_xfer(dev, MSPI_TX, dev_config->control_xfer_mode);
	dev_data->packet.data_buf = &value;
	dev_data->packet.num_bytes = sizeof(value);

	return perform_xfer(dev, M95P32_CMD_WRVR);
}

static int m95p32_set_buffer_mode(const struct device *dev, bool enable)
{
	uint8_t volatile_reg;
	int rc;

	rc = m95p32_write_volatile_reg(dev, enable ? M95P32_VOLATILE_BUFEN : 0U);
	if (rc < 0) {
		return rc;
	}

	rc = m95p32_read_volatile_reg(dev, &volatile_reg);
	if (rc < 0) {
		return rc;
	}

	if (!!(volatile_reg & M95P32_VOLATILE_BUFEN) != enable) {
		LOG_ERR("%s: volatile register 0x%02x after setting BUFEN to %u", dev->name,
			volatile_reg, enable);
		return -EIO;
	}

	return 0;
}

static int m95p32_wait_buffer_free(const struct device *dev)
{
	uint8_t volatile_reg;
	int rc;

	while (true) {
		rc = m95p32_read_volatile_reg(dev, &volatile_reg);
		if (rc < 0) {
			return rc;
		}
		if ((volatile_reg & M95P32_VOLATILE_BUFEN) == 0U) {
			LOG_ERR("%s: BUFEN cleared while waiting, volatile register 0x%02x",
				dev->name, volatile_reg);
			return -EIO;
		}
		if ((volatile_reg & M95P32_VOLATILE_BUFLD) == 0U) {
			return 0;
		}

		k_sleep(K_USEC(1));
	}
}

static int m95p32_page_program_with_buffer_load(const struct device *dev, uint32_t addr,
						const uint8_t *data, size_t length)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;

	m95p32_set_up_xfer_with_addr(dev, MSPI_TX, addr, dev_config->data_xfer_mode);
	dev_data->packet.data_buf = (uint8_t *)data;
	dev_data->packet.num_bytes = length;

	return perform_xfer(dev, M95P32_CMD_PGPR);
}

static int m95p32_ex_op_page_program_with_buffer_load(
	const struct device *dev,
	const struct flash_m95p32_ex_op_page_program_with_buffer_load_in *op_in)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	const uint16_t page_size = dev_config->page_size;
	const uint8_t *data;
	uint32_t addr;
	size_t remaining;
	bool buffer_enabled = false;
	int cleanup_rc;
	int rc;

	if (op_in == NULL) {
		return -EINVAL;
	}
	if (op_in->length == 0U) {
		return 0;
	}
	if (op_in->data == NULL) {
		return -EINVAL;
	}
	if (op_in->offset < 0 || op_in->offset >= dev_config->flash_size ||
	    op_in->length > dev_config->flash_size - (size_t)op_in->offset ||
	    page_size != M95P32_PAGE_SIZE) {
		return -EINVAL;
	}
	if (dev_data->cmd_info.pp_cmd != M95P32_CMD_PGPR) {
		return -ENOTSUP;
	}

	addr = (uint32_t)op_in->offset;
	data = op_in->data;
	remaining = op_in->length;

	rc = m95p32_set_buffer_mode(dev, true);
	if (rc < 0) {
		LOG_ERR("%s: failed to enable buffer mode: %d", dev->name, rc);
		return rc;
	}
	buffer_enabled = true;

	if (rc == 0) {
		rc = cmd_wren(dev);
		if (rc < 0) {
			LOG_ERR("%s: WREN before page program with buffer load failed: %d",
				dev->name, rc);
		}
	}
	while (rc == 0 && remaining > 0U) {
		size_t page_left = page_size - (addr % page_size);
		size_t to_write = MIN(remaining, page_left);

		if (dev_config->packet_data_limit != 0U) {
			to_write = MIN(to_write, dev_config->packet_data_limit);
		}

		rc = m95p32_wait_buffer_free(dev);
		if (rc < 0) {
			LOG_ERR("%s: buffer was not available: %d", dev->name, rc);
			break;
		}

		rc = m95p32_page_program_with_buffer_load(dev, addr, data, to_write);
		if (rc < 0) {
			LOG_ERR("%s: page program with buffer load at 0x%06x failed: %d", dev->name,
				addr, rc);
			break;
		}

		addr += to_write;
		data += to_write;
		remaining -= to_write;
	}

	cleanup_rc = wait_until_ready(dev, K_MSEC(1));
	if (cleanup_rc < 0) {
		LOG_ERR("%s: final WIP wait failed: %d", dev->name, cleanup_rc);
	}
	if (rc == 0) {
		rc = cleanup_rc;
	}

	if (buffer_enabled && cleanup_rc == 0) {
		cleanup_rc = m95p32_set_buffer_mode(dev, false);
		if (cleanup_rc < 0) {
			LOG_ERR("%s: failed to disable buffer mode: %d", dev->name, cleanup_rc);
		}
		if (rc == 0) {
			rc = cleanup_rc;
		}
	}

	return rc;
}

static int m95p32_ex_op(const struct device *dev, uint16_t code, const uintptr_t in, void *out)
{
	switch (code) {
	case FLASH_M95P32_EX_OP_READ_ID_PAGE:
		return m95p32_ex_op_read_id_page(
			dev, (const struct flash_m95p32_ex_op_read_id_page_in *)in, out);
	case FLASH_M95P32_EX_OP_WRITE_ID_PAGE:
		return m95p32_ex_op_write_id_page(
			dev, (const struct flash_m95p32_ex_op_write_id_page_in *)in);
	case FLASH_M95P32_EX_OP_PAGE_PROGRAM_WITH_BUFFER_LOAD:
		return m95p32_ex_op_page_program_with_buffer_load(
			dev,
			(const struct flash_m95p32_ex_op_page_program_with_buffer_load_in *)in);
	default:
		return -ENOTSUP;
	}
}

static struct flash_mspi_nor_quirks flash_quirks_st_m95p32 = {
	.ex_op = m95p32_ex_op,
};

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_MSPI_NOR_QUIRKS_M95P32_H_ */
