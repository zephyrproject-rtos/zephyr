/*
 * Copyright (c) 2022-2025 Macronix International Co., Ltd.
 * Copyright (c) 2025 Embeint Pty Ltd
 * Copyright (c) 2026 CodeWrights GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_mspi_nand

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "spi_nand.h"

struct mspi_nand_config {
	/* MSPI controller the device is connected to */
	const struct device *bus;
	/* Device identification on the MSPI bus */
	struct mspi_dev_id mspi_id;
	/* Devicetree MSPI device configuration */
	struct mspi_dev_cfg mspi_cfg;
	/* Maximum duration of a single MSPI transfer in milliseconds */
	uint32_t transfer_timeout;
	/* Controller is shared with other devices under software control */
	bool multiperipheral_bus;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	/* Flash page layout can be determined from devicetree. */
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
	/* Flash parameters */
	const struct flash_parameters *parameters;
#ifdef CONFIG_MSPI_NAND_BAD_BLOCK_CACHE
	/* Bad block table */
	uint8_t *bad_block_table;
#endif /* CONFIG_MSPI_NAND_BAD_BLOCK_CACHE */
	/* Size of device in bytes */
	uint32_t flash_size;
	/* Size of erase unit in bytes */
	uint32_t block_size;
	/* Maximum duration to erase a block */
	uint16_t block_erase_us;
	/* Maximum duration to program a page */
	uint16_t page_program_us;
	/* Maximum duration to read a page to cache */
	uint16_t page_read_us;
	/* Maximum duration for RESET command to execute */
	uint16_t reset_us;
	/* Mask to get column address */
	uint32_t addr_offset_mask;
	/* Shift to apply to get page address */
	uint8_t addr_page_shift;
	/* Shift to apply to get erase block address */
	uint8_t addr_block_shift;
	/* Number of bits used for plane selection */
	uint8_t plane_addr_bits;
	/* Expected JEDEC ID, from jedec-id property */
	const uint8_t *jedec_id;
	/* Length of the JEDEC ID */
	uint8_t jedec_id_len;
	/* Program commands support plane select */
	bool has_program_plane_select;
	/* Read commands support plane select */
	bool has_read_plane_select;
};

struct mspi_nand_data {
	/* Access semaphore */
	struct k_sem sem;
};

/* Device configuration applied to the controller: the command, address and
 * dummy settings of memory mapped access are set per transfer instead.
 */
#define MSPI_NAND_DEV_CFG_MASK                                                                     \
	(MSPI_DEVICE_CONFIG_ALL &                                                                  \
	 ~(MSPI_DEVICE_CONFIG_CMD_LEN | MSPI_DEVICE_CONFIG_ADDR_LEN |                              \
	   MSPI_DEVICE_CONFIG_READ_CMD | MSPI_DEVICE_CONFIG_WRITE_CMD |                            \
	   MSPI_DEVICE_CONFIG_RX_DUMMY | MSPI_DEVICE_CONFIG_TX_DUMMY))

/* Indicates that an access command includes bytes for the address.
 * If not provided the opcode is not followed by address bytes.
 */
#define NAND_ACCESS_ADDRESSED BIT(0)

/* Indicates that addressed access uses a 8-bit address */
#define NAND_ACCESS_8BIT_ADDR BIT(1)

/* Indicates that addressed access uses a 16-bit address */
#define NAND_ACCESS_16BIT_ADDR BIT(2)

/* Indicates that addressed access uses a 24-bit address */
#define NAND_ACCESS_24BIT_ADDR BIT(3)

/* Indicates that addressed access uses a 32-bit address */
#define NAND_ACCESS_32BIT_ADDR BIT(4)

/* Indicates that an access command is performing a write.  If not
 * provided access is a read.
 */
#define NAND_ACCESS_WRITE BIT(5)

/* Indicates that a dummy byte is to be sent following the address.
 */
#define NAND_ACCESS_DUMMY_BYTE BIT(6)

/* Indicates that an access command transfers cache data.  If not
 * provided the access is a command or register access.
 */
#define NAND_ACCESS_DATA BIT(7)

/* Offset of the bad block marker in the spare area */
#define BAD_BLOCK_MARKER_OFFSET 0x00

LOG_MODULE_REGISTER(mspi_nand, CONFIG_FLASH_LOG_LEVEL);

/* Everything necessary to acquire owning access to the device. */
static int acquire_device(const struct device *dev)
{
	const struct mspi_nand_config *config = dev->config;
	struct mspi_nand_data *const data = dev->data;
	enum mspi_dev_cfg_mask mask;
	int ret;

	k_sem_take(&data->sem, K_FOREVER);
	ret = pm_device_runtime_get(config->bus);
	if (ret != 0) {
		LOG_ERR_PM_DEVICE_RUNTIME_GET(config->bus, ret);
		k_sem_give(&data->sem);
		return ret;
	}

	/* Acquire the controller. On a shared bus the device configuration is
	 * applied again, otherwise the one applied earlier remains in effect.
	 */
	mask = config->multiperipheral_bus ? MSPI_NAND_DEV_CFG_MASK : MSPI_DEVICE_CONFIG_NONE;
	ret = mspi_dev_config(config->bus, &config->mspi_id, mask, &config->mspi_cfg);
	if (ret != 0) {
		LOG_ERR("MSPI device configuration failed (%d)", ret);
		(void)pm_device_runtime_put(config->bus);
		k_sem_give(&data->sem);
	}

	return ret;
}

/* Everything necessary to release access to the device. */
static void release_device(const struct device *dev)
{
	const struct mspi_nand_config *config = dev->config;
	struct mspi_nand_data *const data = dev->data;

	/* Release the controller */
	(void)mspi_get_channel_status(config->bus, 0);
	(void)pm_device_runtime_put(config->bus);
	k_sem_give(&data->sem);
}

/*
 * @brief Send an MSPI command
 *
 * @param dev Device struct
 * @param opcode The command to send
 * @param access flags that determine how the command is constructed.
 *        See NAND_ACCESS_*.
 * @param addr The address to send
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @return 0 on success, negative errno code otherwise
 */
static int mspi_nand_access(const struct device *const dev, uint8_t opcode, unsigned int access,
			    off_t addr, void *data, size_t length)
{
	const struct mspi_nand_config *config = dev->config;
	bool is_addressed = (access & NAND_ACCESS_ADDRESSED) != 0U;
	bool is_write = (access & NAND_ACCESS_WRITE) != 0U;
	bool use_dma = ((access & NAND_ACCESS_DATA) != 0U)
			       ? IS_ENABLED(CONFIG_MSPI_NAND_DMA_DATA_XFER)
			       : IS_ENABLED(CONFIG_MSPI_NAND_DMA_CONTROL_XFER);
	const struct mspi_xfer_packet packet = {
		/* Commands without data only transmit */
		.dir = (is_write || (length == 0)) ? MSPI_TX : MSPI_RX,
		.cmd = opcode,
		.address = addr,
		.num_bytes = length,
		.data_buf = data,
	};
	struct mspi_xfer xfer = {
		.xfer_mode = use_dma ? MSPI_DMA : MSPI_PIO,
		.cmd_length = 1,
		.packets = &packet,
		.num_packet = 1,
		.timeout = config->transfer_timeout,
	};
	int ret;

	if (is_addressed) {
		if ((access & NAND_ACCESS_32BIT_ADDR) != 0U) {
			xfer.addr_length = 4;
		} else if ((access & NAND_ACCESS_24BIT_ADDR) != 0U) {
			xfer.addr_length = 3;
		} else if ((access & NAND_ACCESS_16BIT_ADDR) != 0U) {
			xfer.addr_length = 2;
		} else if ((access & NAND_ACCESS_8BIT_ADDR) != 0U) {
			xfer.addr_length = 1;
		} else {
			xfer.addr_length = 0;
		}
	}

	if (access & NAND_ACCESS_DUMMY_BYTE) {
		if (packet.dir == MSPI_TX) {
			xfer.tx_dummy = BITS_PER_BYTE;
		} else {
			xfer.rx_dummy = BITS_PER_BYTE;
		}
	}

	ret = mspi_transceive(config->bus, &config->mspi_id, &xfer);
	if (ret != 0) {
		LOG_DBG("MSPI transaction failed (%d)", ret);
	}
	return ret;
}

#define mspi_nand_cmd_read(dev, opcode, dest, length)                                              \
	mspi_nand_access(dev, opcode, 0, 0, dest, length)
#define mspi_nand_cmd_read_dummy(dev, opcode, dest, length)                                        \
	mspi_nand_access(dev, opcode, NAND_ACCESS_DUMMY_BYTE, 0, dest, length)
#define mspi_nand_cmd_write(dev, opcode)                                                           \
	mspi_nand_access(dev, opcode, NAND_ACCESS_WRITE, 0, NULL, 0)

/* Read feature data from a register */
static int mspi_nand_get_feature(const struct device *dev, uint8_t reg, uint8_t *feature)
{
	return mspi_nand_access(dev, SPI_NAND_CMD_GET_FEATURE,
				NAND_ACCESS_ADDRESSED | NAND_ACCESS_8BIT_ADDR, reg, feature,
				sizeof(*feature));
}

/* Write feature data to a register */
static int mspi_nand_set_feature(const struct device *dev, uint8_t reg, uint8_t feature)
{
	return mspi_nand_access(dev, SPI_NAND_CMD_SET_FEATURE,
				NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED | NAND_ACCESS_8BIT_ADDR,
				reg, &feature, sizeof(feature));
}

/* Wait until all operations are complete */
static int mspi_nand_wait_until_ready(const struct device *dev, const char *op,
				      uint32_t timeout_us, uint32_t poll_us, uint8_t *status)
{
	k_ticks_t t = k_uptime_ticks();
	k_timepoint_t timeout;
	bool expired;
	int ret;

	timeout = sys_timepoint_calc(K_USEC(timeout_us));
	do {
		/* Determine the expiry status before the read to ensure that this function only
		 * returns ETIMEDOUT if the flash still isn't ready *after* the provided timeout_us.
		 * Checking after the read (in the while condition) can result in the status being
		 * read once on function entry, then the thread not resuming from the k_sleep until
		 * after the timeout expires.
		 */
		expired = sys_timepoint_expired(timeout);

		/* Query the flash status */
		ret = mspi_nand_get_feature(dev, SPI_NAND_FEATURE_ADDR_STATUS, status);
		if (ret != 0) {
			return ret;
		}

		if ((*status & SPI_NAND_FEATURE_STATUS_OIP) == 0U) {
			t = k_uptime_ticks() - t;
			LOG_DBG("Ready after %u us (Op %s, Status %02X)", k_ticks_to_us_near32(t),
				op, *status);
			return ret;
		}

		if (!expired) {
			k_sleep(K_USEC(poll_us));
		}
	} while (!expired);

	LOG_ERR("Ready timeout (Op %s, Status %02X)", op, *status);
	return -ETIMEDOUT;
}

/* Read page to cache, assumes device already acquired */
static int mspi_nand_page_read_to_cache(const struct device *dev, uint32_t page)
{
	const struct mspi_nand_config *config = dev->config;
	uint8_t ecc_status;
	uint8_t status;
	int ret;

	/* Trigger the read to cache */
	ret = mspi_nand_access(dev, SPI_NAND_CMD_PAGE_READ,
			       NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR, page, NULL, 0);
	if (ret != 0) {
		return ret;
	}

	/* Wait until the read to cache completes (poll with no delays) */
	ret = mspi_nand_wait_until_ready(dev, "read", config->page_read_us, 0, &status);
	if (ret != 0) {
		return ret;
	}

	/* Validate data integrity from ECC */
	ecc_status = status & SPI_NAND_FEATURE_ECC_MASK;
	switch (ecc_status) {
	case SPI_NAND_FEATURE_ECC_ERROR_NOT_CORRECTED:
		LOG_WRN("ECC uncorrectable error on page %06x", page);
		/* Unique error code for corrupt data (retrying the read won't work) */
		return -EBADMSG;
	case SPI_NAND_FEATURE_ECC_ERROR_CORRECTED_REFRESH:
	case SPI_NAND_FEATURE_ECC_ERROR_CORRECTED:
		LOG_DBG("ECC errors corrected on page %06x", page);
		break;
	case SPI_NAND_FEATURE_ECC_NO_ERRORS:
		break;
	default:
		__ASSERT(false, "Unreachable");
	}
	return 0;
}

/** Read data from cache, assumes device already acquired */
static int mspi_nand_read_from_cache(const struct device *dev, uint8_t plane, uint16_t offset,
				     void *dest, size_t size)
{
	const struct mspi_nand_config *config = dev->config;

	/* Some chips require plane address for read from cache command */
	if (config->has_read_plane_select) {
		offset |= (plane << (config->addr_page_shift + 1));
	}

	return mspi_nand_access(dev, SPI_NAND_CMD_READ_CACHE,
				NAND_ACCESS_DATA | NAND_ACCESS_ADDRESSED | NAND_ACCESS_16BIT_ADDR |
					NAND_ACCESS_DUMMY_BYTE,
				offset, dest, size);
}

/** Write data to cache, assumes device already acquired */
static int mspi_nand_write_to_cache(const struct device *dev, uint8_t plane, uint16_t offset,
				    void *src, size_t size)
{
	const struct mspi_nand_config *config = dev->config;

	/* Some chips require plane address for write to cache command */
	if (config->has_program_plane_select) {
		offset |= (plane << (config->addr_page_shift + 1));
	}

	return mspi_nand_access(dev, SPI_NAND_CMD_PROGRAM_LOAD,
				NAND_ACCESS_DATA | NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED |
					NAND_ACCESS_16BIT_ADDR,
				offset, src, size);
}

static bool valid_region(const struct device *dev, off_t addr, size_t size)
{
	const struct mspi_nand_config *config = dev->config;

	if ((addr < 0) || (addr >= config->flash_size) || (size > config->flash_size) ||
	    ((config->flash_size - addr) < size)) {
		return false;
	}
	return true;
}

static int mspi_nand_read(const struct device *dev, off_t addr, void *dest, size_t size)
{
	const struct mspi_nand_config *config = dev->config;
	uint8_t *dest_u8 = dest;
	uint32_t page_address;
	uint8_t plane;
	uint16_t page_offset;
	uint16_t bytes_to_end;
	uint16_t bytes_to_read;
	int ret;

	if (size == 0) {
		/* No work to do */
		return 0;
	}

	/* Read area must be subregion of device */
	if (!valid_region(dev, addr, size)) {
		return -EINVAL;
	}

	ret = acquire_device(dev);
	if (ret != 0) {
		return ret;
	}

	while (size > 0) {
		page_address = addr >> config->addr_page_shift;
		page_offset = addr & config->addr_offset_mask;
		plane = config->plane_addr_bits > 0 ? (addr >> config->addr_block_shift) &
							      ((1 << config->plane_addr_bits) - 1)
						    : 0;
		bytes_to_end = config->parameters->write_block_size - page_offset;
		bytes_to_read = MIN(size, bytes_to_end);

		/* Copy data from main storage to cache */
		LOG_DBG("Read %d from %06x:%03x", bytes_to_read, page_address, page_offset);
		ret = mspi_nand_page_read_to_cache(dev, page_address);
		if (ret != 0) {
			LOG_DBG("Copy from NAND to device cache failed (%d)", ret);
			break;
		}

		/* Read data out of cache */
		ret = mspi_nand_read_from_cache(dev, plane, page_offset, dest_u8, bytes_to_read);
		if (ret != 0) {
			LOG_DBG("Read from device cache failed (%d)", ret);
			break;
		}

		/* Update for next iteration */
		dest_u8 += bytes_to_read;
		addr += bytes_to_read;
		size -= bytes_to_read;
	}

	release_device(dev);
	return ret;
}

static int mspi_nand_write(const struct device *dev, off_t addr, const void *src, size_t size)
{
	const struct mspi_nand_config *config = dev->config;
	uint32_t write_block = config->parameters->write_block_size;
	uint8_t *src_u8 = (void *)src;
	uint8_t plane = config->plane_addr_bits > 0 ? (addr >> config->addr_block_shift) &
							      ((1 << config->plane_addr_bits) - 1)
						    : 0;
	uint32_t page_address;
	uint8_t status;
	int ret;

	if (size == 0) {
		/* No work to do */
		return 0;
	}

	/* Write area must be subregion of device */
	if (!valid_region(dev, addr, size)) {
		return -EINVAL;
	}

	/* All writes must be page aligned in both start address and size */
	if (addr % write_block) {
		return -EINVAL;
	}
	if (size % write_block) {
		return -EINVAL;
	}

	ret = acquire_device(dev);
	if (ret != 0) {
		return ret;
	}

	while (size > 0) {
		/* Enable write operation */
		ret = mspi_nand_cmd_write(dev, SPI_NAND_CMD_WRITE_ENABLE);
		if (ret != 0) {
			break;
		}

		page_address = addr >> config->addr_page_shift;
		LOG_DBG("Write %d to %06x:000", write_block, page_address);

		/* Copy data to cache (at offset 0) */
		ret = mspi_nand_write_to_cache(dev, plane, 0, src_u8, write_block);
		if (ret != 0) {
			LOG_DBG("Copy to device cache failed (%d)", ret);
			break;
		}

		/* Program the cache to the appropriate page */
		ret = mspi_nand_access(dev, SPI_NAND_CMD_PROGRAM_EXECUTE,
				       NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED |
					       NAND_ACCESS_24BIT_ADDR,
				       page_address, NULL, 0);
		if (ret != 0) {
			LOG_DBG("Program from cache to NAND failed (%d)", ret);
			break;
		}

		/* Wait for the write to complete (poll every 0.1ms) */
		ret = mspi_nand_wait_until_ready(dev, "write", config->page_program_us, 100,
						 &status);
		if (ret != 0) {
			break;
		}
		if (status & SPI_NAND_FEATURE_STATUS_PROGRAM_FAIL) {
			LOG_ERR("Program operation failed");
			ret = -EIO;
			break;
		}

		/* Update for next iteration */
		src_u8 += write_block;
		size -= write_block;
		addr += write_block;
	}

	release_device(dev);
	return ret;
}

static int mspi_nand_erase(const struct device *dev, off_t addr, size_t size)
{
	const struct mspi_nand_config *config = dev->config;
	uint32_t page_address;
	uint8_t status;
	int ret;

	if (size == 0) {
		/* No work to do */
		return 0;
	}

	/* Erase area must be subregion of device */
	if (!valid_region(dev, addr, size)) {
		return -EINVAL;
	}

	/* All erases must be block aligned in both start address and size */
	if (addr % config->block_size) {
		return -EINVAL;
	}
	if (size % config->block_size) {
		return -EINVAL;
	}

	ret = acquire_device(dev);
	if (ret != 0) {
		return ret;
	}

	while (size > 0) {
		/* Enable write (erase) operation */
		ret = mspi_nand_cmd_write(dev, SPI_NAND_CMD_WRITE_ENABLE);
		if (ret != 0) {
			break;
		}

		/* Start the block erase */
		page_address = addr >> config->addr_page_shift;
		LOG_DBG("Erasing block starting at %06x", page_address);
		ret = mspi_nand_access(dev, SPI_NAND_CMD_BLOCK_ERASE,
				       NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR, page_address,
				       NULL, 0);
		if (ret != 0) {
			break;
		}
		/* Wait for the erase to complete (poll every 0.5ms) */
		ret = mspi_nand_wait_until_ready(dev, "erase", config->block_erase_us, 500,
						 &status);
		if (ret != 0) {
			break;
		}
		if (status & SPI_NAND_FEATURE_STATUS_ERASE_FAIL) {
			LOG_ERR("Erase operation failed");
			ret = -EIO;
			break;
		}

		/* Update for next iteration */
		addr += config->block_size;
		size -= config->block_size;
	}

	release_device(dev);
	return ret;
}

static int mspi_nand_reset(const struct device *dev)
{
	const struct mspi_nand_config *config = dev->config;
	uint8_t status;
	int ret;

	ret = mspi_nand_cmd_write(dev, SPI_NAND_CMD_RESET);
	if (ret != 0) {
		return ret;
	}
	ret = mspi_nand_wait_until_ready(dev, "reset", config->reset_us, 100, &status);

	return ret;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)

static void mspi_nand_pages_layout(const struct device *dev,
				   const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct mspi_nand_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if defined(CONFIG_FLASH_EX_OP_ENABLED)

static int mspi_nand_is_bad_block(const struct device *dev, off_t addr,
				  enum flash_block_status *status)
{
	const struct mspi_nand_config *config = dev->config;
	const uint32_t bad_block_marker_offset =
		config->parameters->write_block_size + BAD_BLOCK_MARKER_OFFSET;
	uint8_t plane = config->plane_addr_bits > 0 ? (addr >> config->addr_block_shift) &
							      ((1 << config->plane_addr_bits) - 1)
						    : 0;
	uint32_t page_address;
	uint8_t bad_block_marker;
	int ret;

	/* Address must be in subregion of device */
	if (!valid_region(dev, addr, 1)) {
		return -EINVAL;
	}

	/* Address must be aligned to erase block */
	if (addr % config->block_size) {
		return -EINVAL;
	}

	page_address = addr >> config->addr_page_shift;

	/* Copy data from main storage to cache (ignore ECC errors) */
	ret = mspi_nand_page_read_to_cache(dev, page_address);
	if ((ret != 0) && (ret != -EBADMSG)) {
		LOG_DBG("Copy from NAND to device cache failed (%d)", ret);
		return ret;
	}

	/* Read bad block marker out of cache */
	ret = mspi_nand_read_from_cache(dev, plane, bad_block_marker_offset, &bad_block_marker,
					sizeof(bad_block_marker));
	if (ret != 0) {
		LOG_DBG("Read from device cache failed (%d)", ret);
		return ret;
	}

	/* Verify bad block marker */
	if (bad_block_marker != 0xff) {
		LOG_DBG("Block at address %06x is bad (marker %02x)", page_address,
			bad_block_marker);
		*status = FLASH_BLOCK_BAD;
	} else {
		LOG_DBG("Block at address %06x is good", page_address);
		*status = FLASH_BLOCK_GOOD;
	}

	return 0;
}

static int mspi_nand_mark_bad_block(const struct device *dev, off_t addr)
{
	const struct mspi_nand_config *config = dev->config;
	const uint32_t bad_block_marker_offset =
		config->parameters->write_block_size + BAD_BLOCK_MARKER_OFFSET;
	uint8_t plane = config->plane_addr_bits > 0 ? (addr >> config->addr_block_shift) &
							      ((1 << config->plane_addr_bits) - 1)
						    : 0;
	uint8_t bad_block_marker = 0x00;
	uint32_t page_address;
	uint8_t status;
	int ret;

	/* Address must be in subregion of device */
	if (!valid_region(dev, addr, 1)) {
		return -EINVAL;
	}

	/* Address must be aligned to erase block */
	if (addr % config->block_size) {
		return -EINVAL;
	}

	page_address = addr >> config->addr_page_shift;
	LOG_DBG("Marking block starting at %06x as bad", page_address);

	/* Enable write operation */
	ret = mspi_nand_cmd_write(dev, SPI_NAND_CMD_WRITE_ENABLE);
	if (ret != 0) {
		return ret;
	}

	/* Copy bad block marker to cache (all other bytes stay reset at 0xff) */
	ret = mspi_nand_write_to_cache(dev, plane, bad_block_marker_offset, &bad_block_marker,
				       sizeof(bad_block_marker));
	if (ret != 0) {
		LOG_DBG("Copy to device cache failed (%d)", ret);
		return ret;
	}

	/* Program the cache to the appropriate page */
	ret = mspi_nand_access(dev, SPI_NAND_CMD_PROGRAM_EXECUTE,
			       NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR,
			       page_address, NULL, 0);
	if (ret != 0) {
		LOG_DBG("Program from cache to NAND failed (%d)", ret);
		return ret;
	}

	/* Wait for the write to complete (poll every 0.1ms) */
	ret = mspi_nand_wait_until_ready(dev, "write", config->page_program_us, 100, &status);
	if (ret != 0) {
		return ret;
	}
	if (status & SPI_NAND_FEATURE_STATUS_PROGRAM_FAIL) {
		LOG_ERR("Program operation failed");
		ret = -EIO;
		return ret;
	}

#ifdef CONFIG_MSPI_NAND_BAD_BLOCK_CACHE
	/* Update the bad block table */
	uint32_t bbt_idx = addr / config->block_size;

	config->bad_block_table[bbt_idx / BITS_PER_BYTE] |= BIT(bbt_idx % BITS_PER_BYTE);
#endif /* CONFIG_MSPI_NAND_BAD_BLOCK_CACHE */

	return 0;
}

#ifdef CONFIG_MSPI_NAND_BAD_BLOCK_CACHE
static int bad_block_table_is_bad_block(const struct device *dev, off_t addr,
					enum flash_block_status *status)
{
	const struct mspi_nand_config *config = dev->config;
	uint32_t bbt_idx;

	/* Address must be in subregion of device */
	if (!valid_region(dev, addr, 1)) {
		return -EINVAL;
	}

	/* Address must be aligned to erase block */
	if (addr % config->block_size) {
		return -EINVAL;
	}

	bbt_idx = addr / config->block_size;
	if (config->bad_block_table[bbt_idx / BITS_PER_BYTE] & BIT(bbt_idx % BITS_PER_BYTE)) {
		*status = FLASH_BLOCK_BAD;
	} else {
		*status = FLASH_BLOCK_GOOD;
	}

	return 0;
}

static int build_bad_block_table(const struct device *dev)
{
	int ret;
	enum flash_block_status status;
	const struct mspi_nand_config *config = dev->config;
	const uint32_t bad_block_table_size =
		DIV_ROUND_UP(config->flash_size / config->block_size, BITS_PER_BYTE);

	/* Init all blocks as good */
	memset(config->bad_block_table, 0, bad_block_table_size);

	/* Scan all blocks */
	for (uint32_t addr = 0; addr < config->flash_size; addr += config->block_size) {
		ret = mspi_nand_is_bad_block(dev, addr, &status);
		if (ret != 0) {
			return ret;
		}

		if (status == FLASH_BLOCK_BAD) {
			uint32_t idx = addr / config->block_size;

			config->bad_block_table[idx / BITS_PER_BYTE] |= BIT(idx % BITS_PER_BYTE);
		}
	}

	LOG_HEXDUMP_DBG(config->bad_block_table, bad_block_table_size, "Bad block table");

	return 0;
}
#endif /* CONFIG_MSPI_NAND_BAD_BLOCK_CACHE */

static int mspi_nand_ex_op(const struct device *dev, uint16_t code, const uintptr_t in, void *out)
{
	int ret;

	ret = acquire_device(dev);
	if (ret != 0) {
		return ret;
	}

	switch (code) {
	case FLASH_EX_OP_RESET:
		ret = mspi_nand_reset(dev);
		break;
	case FLASH_EX_OP_IS_BAD_BLOCK:
#ifdef CONFIG_MSPI_NAND_BAD_BLOCK_CACHE
		ret = bad_block_table_is_bad_block(dev, *(const off_t *)in,
						   (enum flash_block_status *)out);
#else
		ret = mspi_nand_is_bad_block(dev, *(const off_t *)in,
					     (enum flash_block_status *)out);
#endif
		break;
	case FLASH_EX_OP_MARK_BAD_BLOCK:
		ret = mspi_nand_mark_bad_block(dev, *(const off_t *)in);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	release_device(dev);

	return ret;
}

#endif /* CONFIG_FLASH_EX_OP_ENABLED */

static const struct flash_parameters *flash_nand_get_parameters(const struct device *dev)
{
	const struct mspi_nand_config *config = dev->config;

	return config->parameters;
}

static int flash_nand_get_size(const struct device *dev, uint64_t *size)
{
	const struct mspi_nand_config *config = dev->config;

	*size = config->flash_size;
	return 0;
}

static int onfi_parameters_load(const struct device *dev)
{
	const struct mspi_nand_config *config = dev->config;
	struct spi_nand_onfi_parameter_page onfi;
	uint16_t computed_crc;
	uint32_t total_size;
	uint32_t block_size;
	uint32_t page_size;
	uint8_t cfg;
	int ret;

	/* Configure device to allow reading parameter page */
	ret = mspi_nand_get_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, &cfg);
	if (ret != 0) {
		return ret;
	}
	cfg |= SPI_NAND_FEATURE_CONFIG_OTP_EN;
	ret = mspi_nand_set_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, cfg);
	if (ret != 0) {
		return ret;
	}

	/* Validate that OTP_EN bit was set, otherwise read can never succeed */
	ret = mspi_nand_get_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, &cfg);
	if (ret != 0) {
		return ret;
	}
	if (!(cfg & SPI_NAND_FEATURE_CONFIG_OTP_EN)) {
		LOG_ERR("Failed to enable OTP bit to read parameter page");
		return -EIO;
	}

	/* Sanity check the on-chip ECC configuration */
	if (!(cfg & SPI_NAND_FEATURE_CONFIG_ECC_EN)) {
		LOG_WRN("On-chip ECC not enabled");
	}

	/* Load parameter info into cache (ignoring ECC errors) */
	ret = mspi_nand_page_read_to_cache(dev, 1);
	if ((ret != 0) && (ret != -EBADMSG)) {
		return ret;
	}

	/* The compiler does not know that config->parameters->write_block_size is always non-zero.
	 * As a result, it believes it is possible for the loop to be skipped, leaving both CRC
	 * variables uninitialized at the comparison point below the loop. Explicitly set the values
	 * to suppress the warnings.
	 */
	computed_crc = 0;
	onfi.integrity_crc = 1;

	/* Scan through the loaded info until we find a valid CRC.
	 * Use the assumed page size from devicetree.
	 */
	page_size = config->parameters->write_block_size;
	for (int i = 0; i < page_size; i += sizeof(onfi)) {
		ret = mspi_nand_read_from_cache(dev, 0, i, &onfi, sizeof(onfi));
		if (ret != 0) {
			return ret;
		}
		computed_crc =
			crc16(CRC16_POLY, 0x4F4E, (void *)&onfi, sizeof(onfi) - sizeof(uint16_t));
		if (computed_crc == sys_le16_to_cpu(onfi.integrity_crc)) {
			LOG_DBG("Valid CRC: %04X", computed_crc);
			break;
		}
		LOG_DBG("Parameters at offset %u corrupt (%04X != %04X)", i, computed_crc,
			sys_le16_to_cpu(onfi.integrity_crc));
	}
	if (computed_crc != sys_le16_to_cpu(onfi.integrity_crc)) {
		LOG_ERR("No valid ONFI parameters blocks found");
		return -ENOSPC;
	}

	/* Display parameters from ONFI block.
	 * "%{N}s" pads a string to N characters.
	 * "%.{N}s" prints at most N characters.
	 */
	LOG_DBG("      Manufacturer: %.12s", onfi.device_manufacturer);
	LOG_DBG("             Model: %.20s", onfi.device_model);
	LOG_DBG("  Page Size (data): %d", sys_le32_to_cpu(onfi.data_bytes_per_page));
	LOG_DBG(" Page Size (spare): %d", sys_le16_to_cpu(onfi.spare_bytes_per_page));
	LOG_DBG("   Pages per Block: %d", sys_le32_to_cpu(onfi.pages_per_block));
	LOG_DBG("   Blocks per Unit: %d", sys_le32_to_cpu(onfi.blocks_per_lun));
	LOG_DBG("             Units: %d", onfi.num_lun);
	LOG_DBG("Plane Address Bits: %d", onfi.plane_address_bits);

	/* Validate ONFI data against devicetree */
	block_size = sys_le32_to_cpu(onfi.data_bytes_per_page) *
		     sys_le32_to_cpu(onfi.pages_per_block);
	total_size = block_size * sys_le32_to_cpu(onfi.blocks_per_lun) * onfi.num_lun;
	if (sys_le32_to_cpu(onfi.data_bytes_per_page) != config->parameters->write_block_size) {
		LOG_WRN("Devicetree page size does not match ONFI page size (%d != %d)",
			sys_le32_to_cpu(onfi.data_bytes_per_page),
			config->parameters->write_block_size);
	}
	if (block_size != config->block_size) {
		LOG_WRN("Devicetree block size does not match ONFI block size (%d != %d)",
			block_size, config->block_size);
	}
	if (onfi.plane_address_bits != config->plane_addr_bits) {
		LOG_WRN("Devicetree plane address bits does not match ONFI plane address bits (%d "
			"!= %d)",
			onfi.plane_address_bits, config->plane_addr_bits);
	}
	if (total_size != config->flash_size) {
		LOG_WRN("Devicetree total size does not match ONFI total size (%d != %d)",
			total_size, config->flash_size);
	}

	/* Clear the parameter page read feature */
	cfg &= ~SPI_NAND_FEATURE_CONFIG_OTP_EN;
	return mspi_nand_set_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, cfg);
}

/**
 * @brief Configure the flash
 *
 * @param dev The flash device structure
 * @param info The flash info structure
 * @return 0 on success, negative errno code otherwise
 */
static int mspi_nand_configure(const struct device *dev)
{
	const struct mspi_nand_config *config = dev->config;
	struct mspi_nand_data *const data = dev->data;
	uint8_t jedec_id[SPI_NAND_MAX_ID_LEN];
	int ret;

	/* Validate bus is ready */
	if (!device_is_ready(config->bus)) {
		LOG_ERR("Device %s is not ready", config->bus->name);
		return -ENODEV;
	}

	k_sem_take(&data->sem, K_FOREVER);
	ret = pm_device_runtime_get(config->bus);
	if (ret != 0) {
		LOG_ERR_PM_DEVICE_RUNTIME_GET(config->bus, ret);
		k_sem_give(&data->sem);
		return ret;
	}

	/* Acquire the controller with the device configuration applied:
	 * acquire_device() only applies it on a shared bus.
	 */
	ret = mspi_dev_config(config->bus, &config->mspi_id, MSPI_NAND_DEV_CFG_MASK,
			      &config->mspi_cfg);
	if (ret != 0) {
		LOG_ERR("MSPI device configuration failed (%d)", ret);
		(void)pm_device_runtime_put(config->bus);
		k_sem_give(&data->sem);
		return ret;
	}

	/* Soft RESET chip and wait until ready again */
	ret = mspi_nand_reset(dev);
	if (ret != 0) {
		goto release;
	}

	/* Validate JEDEC ID */
	ret = mspi_nand_cmd_read_dummy(dev, SPI_NAND_CMD_READ_ID, jedec_id, config->jedec_id_len);
	if (ret != 0) {
		goto release;
	}
	if (memcmp(jedec_id, config->jedec_id, config->jedec_id_len) != 0) {
		LOG_HEXDUMP_ERR(config->jedec_id, config->jedec_id_len, "Expected JEDEC ID");
		LOG_HEXDUMP_ERR(jedec_id, config->jedec_id_len, "Queried JEDEC ID");
		ret = -EINVAL;
		goto release;
	}

	/* Load the ONFI parameter information */
	ret = onfi_parameters_load(dev);
	if (ret != 0) {
		goto release;
	}

	/* Unlock all blocks */
	ret = mspi_nand_set_feature(dev, SPI_NAND_FEATURE_ADDR_BLOCK_PROT,
				    SPI_NAND_FEATURE_BLOCK_PROT_DISABLE_ALL);
	if (ret != 0) {
		goto release;
	}

#ifdef CONFIG_MSPI_NAND_BAD_BLOCK_CACHE
	/* Build the in-memory bad block table */
	ret = build_bad_block_table(dev);
#endif /* CONFIG_MSPI_NAND_BAD_BLOCK_CACHE */

release:
	release_device(dev);
	return ret;
}

static int mspi_nand_pm_control(const struct device *dev, enum pm_device_action action)
{
	int rc = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_RESUME:
		/* Some Macronix parts support a "Deep Power Down" mode.
		 * Not implemented.
		 */
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		/* Coming out of power off */
		rc = mspi_nand_configure(dev);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		rc = -ENOSYS;
	}

	return rc;
}

/**
 * @brief Initialize and configure the flash
 *
 * @param name The flash name
 * @return 0 on success, negative errno code otherwise
 */
static int mspi_nand_init(const struct device *dev)
{
	struct mspi_nand_data *const data = dev->data;

	k_sem_init(&data->sem, 1, K_SEM_MAX_LIMIT);

	return pm_device_driver_init(dev, mspi_nand_pm_control);
}

static DEVICE_API(flash, mspi_nand_api) = {
	.read = mspi_nand_read,
	.write = mspi_nand_write,
	.erase = mspi_nand_erase,
	.get_parameters = flash_nand_get_parameters,
	.get_size = flash_nand_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = mspi_nand_pages_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#if defined(CONFIG_FLASH_EX_OP_ENABLED)
	.ex_op = mspi_nand_ex_op,
#endif /* CONFIG_FLASH_EX_OP_ENABLED */
};

#define MSPI_NAND_PACKET_DATA_LIMIT(idx) DT_PROP_OR(DT_INST_BUS(idx), packet_data_limit, 0)

/* clang-format off */
#define DEFINE_PAGE_LAYOUT(idx)                                                                    \
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,                                                       \
		   (.layout = {                                                                    \
			    .pages_count = DT_INST_PROP(idx, size_bytes) /                         \
					   DT_INST_PROP(idx, erase_block_size),                    \
			    .pages_size = DT_INST_PROP(idx, erase_block_size),                     \
		    },))
/* clang-format on */

#define MSPI_NAND_INST(idx)                                                                        \
	BUILD_ASSERT(IS_POWER_OF_TWO(DT_INST_PROP(idx, write_block_size)),                         \
		     "write-block-size must be a power of 2");                                     \
	BUILD_ASSERT(IS_POWER_OF_TWO(DT_INST_PROP(idx, erase_block_size)),                         \
		     "erase-block-size must be a power of 2");                                     \
	BUILD_ASSERT(IS_POWER_OF_TWO(DT_INST_PROP(idx, plane_bytes)),                              \
		     "plane-bytes must be a power of 2");                                          \
	BUILD_ASSERT(DT_INST_PROP(idx, erase_block_size) % DT_INST_PROP(idx, write_block_size) ==  \
			     0,                                                                    \
		     "erase-block-size must be a multiple of write-block-size");                   \
	BUILD_ASSERT(DT_INST_PROP(idx, plane_bytes) % DT_INST_PROP(idx, erase_block_size) == 0,    \
		     "plane-bytes must be a multiple of erase-block-size");                        \
	BUILD_ASSERT(DT_INST_PROP(idx, size_bytes) % DT_INST_PROP(idx, plane_bytes) == 0,          \
		     "size-bytes must be a multiple of plane-bytes");                              \
                                                                                                   \
	BUILD_ASSERT((MSPI_NAND_PACKET_DATA_LIMIT(idx) == 0) ||                                    \
			     (DT_INST_PROP(idx, write_block_size) <=                               \
			      MSPI_NAND_PACKET_DATA_LIMIT(idx)),                                   \
		     "write-block-size exceeds the controller packet data limit");                 \
                                                                                                   \
	BUILD_ASSERT(DT_INST_ENUM_IDX_OR(idx, mspi_io_mode, MSPI_IO_MODE_SINGLE) ==                \
			     MSPI_IO_MODE_SINGLE,                                                  \
		     "mspi-io-mode must be MSPI_IO_MODE_SINGLE");                                  \
	BUILD_ASSERT(DT_INST_ENUM_IDX_OR(idx, mspi_data_rate, MSPI_DATA_RATE_SINGLE) ==            \
			     MSPI_DATA_RATE_SINGLE,                                                \
		     "mspi-data-rate must be MSPI_DATA_RATE_SINGLE");                              \
                                                                                                   \
	static const struct flash_parameters mspi_nand_##idx##_parameters = {                      \
		.write_block_size = DT_INST_PROP(idx, write_block_size),                           \
		.erase_value = 0xff,                                                               \
	};                                                                                         \
                                                                                                   \
	static const uint8_t mspi_nand_##idx##_jedec_id[] = DT_INST_PROP(idx, jedec_id);           \
	BUILD_ASSERT(ARRAY_SIZE(mspi_nand_##idx##_jedec_id) <= SPI_NAND_MAX_ID_LEN);               \
                                                                                                   \
	IF_ENABLED(CONFIG_MSPI_NAND_BAD_BLOCK_CACHE, (                                             \
	static uint8_t mspi_nand_##idx##_bad_block_table[DIV_ROUND_UP(                             \
		DT_INST_PROP(idx, size_bytes) / DT_INST_PROP(idx, erase_block_size),               \
		BITS_PER_BYTE)];                                                                   \
	))                                                                                         \
                                                                                                   \
	static const struct mspi_nand_config mspi_nand_##idx##_config = {                          \
		.bus = DEVICE_DT_GET(DT_INST_BUS(idx)),                                            \
		.mspi_id = MSPI_DEVICE_ID_DT_INST(idx),                                            \
		.mspi_cfg = MSPI_DEVICE_CONFIG_DT_INST(idx),                                       \
		.transfer_timeout = DT_INST_PROP(idx, transfer_timeout),                           \
		.multiperipheral_bus = DT_PROP(DT_INST_BUS(idx), software_multiperipheral),        \
		.parameters = &mspi_nand_##idx##_parameters,                                       \
		.flash_size = DT_INST_PROP(idx, size_bytes),                                       \
		.block_size = DT_INST_PROP(idx, erase_block_size),                                 \
		.block_erase_us = DT_INST_PROP(idx, block_erase_duration_max),                     \
		.page_program_us = DT_INST_PROP(idx, page_program_duration_max),                   \
		.page_read_us = DT_INST_PROP(idx, page_read_duration_max),                         \
		.reset_us = DT_INST_PROP(idx, reset_duration_max),                                 \
		.addr_offset_mask = DT_INST_PROP(idx, write_block_size) - 1,                       \
		.addr_page_shift = LOG2(DT_INST_PROP(idx, write_block_size)),                      \
		.addr_block_shift = LOG2(DT_INST_PROP(idx, erase_block_size)),                     \
		.plane_addr_bits =                                                                 \
			LOG2(DT_INST_PROP(idx, size_bytes) / DT_INST_PROP(idx, plane_bytes)),      \
		.jedec_id = mspi_nand_##idx##_jedec_id,                                            \
		.jedec_id_len = ARRAY_SIZE(mspi_nand_##idx##_jedec_id),                            \
		.has_program_plane_select = DT_INST_PROP(idx, has_program_plane_select),           \
		.has_read_plane_select = DT_INST_PROP(idx, has_read_plane_select),                 \
		IF_ENABLED(CONFIG_MSPI_NAND_BAD_BLOCK_CACHE, (                                     \
			.bad_block_table = mspi_nand_##idx##_bad_block_table,                      \
		))                                                                                 \
		DEFINE_PAGE_LAYOUT(idx)};                                                          \
                                                                                                   \
	static struct mspi_nand_data mspi_nand_##idx##_data;                                       \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(idx, mspi_nand_pm_control);                                       \
	DEVICE_DT_INST_DEFINE(idx, &mspi_nand_init, PM_DEVICE_DT_INST_GET(idx),                    \
			      &mspi_nand_##idx##_data, &mspi_nand_##idx##_config, POST_KERNEL,     \
			      CONFIG_MSPI_NAND_INIT_PRIORITY, &mspi_nand_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_NAND_INST)
