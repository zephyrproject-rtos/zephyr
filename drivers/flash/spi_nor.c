/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 *
 * This driver is heavily inspired from the spi_flash_w25qxxdv.c SPI NOR driver.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <flash.h>
#include <spi.h>
#include <init.h>
#include <string.h>
#include "spi_nor.h"
#include "flash_priv.h"

#define SZ_256  0x100
#define SZ_512  0x200
#define SZ_1024 0x400
#define SZ_4K   0x1000
#define SZ_32K  0x8000
#define SZ_64K  0x10000

#define MASK_256 0xFF
#define MASK_4K  0xFFF
#define MASK_32K 0x7FFF
#define MASK_64K 0xFFFF

#define BLOCK_ERASE_32K BIT(1)
#define BLOCK_ERASE_64K BIT(2)

#define SPI_NOR_MAX_ID_LEN 3
#define SPI_NOR_MAX_ADDR_WIDTH 4

#define JEDEC_ID(x)		    \
	{			    \
		((x) >> 16) & 0xFF, \
		((x) >> 8) & 0xFF,  \
		(x) & 0xFF,	    \
	}

/**
 * struct flash_info - Structure for defining the SPI NOR layer
 * @name: Name of the flash
 * @id: Jedec ID
 * @page_size: Size of the pages
 * @sector_size: Size of the sectors
 * @n_sectors: Number of sector
 * @block_size: Size of the blocks
 * @flag: Flags of the flash
 */
static const struct flash_info {
	char *name;
	u8_t id[SPI_NOR_MAX_ID_LEN];
	unsigned page_size;
	unsigned sector_size;
	unsigned n_sectors;
	unsigned block_size;
	uint32_t flag;
} flash_ids[] = {
	{ "W25Q80",     JEDEC_ID(0xef4014), SZ_256, SZ_4K, SZ_256, SZ_64K, BLOCK_ERASE_32K | BLOCK_ERASE_64K },
	{ "W25Q16",     JEDEC_ID(0xef4015), SZ_256, SZ_4K, SZ_512, SZ_64K, BLOCK_ERASE_32K | BLOCK_ERASE_64K },
	{ "W25Q32",     JEDEC_ID(0xef4016), SZ_256, SZ_4K, SZ_1024, SZ_64K, BLOCK_ERASE_32K | BLOCK_ERASE_64K },
	{ "S25FL216K",  JEDEC_ID(0x014015), SZ_256, SZ_4K, SZ_512, SZ_64K, BLOCK_ERASE_64K },
	{ },
};

/**
 * struct spi_nor_parameters - Structure for defining the SPI NOR parameters
 * @flash_size:	Size of the flash
 * @sector_size: Size of the sectors
 * @block_size:	Size of the blocks
 * @page_size: Size of the pages
 * @flag: Flags of the flash
 */
struct spi_nor_parameters {
	unsigned flash_size;
	unsigned sector_size;
	unsigned block_size;
	unsigned page_size;
	uint32_t flag;
};

/**
 * struct spi_nor_data - Structure for defining the SPI NOR access
 * @spi: The SPI device
 * @spi_cfg: The SPI configuration
 * @cs_ctrl: The GPIO pin used to emulate the SPI CS if required
 * @params: The SPI NOR parameters
 * @sem: The semaphore to access to the flash
 */
struct spi_nor_data {
	struct device *spi;
	struct spi_config spi_cfg;
#if defined(CONFIG_SPI_NOR_GPIO_SPI_CS)
	struct spi_cs_control cs_ctrl;
#endif /* CONFIG_SPI_NOR_GPIO_SPI_CS */
	struct spi_nor_parameters params;
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
		return spi_write(driver_data->spi, &driver_data->spi_cfg, &tx_set);
	}

	return spi_transceive(driver_data->spi, &driver_data->spi_cfg, &tx_set, &rx_set);
}
#define spi_nor_cmd_read(dev, opcode, dest, length) \
	spi_nor_access(dev, opcode, false, 0, dest, length, false)
#define spi_nor_cmd_addr_read(dev, opcode, addr, dest, length) \
	spi_nor_access(dev, opcode, true, addr, dest, length, false)
#define spi_nor_cmd_write(dev, opcode) \
	spi_nor_access(dev, opcode, false, 0, NULL, 0, true)
#define spi_nor_cmd_addr_write(dev, opcode, addr, src, length) \
	spi_nor_access(dev, opcode, true, addr, src, length, true)

/**
 * @brief Retrieve the Flash JEDEC ID and compare it with the one expected
 *
 * @param dev The device structure
 * @param flash_id The flash info structure which contains the expected JEDEC ID
 * @return 0 on success, negative errno code otherwise
 */
static inline int spi_nor_read_id(struct device *dev, const struct flash_info *const flash_id)
{
	u8_t buf[SPI_NOR_MAX_ID_LEN];

	if (spi_nor_cmd_read(dev, SPI_NOR_CMD_RDID, buf, SPI_NOR_MAX_ID_LEN) != 0) {
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
static inline int spi_nor_wait_until_ready(struct device *dev)
{
	int ret;
	u8_t reg;

	do {
		ret = spi_nor_cmd_read(dev, SPI_NOR_CMD_RDSR, &reg, 1);
	} while (!ret && (reg & SPI_NOR_WIP_BIT));

	return ret;
}

/**
 * @brief Read data from the flash
 *
 * @param dev The flash device structure
 * @param addr The address to read the value from
 * @param dest The buffer to store the read value
 * @param size The size of the buffer
 * @return number of byte read on success, negative errno code otherwise
 */
static int spi_nor_read(struct device *dev, off_t addr, void *dest,
			size_t size)
{
	struct spi_nor_data *const driver_data = dev->driver_data;
	struct spi_nor_parameters *const params = &driver_data->params;
	const void *const p_dest = dest;
	int ret;
	int to_read;

	/* should be between 0 and flash size */
	if ((addr < 0) || (addr + size) > params->flash_size) {
		return -EINVAL;
	}

	/* start address should be aligned on a page size */
	if ((addr & (params->page_size - 1)) != 0) {
		return -EINVAL;
	}

	SYNC_LOCK();

	spi_nor_wait_until_ready(dev);

	while (size) {
		to_read = size;
		if (size > params->page_size) {
			to_read = params->page_size;
		}

		ret = spi_nor_cmd_addr_read(dev, SPI_NOR_CMD_READ, addr, dest, to_read);
		if (ret != 0) {
			goto err;
		}
		size -= to_read;
		addr += to_read;
		dest += to_read;
	}

err:
	SYNC_UNLOCK();

	return (dest - p_dest);
}

/**
 * @brief Write data to the flash
 *
 * @param dev The flash device structure
 * @param addr The address to write the value to
 * @param src The buffer to read the value from
 * @param size The size of the buffer
 * @return number of byte read on success, negative errno code otherwise
 */
static int spi_nor_write(struct device *dev, off_t addr, const void *src, size_t size)
{
	struct spi_nor_data *const driver_data = dev->driver_data;
	struct spi_nor_parameters *const params = &driver_data->params;
	const void *const p_src = src;
	int ret;
	size_t to_write;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((size + addr) > params->flash_size)) {
		return -EINVAL;
	}

	/* start address should be aligned on a page size */
	if ((addr & (params->page_size - 1)) != 0) {
		return -EINVAL;
	}

	SYNC_LOCK();

	while (size) {
		/* write enable */
		spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);

		to_write = size;
		if (size >= params->page_size) {
			to_write = params->page_size;
		}

		ret = spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_PP, addr, (void *)src, to_write);
		if (ret != 0) {
			goto err;
		}
		size -= to_write;
		addr += to_write;
		src += to_write;

		spi_nor_wait_until_ready(dev);
	}

err:
	SYNC_UNLOCK();

	return (src - p_src);
}

/**
 * @brief Erase data in nor
 *
 * @param dev The flash device structure
 * @param addr The address to start the erase from
 * @param size The size to erase
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_erase(struct device *dev, off_t addr, size_t size)
{
	struct spi_nor_data *const driver_data = dev->driver_data;
	struct spi_nor_parameters *const params = &driver_data->params;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((size + addr) > params->flash_size)) {
		return -ENODEV;
	}

	SYNC_LOCK();

	while (size) {
		/* write enable */
		spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);

		if (size == params->flash_size) {
			/* chip erase */
			spi_nor_cmd_write(dev, SPI_NOR_CMD_CE);
			size -= params->flash_size;
		} else if ((params->flag & BLOCK_ERASE_64K) && (size >= SZ_64K) &&
			   ((addr & MASK_64K) == 0)) {
			/* 64 KiB block erase */
			spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_BE, addr, NULL, 0);
			addr += SZ_64K;
			size -= SZ_64K;
		} else if ((params->flag & BLOCK_ERASE_32K) && (size >= SZ_32K) &&
			   ((addr & MASK_32K) == 0)) {
			/* 32 KiB block erase */
			spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_BE_32K, addr, NULL, 0);
			addr += SZ_32K;
			size -= SZ_32K;
		} else if ((size >= params->sector_size) && ((addr & (params->sector_size - 1)) == 0)) {
			/* sector erase */
			spi_nor_cmd_addr_write(dev, SPI_NOR_CMD_SE, addr, NULL, 0);
			addr += params->sector_size;
			size -= params->sector_size;
		} else {
			/* minimal erase size is at least a sector size */
			SYNC_UNLOCK();
			return -EINVAL;
		}

		spi_nor_wait_until_ready(dev);
	}

	SYNC_UNLOCK();

	return 0;
}

/**
 * @brief Enable or Disable the flash write protection
 *
 * @param dev The flash device structure
 * @param write_protect Enable of disable the write protection
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_write_protection_set(struct device *dev, bool write_protect)
{
	struct spi_nor_data *const driver_data = dev->driver_data;
	int ret;

	SYNC_LOCK();

	spi_nor_wait_until_ready(dev);

	ret = spi_nor_cmd_write(dev, (write_protect) ? SPI_NOR_CMD_WRDI : SPI_NOR_CMD_WREN);

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
static int spi_nor_configure(struct device *dev, const struct flash_info *const info)
{
	struct spi_nor_data *data = dev->driver_data;
	struct spi_nor_parameters *const params = &data->params;

	data->spi = device_get_binding(CONFIG_SPI_NOR_SPI_NAME);
	if (!data->spi) {
		return -EINVAL;
	}

	data->spi_cfg.frequency = CONFIG_SPI_NOR_SPI_FREQ_0;
	data->spi_cfg.operation = SPI_WORD_SET(8);
	data->spi_cfg.slave = CONFIG_SPI_NOR_SPI_SLAVE;

#if defined(CONFIG_SPI_NOR_GPIO_SPI_CS)
	data->cs_ctrl.gpio_dev =
		device_get_binding(CONFIG_SPI_NOR_GPIO_SPI_CS_DRV_NAME);
	if (!data->cs_ctrl.gpio_dev) {
		return -ENODEV;
	}

	data->cs_ctrl.gpio_pin = CONFIG_SPI_NOR_GPIO_SPI_CS_PIN;
	data->cs_ctrl.delay = CONFIG_SPI_NOR_GPIO_SPI_CS_WAIT_DELAY;

	data->spi_cfg.cs = &data->cs_ctrl;
#endif /* CONFIG_SPI_NOR_GPIO_SPI_CS */

	/* now the spi bus is configured, we can verify the flash id */
	if (spi_nor_read_id(dev, info) != 0) {
		return -ENODEV;
	}

	/* retrieve the flash parameters */
	params->flash_size = info->sector_size * info->n_sectors;
	params->page_size = info->page_size;
	params->sector_size = info->sector_size;
	params->block_size = info->block_size;
	params->flag = info->flag;

	return 0;
}

/**
 * @brief Retrieve the flash info based on its name
 *
 * @param name The flash name
 * @return The flash info on success, Null otherwise
 */
static const struct flash_info *spi_nor_match_id(const char *name)
{
	const struct flash_info *id = flash_ids;

	while (id->name) {
		if (!strcmp(name, id->name)) {
			return id;
		}
		id++;
	}

	return NULL;
}

/**
 * @brief Initialize and configure the flash
 *
 * @param name The flash name
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_init(struct device *dev)
{
	const struct flash_info *info;

	SYNC_INIT();

	/* retrieve the flash info from driver name */
	info = spi_nor_match_id(CONFIG_SPI_NOR_DRV_NAME);
	if (!info) {
		return -EINVAL;
	}

	return spi_nor_configure(dev, info);
}

static const struct flash_driver_api spi_nor_api = {
	.read = spi_nor_read,
	.write = spi_nor_write,
	.erase = spi_nor_erase,
	.write_protection = spi_nor_write_protection_set,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = (flash_api_pages_layout)
		       flash_page_layout_not_implemented,
#endif
	.write_block_size = 1,
};

static struct spi_nor_data spi_nor_memory_data;

DEVICE_AND_API_INIT(spi_flash_memory, CONFIG_SPI_NOR_DRV_NAME,
		    &spi_nor_init, &spi_nor_memory_data,
		    NULL, POST_KERNEL, CONFIG_SPI_NOR_INIT_PRIORITY,
		    &spi_nor_api);
