/*
 * Copyright (c) 2025 Siemens Mobility GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_s25h_flash

#include "flash_mspi_infineon_s25h.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/device_mmio.h>

LOG_MODULE_REGISTER(flash_mspi_infineon_s25h, CONFIG_FLASH_LOG_LEVEL);

struct flash_mspi_infineon_s25h_cfg {
	DEVICE_MMIO_ROM;
	const struct device *bus;
	const struct pinctrl_dev_config *pinctrl;
	k_timeout_t reset_startup_duration;

	const struct mspi_dev_cfg mspi_dev_cfg;

	const struct flash_pages_layout page_layout;
	const struct flash_parameters parameters;

	const struct mspi_dev_id dev_id;
};

struct flash_mspi_infineon_s25h_data {
	struct mspi_dev_cfg mspi_dev_cfg;
	uint8_t read_jedec_cmd;
	uint8_t read_flash_cmd;
	uint8_t read_flash_dummy_cycles;
};

static int flash_mspi_infineon_s25h_prepare_mspi_bus(const struct device *dev)
{
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;
	struct flash_mspi_infineon_s25h_data *data = dev->data;

	return mspi_dev_config(config->bus, &config->dev_id,
			       MSPI_DEVICE_CONFIG_CE_NUM | MSPI_DEVICE_CONFIG_IO_MODE |
				       MSPI_DEVICE_CONFIG_CPP | MSPI_DEVICE_CONFIG_CE_POL |
				       MSPI_DEVICE_CONFIG_DQS | MSPI_DEVICE_CONFIG_DATA_RATE |
				       MSPI_DEVICE_CONFIG_ENDIAN,
			       &data->mspi_dev_cfg);
}

static int flash_mspi_infineon_s25h_reset(const struct device *dev)
{
	int ret = 0;
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;

	const struct mspi_xfer_packet reset_packets[] = {
		{
			.dir = MSPI_TX,
			.cmd = INF_MSPI_S25H_OPCODE_RESET_ENABLE,
			.num_bytes = 0,
		},
		{
			.dir = MSPI_TX,
			.cmd = INF_MSPI_S25H_OPCODE_RESET,
			.num_bytes = 0,
		}};

	struct mspi_xfer xfer = {
		INF_MSPI_S25H_DEFAULT_XFER_DATA,
		.rx_dummy = 0,
		.addr_length = 0,
		.num_packet = 2,
		.packets = reset_packets,
		.timeout = INF_MSPI_S25H_DEFAULT_MSPI_TIMEOUT,
	};

	ret = mspi_transceive(config->bus, &config->dev_id, &xfer);
	if (ret < 0) {
		return ret;
	}

	k_sleep(config->reset_startup_duration);

	return 0;
}

static int flash_mspi_infineon_s25h_set_writing_forbidden(const struct device *dev,
							  bool writing_forbidden)
{
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;
	uint8_t cmd = writing_forbidden ? INF_MSPI_S25H_OPCODE_WRITE_DISABLE
					: INF_MSPI_S25H_OPCODE_WRITE_ENABLE;
	const struct mspi_xfer_packet packet = {
		.dir = MSPI_TX,
		.cmd = cmd,
		.num_bytes = 0,
	};

	struct mspi_xfer xfer = {
		INF_MSPI_S25H_DEFAULT_XFER_DATA_SINGLE_CMD,
		.packets = &packet,
		.timeout = INF_MSPI_S25H_DEFAULT_MSPI_TIMEOUT,
	};

	return mspi_transceive(config->bus, &config->dev_id, &xfer);
}

static int flash_mspi_infineon_s25h_rw_any_register(const struct device *dev, uint32_t address,
						    uint8_t *value, uint32_t delay,
						    enum mspi_xfer_direction dir)
{
	int ret;
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;
	struct flash_mspi_infineon_s25h_data *dev_data = dev->data;
	uint32_t cmd;
	uint32_t rx_dummy;

	if (dir == MSPI_TX) {
		ret = flash_mspi_infineon_s25h_set_writing_forbidden(dev, false);
		if (ret < 0) {
			LOG_ERR("Error disabling write protection before changing configuration");
			return ret;
		}
	}

	if (dir == MSPI_RX) {
		cmd = INF_MSPI_S25H_OPCODE_READ_ANY_REGISTER;
		rx_dummy = delay;
	} else {
		cmd = INF_MSPI_S25H_OPCODE_WRITE_ANY_REGISTER;
		rx_dummy = 0;
	}

	const struct mspi_xfer_packet packet = {
		.dir = dir,
		.cmd = cmd,
		.num_bytes = 1,
		.data_buf = value,
		.address = address,
	};

	struct mspi_xfer xfer = {
		INF_MSPI_S25H_DEFAULT_XFER_DATA,
		.addr_length = dev_data->mspi_dev_cfg.addr_length,
		.rx_dummy = rx_dummy,
		.packets = &packet,
		.num_packet = 1,
		.timeout = INF_MSPI_S25H_DEFAULT_MSPI_TIMEOUT,
	};

	return mspi_transceive(config->bus, &config->dev_id, &xfer);
}

static int flash_mspi_infineon_s25h_is_write_protection_enabled(const struct device *dev,
								uint8_t *is_enabled)
{
	int ret = 0;
	uint8_t val = 0;

	ret = flash_mspi_infineon_s25h_rw_any_register(dev, INF_MSPI_S25H_ADDRESS_VOLATILE_STATUS_1,
						       &val, 0, MSPI_RX);
	if (ret < 0) {
		return ret;
	}
	*is_enabled = (val & INF_MSPI_S25H_STATUS_1_WRPGEN_BIT);
	return 0;
}

static int flash_mspi_infineon_s25h_wait_for_idle(const struct device *dev, uint32_t timeout_ms)
{
	int ret = 0;
	uint8_t status_1 = 0;
	uint32_t retries = timeout_ms / INF_MSPI_S25H_TIMEOUT_IDLE_RETRY_INTERVAL_MS;

	ret = flash_mspi_infineon_s25h_rw_any_register(dev, INF_MSPI_S25H_ADDRESS_VOLATILE_STATUS_1,
						       &status_1, 0, MSPI_RX);
	if (ret < 0) {
		return ret;
	}

	while ((status_1 & INF_MSPI_S25H_STATUS_1_RDYBSY_BIT) != 0 && retries > 0) {
		k_sleep(INF_MSPI_S25H_TIMEOUT_IDLE_RETRY_INTERVAL);
		ret = flash_mspi_infineon_s25h_rw_any_register(
			dev, INF_MSPI_S25H_ADDRESS_VOLATILE_STATUS_1, &status_1, 0, MSPI_RX);
		if (ret < 0) {
			return ret;
		}
		--retries;
	}

	if (retries == 0) {
		LOG_ERR("Waiting for flash to enter idle. Status 1 register: 0x%X", status_1);
		return -EIO;
	}

	return 0;
}

static int flash_mspi_infineon_s25h_read_jedec_id(const struct device *dev, uint8_t *buf)
{
	int ret = 0;
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;
	struct flash_mspi_infineon_s25h_data *data = dev->data;

	const struct mspi_xfer_packet packet = {
		.dir = MSPI_RX,
		.cmd = data->read_jedec_cmd,
		.num_bytes = 3,
		.data_buf = buf,
		.address = 0,
	};

	struct mspi_xfer xfer = {
		INF_MSPI_S25H_DEFAULT_XFER_DATA,
		.addr_length = 0,
		.rx_dummy = 0,
		.packets = &packet,
		.num_packet = 1,
		.timeout = INF_MSPI_S25H_DEFAULT_MSPI_TIMEOUT,
	};

	ret = mspi_transceive(config->bus, &config->dev_id, &xfer);
	if (ret < 0) {
		LOG_ERR("Error reading JEDEC id");
		return ret;
	}

	return 0;
}

static int flash_mspi_infineon_s25h_read(const struct device *dev, off_t addr, void *data,
					 size_t size)
{
	int ret = 0;
	bool requires_cleanup = false;
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;
	struct flash_mspi_infineon_s25h_data *dev_data = dev->data;

	/* The S25H allows for continuous read operations which happen by sending
	 * 0xAX after an address. The driver doesn't implement this which is why we
	 * don't want this and instead wait for 2 cycles. However since the flash
	 * pins could be in high impedance state from the MSPI controller after the
	 * address was sent an address ending with 0xA could put the flash into a
	 * continuous read mode. To prevent this the driver will read the jedec id,
	 * if the address wasn't aligned to prevent accidentally fulfilling the
	 * requirement.
	 */
	if (dev_data->mspi_dev_cfg.io_mode == MSPI_IO_MODE_QUAD && (addr % 16 != 0)) {
		requires_cleanup = true;
	}

	ret = flash_mspi_infineon_s25h_prepare_mspi_bus(dev);
	if (ret < 0) {
		LOG_ERR("Error setting up the MSPI bus for the flash device");
		return ret;
	}

	const struct mspi_xfer_packet packet = {
		.address = addr,
		.cmd = dev_data->read_flash_cmd,
		.data_buf = data,
		.dir = MSPI_RX,
		.num_bytes = size,
	};

	struct mspi_xfer xfer = {
		INF_MSPI_S25H_DEFAULT_XFER_DATA,
		.addr_length = dev_data->mspi_dev_cfg.addr_length,
		.rx_dummy = dev_data->read_flash_dummy_cycles,
		.packets = &packet,
		.num_packet = 1,
		/* 20 milliseconds + 1 ms per 4KiB; for 256 KiB this results in 84 ms */
		.timeout = (size / 4096) + 20,
	};

	ret = mspi_transceive(config->bus, &config->dev_id, &xfer);
	if (ret < 0) {
		return ret;
	}

	if (requires_cleanup) {
		uint8_t unused[3];
		(void)unused;
		return flash_mspi_infineon_s25h_read_jedec_id(dev, unused);
	}

	return ret;
}

static int flash_mspi_infineon_s25h_single_block_write(const struct device *dev,
						       const struct mspi_xfer *xfer_write)
{
	int ret;
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;
	uint8_t status_1;

	ret = flash_mspi_infineon_s25h_set_writing_forbidden(dev, false);
	if (ret < 0) {
		LOG_ERR("Error disabling write protection before trying to write data into "
			"flash");
		return ret;
	}

	ret = mspi_transceive(config->bus, &config->dev_id, xfer_write);
	if (ret < 0) {
		LOG_ERR("Error writing flash memory");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_wait_for_idle(dev,
						     INF_MSPI_S25H_TIMEOUT_IDLE_WRITE_BLOCK_MS);
	if (ret < 0) {
		LOG_ERR("Error waiting for flash to enter idle after writing");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_rw_any_register(dev, INF_MSPI_S25H_ADDRESS_VOLATILE_STATUS_1,
						       &status_1, 0, MSPI_RX);

	if (ret < 0) {
		LOG_ERR("Error reading back status 1 register to confirm valid write");
		return ret;
	}

	if (status_1 & INF_MSPI_S25H_STATUS_1_PRGERR_BIT) {
		LOG_ERR("Last programming transaction wasn't successful");
		return -EIO;
	}

	return 0;
}

static int flash_mspi_infineon_s25h_write(const struct device *dev, off_t addr,
					  const void *transmission_data, size_t size)
{
	int ret = 0;
	struct flash_mspi_infineon_s25h_data *dev_data = dev->data;
	uint8_t old_write_protection;
	uint8_t *data_buf = (uint8_t *)transmission_data;

	/* Check whether we are not aligned and would write over a block boundary */
	if ((addr % INF_MSPI_S25H_WRITE_BLOCK_SIZE) != 0 &&
	    (addr % INF_MSPI_S25H_WRITE_BLOCK_SIZE) + size >= INF_MSPI_S25H_WRITE_BLOCK_SIZE) {
		LOG_ERR("Non-aligned write that goes above another block isn't supported");
		return -ENOSYS;
	}

	struct mspi_xfer_packet packet_write = {
		.cmd = INF_MSPI_S25H_OPCODE_WRITE_FLASH,
		.dir = MSPI_TX,
	};

	struct mspi_xfer xfer_write = {
		INF_MSPI_S25H_DEFAULT_XFER_DATA,
		.addr_length = dev_data->mspi_dev_cfg.addr_length,
		.rx_dummy = 0,
		.packets = &packet_write,
		.num_packet = 1,
		/* 20 milliseconds + 1 ms per 16KiB; for 256 KiB this results in 36 ms */
		.timeout = (size / 16384) + 20,
	};

	ret = flash_mspi_infineon_s25h_prepare_mspi_bus(dev);
	if (ret < 0) {
		LOG_ERR("Error setting up the MSPI bus for the flash device");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_is_write_protection_enabled(dev, &old_write_protection);
	if (ret < 0) {
		LOG_ERR("Error querying whether write protection is enabled");
		return ret;
	}

	uint32_t remaining_bytes = size;
	uint32_t write_index = 0;
	uint32_t write_block_count = size / INF_MSPI_S25H_WRITE_BLOCK_SIZE;
	uint32_t current_transaction_transfer_size;

	if (size % INF_MSPI_S25H_WRITE_BLOCK_SIZE != 0) {
		++write_block_count;
	}

	do {
		current_transaction_transfer_size =
			MIN(remaining_bytes, INF_MSPI_S25H_WRITE_BLOCK_SIZE);
		packet_write.num_bytes = current_transaction_transfer_size;
		packet_write.address = addr + (write_index * INF_MSPI_S25H_WRITE_BLOCK_SIZE);
		packet_write.data_buf = &data_buf[write_index * INF_MSPI_S25H_WRITE_BLOCK_SIZE];

		ret = flash_mspi_infineon_s25h_single_block_write(dev, &xfer_write);
		if (ret < 0) {
			return ret;
		}

		remaining_bytes -= current_transaction_transfer_size;
		++write_index;

	} while (remaining_bytes > 0);

	if (old_write_protection) {
		ret = flash_mspi_infineon_s25h_set_writing_forbidden(dev, false);
		if (ret < 0) {
			LOG_ERR("Error re-enabling write protection after writing data into flash");
			return ret;
		}
	}

	return 0;
}

static int flash_mspi_infineon_s25h_erase(const struct device *dev, off_t addr, size_t size)
{
	int ret = 0;
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;
	struct flash_mspi_infineon_s25h_data *dev_data = dev->data;
	uint8_t old_write_protection;

	struct mspi_xfer_packet packet_erase = {
		.cmd = INF_MSPI_S25H_OPCODE_ERASE_256K,
		.data_buf = NULL,
		.num_bytes = 0,
		.dir = MSPI_TX,
	};

	const struct mspi_xfer xfer_erase = {
		INF_MSPI_S25H_DEFAULT_XFER_DATA,
		.addr_length = dev_data->mspi_dev_cfg.addr_length,
		.rx_dummy = 0,
		.packets = &packet_erase,
		.num_packet = 1,
		/* 20 milliseconds + 1 ms per 16KiB; for 256 KiB this results in 36 ms */
		.timeout = (size / 16384) + 20,
	};

	if (addr % INF_MSPI_S25H_ERASE_SECTOR_SIZE != 0) {
		LOG_WRN("Erase sector is not aligned! This might erase data you don't want to "
			"erase");
	}

	ret = flash_mspi_infineon_s25h_prepare_mspi_bus(dev);
	if (ret < 0) {
		LOG_ERR("Error setting up the MSPI bus for the flash device");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_is_write_protection_enabled(dev, &old_write_protection);
	if (ret < 0) {
		LOG_ERR("Error querying whether write protection is enabled");
		return ret;
	}

	uint32_t count = size / INF_MSPI_S25H_ERASE_SECTOR_SIZE;

	if (size % INF_MSPI_S25H_ERASE_SECTOR_SIZE != 0) {
		++count;
	}

	for (uint32_t i = 0; i < count; ++i) {
		packet_erase.address = addr + (i * INF_MSPI_S25H_ERASE_SECTOR_SIZE);
		ret = flash_mspi_infineon_s25h_set_writing_forbidden(dev, false);
		if (ret < 0) {
			LOG_ERR("Error disabling write protection before flash erase");
			return ret;
		}

		ret = mspi_transceive(config->bus, &config->dev_id, &xfer_erase);
		if (ret) {
			LOG_ERR("Error sending erase command");
			return ret;
		}

		ret = flash_mspi_infineon_s25h_wait_for_idle(
			dev, INF_MSPI_S25H_TIMEOUT_IDLE_ERASE_SECTOR_MS);
		if (ret < 0) {
			LOG_ERR("Error waiting for flash to enter idle after erasing");
			return ret;
		}
	}

	if (old_write_protection) {
		ret = flash_mspi_infineon_s25h_set_writing_forbidden(dev, false);
		if (ret < 0) {
			LOG_ERR("Error re-enabling write protection after flash erase");
			return ret;
		}
	}

	return 0;
}

static const struct flash_parameters *
flash_mspi_infineon_s25h_get_parameters(const struct device *dev)
{
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;

	return &config->parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_mspi_infineon_s25h_pages_layout(const struct device *dev,
						  const struct flash_pages_layout **layout,
						  size_t *layout_size)
{
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;

	*layout = &config->page_layout;
	*layout_size = 1;
}
#endif

static int flash_mspi_infineon_s25h_verify_jedec_id(const struct device *dev)
{
	uint8_t id[3];
	int ret;

	ret = flash_mspi_infineon_s25h_read_jedec_id(dev, id);
	if (ret < 0) {
		LOG_ERR("Error reading JEDEC ids from flash");
		return ret;
	}

	uint8_t manufacturer_id = id[0];
	uint16_t device_id = (id[1] << 8) | id[2];

	if (manufacturer_id != INF_MSPI_S25H_MANUFACTURER_ID ||
	    device_id != INF_MSPI_S25H_DEVICE_ID) {
		LOG_ERR("Rear JEDEC ids don't match expected ids. The communication is possibly "
			"broken or the non-volatile flash configuration is something unexpected");
		LOG_ERR("Read manufacturer id: 0x%02X. Expected: 0x%02X", manufacturer_id,
			INF_MSPI_S25H_MANUFACTURER_ID);
		LOG_ERR("Read device id: 0x%04X. Expected: 0x%04X", device_id,
			INF_MSPI_S25H_DEVICE_ID);
		return -EIO;
	}

	return 0;
}

static int flash_mspi_infineon_s25h_switch_to_quad_transfer(const struct device *dev)
{
	int ret;
	struct flash_mspi_infineon_s25h_data *data = dev->data;

	uint8_t cfg_value;

	ret = flash_mspi_infineon_s25h_rw_any_register(dev, INF_MSPI_S25H_ADDRESS_VOLATILE_CFG_1,
						       &cfg_value, 0, MSPI_RX);
	if (ret < 0) {
		LOG_ERR("Error reading flash register");
		return ret;
	}

	cfg_value |= INF_MSPI_S25H_CFG_1_QUADIT_BIT;

	ret = flash_mspi_infineon_s25h_rw_any_register(dev, INF_MSPI_S25H_ADDRESS_VOLATILE_CFG_1,
						       &cfg_value, 0, MSPI_TX);
	if (ret < 0) {
		LOG_ERR("Error writing flash register");
		return ret;
	}

	/* set address + data to 4 lanes */
	data->mspi_dev_cfg.io_mode = MSPI_IO_MODE_QUAD_1_4_4;
	ret = flash_mspi_infineon_s25h_prepare_mspi_bus(dev);

	if (ret < 0) {
		LOG_ERR("Error switching MSPI mode to 4 lane data width");
		return ret;
	}

	data->read_flash_cmd = INF_MSPI_S25H_OPCODE_READ_FLASH_QUAD;
	ret = flash_mspi_infineon_s25h_rw_any_register(dev, INF_MSPI_S25H_ADDRESS_VOLATILE_CFG_1,
						       &cfg_value, 0, MSPI_RX);
	if (ret < 0) {
		LOG_ERR("Error reading flash register");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_verify_jedec_id(dev);
	if (ret < 0) {
		LOG_ERR("JEDEC ID mismatch after switching to 4 lane MSPI. Communication is "
			"broken");
		return ret;
	}

	/* set command to 4 lanes */
	ret = flash_mspi_infineon_s25h_rw_any_register(dev, INF_MSPI_S25H_ADDRESS_VOLATILE_CFG_2,
						       &cfg_value, 0, MSPI_RX);
	if (ret < 0) {
		LOG_ERR("Error reading flash register");
		return ret;
	}

	cfg_value |= INF_MSPI_S25H_CFG_2_QPI_IT_BIT;
	ret = flash_mspi_infineon_s25h_rw_any_register(dev, INF_MSPI_S25H_ADDRESS_VOLATILE_CFG_2,
						       &cfg_value, 0, MSPI_TX);
	if (ret < 0) {
		LOG_ERR("Error writing flash register");
		return ret;
	}

	data->mspi_dev_cfg.io_mode = MSPI_IO_MODE_QUAD;
	data->read_jedec_cmd = INF_MSPI_S25H_OPCODE_READ_JEDEC_ID_QUAD;
	data->read_flash_dummy_cycles = INF_MSPI_S25H_DELAY_READ_QUADSPI;

	ret = flash_mspi_infineon_s25h_prepare_mspi_bus(dev);
	if (ret < 0) {
		LOG_ERR("Error switching bus mode to full quad MSPI mode");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_verify_jedec_id(dev);
	if (ret < 0) {
		LOG_ERR("JEDEC ID mismatch after switching to full quad MSPI mode. Communication "
			"is broken");
		return ret;
	}

	return 0;
}

static int flash_mspi_infineon_s25h_disable_hybrid_sector_mode(const struct device *dev)
{
	/* This driver needs the hybrid sector mode to be disabled. So if it's found to be turned on
	 * it gets changed. This requires changing the non-volatile configuration and also a reset
	 */
	int ret = 0;
	uint8_t conf3 = 0;

	ret = flash_mspi_infineon_s25h_rw_any_register(dev, INF_MSPI_S25H_ADDRESS_VOLATILE_CFG_3,
						       &conf3, 0, MSPI_RX);
	if (ret < 0) {
		LOG_ERR("Error reading volatile configuration register 3");
		return ret;
	}

	if ((conf3 & INF_MSPI_S25H_CFG_3_UNHYSA_BIT) == 0) {
		LOG_INF("Flash is in hybrid sector mode. Changing non-volatile config to correct "
			"this");

		conf3 |= INF_MSPI_S25H_CFG_3_UNHYSA_BIT;

		ret = flash_mspi_infineon_s25h_rw_any_register(
			dev, INF_MSPI_S25H_ADDRESS_NON_VOLATILE_CFG_3, &conf3, 0, MSPI_TX);
		if (ret < 0) {
			LOG_ERR("Error changing non-volatile configuration of flash");
			return ret;
		}

		ret = flash_mspi_infineon_s25h_wait_for_idle(dev,
							     INF_MSPI_S25H_TIMEOUT_IDLE_STARTUP);
		if (ret < 0) {
			LOG_ERR("Error waiting for flash to enter idle after disabling hybrid "
				"sector mode");
			return ret;
		}

		ret = flash_mspi_infineon_s25h_reset(dev);
		if (ret < 0) {
			LOG_ERR("Error resetting flash via reset command");
			return ret;
		}

		conf3 = 0;
		ret = flash_mspi_infineon_s25h_rw_any_register(
			dev, INF_MSPI_S25H_ADDRESS_VOLATILE_CFG_3, &conf3, 0, MSPI_RX);
		if (ret < 0) {
			LOG_ERR("Error reading volatile config 3 register of flash");
			return ret;
		}

		if ((conf3 & INF_MSPI_S25H_CFG_3_UNHYSA_BIT) == 0) {
			LOG_ERR("Changing the flash configuration to Uniform mode didn't work");
			return -EIO;
		}

		ret = flash_mspi_infineon_s25h_set_writing_forbidden(dev, true);
		if (ret < 0) {
			LOG_ERR("Error re-enabling the write protection");
			return ret;
		}
	}

	return 0;
}

static int flash_mspi_infineon_s25h_enter_4_byte_address_mode(const struct device *dev)
{
	int ret = 0;
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;
	struct flash_mspi_infineon_s25h_data *data = dev->data;

	const struct mspi_xfer_packet enter_4_byte_cmd = {
		.dir = MSPI_TX,
		.cmd = INF_MSPI_S25H_OPCODE_ENABLE_4_BYTE_ADDR_MODE,
		.num_bytes = 0,
	};

	struct mspi_xfer xfer = {
		INF_MSPI_S25H_DEFAULT_XFER_DATA,
		.rx_dummy = 0,
		.addr_length = 0,
		.num_packet = 1,
		.packets = &enter_4_byte_cmd,
		.timeout = INF_MSPI_S25H_DEFAULT_MSPI_TIMEOUT,
	};

	ret = mspi_transceive(config->bus, &config->dev_id, &xfer);
	if (ret < 0) {
		LOG_ERR("Error sending command to enter 4 byte address mode");
		return ret;
	}

	data->mspi_dev_cfg.addr_length = 4;

	if (ret < 0) {
		LOG_ERR("Error setting up MSPI bus after changing address length");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_verify_jedec_id(dev);
	if (ret < 0) {
		LOG_ERR("Error verifying JEDEC id after entering 4 byte address mode");
		return ret;
	}

	return 0;
}

static int flash_mspi_infineon_s25h_init(const struct device *dev)
{
	int ret = 0;
	const struct flash_mspi_infineon_s25h_cfg *config = dev->config;

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_prepare_mspi_bus(dev);
	if (ret < 0) {
		LOG_ERR("Error switching MSPI configuration to the requirements of the flash "
			"device");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_reset(dev);
	if (ret < 0) {
		LOG_ERR("Error resetting flash device");
		return ret;
	}

	ret = flash_mspi_infineon_s25h_verify_jedec_id(dev);
	if (ret < 0) {
		return ret;
	}

	ret = flash_mspi_infineon_s25h_disable_hybrid_sector_mode(dev);
	if (ret < 0) {
		return ret;
	}

	ret = flash_mspi_infineon_s25h_enter_4_byte_address_mode(dev);
	if (ret < 0) {
		return ret;
	}

	ret = flash_mspi_infineon_s25h_switch_to_quad_transfer(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static DEVICE_API(flash, flash_mspi_infineon_s25h_driver_api) = {
	.read = flash_mspi_infineon_s25h_read,
	.write = flash_mspi_infineon_s25h_write,
	.erase = flash_mspi_infineon_s25h_erase,
	.get_parameters = flash_mspi_infineon_s25h_get_parameters,
#if defined(CONFIG_FLASH_JESD216_API)
	.read_jedec_id = flash_mspi_infineon_s25h_read_jedec_id,
#endif
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mspi_infineon_s25h_pages_layout,
#endif
};

#define INFINEON_MSPI_FLASH_S25H_CHECK_PROP_IS_UNDEFINED(n, prop)                                  \
	BUILD_ASSERT(DT_NODE_HAS_PROP(DT_DRV_INST(n), prop) == 0,                                  \
		     "The Infineon S25H driver ignores the property " #prop ". Don't use it")

/* Check for ignored/wrong values in the devicetree */
#define INFINEON_MSPI_FLASH_S25H_CHECK_DEVICETREE_CONFIG(n)                                        \
	INFINEON_MSPI_FLASH_S25H_CHECK_PROP_IS_UNDEFINED(n, rx_dummy);                             \
	INFINEON_MSPI_FLASH_S25H_CHECK_PROP_IS_UNDEFINED(n, tx_dummy);                             \
	INFINEON_MSPI_FLASH_S25H_CHECK_PROP_IS_UNDEFINED(n, read_command);                         \
	INFINEON_MSPI_FLASH_S25H_CHECK_PROP_IS_UNDEFINED(n, write_command);                        \
	INFINEON_MSPI_FLASH_S25H_CHECK_PROP_IS_UNDEFINED(n, xip_config);                           \
	INFINEON_MSPI_FLASH_S25H_CHECK_PROP_IS_UNDEFINED(n, scramble_config);                      \
	INFINEON_MSPI_FLASH_S25H_CHECK_PROP_IS_UNDEFINED(n, ce_break_config);                      \
	BUILD_ASSERT(DT_ENUM_HAS_VALUE(DT_DRV_INST(n), command_length, INSTR_1_BYTE) == 1,         \
		     "The Infineon S25H chip uses only 1 byte opcodes")

#define INFINFEON_MSPI_FLASH_S25H_DEFINE(n)                                                        \
	INFINEON_MSPI_FLASH_S25H_CHECK_DEVICETREE_CONFIG(n);                                       \
	PINCTRL_DT_DEFINE(DT_DRV_INST(n));                                                         \
	static const struct flash_mspi_infineon_s25h_cfg flash_mspi_infineon_s25h_cfg_##n = {      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.bus = DEVICE_DT_GET(DT_BUS(DT_DRV_INST(n))),                                      \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.reset_startup_duration = K_USEC(DT_INST_PROP(n, reset_startup_time_us)),          \
		.dev_id = MSPI_DEVICE_ID_DT_INST(n),                                               \
		.page_layout = {.pages_count = DT_INST_PROP(n, flash_size) /                       \
					       DT_INST_PROP(n, erase_block_size),                  \
				.pages_size = DT_INST_PROP(n, erase_block_size)},                  \
		.parameters = {.erase_value = 0xFF,                                                \
			       .write_block_size = DT_INST_PROP(n, write_block_size)}};            \
	static struct flash_mspi_infineon_s25h_data flash_mspi_infineon_s25h_data_##n = {          \
		.mspi_dev_cfg = MSPI_DEVICE_CONFIG_DT_INST(n),                                     \
		.read_jedec_cmd = INF_MSPI_S25H_OPCODE_READ_JEDEC_ID,                              \
		.read_flash_cmd = INF_MSPI_S25H_OPCODE_READ_FLASH,                                 \
		.read_flash_dummy_cycles = 0,                                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, flash_mspi_infineon_s25h_init, NULL,                              \
			      &flash_mspi_infineon_s25h_data_##n,                                  \
			      &flash_mspi_infineon_s25h_cfg_##n, POST_KERNEL,                      \
			      CONFIG_FLASH_INIT_PRIORITY, &flash_mspi_infineon_s25h_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINFEON_MSPI_FLASH_S25H_DEFINE)
