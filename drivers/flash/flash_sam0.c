/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_nvmctrl
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_sam0);

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <string.h>

#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
BUILD_ASSERT((FLASH_WRITE_BLK_SZ % sizeof(uint32_t)) == 0, "unsupported write-block-size");

/*
 * Zephyr and the SAM0 series use different and conflicting names for
 * the erasable units and programmable units:
 *
 * The erase unit is a row, which is a 'page' in Zephyr terms.
 * The program unit is a page, which is a 'write_block' in Zephyr.
 *
 * This file uses the SAM0 names internally and the Zephyr names in
 * any error messages.
 */

/*
 * Number of lock regions.  The number is fixed and the region size
 * grows with the flash size.
 */
#define LOCK_REGIONS DT_INST_PROP(0, lock_regions)
#define LOCK_REGION_SIZE (FLASH_SIZE / LOCK_REGIONS)

#if defined(NVMCTRL_BLOCK_SIZE)
#define ROW_SIZE NVMCTRL_BLOCK_SIZE
#elif defined(NVMCTRL_ROW_SIZE)
#define ROW_SIZE NVMCTRL_ROW_SIZE
#endif

#define PAGES_PER_ROW (ROW_SIZE / FLASH_PAGE_SIZE)

#define FLASH_MEM(_a) ((uint32_t *)((uint8_t *)((_a) + CONFIG_FLASH_BASE_ADDRESS)))

struct flash_sam0_data {
#if CONFIG_SOC_FLASH_SAM0_EMULATE_BYTE_PAGES
	/* NOTE: this buffer can be large, avoid placing it on the stack... */
	uint8_t buf[ROW_SIZE];
#endif

#if defined(CONFIG_MULTITHREADING)
	struct k_sem sem;
#endif
};

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_sam0_pages_layout = {
	.pages_count = CONFIG_FLASH_SIZE * 1024 / ROW_SIZE,
	.pages_size = ROW_SIZE,
};
#endif

static const struct flash_parameters flash_sam0_parameters = {
#if CONFIG_SOC_FLASH_SAM0_EMULATE_BYTE_PAGES
	.write_block_size = 1,
#else
	.write_block_size = FLASH_WRITE_BLK_SZ,
#endif
	.erase_value = 0xff,
};

static int flash_sam0_write_protection(const struct device *dev, bool enable);

static inline void flash_sam0_sem_take(const struct device *dev)
{
#if defined(CONFIG_MULTITHREADING)
	struct flash_sam0_data *ctx = dev->data;

	k_sem_take(&ctx->sem, K_FOREVER);
#endif
}

static inline void flash_sam0_sem_give(const struct device *dev)
{
#if defined(CONFIG_MULTITHREADING)
	struct flash_sam0_data *ctx = dev->data;

	k_sem_give(&ctx->sem);
#endif
}

static int flash_sam0_valid_range(off_t offset, size_t len)
{
	if (offset < 0) {
		LOG_WRN("0x%lx: before start of flash", (long)offset);
		return -EINVAL;
	}
	if ((offset + len) > CONFIG_FLASH_SIZE * 1024) {
		LOG_WRN("0x%lx: ends past the end of flash", (long)offset);
		return -EINVAL;
	}

	return 0;
}

static void flash_sam0_wait_ready(void)
{
#ifdef NVMCTRL_STATUS_READY
	while (NVMCTRL->STATUS.bit.READY == 0) {
	}
#else
	while (NVMCTRL->INTFLAG.bit.READY == 0) {
	}
#endif
}

static int flash_sam0_check_status(off_t offset)
{
	flash_sam0_wait_ready();

#ifdef NVMCTRL_INTFLAG_PROGE
	NVMCTRL_INTFLAG_Type status = NVMCTRL->INTFLAG;

	/* Clear any flags */
	NVMCTRL->INTFLAG.reg = status.reg;
#else
	NVMCTRL_STATUS_Type status = NVMCTRL->STATUS;

	/* Clear any flags */
	NVMCTRL->STATUS = status;
#endif

	if (status.bit.PROGE) {
		LOG_ERR("programming error at 0x%lx", (long)offset);
		return -EIO;
	} else if (status.bit.LOCKE) {
		LOG_ERR("lock error at 0x%lx", (long)offset);
		return -EROFS;
	} else if (status.bit.NVME) {
		LOG_ERR("NVM error at 0x%lx", (long)offset);
		return -EIO;
	}

	return 0;
}

/*
 * Data to be written to the NVM block are first written to and stored
 * in an internal buffer called the page buffer. The page buffer contains
 * the same number of bytes as an NVM page. Writes to the page buffer must
 * be 16 or 32 bits. 8-bit writes to the page buffer are not allowed and
 * will cause a system exception
 */
static int flash_sam0_write_page(const struct device *dev, off_t offset,
				 const void *data, size_t len)
{
	const uint32_t *src = data;
	const uint32_t *end = src + (len / sizeof(*src));
	uint32_t *dst = FLASH_MEM(offset);
	int err;

#ifdef NVMCTRL_CTRLA_CMD_PBC
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_PBC | NVMCTRL_CTRLA_CMDEX_KEY;
#else
	NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMD_PBC | NVMCTRL_CTRLB_CMDEX_KEY;
#endif
	flash_sam0_wait_ready();

	/* Ensure writes happen 32 bits at a time. */
	for (; src != end; src++, dst++) {
		*dst = UNALIGNED_GET((uint32_t *)src);
	}

#ifdef NVMCTRL_CTRLA_CMD_WP
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_WP | NVMCTRL_CTRLA_CMDEX_KEY;
#else
	NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMD_WP | NVMCTRL_CTRLB_CMDEX_KEY;
#endif

	err = flash_sam0_check_status(offset);
	if (err != 0) {
		return err;
	}

	if (memcmp(data, FLASH_MEM(offset), len) != 0) {
		LOG_ERR("verify error at offset 0x%lx", (long)offset);
		return -EIO;
	}

	return 0;
}

static int flash_sam0_erase_row(const struct device *dev, off_t offset)
{
	*FLASH_MEM(offset) = 0U;
#ifdef NVMCTRL_CTRLA_CMD_ER
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
#else
	NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMD_EB | NVMCTRL_CTRLB_CMDEX_KEY;
#endif
	return flash_sam0_check_status(offset);
}

#if CONFIG_SOC_FLASH_SAM0_EMULATE_BYTE_PAGES

static int flash_sam0_commit(const struct device *dev, off_t base)
{
	struct flash_sam0_data *ctx = dev->data;
	int err;
	int page;

	err = flash_sam0_erase_row(dev, base);
	if (err != 0) {
		return err;
	}

	for (page = 0; page < PAGES_PER_ROW; page++) {
		err = flash_sam0_write_page(
			dev, base + page * FLASH_PAGE_SIZE,
			&ctx->buf[page * FLASH_PAGE_SIZE], ROW_SIZE);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static int flash_sam0_write(const struct device *dev, off_t offset,
			    const void *data, size_t len)
{
	struct flash_sam0_data *ctx = dev->data;
	const uint8_t *pdata = data;
	int err;

	LOG_DBG("0x%lx: len %zu", (long)offset, len);

	err = flash_sam0_valid_range(offset, len);
	if (err != 0) {
		return err;
	}

	if (len == 0) {
		return 0;
	}

	flash_sam0_sem_take(dev);

	err = flash_sam0_write_protection(dev, false);

	size_t pos = 0;

	while ((err == 0) && (pos < len)) {
		off_t  start    = offset % sizeof(ctx->buf);
		off_t  base     = offset - start;
		size_t len_step = sizeof(ctx->buf) - start;
		size_t len_copy = MIN(len - pos, len_step);

		if (len_copy < sizeof(ctx->buf)) {
			memcpy(ctx->buf, (void *)base, sizeof(ctx->buf));
		}
		memcpy(&(ctx->buf[start]), &(pdata[pos]), len_copy);
		err = flash_sam0_commit(dev, base);

		offset += len_step;
		pos    += len_copy;
	}

	int err2 = flash_sam0_write_protection(dev, true);

	if (!err) {
		err = err2;
	}

	flash_sam0_sem_give(dev);

	return err;
}

#else /* CONFIG_SOC_FLASH_SAM0_EMULATE_BYTE_PAGES */

static int flash_sam0_write(const struct device *dev, off_t offset,
			    const void *data, size_t len)
{
	const uint8_t *pdata = data;
	int err;

	err = flash_sam0_valid_range(offset, len);
	if (err != 0) {
		return err;
	}

	if ((offset % FLASH_WRITE_BLK_SZ) != 0) {
		LOG_WRN("0x%lx: not on a write block boundary", (long)offset);
		return -EINVAL;
	}

	if ((len % FLASH_WRITE_BLK_SZ) != 0) {
		LOG_WRN("%zu: not a integer number of write blocks", len);
		return -EINVAL;
	}

	flash_sam0_sem_take(dev);

	err = flash_sam0_write_protection(dev, false);
	if (err == 0) {
		/* Maximum size without crossing a page */
		size_t eop_len = FLASH_PAGE_SIZE - (offset % FLASH_PAGE_SIZE);
		size_t write_len = MIN(len, eop_len);

		while (len > 0) {
			err = flash_sam0_write_page(dev, offset, pdata, write_len);
			if (err != 0) {
				break;
			}

			offset += write_len;
			pdata += write_len;
			len -= write_len;
			write_len = MIN(len, FLASH_PAGE_SIZE);
		}
	}

	int err2 = flash_sam0_write_protection(dev, true);

	if (!err) {
		err = err2;
	}

	flash_sam0_sem_give(dev);

	return err;
}

#endif

static int flash_sam0_read(const struct device *dev, off_t offset, void *data,
			   size_t len)
{
	int err;

	err = flash_sam0_valid_range(offset, len);
	if (err != 0) {
		return err;
	}

	memcpy(data, (uint8_t *)CONFIG_FLASH_BASE_ADDRESS + offset, len);

	return 0;
}

static int flash_sam0_erase(const struct device *dev, off_t offset,
			    size_t size)
{
	int err;

	err = flash_sam0_valid_range(offset, ROW_SIZE);
	if (err != 0) {
		return err;
	}

	if ((offset % ROW_SIZE) != 0) {
		LOG_WRN("0x%lx: not on a page boundary", (long)offset);
		return -EINVAL;
	}

	if ((size % ROW_SIZE) != 0) {
		LOG_WRN("%zu: not a integer number of pages", size);
		return -EINVAL;
	}

	flash_sam0_sem_take(dev);

	err = flash_sam0_write_protection(dev, false);
	if (err == 0) {
		for (size_t addr = offset; addr < offset + size;
		     addr += ROW_SIZE) {
			err = flash_sam0_erase_row(dev, addr);
			if (err != 0) {
				break;
			}
		}
	}

	int err2 = flash_sam0_write_protection(dev, true);

	if (!err) {
		err = err2;
	}

	flash_sam0_sem_give(dev);

	return err;
}

static int flash_sam0_write_protection(const struct device *dev, bool enable)
{
	off_t offset;
	int err;

	for (offset = 0; offset < CONFIG_FLASH_SIZE * 1024;
	     offset += LOCK_REGION_SIZE) {
		NVMCTRL->ADDR.reg = offset + CONFIG_FLASH_BASE_ADDRESS;

#ifdef NVMCTRL_CTRLA_CMD_LR
		if (enable) {
			NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_LR |
					     NVMCTRL_CTRLA_CMDEX_KEY;
		} else {
			NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_UR |
					     NVMCTRL_CTRLA_CMDEX_KEY;
		}
#else
		if (enable) {
			NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMD_LR |
					     NVMCTRL_CTRLB_CMDEX_KEY;
		} else {
			NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMD_UR |
					     NVMCTRL_CTRLB_CMDEX_KEY;
		}
#endif
		err = flash_sam0_check_status(offset);
		if (err != 0) {
			goto done;
		}
	}

done:
	return err;
}

#if CONFIG_FLASH_PAGE_LAYOUT
void flash_sam0_page_layout(const struct device *dev,
			    const struct flash_pages_layout **layout,
			    size_t *layout_size)
{
	*layout = &flash_sam0_pages_layout;
	*layout_size = 1;
}
#endif

static const struct flash_parameters *
flash_sam0_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_sam0_parameters;
}

static int flash_sam0_get_size(const struct device *dev, uint64_t *size)
{
	*size = (uint64_t)CONFIG_FLASH_SIZE;

	return 0;
}

static int flash_sam0_init(const struct device *dev)
{
#if defined(CONFIG_MULTITHREADING)
	struct flash_sam0_data *ctx = dev->data;

	k_sem_init(&ctx->sem, 1, 1);
#endif

#ifdef PM_APBBMASK_NVMCTRL
	/* Ensure the clock is on. */
	PM->APBBMASK.bit.NVMCTRL_ = 1;
#else
	MCLK->APBBMASK.reg |= MCLK_APBBMASK_NVMCTRL;
#endif

#ifdef NVMCTRL_CTRLB_MANW
	/* Require an explicit write command */
	NVMCTRL->CTRLB.bit.MANW = 1;
#elif NVMCTRL_CTRLA_WMODE
	/* Set manual write mode */
	NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN_Val;
#endif

	return flash_sam0_write_protection(dev, false);
}

static DEVICE_API(flash, flash_sam0_api) = {
	.erase = flash_sam0_erase,
	.write = flash_sam0_write,
	.read = flash_sam0_read,
	.get_parameters = flash_sam0_get_parameters,
	.get_size = flash_sam0_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_sam0_page_layout,
#endif
};

static struct flash_sam0_data flash_sam0_data_0;

DEVICE_DT_INST_DEFINE(0, flash_sam0_init, NULL,
		    &flash_sam0_data_0, NULL, POST_KERNEL,
		    CONFIG_FLASH_INIT_PRIORITY, &flash_sam0_api);
