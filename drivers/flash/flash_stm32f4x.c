/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <flash.h>
#include <init.h>
#include <soc.h>

#include <flash_registers.h>
#include <flash_map.h>

struct flash_priv {
	struct stm32f4x_flash *regs;
	struct k_sem sem;
};

static bool valid_range(off_t offset, u32_t len)
{
	return offset >= 0 && (offset + len - 1 <= STM32F4X_FLASH_END);
}

static int check_status(struct stm32f4x_flash *regs)
{
	u32_t const error =
		FLASH_FLAG_WRPERR |
		FLASH_FLAG_PGAERR |
#if defined(FLASH_FLAG_RDERR)
		FLASH_FLAG_RDERR  |
#endif
		FLASH_FLAG_PGPERR |
		FLASH_FLAG_PGSERR |
		FLASH_FLAG_OPERR;

	if (regs->status & error) {
		return -EIO;
	}

	return 0;
}

static int wait_flash_idle(struct stm32f4x_flash *regs)
{
	u32_t timeout = STM32F4X_FLASH_TIMEOUT;
	int rc;

	rc = check_status(regs);
	if (rc < 0) {
		return -EIO;
	}

	while ((regs->status & FLASH_FLAG_BSY) && timeout) {
		timeout--;
	}

	if (!timeout) {
		return -EIO;
	}

	return 0;
}

static int write_byte(off_t offset, u8_t val, struct stm32f4x_flash *regs)
{
	u32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->ctrl & FLASH_CR_LOCK) {
		return -EIO;
	}

	rc = wait_flash_idle(regs);
	if (rc < 0) {
		return rc;
	}

	regs->ctrl &= CR_PSIZE_MASK;
	regs->ctrl |= FLASH_PSIZE_BYTE;
	regs->ctrl |= FLASH_CR_PG;

	/* flush the register write */
	tmp = regs->ctrl;

	*((u8_t *) offset + CONFIG_FLASH_BASE_ADDRESS) = val;

	rc = wait_flash_idle(regs);
	regs->ctrl &= (~FLASH_CR_PG);

	return rc;
}

static int erase_sector(u16_t sector, struct stm32f4x_flash *regs)
{
	u32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->ctrl & FLASH_CR_LOCK) {
		return -EIO;
	}

	rc = wait_flash_idle(regs);
	if (rc < 0) {
		return rc;
	}

	regs->ctrl &= STM32F4X_SECTOR_MASK;
	regs->ctrl |= FLASH_CR_SER | (sector << 3);
	regs->ctrl |= FLASH_CR_STRT;

	/* flush the register write */
	tmp = regs->ctrl;

	rc = wait_flash_idle(regs);
	regs->ctrl &= (FLASH_CR_SER | FLASH_CR_SNB);

	return rc;
}

static void flush_caches(struct stm32f4x_flash *regs)
{
	if (regs->acr.val & FLASH_ACR_ICEN) {
		regs->acr.val &= ~FLASH_ACR_ICEN;
		regs->acr.val |= FLASH_ACR_ICRST;
		regs->acr.val &= ~FLASH_ACR_ICRST;
		regs->acr.val |= FLASH_ACR_ICEN;
	}

	if (regs->acr.val & FLASH_ACR_DCEN) {
		regs->acr.val &= ~FLASH_ACR_DCEN;
		regs->acr.val |= FLASH_ACR_DCRST;
		regs->acr.val &= ~FLASH_ACR_DCRST;
		regs->acr.val |= FLASH_ACR_DCEN;
	}
}

static int flash_stm32f4x_erase(struct device *dev, off_t offset, size_t len)
{
	struct flash_priv *p = dev->driver_data;
	int i, rc = 0;

	if (!valid_range(offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	k_sem_take(&p->sem, K_FOREVER);

	i = stm32f4x_get_sector(offset);
	for (; i <= stm32f4x_get_sector(offset + len - 1); i++) {
		rc = erase_sector(i, p->regs);
		if (rc < 0) {
			break;
		}
	}
	flush_caches(p->regs);

	k_sem_give(&p->sem);

	return rc;
}

static int flash_stm32f4x_read(struct device *dev, off_t offset, void *data,
	size_t len)
{
	if (!valid_range(offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	memcpy(data, (void *) CONFIG_FLASH_BASE_ADDRESS + offset, len);

	return 0;
}

static int flash_stm32f4x_write(struct device *dev, off_t offset,
	const void *data, size_t len)
{
	struct flash_priv *p = dev->driver_data;
	int rc, i;

	if (!valid_range(offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	k_sem_take(&p->sem, K_FOREVER);

	for (i = 0; i < len; i++, offset++) {
		rc = write_byte(offset, ((const u8_t *) data)[i], p->regs);
		if (rc < 0) {
			k_sem_give(&p->sem);
			return rc;
		}
	}

	k_sem_give(&p->sem);

	return 0;
}

static int flash_stm32f4x_write_protection(struct device *dev, bool enable)
{
	struct flash_priv *p = dev->driver_data;
	struct stm32f4x_flash *regs = p->regs;
	int rc = 0;

	k_sem_take(&p->sem, K_FOREVER);

	if (enable) {
		rc = wait_flash_idle(regs);
		if (rc) {
			k_sem_give(&p->sem);
			return rc;
		}
		regs->ctrl |= FLASH_CR_LOCK;
	} else {
		if (regs->ctrl & FLASH_CR_LOCK) {
			regs->key = FLASH_KEY1;
			regs->key = FLASH_KEY2;
		}
	}

	k_sem_give(&p->sem);

	return rc;
}

static struct flash_priv flash_data = {
	.regs = (struct stm32f4x_flash *) FLASH_R_BASE,
};

static const struct flash_driver_api flash_stm32f4x_api = {
	.write_protection = flash_stm32f4x_write_protection,
	.erase = flash_stm32f4x_erase,
	.write = flash_stm32f4x_write,
	.read = flash_stm32f4x_read,
};

static int stm32f4x_flash_init(struct device *dev)
{
	struct flash_priv *p = dev->driver_data;

	k_sem_init(&p->sem, 1, 1);

	return flash_stm32f4x_write_protection(dev, false);
}

DEVICE_AND_API_INIT(stm32f4x_flash,
	CONFIG_SOC_FLASH_STM32_DEV_NAME,
	stm32f4x_flash_init,
	&flash_data,
	NULL,
	POST_KERNEL,
	CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	&flash_stm32f4x_api);

