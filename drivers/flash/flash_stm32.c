/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS.
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <init.h>
#include <soc.h>

#include "flash_stm32.h"

/* STM32F0: maximum erase time of 40ms for a 2K sector */
#if defined(CONFIG_SOC_SERIES_STM32F0X)
#define STM32_FLASH_MAX_ERASE_TIME	(K_MSEC(40))
/* STM32F3: maximum erase time of 40ms for a 2K sector */
#elif defined(CONFIG_SOC_SERIES_STM32F3X)
#define STM32_FLASH_MAX_ERASE_TIME	(K_MSEC(40))
/* STM32F4: maximum erase time of 4s for a 128K sector */
#elif defined(CONFIG_SOC_SERIES_STM32F4X)
#define STM32_FLASH_MAX_ERASE_TIME	(K_MSEC(4000))
/* STM32F7: maximum erase time of 4s for a 256K sector */
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
#define STM32_FLASH_MAX_ERASE_TIME	(K_MSEC(4000))
/* STM32L4: maximum erase time of 24.47ms for a 2K sector */
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
#define STM32_FLASH_MAX_ERASE_TIME	(K_MSEC(25))
/* STM32WB: maximum erase time of 24.5ms for a 4K sector */
#elif defined(CONFIG_SOC_SERIES_STM32WBX)
#define STM32_FLASH_MAX_ERASE_TIME	(K_MSEC(25))
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
/* STM32G0: maximum erase time of 40ms for a 2K sector */
#define STM32_FLASH_MAX_ERASE_TIME	(K_MSEC(40))
/* STM32G4: maximum erase time of 24.47ms for a 2K sector */
#elif defined(CONFIG_SOC_SERIES_STM32G4X)
#define STM32_FLASH_MAX_ERASE_TIME	(K_MSEC(25))
#endif

/* Let's wait for double the max erase time to be sure that the operation is
 * completed.
 */
#define STM32_FLASH_TIMEOUT	(2 * STM32_FLASH_MAX_ERASE_TIME)

#define CFG_HW_FLASH_SEMID	2

/*
 * This is named flash_stm32_sem_take instead of flash_stm32_lock (and
 * similarly for flash_stm32_sem_give) to avoid confusion with locking
 * actual flash pages.
 */
static inline void flash_stm32_sem_take(struct device *dev)
{

#ifdef CONFIG_SOC_SERIES_STM32WBX
	while (LL_HSEM_1StepLock(HSEM, CFG_HW_FLASH_SEMID)) {
	}
#endif /* CONFIG_SOC_SERIES_STM32WBX */

	k_sem_take(&FLASH_STM32_PRIV(dev)->sem, K_FOREVER);
}

static inline void flash_stm32_sem_give(struct device *dev)
{

	k_sem_give(&FLASH_STM32_PRIV(dev)->sem);

#ifdef CONFIG_SOC_SERIES_STM32WBX
	LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);
#endif /* CONFIG_SOC_SERIES_STM32WBX */

}

#if !defined(CONFIG_SOC_SERIES_STM32WBX)
static int flash_stm32_check_status(struct device *dev)
{
	u32_t const error =
#if defined(FLASH_FLAG_PGAERR)
		FLASH_FLAG_PGAERR |
#endif
#if defined(FLASH_FLAG_RDERR)
		FLASH_FLAG_RDERR  |
#endif
#if defined(FLASH_FLAG_PGPERR)
		FLASH_FLAG_PGPERR |
#endif
#if defined(FLASH_FLAG_PGSERR)
		FLASH_FLAG_PGSERR |
#endif
#if defined(FLASH_FLAG_OPERR)
		FLASH_FLAG_OPERR |
#endif
#if defined(FLASH_FLAG_PGERR)
		FLASH_FLAG_PGERR |
#endif
		FLASH_FLAG_WRPERR;

	if (FLASH_STM32_REGS(dev)->sr & error) {
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_SOC_SERIES_STM32WBX */

int flash_stm32_wait_flash_idle(struct device *dev)
{
	s64_t timeout_time = k_uptime_get() + STM32_FLASH_TIMEOUT;
	int rc;

	rc = flash_stm32_check_status(dev);
	if (rc < 0) {
		return -EIO;
	}

	while ((FLASH_STM32_REGS(dev)->sr & FLASH_SR_BSY)) {
		if (k_uptime_get() > timeout_time) {
			return -EIO;
		}
	}

	return 0;
}

static void flash_stm32_flush_caches(struct device *dev,
				     off_t offset, size_t len)
{
#if defined(CONFIG_SOC_SERIES_STM32F0X) || defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X)
	ARG_UNUSED(dev);
	ARG_UNUSED(offset);
	ARG_UNUSED(len);
#elif defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	ARG_UNUSED(offset);
	ARG_UNUSED(len);
#if defined(CONFIG_SOC_SERIES_STM32F4X)
	struct stm32f4x_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	struct stm32l4x_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32WBX)
	struct stm32wbx_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32G4X)
	struct stm32g4x_flash *regs = FLASH_STM32_REGS(dev);
#endif
	if (regs->acr.val & FLASH_ACR_DCEN) {
		regs->acr.val &= ~FLASH_ACR_DCEN;
		regs->acr.val |= FLASH_ACR_DCRST;
		regs->acr.val &= ~FLASH_ACR_DCRST;
		regs->acr.val |= FLASH_ACR_DCEN;
	}
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
	SCB_InvalidateDCache_by_Addr((uint32_t *)(CONFIG_FLASH_BASE_ADDRESS
						  + offset), len);
#endif
}

static int flash_stm32_read(struct device *dev, off_t offset, void *data,
			    size_t len)
{
	if (!flash_stm32_valid_range(dev, offset, len, false)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	memcpy(data, (u8_t *) CONFIG_FLASH_BASE_ADDRESS + offset, len);

	return 0;
}

static int flash_stm32_erase(struct device *dev, off_t offset, size_t len)
{
	int rc;

	if (!flash_stm32_valid_range(dev, offset, len, true)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	flash_stm32_sem_take(dev);

	rc = flash_stm32_block_erase_loop(dev, offset, len);

	flash_stm32_flush_caches(dev, offset, len);

	flash_stm32_sem_give(dev);

	return rc;
}

static int flash_stm32_write(struct device *dev, off_t offset,
			     const void *data, size_t len)
{
	int rc;

	if (!flash_stm32_valid_range(dev, offset, len, true)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	flash_stm32_sem_take(dev);

	rc = flash_stm32_write_range(dev, offset, data, len);

	flash_stm32_sem_give(dev);

	return rc;
}

static int flash_stm32_write_protection(struct device *dev, bool enable)
{
#if defined(CONFIG_SOC_SERIES_STM32F4X)
	struct stm32f4x_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
	struct stm32f7x_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32F0X)
	struct stm32f0x_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32F3X)
	struct stm32f3x_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	struct stm32l4x_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32WBX)
	struct stm32wbx_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
	struct stm32g0x_flash *regs = FLASH_STM32_REGS(dev);
#elif defined(CONFIG_SOC_SERIES_STM32G4X)
	struct stm32g4x_flash *regs = FLASH_STM32_REGS(dev);
#endif
	int rc = 0;

	flash_stm32_sem_take(dev);

	if (enable) {
		rc = flash_stm32_wait_flash_idle(dev);
		if (rc) {
			flash_stm32_sem_give(dev);
			return rc;
		}
		regs->cr |= FLASH_CR_LOCK;
	} else {
		if (regs->cr & FLASH_CR_LOCK) {
			regs->keyr = FLASH_KEY1;
			regs->keyr = FLASH_KEY2;
		}
	}

	flash_stm32_sem_give(dev);

	return rc;
}

static struct flash_stm32_priv flash_data = {
#if defined(CONFIG_SOC_SERIES_STM32F0X)
	.regs = (struct stm32f0x_flash *) DT_FLASH_DEV_BASE_ADDRESS,
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_FLASH },
#elif defined(CONFIG_SOC_SERIES_STM32F3X)
	.regs = (struct stm32f3x_flash *) DT_FLASH_DEV_BASE_ADDRESS,
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_FLASH },
#elif defined(CONFIG_SOC_SERIES_STM32F4X)
	.regs = (struct stm32f4x_flash *) DT_FLASH_DEV_BASE_ADDRESS,
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
	.regs = (struct stm32f7x_flash *) DT_FLASH_DEV_BASE_ADDRESS,
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	.regs = (struct stm32l4x_flash *) DT_FLASH_DEV_BASE_ADDRESS,
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_FLASH },
#elif defined(CONFIG_SOC_SERIES_STM32WBX)
	.regs = (struct stm32wbx_flash *) DT_FLASH_DEV_BASE_ADDRESS,
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
	.regs = (struct stm32g0x_flash *) DT_FLASH_DEV_BASE_ADDRESS,
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_FLASH },
#elif defined(CONFIG_SOC_SERIES_STM32G4X)
	.regs = (struct stm32g4x_flash *) DT_FLASH_DEV_BASE_ADDRESS,
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_FLASH },
#endif
};

static const struct flash_driver_api flash_stm32_api = {
	.write_protection = flash_stm32_write_protection,
	.erase = flash_stm32_erase,
	.write = flash_stm32_write,
	.read = flash_stm32_read,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_stm32_page_layout,
#endif
#ifdef DT_INST_0_SOC_NV_FLASH_WRITE_BLOCK_SIZE
	.write_block_size = DT_INST_0_SOC_NV_FLASH_WRITE_BLOCK_SIZE,
#else
#error Flash write block size not available
	/* Flash Write block size is extracted from device tree */
	/* as flash node property 'write-block-size' */
#endif
};

static int stm32_flash_init(struct device *dev)
{
	struct flash_stm32_priv *p = FLASH_STM32_PRIV(dev);
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X)
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	/*
	 * On STM32F0, Flash interface clock source is always HSI,
	 * so statically enable HSI here.
	 */
#if defined(CONFIG_SOC_SERIES_STM32F0X) || defined(CONFIG_SOC_SERIES_STM32F3X)
	LL_RCC_HSI_Enable();

	while (!LL_RCC_HSI_IsReady()) {
	}
#endif

	/* enable clock */
	if (clock_control_on(clk, (clock_control_subsys_t *)&p->pclken) != 0) {
		return -EIO;
	}
#endif

#ifdef CONFIG_SOC_SERIES_STM32WBX
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_HSEM);
#endif /* CONFIG_SOC_SERIES_STM32WBX */

	k_sem_init(&p->sem, 1, 1);

	return flash_stm32_write_protection(dev, false);
}

DEVICE_AND_API_INIT(stm32_flash, DT_FLASH_DEV_NAME,
		    stm32_flash_init, &flash_data, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_stm32_api);
