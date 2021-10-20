/*
 * Copyright (c) 2021 Macronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_spi_nand

#include <errno.h>
#include <drivers/flash.h>
#include <drivers/spi.h>
#include <init.h>
#include <string.h>
#include <logging/log.h>

#include "spi_nand.h"

LOG_MODULE_REGISTER(spi_nand, CONFIG_FLASH_LOG_LEVEL);

/* Build-time data associated with the device. */
struct spi_nand_config {
	/* Devicetree SPI configuration */
	struct spi_dt_spec spi;
	uint8_t id[2];
};

/**
 * struct spi_nand_data - Structure for defining the SPI NAND access
 * @sem: The semaphore to access to the flash
 */
struct spi_nand_data {
	struct k_sem sem;

	/* Number of bytes per page */
	uint16_t page_size;

	/* Size of flash, in bytes */
	uint32_t flash_size;
};

static const struct flash_parameters flash_nand_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff,
};

/* Indicates that an access command includes bytes for the address.
 * If not provided the opcode is not followed by address bytes.
 */
#define NAND_ACCESS_ADDRESSED BIT(0)

/* Indicates that addressed access uses a 8-bit address regardless of
 * spi_nand_data::flag_8bit_addr.
 */
#define NAND_ACCESS_8BIT_ADDR BIT(1)

/* Indicates that addressed access uses a 16-bit address regardless of
 * spi_nand_data::flag_16bit_addr.
 */
#define NAND_ACCESS_16BIT_ADDR BIT(2)

/* Indicates that addressed access uses a 24-bit address regardless of
 * spi_nand_data::flag_32bit_addr.
 */
#define NAND_ACCESS_24BIT_ADDR BIT(3)

/* Indicates that addressed access uses a 32-bit address regardless of
 * spi_nand_data::flag_32bit_addr.
 */
#define NAND_ACCESS_32BIT_ADDR BIT(4)

/* Indicates that an access command is performing a write.  If not
 * provided access is a read.
 */
#define NAND_ACCESS_WRITE BIT(5)

#define NAND_ACCESS_DUMMY BIT(6)

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
static int spi_nand_access(const struct device *const dev,
			  uint8_t opcode, unsigned int access,
			  off_t addr, void *data, size_t length)
{
	const struct spi_nand_config *const driver_cfg = dev->config;
	bool is_addressed = (access & NAND_ACCESS_ADDRESSED) != 0U;
	bool is_write = (access & NAND_ACCESS_WRITE) != 0U;
	uint8_t buf[5] = { 0 };
	struct spi_buf spi_buf[2] = {
		{
			.buf = buf,
			.len = 1,
		},
		{
			.buf = data,
			.len = length
		}
	};

	buf[0] = opcode;
	if (is_addressed) {
		union {
			uint32_t u32;
			uint8_t u8[4];
		} addr32 = {
			.u32 = sys_cpu_to_be32(addr),
		};

		if (access & NAND_ACCESS_32BIT_ADDR) {
			memcpy(&buf[1], &addr32.u8[0], 4);
			spi_buf[0].len += 4;
		} else if (access & NAND_ACCESS_24BIT_ADDR){
			memcpy(&buf[1], &addr32.u8[1], 3);
			spi_buf[0].len += 3;
		} else if (access & NAND_ACCESS_16BIT_ADDR){
			memcpy(&buf[1], &addr32.u8[2], 2);
			spi_buf[0].len += 2;
		} else if (access & NAND_ACCESS_8BIT_ADDR){
			memcpy(&buf[1], &addr32.u8[3], 1);
			spi_buf[0].len += 1;
		}
	};

	if (access & NAND_ACCESS_DUMMY) {
		spi_buf[0].len += 1;
	}

	const struct spi_buf_set tx_set = {
		.buffers = spi_buf,
		.count = (length != 0) ? 2 : 1,
	};

	const struct spi_buf_set rx_set = {
		.buffers = spi_buf,
		.count = 2,
	};

	if (is_write) {
		return spi_write_dt(&driver_cfg->spi, &tx_set);
	}

	return spi_transceive_dt(&driver_cfg->spi, &tx_set, &rx_set);
}

#define spi_nand_cmd_read(dev, opcode, dest, length) \
	spi_nand_access(dev, opcode, 0, 0, dest, length)
#define spi_nand_cmd_addr_read(dev, opcode, addr, dest, length) \
	spi_nand_access(dev, opcode, NAND_ACCESS_ADDRESSED, addr, dest, length)
#define spi_nand_cmd_write(dev, opcode) \
	spi_nand_access(dev, opcode, NAND_ACCESS_WRITE, 0, NULL, 0)
#define spi_nand_cmd_addr_write(dev, opcode, addr, src, length) \
	spi_nand_access(dev, opcode, NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED, \
		       addr, (void *)src, length)

/**
 * @brief Wait until the flash is ready
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * This function should be invoked after every ERASE, PROGRAM, or
 * WRITE_STATUS operation before continuing.  This allows us to assume
 * that the device is ready to accept new commands at any other point
 * in the code.
 *
 * @param dev The device structure
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nand_wait_until_ready(const struct device *dev)
{
	int ret;
	uint8_t reg;

	do {
		ret = spi_nand_access(dev, SPI_NAND_CMD_GET_FEATURE, NAND_ACCESS_ADDRESSED | NAND_ACCESS_8BIT_ADDR, SPI_NAND_FEA_ADDR_STATUS, &reg, sizeof(reg));
	} while (!ret && (reg & SPI_NAND_WIP_BIT));

	return ret;
}

/* Everything necessary to acquire owning access to the device.
 *
 * This means taking the lock .
 */
static void acquire_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nand_data *const driver_data = dev->data;

		k_sem_take(&driver_data->sem, K_FOREVER);
	}
}

/* Everything necessary to release access to the device.
 *
 * This means releasing the lock.
 */
static void release_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nand_data *const driver_data = dev->data;

		k_sem_give(&driver_data->sem);
	}
}

static int spi_nand_read(const struct device *dev, off_t addr, void *dest,
			size_t size)
{
	//const size_t flash_size = dev_flash_size(dev);
	int ret = 0;
	uint32_t offset = 0;
	uint32_t chunk = 0;
	uint32_t read_bytes = 0;

	acquire_device(dev);

	while (size > 0) {
		// Read on _page_size_bytes boundaries (Default 2048 bytes a page)
		offset = addr % SPI_NAND_PAGE_SIZE;
		chunk = (offset + size < SPI_NAND_PAGE_SIZE) ? size : (SPI_NAND_PAGE_SIZE - offset);
		read_bytes = chunk;

		ret = spi_nand_access(dev, SPI_NAND_CMD_PAGE_READ, NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR, addr >> 12, NULL, 0);

		ret = spi_nand_wait_until_ready(dev);

		ret = spi_nand_access(dev, SPI_NAND_CMD_READ_CACHE, NAND_ACCESS_ADDRESSED | NAND_ACCESS_16BIT_ADDR | NAND_ACCESS_DUMMY, (addr & SPI_NAND_PAGE_MASK) | 0x1000, dest, read_bytes);

		dest = (uint8_t *)dest + chunk;
		addr += SPI_NAND_PAGE_OFFSET;
		size -= chunk;
	}

	release_device(dev);

	return ret;
}

static int spi_nand_write(const struct device *dev, off_t addr,
			 const void *src,
			 size_t size)
{
	//const size_t flash_size = dev_flash_size(dev);
	//const uint16_t page_size = dev_page_size(dev);
	int ret = 0;
	uint32_t offset = 0;
	uint32_t chunk = 0;
	uint32_t written_bytes = 0;

		while (size > 0) {

			/* Don't write more than a page. */
        		offset = addr % SPI_NAND_PAGE_SIZE;
        		chunk = (offset + size < SPI_NAND_PAGE_SIZE) ? size : (SPI_NAND_PAGE_SIZE - offset);
        		written_bytes = chunk;

			spi_nand_cmd_write(dev, SPI_NAND_CMD_WREN);

			ret = spi_nand_access(dev, SPI_NAND_CMD_PP_LOAD, NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED | NAND_ACCESS_16BIT_ADDR, (addr & SPI_NAND_PAGE_MASK) | 0x1000, (void *)src, written_bytes);


			ret = spi_nand_access(dev, SPI_NAND_CMD_PROGRAM_EXEC, NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR, addr >> 12, NULL, 0);

        		src = (const uint8_t *)(src) + chunk;
        		addr += SPI_NAND_PAGE_OFFSET;
        		size -= chunk;

			ret = spi_nand_wait_until_ready(dev);
		}

	return ret;
}

static int spi_nand_erase(const struct device *dev, off_t addr, size_t size)
{
	//const size_t flash_size = dev_flash_size(dev);
	int ret = 0;

	spi_nand_cmd_write(dev, SPI_NAND_CMD_WREN);

	ret = spi_nand_access(dev, SPI_NAND_CMD_BE, NAND_ACCESS_ADDRESSED | NAND_ACCESS_24BIT_ADDR, addr >> 12, NULL, 0);

			ret = spi_nand_wait_until_ready(dev);

	return ret;
}

static int spi_nand_check_id(const struct device *dev)
{
	const struct spi_nand_config *cfg = dev->config;
	uint8_t const *expected_id = cfg->id;
	uint8_t read_id[SPI_NAND_ID_LEN];
	if (read_id == NULL) {
		return -EINVAL;
	}

	acquire_device(dev);

        int ret = spi_nand_access(dev, SPI_NAND_CMD_RDID, NAND_ACCESS_DUMMY, 0, read_id, SPI_NAND_ID_LEN);

	release_device(dev);

	if (memcmp(expected_id, read_id, sizeof(read_id)) != 0) {
		LOG_ERR("Wrong ID: %02X %02X , "
			"expected: %02X %02X ",
			read_id[0], read_id[1], 
			expected_id[0], expected_id[1]);
		return -ENODEV;
	}

	return ret;
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
	const struct spi_nand_config *cfg = dev->config;

	uint8_t reg = 0;
	int rc;

	/* Validate bus and CS is ready */
	if (!spi_is_ready(&cfg->spi)) {
		return -ENODEV;
	}

	rc = spi_nand_check_id(dev);
	if (rc != 0) {
		LOG_ERR("Check ID failed: ");
		return -ENODEV;
	}

	/* Check for block protect bits that need to be cleared. */
	rc = spi_nand_access(dev, SPI_NAND_CMD_GET_FEATURE, NAND_ACCESS_ADDRESSED | NAND_ACCESS_8BIT_ADDR, SPI_NAND_FEA_ADDR_BLOCK_PROT, &reg, sizeof(reg));
		
	/* Only clear if GET_FEATURE worked and something's set. */
	if ((rc > 0) && (reg & 0x38)) {
		reg = 0;
		rc = spi_nand_access(dev, SPI_NAND_CMD_SET_FEATURE, NAND_ACCESS_WRITE | NAND_ACCESS_ADDRESSED | NAND_ACCESS_8BIT_ADDR, SPI_NAND_FEA_ADDR_BLOCK_PROT, &reg, sizeof(reg));
	}

	if (rc != 0) {
		LOG_ERR("BP clear failed: %d\n", rc);
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Initialize and configure the flash
 *
 * @param name The flash name
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nand_init(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nand_data *const driver_data = dev->data;

		k_sem_init(&driver_data->sem, 1, K_SEM_MAX_LIMIT);
	}

	return spi_nand_configure(dev);
}

static const struct flash_parameters *
flash_nand_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_nand_parameters;
}

static const struct flash_driver_api spi_nand_api = {
	.read = spi_nand_read,
	.write = spi_nand_write,
	.erase = spi_nand_erase,
	.get_parameters = flash_nand_get_parameters,
};


static const struct spi_nand_config spi_nand_config_0 = {
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8),
				    CONFIG_SPI_NAND_CS_WAIT_DELAY),
	.id = DT_INST_PROP(0, id),
};

static struct spi_nand_data spi_nand_data_0;

DEVICE_DT_INST_DEFINE(0, &spi_nand_init, NULL,
		 &spi_nand_data_0, &spi_nand_config_0,
		 POST_KERNEL, CONFIG_SPI_NAND_INIT_PRIORITY,
		 &spi_nand_api);
