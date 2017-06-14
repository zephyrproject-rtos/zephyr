/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <flash.h>
#include <init.h>
#include <soc.h>

#include <flash_stm32.h>

#define STM32_FLASH_TIMEOUT	((u32_t) 0x000B0000)

int flash_stm32_check_status(struct flash_stm32_priv *p)
{
#if defined(CONFIG_SOC_SERIES_STM32F4X)
	struct stm32f4x_flash *regs = p->regs;
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	struct stm32l4x_flash *regs = p->regs;
#endif

	u32_t const error =
		FLASH_FLAG_WRPERR |
		FLASH_FLAG_PGAERR |
#if defined(FLASH_FLAG_RDERR)
		FLASH_FLAG_RDERR  |
#endif
#if defined(FLASH_FLAG_PGPERR)
		FLASH_FLAG_PGPERR |
#endif
		FLASH_FLAG_PGSERR |
		FLASH_FLAG_OPERR;

	if (regs->sr & error) {
		return -EIO;
	}

	return 0;
}

int flash_stm32_wait_flash_idle(struct flash_stm32_priv *p)
{
#if defined(CONFIG_SOC_SERIES_STM32F4X)
	struct stm32f4x_flash *regs = p->regs;
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	struct stm32l4x_flash *regs = p->regs;
#endif
	u32_t timeout = STM32_FLASH_TIMEOUT;
	int rc;

	rc = flash_stm32_check_status(p);
	if (rc < 0) {
		return -EIO;
	}

	while ((regs->sr & FLASH_SR_BSY) && timeout) {
		timeout--;
	}

	if (!timeout) {
		return -EIO;
	}

	return 0;
}

void flash_stm32_flush_caches(struct flash_stm32_priv *p)
{
#if defined(CONFIG_SOC_SERIES_STM32F4X)
	struct stm32f4x_flash *regs = p->regs;
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	struct stm32l4x_flash *regs = p->regs;
#endif

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

static int flash_stm32_read(struct device *dev, off_t offset, void *data,
			    size_t len)
{
	if (!flash_stm32_valid_range(offset, len, false)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	memcpy(data, (void *) CONFIG_FLASH_BASE_ADDRESS + offset, len);

	return 0;
}

int flash_stm32_erase(struct device *dev, off_t offset, size_t len)
{
	struct flash_stm32_priv *p = dev->driver_data;
	int rc;

	if (!flash_stm32_valid_range(offset, len, true)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	k_sem_take(&p->sem, K_FOREVER);

	rc = flash_stm32_block_erase_loop(offset, len, p);

	flash_stm32_flush_caches(p);

	k_sem_give(&p->sem);

	return rc;
}

int flash_stm32_write(struct device *dev, off_t offset,
		      const void *data, size_t len)
{
	struct flash_stm32_priv *p = dev->driver_data;
	int rc;

	if (!flash_stm32_valid_range(offset, len, true)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	k_sem_take(&p->sem, K_FOREVER);

	rc = flash_stm32_write_range(offset, data, len, p);

	k_sem_give(&p->sem);

	return rc;
}

static int flash_stm32_write_protection(struct device *dev, bool enable)
{
	struct flash_stm32_priv *p = dev->driver_data;
#if defined(CONFIG_SOC_SERIES_STM32F4X)
	struct stm32f4x_flash *regs = p->regs;
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	struct stm32l4x_flash *regs = p->regs;
#endif
	int rc = 0;

	k_sem_take(&p->sem, K_FOREVER);

	if (enable) {
		rc = flash_stm32_wait_flash_idle(p);
		if (rc) {
			k_sem_give(&p->sem);
			return rc;
		}
		regs->cr |= FLASH_CR_LOCK;
	} else {
		if (regs->cr & FLASH_CR_LOCK) {
			regs->keyr = FLASH_KEY1;
			regs->keyr = FLASH_KEY2;
		}
	}

	k_sem_give(&p->sem);

	return rc;
}

static struct flash_stm32_priv flash_data = {
#if defined(CONFIG_SOC_SERIES_STM32F4X)
	.regs = (struct stm32f4x_flash *) FLASH_R_BASE,
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	.regs = (struct stm32l4x_flash *) FLASH_R_BASE,
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_FLASH },
#endif
};

static const struct flash_driver_api flash_stm32_api = {
	.write_protection = flash_stm32_write_protection,
	.erase = flash_stm32_erase,
	.write = flash_stm32_write,
	.read = flash_stm32_read,
};

static int stm32_flash_init(struct device *dev)
{
	struct flash_stm32_priv *p = dev->driver_data;
#if defined(CONFIG_SOC_SERIES_STM32L4X)
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	/* enable clock */
	clock_control_on(clk, (clock_control_subsys_t *)&p->pclken);
#endif

	k_sem_init(&p->sem, 1, 1);

	return flash_stm32_write_protection(dev, false);
}

DEVICE_AND_API_INIT(stm32_flash, CONFIG_SOC_FLASH_STM32_DEV_NAME,
		    stm32_flash_init, &flash_data, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_stm32_api);

