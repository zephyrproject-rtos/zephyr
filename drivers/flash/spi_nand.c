/*
 * Copyright (c) 2022-2025 Macronix International Co., Ltd.
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_spi_nand

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "spi_nand.h"

struct spi_nand_config {
	/* Devicetree SPI configuration */
	struct spi_dt_spec spi;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	/* Flash page layout can be determined from devicetree. */
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
	/* Flash parameters */
	const struct flash_parameters *parameters;
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
	/* Expected JEDEC ID, from jedec-id property */
	const uint8_t *jedec_id;
	/* Length of the JEDEC ID */
	uint8_t jedec_id_len;
};

struct spi_nand_data {
	/* Access semaphore */
	struct k_sem sem;
};

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

LOG_MODULE_REGISTER(spi_nand, CONFIG_FLASH_LOG_LEVEL);

/* Everything necessary to acquire owning access to the device. */
static void acquire_device(const struct device *dev)
{
	const struct spi_nand_config *config = dev->config;
	struct spi_nand_data *const data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
	(void)pm_device_runtime_get(config->spi.bus);
}

/* Everything necessary to release access to the device. */
static void release_device(const struct device *dev)
{
	const struct spi_nand_config *config = dev->config;
	struct spi_nand_data *const data = dev->data;

	(void)pm_device_runtime_put(config->spi.bus);
	k_sem_give(&data->sem);
}

static int spi_nand_transceive(const struct device *dev, const struct spi_buf_set *tx_set,
			       const struct spi_buf_set *rx_set)
{
	const struct spi_nand_config *config = dev->config;
	int ret;

	if (rx_set == NULL) {
		ret = spi_write_dt(&config->spi, tx_set);
	} else {
		ret = spi_transceive_dt(&config->spi, tx_set, rx_set);
	}
	if (ret == 0) {
		/** Communications unreliable without minimal delay between transactions */
		k_sleep(K_TICKS(1));
	} else {
		LOG_DBG("SPI transaction failed (%d)", ret);
	}
	return ret;
}

/*
 * @brief Send an SPI command
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
static int spi_nand_access(const struct device *const dev, uint8_t opcode, unsigned int access,
			   off_t addr, void *data, size_t length)
{
	bool is_addressed = (access & NAND_ACCESS_ADDRESSED) != 0U;
	bool is_write = (access & NAND_ACCESS_WRITE) != 0U;
	uint8_t buf[5] = {0};
	uint8_t address_len;
	struct spi_buf spi_buf[2] = {
		{
			.buf = buf,
			.len = 1,
		},
		{.buf = data, .len = length},
	};

	buf[0] = opcode;
	if (is_addressed) {
		union {
			uint32_t u32;
			uint8_t u8[4];
		} addr32 = {
			.u32 = sys_cpu_to_be32(addr),
		};

		if ((access & NAND_ACCESS_32BIT_ADDR) != 0U) {
			address_len = 4;
		} else if ((access & NAND_ACCESS_24BIT_ADDR) != 0U) {
			address_len = 3;
		} else if ((access & NAND_ACCESS_16BIT_ADDR) != 0U) {
			address_len = 2;
		} else if ((access & NAND_ACCESS_8BIT_ADDR) != 0U) {
			address_len = 1;
		} else {
			address_len = 0;
		}
		memcpy(&buf[1], &addr32.u8[4 - address_len], address_len);
		spi_buf[0].len += address_len;
	}

	if (access & NAND_ACCESS_DUMMY_BYTE) {
		spi_buf[0].len += 1;
	}

	const struct spi_buf_set tx_set = {
		.buffers = spi_buf,
		/* Non zero length means that data follows opcode, so there are two buffers to tx */
		.count = (length != 0) ? 2 : 1,
	};

	const struct spi_buf_set rx_set = {
		.buffers = spi_buf,
		.count = 2,
	};

	return spi_nand_transceive(dev, &tx_set, is_write ? NULL : &rx_set);
}

#define spi_nand_cmd_read(dev, opcode, dest, length)                                               \
	spi_nand_access(dev, opcode, 0, 0, dest, length)
#define spi_nand_cmd_read_dummy(dev, opcode, dest, length)                                         \
	spi_nand_access(dev, opcode, NAND_ACCESS_DUMMY_BYTE, 0, dest, length)
#define spi_nand_cmd_write(dev, opcode) spi_nand_access(dev, opcode, NAND_ACCESS_WRITE, 0, NULL, 0)

/* Single structure describing a GET_FEATURE/SET_FEATURE command */
struct spi_nand_feature_frame {
	/* SPI_NAND_CMD_GET_FEATURE/SPI_NAND_CMD_SET_FEATURE */
	uint8_t command;
	/* Value from SPI_NAND_FEATURE_ADDR_* */
	uint8_t address;
	/* Value to write or is read */
	uint8_t data;
};

/* Optimised version of spi_nand_access to minimise the overhead of polling status registers.
 * Removes the need for the SPI controller to reconfigure the peripheral after sending the first 2
 * bytes.
 */
static int spi_nand_feature_op(const struct device *dev, struct spi_nand_feature_frame *to_nand,
			       struct spi_nand_feature_frame *from_nand)
{
	struct spi_buf spi_buf[2] = {
		{
			.buf = to_nand,
			.len = sizeof(*to_nand),
		},
		{
			.buf = from_nand,
			.len = sizeof(*from_nand),
		},
	};
	const struct spi_buf_set tx_set = {
		.buffers = &spi_buf[0],
		.count = 1,
	};
	const struct spi_buf_set rx_set = {
		.buffers = &spi_buf[1],
		.count = 1,
	};

	return spi_nand_transceive(dev, &tx_set, from_nand ? &rx_set : NULL);
}

/* Read feature data from a register */
static int spi_nand_get_feature(const struct device *dev, uint8_t reg, uint8_t *feature)
{
	struct spi_nand_feature_frame out = {
		.command = SPI_NAND_CMD_GET_FEATURE,
		.address = reg,
	};
	struct spi_nand_feature_frame in;
	int ret;

	ret = spi_nand_feature_op(dev, &out, &in);
	*feature = in.data;
	return ret;
}

/* Write feature data to a register */
static int spi_nand_set_feature(const struct device *dev, uint8_t reg, uint8_t feature)
{
	struct spi_nand_feature_frame out = {
		.command = SPI_NAND_CMD_SET_FEATURE,
		.address = reg,
		.data = feature,
	};

	return spi_nand_feature_op(dev, &out, NULL);
}

/* Wait until all operations are complete */
static int spi_nand_wait_until_ready(const struct device *dev, const char *op, uint32_t timeout_us,
				     uint32_t poll_us, uint8_t *status)
{
	k_ticks_t t = k_uptime_ticks();
	k_timepoint_t timeout;
	int ret;

	timeout = sys_timepoint_calc(K_USEC(timeout_us));
	do {
		ret = spi_nand_get_feature(dev, SPI_NAND_FEATURE_ADDR_STATUS, status);
		if (ret != 0) {
			return ret;
		}

		if ((*status & SPI_NAND_FEATURE_STATUS_OIP) == 0U) {
			t = k_uptime_ticks() - t;
			LOG_DBG("Ready after %u us (Op %s, Status %02X)", k_ticks_to_us_near32(t),
				op, *status);
			return ret;
		}
		k_sleep(K_USEC(poll_us));
	} while (!sys_timepoint_expired(timeout));

	LOG_ERR("Ready timeout (Op %s, Status %02X)", op, *status);
	return -ETIMEDOUT;
}

/* Read page to cache, assumes device already acquired */
static int spi_nand_page_read_to_cache(const struct device *dev, uint32_t page)
{
	const struct spi_nand_config *config = dev->config;
	uint8_t ecc_status;
	uint8_t status;
	int ret;

	/* Trigger the read to cache */
	ret = spi_nand_access(dev, SPI_NAND_CMD_PAGE_READ,
			      NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR, page, NULL, 0);
	if (ret != 0) {
		return ret;
	}

	/* Wait until the read to cache completes (poll with no delays) */
	ret = spi_nand_wait_until_ready(dev, "read", config->page_read_us, 0, &status);
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
static int spi_nand_read_from_cache(const struct device *dev, uint16_t offset, void *dest,
				    size_t size)
{
	return spi_nand_access(dev, SPI_NAND_CMD_READ_CACHE,
			       NAND_ACCESS_ADDRESSED | NAND_ACCESS_16BIT_ADDR |
				       NAND_ACCESS_DUMMY_BYTE,
			       offset, dest, size);
}

static bool valid_region(const struct device *dev, off_t addr, size_t size)
{
	const struct spi_nand_config *config = dev->config;

	if ((addr < 0) || (addr >= config->flash_size) || (size > config->flash_size) ||
	    ((config->flash_size - addr) < size)) {
		return false;
	}
	return true;
}

static int spi_nand_read(const struct device *dev, off_t addr, void *dest, size_t size)
{
	const struct spi_nand_config *config = dev->config;
	uint8_t *dest_u8 = dest;
	uint32_t page_address;
	uint16_t page_offset;
	uint16_t bytes_to_end;
	uint16_t bytes_to_read;
	int ret = 0;

	if (size == 0) {
		/* No work to do */
		return 0;
	}

	/* Read area must be subregion of device */
	if (!valid_region(dev, addr, size)) {
		return -EINVAL;
	}

	acquire_device(dev);

	while (size > 0) {
		page_address = addr >> config->addr_page_shift;
		page_offset = addr & config->addr_offset_mask;
		bytes_to_end = config->parameters->write_block_size - page_offset;
		bytes_to_read = MIN(size, bytes_to_end);

		/* Copy data from main storage to cache */
		LOG_DBG("Read %d from %06x:%03x", bytes_to_read, page_address, page_offset);
		ret = spi_nand_page_read_to_cache(dev, page_address);
		if (ret != 0) {
			LOG_DBG("Copy from NAND to device cache failed (%d)", ret);
			break;
		}

		/* Read data out of cache */
		ret = spi_nand_read_from_cache(dev, page_offset, dest_u8, bytes_to_read);
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

static int spi_nand_write(const struct device *dev, off_t addr, const void *src, size_t size)
{
	const struct spi_nand_config *config = dev->config;
	uint32_t write_block = config->parameters->write_block_size;
	uint8_t *src_u8 = (void *)src;
	uint32_t page_address;
	uint8_t status;
	int ret = 0;

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

	acquire_device(dev);

	while (size > 0) {
		/* Enable write operation */
		ret = spi_nand_cmd_write(dev, SPI_NAND_CMD_WRITE_ENABLE);
		if (ret != 0) {
			break;
		}

		page_address = addr >> config->addr_page_shift;
		LOG_DBG("Write %d to %06x:000", write_block, page_address);

		/* Copy data to cache (at offset 0) */
		ret = spi_nand_access(dev, SPI_NAND_CMD_PROGRAM_LOAD,
				      NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED |
					      NAND_ACCESS_16BIT_ADDR,
				      0, src_u8, write_block);
		if (ret != 0) {
			LOG_DBG("Copy to device cache failed (%d)", ret);
			break;
		}

		/* Program the cache to the appropriate page */
		ret = spi_nand_access(dev, SPI_NAND_CMD_PROGRAM_EXECUTE,
				      NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED |
					      NAND_ACCESS_24BIT_ADDR,
				      page_address, NULL, 0);
		if (ret != 0) {
			LOG_DBG("Program from cache to NAND failed (%d)", ret);
			break;
		}

		/* Wait for the write to complete (poll every 0.1ms) */
		ret = spi_nand_wait_until_ready(dev, "write", config->page_program_us, 100,
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

static int spi_nand_erase(const struct device *dev, off_t addr, size_t size)
{
	const struct spi_nand_config *config = dev->config;
	uint32_t page_address;
	uint8_t status;
	int ret = 0;

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

	acquire_device(dev);

	while (size > 0) {
		/* Enable write (erase) operation */
		ret = spi_nand_cmd_write(dev, SPI_NAND_CMD_WRITE_ENABLE);
		if (ret != 0) {
			break;
		}

		/* Start the block erase */
		page_address = addr >> config->addr_page_shift;
		LOG_DBG("Erasing block starting at %06x", page_address);
		ret = spi_nand_access(dev, SPI_NAND_CMD_BLOCK_ERASE,
				      NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR, page_address,
				      NULL, 0);
		if (ret != 0) {
			break;
		}
		/* Wait for the erase to complete (poll every 0.5ms) */
		ret = spi_nand_wait_until_ready(dev, "erase", config->block_erase_us, 500, &status);
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

#if defined(CONFIG_FLASH_PAGE_LAYOUT)

static void spi_nand_pages_layout(const struct device *dev,
				  const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct spi_nand_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *flash_nand_get_parameters(const struct device *dev)
{
	const struct spi_nand_config *config = dev->config;

	return config->parameters;
}

static int flash_nand_get_size(const struct device *dev, uint64_t *size)
{
	const struct spi_nand_config *config = dev->config;

	*size = config->flash_size;
	return 0;
}

static int onfi_parameters_load(const struct device *dev)
{
	const struct spi_nand_config *config = dev->config;
	struct spi_nand_onfi_parameter_page onfi;
	uint16_t computed_crc;
	uint32_t total_size;
	uint32_t block_size;
	uint32_t page_size;
	uint8_t cfg;
	int ret;

	/* Configure device to allow reading parameter page */
	ret = spi_nand_get_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, &cfg);
	if (ret != 0) {
		return ret;
	}
	cfg |= SPI_NAND_FEATURE_CONFIG_OTP_EN;
	ret = spi_nand_set_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, cfg);
	if (ret != 0) {
		return ret;
	}

	/* Validate that OTP_EN bit was set, otherwise read can never succeed */
	ret = spi_nand_get_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, &cfg);
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

	/* Load parameter info into cache */
	ret = spi_nand_page_read_to_cache(dev, 1);
	if (ret != 0) {
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
		ret = spi_nand_read_from_cache(dev, i, &onfi, sizeof(onfi));
		if (ret != 0) {
			return ret;
		}
		computed_crc =
			crc16(CRC16_POLY, 0x4F4E, (void *)&onfi, sizeof(onfi) - sizeof(uint16_t));
		if (computed_crc == onfi.integrity_crc) {
			LOG_DBG("Valid CRC: %04X", computed_crc);
			break;
		}
		LOG_DBG("Parameters at offset %u corrupt (%04X != %04X)", i, computed_crc,
			onfi.integrity_crc);
	}
	if (computed_crc != onfi.integrity_crc) {
		LOG_ERR("No valid ONFI parameters blocks found");
		return -ENOSPC;
	}

	/* Display parameters from ONFI block.
	 * "%{N}s" pads a string to N characters.
	 * "%.{N}s" prints at most N characters.
	 */
	LOG_DBG("     Manufacturer: %.12s", onfi.device_manufacturer);
	LOG_DBG("            Model: %.20s", onfi.device_model);
	LOG_DBG(" Page Size (data): %d", onfi.data_bytes_per_page);
	LOG_DBG("Page Size (spare): %d", onfi.spare_bytes_per_page);
	LOG_DBG("  Pages per Block: %d", onfi.pages_per_block);
	LOG_DBG("  Blocks per Unit: %d", onfi.blocks_per_lun);
	LOG_DBG("            Units: %d", onfi.num_lun);

	/* Validate ONFI data against devicetree */
	block_size = onfi.data_bytes_per_page * onfi.pages_per_block;
	total_size = block_size * onfi.blocks_per_lun * onfi.num_lun;
	if (onfi.data_bytes_per_page != config->parameters->write_block_size) {
		LOG_WRN("Devicetree page size does not match ONFI page size (%d != %d)",
			onfi.data_bytes_per_page, config->parameters->write_block_size);
	}
	if (block_size != config->block_size) {
		LOG_WRN("Devicetree block size does not match ONFI block size (%d != %d)",
			block_size, config->block_size);
	}
	if (total_size != config->flash_size) {
		LOG_WRN("Devicetree total size does not match ONFI total size (%d != %d)",
			total_size, config->flash_size);
	}

	/* Clear the parameter page read feature */
	cfg &= ~SPI_NAND_FEATURE_CONFIG_OTP_EN;
	return spi_nand_set_feature(dev, SPI_NAND_FEATURE_ADDR_CONFIG, cfg);
}

/**
 * @brief Configure the flash
 *
 * @param dev The flash device structure
 * @param info The flash info structure
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nand_configure(const struct device *dev)
{
	const struct spi_nand_config *config = dev->config;
	uint8_t jedec_id[SPI_NAND_MAX_ID_LEN];
	uint8_t status;
	int ret;

	/* Validate bus and CS is ready */
	if (!spi_is_ready_dt(&config->spi)) {
		return -ENODEV;
	}

	acquire_device(dev);

	/* Soft RESET chip and wait until ready again */
	ret = spi_nand_cmd_write(dev, SPI_NAND_CMD_RESET);
	if (ret != 0) {
		goto release;
	}
	ret = spi_nand_wait_until_ready(dev, "reset", config->reset_us, 100, &status);
	if (ret != 0) {
		goto release;
	}

	/* Validate JEDEC ID */
	ret = spi_nand_cmd_read_dummy(dev, SPI_NAND_CMD_READ_ID, jedec_id, config->jedec_id_len);
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
	ret = spi_nand_set_feature(dev, SPI_NAND_FEATURE_ADDR_BLOCK_PROT,
				   SPI_NAND_FEATURE_BLOCK_PROT_DISABLE_ALL);
release:
	release_device(dev);
	return ret;
}

static int spi_nand_pm_control(const struct device *dev, enum pm_device_action action)
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
		rc = spi_nand_configure(dev);
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
static int spi_nand_init(const struct device *dev)
{
	struct spi_nand_data *const data = dev->data;

	k_sem_init(&data->sem, 1, K_SEM_MAX_LIMIT);

	return pm_device_driver_init(dev, spi_nand_pm_control);
}

static DEVICE_API(flash, spi_nand_api) = {
	.read = spi_nand_read,
	.write = spi_nand_write,
	.erase = spi_nand_erase,
	.get_parameters = flash_nand_get_parameters,
	.get_size = flash_nand_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_nand_pages_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

/* clang-format off */
#define DEFINE_PAGE_LAYOUT(idx)                                                                    \
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,                                                       \
		   (.layout = {                                                                    \
			    .pages_count = DT_INST_PROP(idx, size_bytes) /                         \
					   DT_INST_PROP(idx, erase_block_size),                    \
			    .pages_size = DT_INST_PROP(idx, erase_block_size),                     \
		    },))
/* clang-format on */

#define SPI_NAND_INST(idx)                                                                         \
	BUILD_ASSERT(IS_POWER_OF_TWO(DT_INST_PROP(idx, write_block_size)),                         \
		     "write-block-size must be a power of 2");                                     \
	BUILD_ASSERT(IS_POWER_OF_TWO(DT_INST_PROP(idx, erase_block_size)),                         \
		     "erase-block-size must be a power of 2");                                     \
	BUILD_ASSERT(DT_INST_PROP(idx, erase_block_size) % DT_INST_PROP(idx, write_block_size) ==  \
			     0,                                                                    \
		     "erase-block-size must be a multiple of write-block-size");                   \
	BUILD_ASSERT(DT_INST_PROP(idx, size_bytes) % DT_INST_PROP(idx, erase_block_size) == 0,     \
		     "size-bytes must be a multiple of erase-block-size");                         \
	static const struct flash_parameters spi_nand_##idx##_parameters = {                       \
		.write_block_size = DT_INST_PROP(idx, write_block_size),                           \
		.erase_value = 0xff,                                                               \
	};                                                                                         \
	static const uint8_t spi_nand_##idx##_jedec_id[] = DT_INST_PROP(idx, jedec_id);            \
	BUILD_ASSERT(ARRAY_SIZE(spi_nand_##idx##_jedec_id) <= SPI_NAND_MAX_ID_LEN);                \
	static const struct spi_nand_config spi_nand_##idx##_config = {                            \
		.spi = SPI_DT_SPEC_INST_GET(idx, SPI_WORD_SET(8)),                                 \
		.parameters = &spi_nand_##idx##_parameters,                                        \
		.flash_size = DT_INST_PROP(idx, size_bytes),                                       \
		.block_size = DT_INST_PROP(idx, erase_block_size),                                 \
		.block_erase_us = DT_INST_PROP(idx, block_erase_duration_max),                     \
		.page_program_us = DT_INST_PROP(idx, page_program_duration_max),                   \
		.page_read_us = DT_INST_PROP(idx, page_read_duration_max),                         \
		.reset_us = DT_INST_PROP(idx, reset_duration_max),                                 \
		.addr_offset_mask = DT_INST_PROP(idx, write_block_size) - 1,                       \
		.addr_page_shift = LOG2(DT_INST_PROP(idx, write_block_size)),                      \
		.jedec_id = spi_nand_##idx##_jedec_id,                                             \
		.jedec_id_len = ARRAY_SIZE(spi_nand_##idx##_jedec_id),                             \
		DEFINE_PAGE_LAYOUT(idx)};                                                          \
	static struct spi_nand_data spi_nand_##idx##_data;                                         \
	PM_DEVICE_DT_INST_DEFINE(idx, spi_nand_pm_control);                                        \
	DEVICE_DT_INST_DEFINE(idx, &spi_nand_init, PM_DEVICE_DT_INST_GET(idx),                     \
			      &spi_nand_##idx##_data, &spi_nand_##idx##_config, POST_KERNEL,       \
			      CONFIG_SPI_NAND_INIT_PRIORITY, &spi_nand_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_NAND_INST)
