/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <string.h>

#include <driverlib/flash.h>
#include <driverlib/vims.h>

#define DT_DRV_COMPAT     ti_cc23x0_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#define FLASH_ADDR       DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define FLASH_SIZE       DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define FLASH_ERASE_SIZE DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define FLASH_WRITE_SIZE DT_PROP(SOC_NV_FLASH_NODE, write_block_size)

struct flash_cc23x0_data {
	struct k_sem mutex;
};

static const struct flash_parameters flash_cc23x0_parameters = {
	.write_block_size = FLASH_WRITE_SIZE,
	.erase_value = 0xff,
};

static int flash_cc23x0_init(const struct device *dev)
{
	struct flash_cc23x0_data *data = dev->data;

	k_sem_init(&data->mutex, 1, 1);

	return 0;
}

static void flash_cc23x0_cache_restore(uint32_t vims_mode)
{
	while (VIMSModeGet(VIMS_BASE) == VIMS_MODE_CHANGING) {
		;
	}

	/* Restore VIMS mode and line buffers */
	if (vims_mode != VIMS_MODE_DISABLED) {
		VIMSModeSafeSet(VIMS_BASE, vims_mode, true);
	}

	VIMSLineBufEnable(VIMS_BASE);
}

static uint32_t flash_cc23x0_cache_disable(void)
{
	uint32_t vims_mode;

	/* VIMS and both line buffers should be off during flash update */
	VIMSLineBufDisable(VIMS_BASE);

	while (VIMSModeGet(VIMS_BASE) == VIMS_MODE_CHANGING) {
		;
	}

	/* Save current VIMS mode for restoring it later */
	vims_mode = VIMSModeGet(VIMS_BASE);
	if (vims_mode != VIMS_MODE_DISABLED) {
		VIMSModeSafeSet(VIMS_BASE, VIMS_MODE_DISABLED, true);
	}

	return vims_mode;
}

static int flash_cc23x0_erase(const struct device *dev, off_t offs, size_t size)
{
	struct flash_cc23x0_data *data = dev->data;
	uint32_t vims_mode;
	unsigned int key;
	int i;
	int rc = 0;
	size_t cnt;

	if (!size) {
		return 0;
	}

	/* Offset and length should be multiple of erase size */
	if (((offs % FLASH_ERASE_SIZE) != 0) || ((size % FLASH_ERASE_SIZE) != 0)) {
		return -EINVAL;
	}

	if (k_sem_take(&data->mutex, K_FOREVER)) {
		return -EACCES;
	}

	vims_mode = flash_cc23x0_cache_disable();
	/*
	 * Disable all interrupts to prevent flash read, from TI's TRF:
	 *
	 * During a FLASH memory write or erase operation, the FLASH memory
	 * must not be read.
	 */
	key = irq_lock();

	/* Erase sector/page one by one, break out in case of an error */
	cnt = size / FLASH_ERASE_SIZE;
	for (i = 0; i < cnt; i++, offs += FLASH_ERASE_SIZE) {
		while (FlashCheckFsmForReady() != FAPI_STATUS_FSM_READY) {
			;
		}

		rc = FlashEraseSector(offs);
		if (rc != FAPI_STATUS_SUCCESS) {
			rc = -EIO;
			break;
		}
	}

	irq_unlock(key);

	flash_cc23x0_cache_restore(vims_mode);

	k_sem_give(&data->mutex);
	return rc;
}

static int flash_cc23x0_write(const struct device *dev, off_t offs, const void *data, size_t size)
{
	struct flash_cc23x0_data *flash_data = dev->data;
	uint32_t vims_mode;
	unsigned int key;
	int rc = 0;

	if (!size) {
		return 0;
	}

	if (offs < 0 || size < 1) {
		return -EINVAL;
	}

	if (offs + size > FLASH_SIZE) {
		return -EINVAL;
	}

	/*
	 * From TI's HAL 'driverlib/flash.h':
	 *
	 * The pui8DataBuffer pointer can not point to flash.
	 */
	if ((data >= (void *)FLASH_ADDR) && (data <= (void *)(FLASH_ADDR + FLASH_SIZE))) {
		return -EINVAL;
	}

	if (k_sem_take(&flash_data->mutex, K_FOREVER)) {
		return -EACCES;
	}

	vims_mode = flash_cc23x0_cache_disable();

	key = irq_lock();

	while (FlashCheckFsmForReady() != FAPI_STATUS_FSM_READY) {
		;
	}

	rc = FlashProgram((uint8_t *)data, offs, size);
	if (rc != FAPI_STATUS_SUCCESS) {
		rc = -EIO;
	}

	irq_unlock(key);

	flash_cc23x0_cache_restore(vims_mode);

	k_sem_give(&flash_data->mutex);

	return rc;
}

static int flash_cc23x0_read(const struct device *dev, off_t offs, void *data, size_t size)
{
	ARG_UNUSED(dev);

	if (!size) {
		return 0;
	}

	if (offs < 0 || size < 1) {
		return -EINVAL;
	}

	if (offs + size > FLASH_SIZE) {
		return -EINVAL;
	}

	memcpy(data, (void *)offs, size);

	return 0;
}

static const struct flash_parameters *flash_cc23x0_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_cc23x0_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = FLASH_SIZE / FLASH_ERASE_SIZE,
	.pages_size = FLASH_ERASE_SIZE,
};

static void flash_cc23x0_layout(const struct device *dev, const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_cc23x0_api) = {
	.erase = flash_cc23x0_erase,
	.write = flash_cc23x0_write,
	.read = flash_cc23x0_read,
	.get_parameters = flash_cc23x0_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_cc23x0_layout,
#endif
};

static struct flash_cc23x0_data cc23x0_flash_data;

DEVICE_DT_INST_DEFINE(0, flash_cc23x0_init, NULL, &cc23x0_flash_data, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_cc23x0_api);
