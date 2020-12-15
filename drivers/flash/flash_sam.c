/*
 * Copyright (c) 2018 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#include <device.h>
#include <drivers/flash.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <string.h>

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(flash_sam0);

/*
 * The SAM flash memories use very different granularity for writing,
 * erasing and locking. In addition the first sector is composed of two
 * 8-KiB small sectors with a minimum 512-byte erase size, while the
 * other sectors have a minimum 8-KiB erase size.
 *
 * For simplicity reasons this flash controller driver only addresses the
 * flash by 8-KiB blocks (called "pages" in the Zephyr terminology).
 */


/*
 * We only use block mode erases. The datasheet gives a maximum erase time
 * of 200ms for a 8KiB block.
 */
#define SAM_FLASH_TIMEOUT_MS 220

struct flash_sam_dev_cfg {
	Efc *regs;
};

struct flash_sam_dev_data {
	struct k_sem sem;
};

static const struct flash_parameters flash_sam_parameters = {
	.write_block_size = FLASH_WRITE_BLK_SZ,
	.erase_value = 0xff,
};

#define DEV_CFG(dev) \
	((const struct flash_sam_dev_cfg *const)(dev)->config)

#define DEV_DATA(dev) \
	((struct flash_sam_dev_data *const)(dev)->data)


static inline void flash_sam_sem_take(const struct device *dev)
{
	k_sem_take(&DEV_DATA(dev)->sem, K_FOREVER);
}

static inline void flash_sam_sem_give(const struct device *dev)
{
	k_sem_give(&DEV_DATA(dev)->sem);
}

/* Check that the offset is within the flash */
static bool flash_sam_valid_range(const struct device *dev, off_t offset,
				  size_t len)
{
	if (offset > CONFIG_FLASH_SIZE * 1024) {
		return false;
	}

	if (len && ((offset + len - 1) > (CONFIG_FLASH_SIZE * 1024))) {
		return false;
	}

	return true;
}

/* Convert an offset in the flash into a page number */
static off_t flash_sam_get_page(off_t offset)
{
	return offset / IFLASH_PAGE_SIZE;
}

/*
 * This function checks for errors and waits for the end of the
 * previous command.
 */
static int flash_sam_wait_ready(const struct device *dev)
{
	Efc *const efc = DEV_CFG(dev)->regs;

	uint64_t timeout_time = k_uptime_get() + SAM_FLASH_TIMEOUT_MS;
	uint32_t fsr;

	do {
		fsr = efc->EEFC_FSR;

		/* Flash Error Status */
		if (fsr & EEFC_FSR_FLERR) {
			return -EIO;
		}
		/* Flash Lock Error Status */
		if (fsr & EEFC_FSR_FLOCKE) {
			return -EACCES;
		}
		/* Flash Command Error */
		if (fsr & EEFC_FSR_FCMDE) {
			return -EINVAL;
		}

		/*
		 * ECC error bits are intentionally not checked as they
		 * might be set outside of the programming code.
		 */

		/* Check for timeout */
		if (k_uptime_get() > timeout_time) {
			return -ETIMEDOUT;
		}
	} while (!(fsr & EEFC_FSR_FRDY));

	return 0;
}

/* This function writes a single page, either fully or partially. */
static int flash_sam_write_page(const struct device *dev, off_t offset,
				const void *data, size_t len)
{
	Efc *const efc = DEV_CFG(dev)->regs;
	const uint32_t *src = data;
	uint32_t *dst = (uint32_t *)((uint8_t *)CONFIG_FLASH_BASE_ADDRESS + offset);

	LOG_DBG("offset = 0x%lx, len = %zu", (long)offset, len);

	/* We need to copy the data using 32-bit accesses */
	for (; len > 0; len -= sizeof(*src)) {
		*dst++ = *src++;
	}
	__DSB();

	/* Trigger the flash write */
	efc->EEFC_FCR = EEFC_FCR_FKEY_PASSWD |
			EEFC_FCR_FARG(flash_sam_get_page(offset)) |
			EEFC_FCR_FCMD_WP;
	__DSB();

	/* Wait for the flash write to finish */
	return flash_sam_wait_ready(dev);
}

/* Write data to the flash, page by page */
static int flash_sam_write(const struct device *dev, off_t offset,
			    const void *data, size_t len)
{
	int rc;
	const uint8_t *data8 = data;

	LOG_DBG("offset = 0x%lx, len = %zu", (long)offset, len);

	/* Check that the offset is within the flash */
	if (!flash_sam_valid_range(dev, offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	/*
	 * Check that the offset and length are multiples of the write
	 * block size.
	 */
	if ((offset % FLASH_WRITE_BLK_SZ) != 0) {
		return -EINVAL;
	}
	if ((len % FLASH_WRITE_BLK_SZ) != 0) {
		return -EINVAL;
	}

	flash_sam_sem_take(dev);

	rc = flash_sam_wait_ready(dev);
	if (rc < 0) {
		return rc;
	}

	while (len > 0) {
		size_t eop_len, write_len;

		/* Maximum size without crossing a page */
		eop_len = -(offset | ~(IFLASH_PAGE_SIZE - 1));
		write_len = MIN(len, eop_len);

		rc = flash_sam_write_page(dev, offset, data8, write_len);
		if (rc < 0) {
			goto done;
		}

		offset += write_len;
		data8 += write_len;
		len -= write_len;
	}

done:
	flash_sam_sem_give(dev);

	return rc;
}

/* Read data from flash */
static int flash_sam_read(const struct device *dev, off_t offset, void *data,
			  size_t len)
{
	LOG_DBG("offset = 0x%lx, len = %zu", (long)offset, len);

	if (!flash_sam_valid_range(dev, offset, len)) {
		return -EINVAL;
	}

	memcpy(data, (uint8_t *)CONFIG_FLASH_BASE_ADDRESS + offset, len);

	return 0;
}

/* Erase a single 8KiB block */
static int flash_sam_erase_block(const struct device *dev, off_t offset)
{
	Efc *const efc = DEV_CFG(dev)->regs;

	LOG_DBG("offset = 0x%lx", (long)offset);

	efc->EEFC_FCR = EEFC_FCR_FKEY_PASSWD |
			EEFC_FCR_FARG(flash_sam_get_page(offset) | 2) |
			EEFC_FCR_FCMD_EPA;
	__DSB();

	return flash_sam_wait_ready(dev);
}

/* Erase multiple blocks */
static int flash_sam_erase(const struct device *dev, off_t offset, size_t len)
{
	int rc = 0;
	off_t i;

	LOG_DBG("offset = 0x%lx, len = %zu", (long)offset, len);

	if (!flash_sam_valid_range(dev, offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	/*
	 * Check that the offset and length are multiples of the write
	 * erase block size.
	 */
	if ((offset % FLASH_ERASE_BLK_SZ) != 0) {
		return -EINVAL;
	}
	if ((len % FLASH_ERASE_BLK_SZ) != 0) {
		return -EINVAL;
	}

	flash_sam_sem_take(dev);

	/* Loop through the pages to erase */
	for (i = offset; i < offset + len; i += FLASH_ERASE_BLK_SZ) {
		rc = flash_sam_erase_block(dev, i);
		if (rc < 0) {
			goto done;
		}
	}

done:
	flash_sam_sem_give(dev);

	/*
	 * Invalidate the cache addresses corresponding to the erased blocks,
	 * so that they really appear as erased.
	 */
	SCB_InvalidateDCache_by_Addr((void *)(CONFIG_FLASH_BASE_ADDRESS + offset), len);

	return rc;
}

/* Enable or disable the write protection */
static int flash_sam_write_protection(const struct device *dev, bool enable)
{
	Efc *const efc = DEV_CFG(dev)->regs;
	int rc = 0;

	flash_sam_sem_take(dev);

	if (enable) {
		rc = flash_sam_wait_ready(dev);
		if (rc < 0) {
			goto done;
		}
		efc->EEFC_WPMR = EEFC_WPMR_WPKEY_PASSWD | EEFC_WPMR_WPEN;
	} else {
		efc->EEFC_WPMR = EEFC_WPMR_WPKEY_PASSWD;
	}

done:
	flash_sam_sem_give(dev);
	return rc;
}

#if CONFIG_FLASH_PAGE_LAYOUT
/*
 * The notion of pages is different in Zephyr and in the SAM documentation.
 * Here a page refers to the granularity at which the flash can be erased.
 */
static const struct flash_pages_layout flash_sam_pages_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) / FLASH_ERASE_BLK_SZ,
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

void flash_sam_page_layout(const struct device *dev,
			   const struct flash_pages_layout **layout,
			   size_t *layout_size)
{
	*layout = &flash_sam_pages_layout;
	*layout_size = 1;
}
#endif

static const struct flash_parameters *
flash_sam_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_sam_parameters;
}

static int flash_sam_init(const struct device *dev)
{
	struct flash_sam_dev_data *const data = DEV_DATA(dev);

	k_sem_init(&data->sem, 1, 1);

	return 0;
}

static const struct flash_driver_api flash_sam_api = {
	.write_protection = flash_sam_write_protection,
	.erase = flash_sam_erase,
	.write = flash_sam_write,
	.read = flash_sam_read,
	.get_parameters = flash_sam_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_sam_page_layout,
#endif
};

static const struct flash_sam_dev_cfg flash_sam_cfg = {
	.regs = (Efc *)DT_INST_REG_ADDR(0),
};

static struct flash_sam_dev_data flash_sam_data;

DEVICE_DT_INST_DEFINE(0, flash_sam_init, device_pm_control_nop,
		    &flash_sam_data, &flash_sam_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &flash_sam_api);
