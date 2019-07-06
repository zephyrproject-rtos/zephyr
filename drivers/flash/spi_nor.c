/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 *
 * This driver is heavily inspired from the spi_flash_w25qxxdv.c SPI NOR driver.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <drivers/flash.h>
#include <drivers/spi.h>
#include <init.h>
#include <string.h>
#include <logging/log.h>

#include "spi_nor.h"
#include "flash_priv.h"

LOG_MODULE_REGISTER(spi_nor, CONFIG_FLASH_LOG_LEVEL);

#define SPI_NOR_MAX_ADDR_WIDTH 4

/**
 * struct spi_nor_data - Structure for defining the SPI NOR access
 * @spi: The SPI device
 * @spi_cfg: The SPI configuration
 * @cs_ctrl: The GPIO pin used to emulate the SPI CS if required
 * @sem: The semaphore to access to the flash
 */
struct spi_nor_data {
	struct device *spi;
	struct spi_config spi_cfg;
#ifdef DT_INST_0_JEDEC_SPI_NOR_CS_GPIOS_CONTROLLER
	struct spi_cs_control cs_ctrl;
#endif /* DT_INST_0_JEDEC_SPI_NOR_CS_GPIOS_CONTROLLER */
	struct k_sem sem;
};

#if defined(CONFIG_MULTITHREADING)
#define SYNC_INIT() k_sem_init(	\
		&((struct spi_nor_data *)dev->driver_data)->sem, 1, UINT_MAX)
#define SYNC_LOCK() k_sem_take(&driver_data->sem, K_FOREVER)
#define SYNC_UNLOCK() k_sem_give(&driver_data->sem)
#else
#define SYNC_INIT()
#define SYNC_LOCK()
#define SYNC_UNLOCK()
#endif

/*
 * @brief Send an SPI command
 *
 * @param dev Device struct
 * @param opcode The command to send
 * @param is_addressed A flag to define if the command is addressed
 * @param addr The address to send
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @param is_write A flag to define if it's a read or a write command
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_access(const struct device *const dev,
			  u8_t opcode, bool is_addressed, off_t addr,
			  void *data, size_t length, bool is_write)
{
	struct spi_nor_data *const driver_data = dev->driver_data;

	u8_t buf[4] = {
		opcode,
		(addr & 0xFF0000) >> 16,
		(addr & 0xFF00) >> 8,
		(addr & 0xFF),
	};

	struct spi_buf spi_buf[2] = {
		{
			.buf = buf,
			.len = (is_addressed) ? 4 : 1,
		},
		{
			.buf = data,
			.len = length
		}
	};
	const struct spi_buf_set tx_set = {
		.buffers = spi_buf,
		.count = (length) ? 2 : 1
	};

	const struct spi_buf_set rx_set = {
		.buffers = spi_buf,
		.count = 2
	};

	if (is_write) {
		return spi_write(driver_data->spi,
			&driver_data->spi_cfg, &tx_set);
	}

	return spi_transceive(driver_data->spi,
		&driver_data->spi_cfg, &tx_set, &rx_set);
}

#define spi_nor_cmd_read(dev, opcode, dest, length) \
	spi_nor_access(dev, opcode, false, 0, dest, length, false)
#define spi_nor_cmd_addr_read(dev, opcode, addr, dest, length) \
	spi_nor_access(dev, opcode, true, addr, dest, length, false)
#define spi_nor_cmd_write(dev, opcode) \
	spi_nor_access(dev, opcode, false, 0, NULL, 0, true)
#define spi_nor_cmd_addr_write(dev, opcode, addr, src, length) \
	spi_nor_access(dev, opcode, true, addr, (void *)src, length, true)

/**
 * @brief Retrieve the Flash JEDEC ID and compare it with the one expected
 *
 * @param dev The device structure
 * @param flash_id The flash info structure which contains the expected JEDEC ID
 * @return 0 on success, negative errno code otherwise
 */
static inline int spi_nor_read_id(struct device *dev,
				  const struct spi_nor_config *const flash_id)
{
	u8_t buf[SPI_NOR_MAX_ID_LEN];

	if (spi_nor_cmd_read(dev, SPI_NOR_CMD_RDID, buf,
	    SPI_NOR_MAX_ID_LEN) != 0) {
		return -EIO;
	}

	if (memcmp(flash_id->id, buf, SPI_NOR_MAX_ID_LEN) != 0) {
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Wait until the flash is ready
 *
 * @param dev The device structure
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_wait_until_ready(struct device *dev)
{
	int ret;
	u8_t reg;

	do {
		ret = spi_nor_cmd_read(dev, SPI_NOR_CMD_RDSR, &reg, 1);
	} while (!ret && (reg & SPI_NOR_WIP_BIT));

	return ret;
}

static int spi_nor_read(struct device *dev, off_t addr, void *dest,
			size_t size)
{
	struct spi_nor_data *const driver_data = dev->driver_data;
	const struct spi_nor_config *params = dev->config->config_info;
	int ret;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((addr + size) > params->size)) {
		return -EINVAL;
	}

	SYNC_LOCK();

	spi_nor_wait_until_ready(dev);

	ret = spi_nor_cmd_addr_read(dev, SPI_NOR_CMD_READ, addr, dest, size);

	SYNC_UNLOCK();
	return ret;
}

static int spi_nor_write(struct device *dev, off_t addr, const void *src,
			 size_t size)
{
	struct spi_nor_data *const driver_data = dev->driver_data;
	const struct spi_nor_config *params = dev->config->config_info;
	int ret;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((size + addr) > params->size)) {
		return -EINVAL;
	}

	SYNC_LOCK();

	while (size > 0) {
		size_t to_write = size;

		/* Don't write more than a page. */
		if (to_write >= SPI_NOR_PAGE_SIZE) {
			to_write = SPI_NOR_PAGE_SIZE;
		}

		/* Don't write across a page boundary */
		if (((addr + to_write - 1U) / SPI_NOR_PAGE_SIZE)
		    != (addr / SPI_NOR_PAGE_SIZE)) {
			to_write = SPI_NOR_PAGE_SIZE - (addr % SPI_NOR_PAGE_SIZE);
		}

		spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);
		ret = spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_PP, addr,
					     src, to_write);
		if (ret != 0) {
			SYNC_UNLOCK();
			return ret;
		}

		size -= to_write;
		src = (const u8_t *)src + to_write;
		addr += to_write;

		spi_nor_wait_until_ready(dev);
	}

	SYNC_UNLOCK();
	return 0;
}

static int spi_nor_erase(struct device *dev, off_t addr, size_t size)
{
	struct spi_nor_data *const driver_data = dev->driver_data;
	const struct spi_nor_config *params = dev->config->config_info;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((size + addr) > params->size)) {
		return -ENODEV;
	}

	SYNC_LOCK();

	while (size) {
		/* write enable */
		spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);

		if (size == params->size) {
			/* chip erase */
			spi_nor_cmd_write(dev, SPI_NOR_CMD_CE);
			size -= params->size;
		} else if ((size >= SPI_NOR_BLOCK_SIZE)
			   && SPI_NOR_IS_BLOCK_ALIGNED(addr)) {
			/* 64 KiB block erase */
			spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_BE, addr,
			NULL, 0);
			addr += SPI_NOR_BLOCK_SIZE;
			size -= SPI_NOR_BLOCK_SIZE;
		} else if ((size >= SPI_NOR_BLOCK32_SIZE)
			   && SPI_NOR_IS_BLOCK32_ALIGNED(addr)) {
			/* 32 KiB block erase */
			spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_BE_32K, addr,
					       NULL, 0);
			addr += SPI_NOR_BLOCK32_SIZE;
			size -= SPI_NOR_BLOCK32_SIZE;
		} else if ((size >= SPI_NOR_SECTOR_SIZE)
			   && SPI_NOR_IS_SECTOR_ALIGNED(addr)) {
			/* sector erase */
			spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_SE, addr,
					       NULL, 0);
			addr += SPI_NOR_SECTOR_SIZE;
			size -= SPI_NOR_SECTOR_SIZE;
		} else {
			/* minimal erase size is at least a sector size */
			SYNC_UNLOCK();
			LOG_DBG("unsupported at 0x%lx size %zu", (long)addr,
				size);
			return -EINVAL;
		}

		spi_nor_wait_until_ready(dev);
	}

	SYNC_UNLOCK();

	return 0;
}

static int spi_nor_write_protection_set(struct device *dev, bool write_protect)
{
	struct spi_nor_data *const driver_data = dev->driver_data;
	int ret;

	SYNC_LOCK();

	spi_nor_wait_until_ready(dev);

	ret = spi_nor_cmd_write(dev, (write_protect) ?
	      SPI_NOR_CMD_WRDI : SPI_NOR_CMD_WREN);

#if DT_INST_0_JEDEC_SPI_NOR_JEDEC_ID_0 == 0xbf && DT_INST_0_JEDEC_SPI_NOR_JEDEC_ID_1 == 0x26
	if (ret == 0 && !write_protect) {
		ret = spi_nor_cmd_write(dev, SPI_NOR_CMD_MCHP_UNLOCK);
	}
#endif

	SYNC_UNLOCK();

	return ret;
}

/**
 * @brief Configure the flash
 *
 * @param dev The flash device structure
 * @param info The flash info structure
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_configure(struct device *dev)
{
	struct spi_nor_data *data = dev->driver_data;
	const struct spi_nor_config *params = dev->config->config_info;

	data->spi = device_get_binding(DT_INST_0_JEDEC_SPI_NOR_BUS_NAME);
	if (!data->spi) {
		return -EINVAL;
	}

	data->spi_cfg.frequency = DT_INST_0_JEDEC_SPI_NOR_SPI_MAX_FREQUENCY;
	data->spi_cfg.operation = SPI_WORD_SET(8);
	data->spi_cfg.slave = DT_INST_0_JEDEC_SPI_NOR_BASE_ADDRESS;

#ifdef DT_INST_0_JEDEC_SPI_NOR_CS_GPIOS_CONTROLLER
	data->cs_ctrl.gpio_dev =
		device_get_binding(DT_INST_0_JEDEC_SPI_NOR_CS_GPIOS_CONTROLLER);
	if (!data->cs_ctrl.gpio_dev) {
		return -ENODEV;
	}

	data->cs_ctrl.gpio_pin = DT_INST_0_JEDEC_SPI_NOR_CS_GPIOS_PIN;
	data->cs_ctrl.delay = CONFIG_SPI_NOR_CS_WAIT_DELAY;

	data->spi_cfg.cs = &data->cs_ctrl;
#endif /* DT_INST_0_JEDEC_SPI_NOR_CS_GPIOS_CONTROLLER */

	/* now the spi bus is configured, we can verify the flash id */
	if (spi_nor_read_id(dev, params) != 0) {
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
static int spi_nor_init(struct device *dev)
{
	SYNC_INIT();

	return spi_nor_configure(dev);
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)

/* instance 0 size in bytes */
#define INST_0_BYTES (DT_INST_0_JEDEC_SPI_NOR_SIZE / 8)

/* instance 0 page count */
#define LAYOUT_PAGES_COUNT (INST_0_BYTES / CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE)

BUILD_ASSERT_MSG((CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE * LAYOUT_PAGES_COUNT)
		 == INST_0_BYTES,
		 "SPI_NOR_FLASH_LAYOUT_PAGE_SIZE incompatible with flash size");

static const struct flash_pages_layout dev_layout = {
	.pages_count = LAYOUT_PAGES_COUNT,
	.pages_size = CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE,
};
#undef LAYOUT_PAGES_COUNT

static void spi_nor_pages_layout(struct device *dev,
				 const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api spi_nor_api = {
	.read = spi_nor_read,
	.write = spi_nor_write,
	.erase = spi_nor_erase,
	.write_protection = spi_nor_write_protection_set,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_nor_pages_layout,
#endif
	.write_block_size = 1,
};

static const struct spi_nor_config flash_id = {
	.id = DT_INST_0_JEDEC_SPI_NOR_JEDEC_ID,
#ifdef DT_INST_0_JEDEC_SPI_NOR_HAS_BE32K
	.has_be32k = true,
#endif /* DT_INST_0_JEDEC_SPI_NOR_HAS_BE32K */
	.size = DT_INST_0_JEDEC_SPI_NOR_SIZE / 8,
};

static struct spi_nor_data spi_nor_memory_data;

DEVICE_AND_API_INIT(spi_flash_memory, DT_INST_0_JEDEC_SPI_NOR_LABEL,
		    &spi_nor_init, &spi_nor_memory_data,
		    &flash_id, POST_KERNEL, CONFIG_SPI_NOR_INIT_PRIORITY,
		    &spi_nor_api);
