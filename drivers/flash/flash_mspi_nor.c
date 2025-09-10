/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_mspi_nor

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "flash_mspi_nor.h"
#include "flash_mspi_nor_sfdp.h"

LOG_MODULE_REGISTER(flash_mspi_nor, CONFIG_FLASH_LOG_LEVEL);

#define XIP_DEV_CFG_MASK (MSPI_DEVICE_CONFIG_CMD_LEN | \
			  MSPI_DEVICE_CONFIG_ADDR_LEN | \
			  MSPI_DEVICE_CONFIG_READ_CMD | \
			  MSPI_DEVICE_CONFIG_WRITE_CMD | \
			  MSPI_DEVICE_CONFIG_RX_DUMMY | \
			  MSPI_DEVICE_CONFIG_TX_DUMMY)

#define NON_XIP_DEV_CFG_MASK (MSPI_DEVICE_CONFIG_ALL & ~XIP_DEV_CFG_MASK)

static void set_up_xfer(const struct device *dev, enum mspi_xfer_direction dir);
static int perform_xfer(const struct device *dev,
			uint8_t cmd, bool mem_access);
static int cmd_rdsr(const struct device *dev, uint8_t op_code, uint8_t *sr);
static int wait_until_ready(const struct device *dev, k_timeout_t poll_period);
static int cmd_wren(const struct device *dev);
static int cmd_wrsr(const struct device *dev, uint8_t op_code,
		    uint8_t sr_cnt, uint8_t *sr);

#include "flash_mspi_nor_quirks.h"

static void set_up_xfer(const struct device *dev, enum mspi_xfer_direction dir)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;

	memset(&dev_data->xfer, 0, sizeof(dev_data->xfer));
	memset(&dev_data->packet, 0, sizeof(dev_data->packet));

	dev_data->xfer.xfer_mode  = MSPI_PIO;
	dev_data->xfer.packets    = &dev_data->packet;
	dev_data->xfer.num_packet = 1;
	dev_data->xfer.timeout    = dev_config->transfer_timeout;

	dev_data->packet.dir = dir;
}

static void set_up_xfer_with_addr(const struct device *dev,
				  enum mspi_xfer_direction dir,
				  uint32_t addr)
{
	struct flash_mspi_nor_data *dev_data = dev->data;

	set_up_xfer(dev, dir);
	dev_data->xfer.addr_length = dev_data->cmd_info.uses_4byte_addr
				   ? 4 : 3;
	dev_data->packet.address = addr;
}

static uint16_t get_extended_command(const struct device *dev,
				     uint8_t cmd)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	uint8_t cmd_extension = cmd;

	if (dev_data->cmd_info.cmd_extension == CMD_EXTENSION_INVERSE) {
		cmd_extension = ~cmd_extension;
	}

	return ((uint16_t)cmd << 8) | cmd_extension;
}

static int perform_xfer(const struct device *dev,
			uint8_t cmd, bool mem_access)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	const struct mspi_dev_cfg *cfg = NULL;
	int rc;

	if (dev_data->cmd_info.cmd_extension != CMD_EXTENSION_NONE &&
	    dev_data->in_target_io_mode) {
		dev_data->xfer.cmd_length = 2;
		dev_data->packet.cmd = get_extended_command(dev, cmd);
	} else {
		dev_data->xfer.cmd_length = 1;
		dev_data->packet.cmd = cmd;
	}

	if (dev_config->multi_io_cmd ||
	    dev_config->mspi_nor_cfg.io_mode == MSPI_IO_MODE_SINGLE) {
		/* If multiple IO lines are used in all the transfer phases
		 * or in none of them, there's no need to switch the IO mode.
		 */
	} else if (mem_access) {
		/* For commands accessing the flash memory (read and program),
		 * ensure that the target IO mode is active.
		 */
		if (!dev_data->in_target_io_mode) {
			cfg = &dev_config->mspi_nor_cfg;
		}
	} else {
		/* For all other commands, switch to Single IO mode if a given
		 * command needs the data or address phase and in the target IO
		 * mode multiple IO lines are used in these phases.
		 */
		if (dev_data->in_target_io_mode) {
			if (dev_data->packet.num_bytes != 0 ||
			    (dev_data->xfer.addr_length != 0 &&
			     !dev_config->single_io_addr)) {
				/* Only the IO mode is to be changed, so the
				 * initial configuration structure can be used
				 * for this operation.
				 */
				cfg = &dev_config->mspi_nor_init_cfg;
			}
		}
	}

	if (cfg) {
		rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
				     MSPI_DEVICE_CONFIG_IO_MODE, cfg);
		if (rc < 0) {
			LOG_ERR("%s: dev_config() failed: %d", __func__, rc);
			return rc;
		}

		dev_data->in_target_io_mode = mem_access;
	}

	rc = mspi_transceive(dev_config->bus, &dev_config->mspi_id,
			     &dev_data->xfer);
	if (rc < 0) {
		LOG_ERR("%s: transceive() failed: %d", __func__, rc);
		return rc;
	}

	return 0;
}

static int cmd_rdsr(const struct device *dev, uint8_t op_code, uint8_t *sr)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	set_up_xfer(dev, MSPI_RX);
	if (dev_data->in_target_io_mode) {
		dev_data->xfer.rx_dummy    = dev_data->cmd_info.rdsr_dummy;
		dev_data->xfer.addr_length = dev_data->cmd_info.rdsr_addr_4
					   ? 4 : 0;
	}
	dev_data->packet.num_bytes = sizeof(uint8_t);
	dev_data->packet.data_buf  = sr;
	rc = perform_xfer(dev, op_code, false);
	if (rc < 0) {
		LOG_ERR("%s 0x%02x failed: %d", __func__, op_code, rc);
		return rc;
	}

	return 0;
}

static int wait_until_ready(const struct device *dev, k_timeout_t poll_period)
{
	int rc;
	uint8_t status_reg;

	while (true) {
		rc = cmd_rdsr(dev, SPI_NOR_CMD_RDSR, &status_reg);
		if (rc < 0) {
			LOG_ERR("%s - status xfer failed: %d", __func__, rc);
			return rc;
		}

		if (!(status_reg & SPI_NOR_WIP_BIT)) {
			break;
		}

		k_sleep(poll_period);
	}

	return 0;
}

static int cmd_wren(const struct device *dev)
{
	int rc;

	set_up_xfer(dev, MSPI_TX);
	rc = perform_xfer(dev, SPI_NOR_CMD_WREN, false);
	if (rc < 0) {
		LOG_ERR("%s failed: %d", __func__, rc);
		return rc;
	}

	return 0;
}

static int cmd_wrsr(const struct device *dev, uint8_t op_code,
		    uint8_t sr_cnt, uint8_t *sr)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	rc = cmd_wren(dev);
	if (rc < 0) {
		return rc;
	}

	set_up_xfer(dev, MSPI_TX);
	dev_data->packet.num_bytes = sr_cnt;
	dev_data->packet.data_buf  = sr;
	rc = perform_xfer(dev, op_code, false);
	if (rc < 0) {
		LOG_ERR("%s 0x%02x failed: %d", __func__, op_code, rc);
		return rc;
	}

	rc = wait_until_ready(dev, K_USEC(1));
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int acquire(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	k_sem_take(&dev_data->acquired, K_FOREVER);

	rc = pm_device_runtime_get(dev_config->bus);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
	} else {
		enum mspi_dev_cfg_mask mask;

		if (dev_config->multiperipheral_bus) {
			mask = NON_XIP_DEV_CFG_MASK;
		} else {
			mask = MSPI_DEVICE_CONFIG_NONE;
		}

		/* This acquires the MSPI controller and reconfigures it
		 * if needed for the flash device.
		 */
		rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
				     mask, &dev_config->mspi_nor_cfg);
		if (rc < 0) {
			LOG_ERR("mspi_dev_config() failed: %d", rc);
		} else {
			if (dev_config->multiperipheral_bus) {
				dev_data->in_target_io_mode = true;
			}

			return 0;
		}

		(void)pm_device_runtime_put(dev_config->bus);
	}

	k_sem_give(&dev_data->acquired);
	return rc;
}

static void release(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;

	/* This releases the MSPI controller. */
	(void)mspi_get_channel_status(dev_config->bus, 0);

	(void)pm_device_runtime_put(dev_config->bus);

	k_sem_give(&dev_data->acquired);
}

static inline uint32_t dev_flash_size(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;

	return dev_config->flash_size;
}

static inline uint16_t dev_page_size(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;

	return dev_config->page_size;
}

static inline
const struct jesd216_erase_type *dev_erase_types(const struct device *dev)
{
	struct flash_mspi_nor_data *dev_data = dev->data;

	return dev_data->erase_types;
}

static uint8_t get_rx_dummy(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;

	/* If the number of RX dummy cycles is specified in dts, use that value. */
	if (dev_config->rx_dummy_specified) {
		return dev_config->mspi_nor_cfg.rx_dummy;
	}

	/* Since it's not yet possible to specify mode bits with MSPI API,
	 * treat mode bit cycles as just dummy.
	 */
	return dev_data->cmd_info.read_mode_bit_cycles +
	       dev_data->cmd_info.read_dummy_cycles;
}

static int api_read(const struct device *dev, off_t addr, void *dest,
		    size_t size)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	const uint32_t flash_size = dev_flash_size(dev);
	int rc;

	if (size == 0) {
		return 0;
	}

	if ((addr < 0) || ((addr + size) > flash_size)) {
		return -EINVAL;
	}

	rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	set_up_xfer_with_addr(dev, MSPI_RX, addr);
	dev_data->xfer.rx_dummy = get_rx_dummy(dev);
	dev_data->packet.data_buf  = dest;
	dev_data->packet.num_bytes = size;
	rc = perform_xfer(dev, dev_data->cmd_info.read_cmd, true);

	release(dev);

	if (rc < 0) {
		LOG_ERR("Read xfer failed: %d", rc);
		return rc;
	}

	return 0;
}

static int api_write(const struct device *dev, off_t addr, const void *src,
		     size_t size)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	const uint32_t flash_size = dev_flash_size(dev);
	const uint16_t page_size = dev_page_size(dev);
	int rc;

	if (size == 0) {
		return 0;
	}

	if ((addr < 0) || ((addr + size) > flash_size)) {
		return -EINVAL;
	}

	rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	while (size > 0) {
		/* Split write into parts, each within one page only. */
		uint16_t page_offset = (uint16_t)(addr % page_size);
		uint16_t page_left = page_size - page_offset;
		uint16_t to_write = (uint16_t)MIN(size, page_left);

		if (cmd_wren(dev) < 0) {
			break;
		}

		set_up_xfer_with_addr(dev, MSPI_TX, addr);
		dev_data->packet.data_buf  = (uint8_t *)src;
		dev_data->packet.num_bytes = to_write;
		rc = perform_xfer(dev, dev_data->cmd_info.pp_cmd, true);
		if (rc < 0) {
			LOG_ERR("Page program xfer failed: %d", rc);
			break;
		}

		addr += to_write;
		src   = (const uint8_t *)src + to_write;
		size -= to_write;

		rc = wait_until_ready(dev, K_MSEC(1));
		if (rc < 0) {
			break;
		}
	}

	release(dev);

	return rc;
}

static const struct jesd216_erase_type *find_best_erase_type(
	const struct device *dev, off_t addr, size_t size)
{
	const struct jesd216_erase_type *erase_types = dev_erase_types(dev);
	const struct jesd216_erase_type *best_et = NULL;

	for (int i = 0; i < JESD216_NUM_ERASE_TYPES; ++i) {
		const struct jesd216_erase_type *et = &erase_types[i];

		if ((et->exp != 0)
		    && SPI_NOR_IS_ALIGNED(addr, et->exp)
		    && (size >= BIT(et->exp))
		    && ((best_et == NULL) || (et->exp > best_et->exp))) {
			best_et = et;
		}
	}

	return best_et;
}

static int api_erase(const struct device *dev, off_t addr, size_t size)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	const uint32_t flash_size = dev_flash_size(dev);
	int rc = 0;

	if ((addr < 0) || ((addr + size) > flash_size)) {
		return -EINVAL;
	}

	if (!SPI_NOR_IS_SECTOR_ALIGNED(addr)) {
		return -EINVAL;
	}

	if ((size % SPI_NOR_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	while (size > 0) {
		if (cmd_wren(dev) < 0) {
			break;
		}

		if (size == flash_size) {
			/* Chip erase. */
			set_up_xfer(dev, MSPI_TX);
			rc = perform_xfer(dev, SPI_NOR_CMD_CE, false);

			size -= flash_size;
		} else {
			const struct jesd216_erase_type *best_et =
				find_best_erase_type(dev, addr, size);

			if (best_et != NULL) {
				set_up_xfer_with_addr(dev, MSPI_TX, addr);
				rc = perform_xfer(dev, best_et->cmd, false);

				addr += BIT(best_et->exp);
				size -= BIT(best_et->exp);
			} else {
				LOG_ERR("Can't erase %zu at 0x%lx",
					size, (long)addr);
				rc = -EINVAL;
				break;
			}
		}
		if (rc < 0) {
			LOG_ERR("Erase command 0x%02x xfer failed: %d",
				dev_data->packet.cmd, rc);
			break;
		}

		rc = wait_until_ready(dev, K_MSEC(1));
		if (rc < 0) {
			break;
		}
	}

	release(dev);

	return rc;
}

static int api_get_size(const struct device *dev, uint64_t *size)
{
	*size = dev_flash_size(dev);
	return 0;
}

static const
struct flash_parameters *api_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters parameters = {
		.write_block_size = 1,
		.erase_value = 0xff,
	};

	return &parameters;
}

static int sfdp_read(const struct device *dev, off_t addr, void *dest,
		     size_t size)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	set_up_xfer(dev, MSPI_RX);
	if (dev_data->in_target_io_mode) {
		dev_data->xfer.rx_dummy    = dev_data->cmd_info.sfdp_dummy_20
					   ? 20 : 8;
		dev_data->xfer.addr_length = dev_data->cmd_info.sfdp_addr_4
					   ? 4 : 3;
	} else {
		dev_data->xfer.rx_dummy    = 8;
		dev_data->xfer.addr_length = 3;
	}
	dev_data->packet.address   = addr;
	dev_data->packet.data_buf  = dest;
	dev_data->packet.num_bytes = size;
	rc = perform_xfer(dev, JESD216_CMD_READ_SFDP, false);
	if (rc < 0) {
		LOG_ERR("Read SFDP xfer failed: %d", rc);
	}

	return rc;
}

static int read_jedec_id(const struct device *dev, uint8_t *id)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	set_up_xfer(dev, MSPI_RX);
	if (dev_data->in_target_io_mode) {
		dev_data->xfer.rx_dummy    = dev_data->cmd_info.rdid_dummy;
		dev_data->xfer.addr_length = dev_data->cmd_info.rdid_addr_4
					   ? 4 : 0;
	}
	dev_data->packet.data_buf  = id;
	dev_data->packet.num_bytes = JESD216_READ_ID_LEN;
	rc = perform_xfer(dev, SPI_NOR_CMD_RDID, false);
	if (rc < 0) {
		LOG_ERR("Read JEDEC ID failed: %d", rc);
	}

	return rc;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void api_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;

	*layout = &dev_config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if defined(CONFIG_FLASH_JESD216_API)
static int api_sfdp_read(const struct device *dev, off_t addr, void *dest,
			 size_t size)
{
	int rc;

	if (size == 0) {
		return 0;
	}

	rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	rc = sfdp_read(dev, addr, dest, size);

	release(dev);

	return rc;
}

static int api_read_jedec_id(const struct device *dev, uint8_t *id)
{
	int rc = acquire(dev);
	if (rc < 0) {
		return rc;
	}

	rc = read_jedec_id(dev, id);

	release(dev);

	return rc;
}
#endif /* CONFIG_FLASH_JESD216_API  */

static int dev_pm_action_cb(const struct device *dev,
			    enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_RESUME:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int quad_enable_set(const struct device *dev, bool enable)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	uint8_t op_code;
	uint8_t qe_bit;
	uint8_t status_reg;
	uint8_t payload_len;
	uint8_t payload[2];
	int rc;

	switch (dev_data->switch_info.quad_enable_req) {
	case JESD216_DW15_QER_VAL_S1B6:
		op_code = SPI_NOR_CMD_RDSR;
		qe_bit = BIT(6);
		break;
	case JESD216_DW15_QER_VAL_S2B7:
		/* Use special Read status register 2 instruction. */
		op_code = 0x3F;
		qe_bit = BIT(7);
		break;
	case JESD216_DW15_QER_VAL_S2B1v1:
	case JESD216_DW15_QER_VAL_S2B1v4:
	case JESD216_DW15_QER_VAL_S2B1v5:
	case JESD216_DW15_QER_VAL_S2B1v6:
		op_code = SPI_NOR_CMD_RDSR2;
		qe_bit = BIT(1);
		break;
	default:
		LOG_ERR("Unknown Quad Enable Requirement: %u",
			dev_data->switch_info.quad_enable_req);
		return -ENOTSUP;
	}

	rc = cmd_rdsr(dev, op_code, &status_reg);
	if (rc < 0) {
		return rc;
	}

	if (((status_reg & qe_bit) != 0) == enable) {
		/* Nothing to do, the QE bit is already set properly. */
		return 0;
	}

	status_reg ^= qe_bit;

	switch (dev_data->switch_info.quad_enable_req) {
	default:
	case JESD216_DW15_QER_VAL_S1B6:
		payload_len = 1;
		op_code = SPI_NOR_CMD_WRSR;
		break;
	case JESD216_DW15_QER_VAL_S2B7:
		payload_len = 1;
		/* Use special Write status register 2 instruction. */
		op_code = 0x3E;
		break;
	case JESD216_DW15_QER_VAL_S2B1v1:
	case JESD216_DW15_QER_VAL_S2B1v4:
	case JESD216_DW15_QER_VAL_S2B1v5:
		payload_len = 2;
		op_code = SPI_NOR_CMD_WRSR;
		break;
	case JESD216_DW15_QER_VAL_S2B1v6:
		payload_len = 1;
		op_code = SPI_NOR_CMD_WRSR2;
		break;
	}

	if (payload_len == 1) {
		payload[0] = status_reg;
	} else {
		payload[1] = status_reg;

		/* When the Write Status command is to be sent with two data
		 * bytes (this is the case for S2B1v1, S2B1v4, and S2B1v5 QER
		 * values), the first status register needs to be read and
		 * sent as the first byte, so that its value is not modified.
		 */
		rc = cmd_rdsr(dev, SPI_NOR_CMD_RDSR, &payload[0]);
		if (rc < 0) {
			return rc;
		}
	}

	rc = cmd_wrsr(dev, op_code, payload_len, payload);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int octal_enable_set(const struct device *dev, bool enable)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	uint8_t op_code;
	uint8_t oe_bit;
	uint8_t status_reg;
	int rc;

	if (dev_data->switch_info.octal_enable_req != OCTAL_ENABLE_REQ_S2B3) {
		LOG_ERR("Unknown Octal Enable Requirement: %u",
			dev_data->switch_info.octal_enable_req);
		return -ENOTSUP;
	}

	oe_bit = BIT(3);

	/* Use special Read status register 2 instruction 0x65 with one address
	 * byte 0x02 and one dummy byte.
	 */
	op_code = 0x65;
	set_up_xfer(dev, MSPI_RX);
	dev_data->xfer.rx_dummy    = 8;
	dev_data->xfer.addr_length = 1;
	dev_data->packet.address   = 0x02;
	dev_data->packet.num_bytes = sizeof(uint8_t);
	dev_data->packet.data_buf  = &status_reg;
	rc = perform_xfer(dev, op_code, false);
	if (rc < 0) {
		LOG_ERR("cmd_rdsr 0x%02x failed: %d", op_code, rc);
		return rc;
	}

	if (((status_reg & oe_bit) != 0) == enable) {
		/* Nothing to do, the OE bit is already set properly. */
		return 0;
	}

	status_reg ^= oe_bit;

	/* Use special Write status register 2 instruction to clear the bit. */
	op_code = (status_reg & oe_bit) ? SPI_NOR_CMD_WRSR2 : 0x3E;
	rc = cmd_wrsr(dev, op_code, 1, &status_reg);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int enter_4byte_addressing_mode(const struct device *dev)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	if (dev_data->switch_info.enter_4byte_addr == ENTER_4BYTE_ADDR_06_B7) {
		rc = cmd_wren(dev);
		if (rc < 0) {
			return rc;
		}
	}

	set_up_xfer(dev, MSPI_TX);
	rc = perform_xfer(dev, 0xB7, false);
	if (rc < 0) {
		LOG_ERR("Command 0xB7 failed: %d", rc);
		return rc;
	}

	return 0;
}

static int switch_to_target_io_mode(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	enum mspi_io_mode io_mode = dev_config->mspi_nor_cfg.io_mode;
	int rc = 0;

	if (dev_data->switch_info.quad_enable_req != JESD216_DW15_QER_VAL_NONE) {
		bool quad_needed = io_mode == MSPI_IO_MODE_QUAD_1_1_4 ||
				   io_mode == MSPI_IO_MODE_QUAD_1_4_4;

		rc = quad_enable_set(dev, quad_needed);
		if (rc < 0) {
			LOG_ERR("Failed to modify Quad Enable bit: %d", rc);
			return rc;
		}
	}

	if (dev_data->switch_info.octal_enable_req != OCTAL_ENABLE_REQ_NONE) {
		bool octal_needed = io_mode == MSPI_IO_MODE_OCTAL_1_1_8 ||
				    io_mode == MSPI_IO_MODE_OCTAL_1_8_8;

		rc = octal_enable_set(dev, octal_needed);
		if (rc < 0) {
			LOG_ERR("Failed to modify Octal Enable bit: %d", rc);
			return rc;
		}
	}

	if (dev_data->switch_info.enter_4byte_addr != ENTER_4BYTE_ADDR_NONE) {
		rc = enter_4byte_addressing_mode(dev);
		if (rc < 0) {
			LOG_ERR("Failed to enter 4-byte addressing mode: %d", rc);
			return rc;
		}
	}

	if (dev_config->quirks != NULL &&
	    dev_config->quirks->post_switch_mode != NULL) {
		rc = dev_config->quirks->post_switch_mode(dev);
		if (rc < 0) {
			return rc;
		}
	}

	return mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
			       NON_XIP_DEV_CFG_MASK,
			       &dev_config->mspi_nor_cfg);
}

#if defined(WITH_SUPPLY_GPIO)
static int power_supply(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	int rc;

	if (!gpio_is_ready_dt(&dev_config->supply)) {
		LOG_ERR("Device %s is not ready",
			dev_config->supply.port->name);
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&dev_config->supply, GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		LOG_ERR("Failed to activate power supply GPIO: %d", rc);
		return -EIO;
	}

	return 0;
}
#endif

#if defined(WITH_RESET_GPIO)
static int gpio_reset(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	int rc;

	if (!gpio_is_ready_dt(&dev_config->reset)) {
		LOG_ERR("Device %s is not ready",
			dev_config->reset.port->name);
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&dev_config->reset, GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		LOG_ERR("Failed to activate RESET: %d", rc);
		return -EIO;
	}

	if (dev_config->reset_pulse_us != 0) {
		k_busy_wait(dev_config->reset_pulse_us);
	}

	rc = gpio_pin_set_dt(&dev_config->reset, 0);
	if (rc < 0) {
		LOG_ERR("Failed to deactivate RESET: %d", rc);
		return -EIO;
	}

	return 0;
}
#endif

#if defined(WITH_SOFT_RESET)
static int soft_reset_66_99(const struct device *dev)
{
	int rc;

	set_up_xfer(dev, MSPI_TX);
	rc = perform_xfer(dev, SPI_NOR_CMD_RESET_EN, false);
	if (rc < 0) {
		LOG_ERR("CMD_RESET_EN failed: %d", rc);
		return rc;
	}

	set_up_xfer(dev, MSPI_TX);
	rc = perform_xfer(dev, SPI_NOR_CMD_RESET_MEM, false);
	if (rc < 0) {
		LOG_ERR("CMD_RESET_MEM failed: %d", rc);
		return rc;
	}

	return 0;
}

static int soft_reset(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	/* If the flash may expect commands sent in multi-line mode,
	 * send additionally the reset sequence this way.
	 */
	if (dev_config->multi_io_cmd) {
		rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
				     MSPI_DEVICE_CONFIG_IO_MODE,
				     &dev_config->mspi_nor_cfg);
		if (rc < 0) {
			LOG_ERR("%s: dev_config() failed: %d", __func__, rc);
			return rc;
		}

		dev_data->in_target_io_mode = true;

		rc = soft_reset_66_99(dev);
		if (rc < 0) {
			return rc;
		}

		rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
				     MSPI_DEVICE_CONFIG_IO_MODE,
				     &dev_config->mspi_nor_init_cfg);
		if (rc < 0) {
			LOG_ERR("%s: dev_config() failed: %d", __func__, rc);
			return rc;
		}

		dev_data->in_target_io_mode = false;
	}

	rc = soft_reset_66_99(dev);
	if (rc < 0) {
		return rc;
	}

	return 0;
}
#endif /* WITH_SOFT_RESET */

static int flash_chip_init(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	uint8_t id[JESD216_READ_ID_LEN] = {0};
	uint16_t dts_cmd = 0;
	uint32_t sfdp_signature;
	bool flash_reset = false;
	int rc;

	rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
			     MSPI_DEVICE_CONFIG_ALL,
			     &dev_config->mspi_nor_init_cfg);
	if (rc < 0) {
		LOG_ERR("%s: dev_config() failed: %d", __func__, rc);
		return rc;
	}

	dev_data->in_target_io_mode = false;

#if defined(WITH_SUPPLY_GPIO)
	if (dev_config->supply.port) {
		rc = power_supply(dev);
		if (rc < 0) {
			return rc;
		}

		flash_reset = true;
	}
#endif

#if defined(WITH_RESET_GPIO)
	if (dev_config->reset.port) {
		rc = gpio_reset(dev);
		if (rc < 0) {
			return rc;
		}

		flash_reset = true;
	}
#endif

#if defined(WITH_SOFT_RESET)
	if (dev_config->initial_soft_reset) {
		rc = soft_reset(dev);
		if (rc < 0) {
			return rc;
		}

		flash_reset = true;
	}
#endif

	if (flash_reset && dev_config->reset_recovery_us != 0) {
		k_busy_wait(dev_config->reset_recovery_us);
	}

	if (dev_config->quirks != NULL &&
	    dev_config->quirks->pre_init != NULL) {
		rc = dev_config->quirks->pre_init(dev);
	}

	/* Allow users to specify commands for Read and Page Program operations
	 * through dts to override what was taken from SFDP and perhaps altered
	 * in the pre_init quirk. Also the number of dummy cycles for the Read
	 * operation can be overridden this way, see get_rx_dummy().
	 */
	if (dev_config->mspi_nor_cfg.read_cmd != 0) {
		dts_cmd = (uint16_t)dev_config->mspi_nor_cfg.read_cmd;
		if (dev_config->mspi_nor_cfg.cmd_length > 1) {
			dev_data->cmd_info.read_cmd = (uint8_t)(dts_cmd >> 8);
		} else {
			dev_data->cmd_info.read_cmd = (uint8_t)dts_cmd;
		}
	}
	if (dev_config->mspi_nor_cfg.write_cmd != 0) {
		dts_cmd = (uint16_t)dev_config->mspi_nor_cfg.write_cmd;
		if (dev_config->mspi_nor_cfg.cmd_length > 1) {
			dev_data->cmd_info.pp_cmd = (uint8_t)(dts_cmd >> 8);
		} else {
			dev_data->cmd_info.pp_cmd = (uint8_t)dts_cmd;
		}
	}
	if (dts_cmd != 0) {
		if (dev_config->mspi_nor_cfg.cmd_length <= 1) {
			dev_data->cmd_info.cmd_extension = CMD_EXTENSION_NONE;
		} else if ((dts_cmd & 0xFF) == ((dts_cmd >> 8) & 0xFF)) {
			dev_data->cmd_info.cmd_extension = CMD_EXTENSION_SAME;
		} else {
			dev_data->cmd_info.cmd_extension = CMD_EXTENSION_INVERSE;
		}
	}

	if (dev_config->jedec_id_specified) {
		rc = read_jedec_id(dev, id);
		if (rc < 0) {
			LOG_ERR("Failed to read JEDEC ID: %d", rc);
			return rc;
		}

		if (memcmp(id, dev_config->jedec_id, sizeof(id)) != 0) {
			LOG_ERR("JEDEC ID mismatch, read: %02x %02x %02x, "
				"expected: %02x %02x %02x",
				id[0], id[1], id[2],
				dev_config->jedec_id[0],
				dev_config->jedec_id[1],
				dev_config->jedec_id[2]);
			return -ENODEV;
		}
	}

	rc = switch_to_target_io_mode(dev);
	if (rc < 0) {
		LOG_ERR("Failed to switch to target io mode: %d", rc);
		return rc;
	}

	dev_data->in_target_io_mode = true;

	if (IS_ENABLED(CONFIG_FLASH_MSPI_NOR_USE_SFDP)) {
		/* Read the SFDP signature to test if communication with
		 * the flash chip can be successfully performed after switching
		 * to target IO mode.
		 */
		rc = sfdp_read(dev, 0, &sfdp_signature, sizeof(sfdp_signature));
		if (rc < 0) {
			LOG_ERR("Failed to read SFDP signature: %d", rc);
			return rc;
		}

		if (sfdp_signature != JESD216_SFDP_MAGIC) {
			LOG_ERR("SFDP signature mismatch: %08x, expected: %08x",
				sfdp_signature, JESD216_SFDP_MAGIC);
			return -ENODEV;
		}
	}

#if defined(CONFIG_MSPI_XIP)
	/* Enable XIP access for this chip if specified so in DT. */
	if (dev_config->xip_cfg.enable) {
		struct mspi_dev_cfg mspi_cfg = {
			.addr_length = dev_data->cmd_info.uses_4byte_addr
				     ? 4 : 3,
			.rx_dummy = get_rx_dummy(dev),
		};

		if (dev_data->cmd_info.cmd_extension != CMD_EXTENSION_NONE) {
			mspi_cfg.cmd_length = 2;
			mspi_cfg.read_cmd = get_extended_command(dev,
				dev_data->cmd_info.read_cmd);
			mspi_cfg.write_cmd = get_extended_command(dev,
				dev_data->cmd_info.pp_cmd);
		} else {
			mspi_cfg.cmd_length = 1;
			mspi_cfg.read_cmd = dev_data->cmd_info.read_cmd;
			mspi_cfg.write_cmd = dev_data->cmd_info.pp_cmd;
		}

		rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
				     XIP_DEV_CFG_MASK, &mspi_cfg);
		if (rc < 0) {
			LOG_ERR("Failed to configure controller for XIP: %d",
				rc);
			return rc;
		}

		rc = mspi_xip_config(dev_config->bus, &dev_config->mspi_id,
				     &dev_config->xip_cfg);
		if (rc < 0) {
			LOG_ERR("Failed to enable XIP: %d", rc);
			return rc;
		}
	}
#endif

	return 0;
}

static int drv_init(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;

	if (!device_is_ready(dev_config->bus)) {
		LOG_ERR("Device %s is not ready", dev_config->bus->name);
		return -ENODEV;
	}

	memcpy(dev_data->erase_types, dev_config->default_erase_types,
	       sizeof(dev_data->erase_types));
	dev_data->cmd_info = dev_config->default_cmd_info;
	dev_data->switch_info = dev_config->default_switch_info;

	rc = pm_device_runtime_get(dev_config->bus);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
		return rc;
	}

	rc = flash_chip_init(dev);

	/* Release the MSPI controller - it was acquired by the call to
	 * mspi_dev_config() in flash_chip_init().
	 */
	(void)mspi_get_channel_status(dev_config->bus, 0);

	(void)pm_device_runtime_put(dev_config->bus);

	if (rc < 0) {
		return rc;
	}

	if (dev_data->cmd_info.read_cmd == 0) {
		LOG_ERR("Read command not defined for %s, "
			"use \"read-command\" property to specify it.",
			dev->name);
		return -EINVAL;
	}

	if (dev_data->cmd_info.pp_cmd == 0) {
		LOG_ERR("Page Program command not defined for %s, "
			"use \"write-command\" property to specify it.",
			dev->name);
		return -EINVAL;
	}

	LOG_DBG("%s - size: %u, page %u%s",
		dev->name, dev_flash_size(dev), dev_page_size(dev),
		dev_data->cmd_info.uses_4byte_addr ? ", 4-byte addressing" : "");
	LOG_DBG("- read command: 0x%02X with %u mode bit and %u dummy cycles",
		dev_data->cmd_info.read_cmd,
		dev_data->cmd_info.read_mode_bit_cycles,
		dev_data->cmd_info.read_dummy_cycles);
	LOG_DBG("- page program command: 0x%02X",
		dev_data->cmd_info.pp_cmd);
	LOG_DBG("- erase types:");
	for (int i = 0; i < JESD216_NUM_ERASE_TYPES; ++i) {
		const struct jesd216_erase_type *et = &dev_erase_types(dev)[i];

		if (et->exp != 0) {
			LOG_DBG("  - command: 0x%02X, size: %lu",
				et->cmd, BIT(et->exp));
		}
	}

	k_sem_init(&dev_data->acquired, 1, K_SEM_MAX_LIMIT);

	return pm_device_driver_init(dev, dev_pm_action_cb);
}

static DEVICE_API(flash, drv_api) = {
	.read = api_read,
	.write = api_write,
	.erase = api_erase,
	.get_size = api_get_size,
	.get_parameters = api_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = api_page_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = api_sfdp_read,
	.read_jedec_id = api_read_jedec_id,
#endif
};

#define FLASH_INITIAL_CONFIG(inst)					\
{									\
	.ce_num = DT_INST_PROP_OR(inst, mspi_hardware_ce_num, 0),	\
	.freq = MIN(DT_INST_PROP(inst, mspi_max_frequency), MHZ(50)),	\
	.io_mode = MSPI_IO_MODE_SINGLE,					\
	.data_rate = MSPI_DATA_RATE_SINGLE,				\
	.cpp = MSPI_CPP_MODE_0,						\
	.endian = MSPI_XFER_BIG_ENDIAN,					\
	.ce_polarity = MSPI_CE_ACTIVE_LOW,				\
	.dqs_enable = false,						\
}

#define FLASH_QUIRKS(inst) FLASH_MSPI_QUIRKS_GET(DT_DRV_INST(inst))

#define IO_MODE_FLAGS(io_mode) \
	.multi_io_cmd = (io_mode == MSPI_IO_MODE_DUAL || \
			 io_mode == MSPI_IO_MODE_QUAD || \
			 io_mode == MSPI_IO_MODE_OCTAL || \
			 io_mode == MSPI_IO_MODE_HEX || \
			 io_mode == MSPI_IO_MODE_HEX_8_8_16 || \
			 io_mode == MSPI_IO_MODE_HEX_8_16_16), \
	.single_io_addr = (io_mode == MSPI_IO_MODE_DUAL_1_1_2 || \
			   io_mode == MSPI_IO_MODE_QUAD_1_1_4 || \
			   io_mode == MSPI_IO_MODE_OCTAL_1_1_8)

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
BUILD_ASSERT((CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE % 4096) == 0,
	"MSPI_NOR_FLASH_LAYOUT_PAGE_SIZE must be multiple of 4096");
#define FLASH_PAGE_LAYOUT_DEFINE(inst) \
	.layout = { \
		.pages_size = CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE, \
		.pages_count = FLASH_SIZE(inst) \
			     / CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE, \
	},
#define FLASH_PAGE_LAYOUT_CHECK(inst) \
BUILD_ASSERT((FLASH_SIZE(inst) % CONFIG_FLASH_MSPI_NOR_LAYOUT_PAGE_SIZE) == 0, \
	"MSPI_NOR_FLASH_LAYOUT_PAGE_SIZE incompatible with flash size, instance " #inst);
#else
#define FLASH_PAGE_LAYOUT_DEFINE(inst)
#define FLASH_PAGE_LAYOUT_CHECK(inst)
#endif

/* MSPI bus must be initialized before this device. */
#if (CONFIG_MSPI_INIT_PRIORITY < CONFIG_FLASH_INIT_PRIORITY)
#define INIT_PRIORITY CONFIG_FLASH_INIT_PRIORITY
#else
#define INIT_PRIORITY UTIL_INC(CONFIG_MSPI_INIT_PRIORITY)
#endif

#define FLASH_MSPI_NOR_INST(inst)						\
	SFDP_BUILD_ASSERTS(inst);						\
	PM_DEVICE_DT_INST_DEFINE(inst, dev_pm_action_cb);			\
	DEFAULT_ERASE_TYPES_DEFINE(inst);					\
	static struct flash_mspi_nor_data dev##inst##_data;			\
	static const struct flash_mspi_nor_config dev##inst##_config = {	\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),			\
		.flash_size = FLASH_SIZE(inst),					\
		.page_size = FLASH_PAGE_SIZE(inst),				\
		.mspi_id = MSPI_DEVICE_ID_DT_INST(inst),			\
		.mspi_nor_cfg = MSPI_DEVICE_CONFIG_DT_INST(inst),		\
		.mspi_nor_init_cfg = FLASH_INITIAL_CONFIG(inst),		\
	IF_ENABLED(CONFIG_MSPI_XIP,						\
		(.xip_cfg = MSPI_XIP_CONFIG_DT_INST(inst),))			\
	IF_ENABLED(WITH_SUPPLY_GPIO,						\
		(.supply = GPIO_DT_SPEC_INST_GET_OR(inst, supply_gpios, {0}),))	\
	IF_ENABLED(WITH_RESET_GPIO,						\
		(.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),	\
		 .reset_pulse_us = DT_INST_PROP_OR(inst, t_reset_pulse, 0)	\
				 / 1000,))					\
		.reset_recovery_us = DT_INST_PROP_OR(inst, t_reset_recovery, 0)	\
				   / 1000,					\
		.transfer_timeout = DT_INST_PROP(inst, transfer_timeout),	\
		FLASH_PAGE_LAYOUT_DEFINE(inst)					\
		.jedec_id = DT_INST_PROP_OR(inst, jedec_id, {0}),		\
		.quirks = FLASH_QUIRKS(inst),					\
		.default_erase_types = DEFAULT_ERASE_TYPES(inst),		\
		.default_cmd_info = DEFAULT_CMD_INFO(inst),			\
		.default_switch_info = DEFAULT_SWITCH_INFO(inst),		\
		.jedec_id_specified = DT_INST_NODE_HAS_PROP(inst, jedec_id),    \
		.rx_dummy_specified = DT_INST_NODE_HAS_PROP(inst, rx_dummy),    \
		.multiperipheral_bus = DT_PROP(DT_INST_BUS(inst),		\
					       software_multiperipheral),	\
		IO_MODE_FLAGS(DT_INST_ENUM_IDX(inst, mspi_io_mode)),		\
		.initial_soft_reset = DT_INST_PROP(inst, initial_soft_reset),	\
	};									\
	FLASH_PAGE_LAYOUT_CHECK(inst)						\
	DEVICE_DT_INST_DEFINE(inst,						\
		drv_init, PM_DEVICE_DT_INST_GET(inst),				\
		&dev##inst##_data, &dev##inst##_config,				\
		POST_KERNEL, INIT_PRIORITY,					\
		&drv_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MSPI_NOR_INST)
