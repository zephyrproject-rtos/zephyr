/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_nvmctrl

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(flash_sam0);

#include <device.h>
#include <drivers/flash.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <string.h>

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
	uint8_t buf[ROW_SIZE];
	off_t offset;
#endif

	struct k_sem sem;
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
	.write_block_size = FLASH_PAGE_SIZE,
#endif
	.erase_value = 0xff,
};

static inline void flash_sam0_sem_take(const struct device *dev)
{
	struct flash_sam0_data *ctx = dev->data;

	k_sem_take(&ctx->sem, K_FOREVER);
}

static inline void flash_sam0_sem_give(const struct device *dev)
{
	struct flash_sam0_data *ctx = dev->data;

	k_sem_give(&ctx->sem);
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

static int flash_sam0_write_page(const struct device *dev, off_t offset,
				 const void *data)
{
	const uint32_t *src = data;
	const uint32_t *end = src + FLASH_PAGE_SIZE / sizeof(*src);
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

	if (memcmp(data, FLASH_MEM(offset), FLASH_PAGE_SIZE) != 0) {
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

static int flash_sam0_commit(const struct device *dev)
{
	struct flash_sam0_data *ctx = dev->data;
	int err;
	int page;
	off_t offset = ctx->offset;

	ctx->offset = 0;

	if (offset == 0) {
		return 0;
	}

	err = flash_sam0_erase_row(dev, offset);
	if (err != 0) {
		return err;
	}

	for (page = 0; page < PAGES_PER_ROW; page++) {
		err = flash_sam0_write_page(
			dev, offset + page * FLASH_PAGE_SIZE,
			&ctx->buf[page * FLASH_PAGE_SIZE]);
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
	off_t addr;
	int err;

	LOG_DBG("0x%lx: len %zu", (long)offset, len);

	err = flash_sam0_valid_range(offset, len);
	if (err != 0) {
		return err;
	}

	flash_sam0_sem_take(dev);

	for (addr = offset; addr < offset + len; addr++) {
		off_t base = addr & ~(ROW_SIZE - 1);

		if (base != ctx->offset) {
			/* Started a new row.  Flush any pending ones. */
			flash_sam0_commit(dev);
			memcpy(ctx->buf, (void *)base, sizeof(ctx->buf));
			ctx->offset = base;
		}

		ctx->buf[addr % ROW_SIZE] = *pdata++;
	}

	flash_sam0_commit(dev);
	flash_sam0_sem_give(dev);

	return 0;
}

#else /* CONFIG_SOC_FLASH_SAM0_EMULATE_BYTE_PAGES */

static int flash_sam0_write(const struct device *dev, off_t offset,
			    const void *data, size_t len)
{
	const uint8_t *pdata = data;
	int err;
	size_t idx;

	err = flash_sam0_valid_range(offset, len);
	if (err != 0) {
		return err;
	}

	if ((offset % FLASH_PAGE_SIZE) != 0) {
		LOG_WRN("0x%lx: not on a write block boundrary", (long)offset);
		return -EINVAL;
	}

	if ((len % FLASH_PAGE_SIZE) != 0) {
		LOG_WRN("%zu: not a integer number of write blocks", len);
		return -EINVAL;
	}

	flash_sam0_sem_take(dev);

	for (idx = 0; idx < len; idx += FLASH_PAGE_SIZE) {
		err = flash_sam0_write_page(dev, offset + idx, &pdata[idx]);
		if (err != 0) {
			goto done;
		}
	}

done:
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
		LOG_WRN("0x%lx: not on a page boundrary", (long)offset);
		return -EINVAL;
	}

	if ((size % ROW_SIZE) != 0) {
		LOG_WRN("%zu: not a integer number of pages", size);
		return -EINVAL;
	}

	flash_sam0_sem_take(dev);

	for (size_t addr = offset; addr < offset + size; addr += ROW_SIZE) {
		err = flash_sam0_erase_row(dev, addr);
		if (err != 0) {
			goto done;
		}
	}

done:
	flash_sam0_sem_give(dev);

	return err;
}

static int flash_sam0_write_protection(const struct device *dev, bool enable)
{
	off_t offset;
	int err;

	flash_sam0_sem_take(dev);

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
	flash_sam0_sem_give(dev);

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

static int flash_sam0_init(const struct device *dev)
{
	struct flash_sam0_data *ctx = dev->data;

	k_sem_init(&ctx->sem, 1, 1);

#ifdef PM_APBBMASK_NVMCTRL
	/* Ensure the clock is on. */
	PM->APBBMASK.bit.NVMCTRL_ = 1;
#else
	MCLK->APBBMASK.reg |= MCLK_APBBMASK_NVMCTRL;
#endif

#ifdef NVMCTRL_CTRLB_MANW
	/* Require an explicit write command */
	NVMCTRL->CTRLB.bit.MANW = 1;
#endif

	return flash_sam0_write_protection(dev, false);
}

static const struct flash_driver_api flash_sam0_api = {
	.write_protection = flash_sam0_write_protection,
	.erase = flash_sam0_erase,
	.write = flash_sam0_write,
	.read = flash_sam0_read,
	.get_parameters = flash_sam0_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_sam0_page_layout,
#endif
};

static struct flash_sam0_data flash_sam0_data_0;

DEVICE_DT_INST_DEFINE(0, flash_sam0_init, device_pm_control_nop,
		    &flash_sam0_data_0, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_sam0_api);
