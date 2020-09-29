/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS.
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>

#define DT_DRV_COMPAT st_stm32_flash_controller

#include <string.h>
#include <drivers/flash.h>
#include <init.h>
#include <soc.h>
#include <logging/log.h>

#include "flash_stm32.h"
#include "stm32_hsem.h"

LOG_MODULE_REGISTER(flash_stm32, CONFIG_FLASH_LOG_LEVEL);

/* STM32F0: maximum erase time of 40ms for a 2K sector */
#if defined(CONFIG_SOC_SERIES_STM32F0X)
#define STM32_FLASH_MAX_ERASE_TIME	40
/* STM32F3: maximum erase time of 40ms for a 2K sector */
#elif defined(CONFIG_SOC_SERIES_STM32F1X)
#define STM32_FLASH_MAX_ERASE_TIME	40
/* STM32F3: maximum erase time of 40ms for a 2K sector */
#elif defined(CONFIG_SOC_SERIES_STM32F3X)
#define STM32_FLASH_MAX_ERASE_TIME	40
/* STM32F4: maximum erase time of 4s for a 128K sector */
#elif defined(CONFIG_SOC_SERIES_STM32F4X)
#define STM32_FLASH_MAX_ERASE_TIME	4000
/* STM32F7: maximum erase time of 4s for a 256K sector */
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
#define STM32_FLASH_MAX_ERASE_TIME	4000
/* STM32L0: maximum erase time of 3.2ms for a 128B page */
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
#define STM32_FLASH_MAX_ERASE_TIME	4
/* STM32L4: maximum erase time of 24.47ms for a 2K sector */
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
#define STM32_FLASH_MAX_ERASE_TIME	25
/* STM32WB: maximum erase time of 24.5ms for a 4K sector */
#elif defined(CONFIG_SOC_SERIES_STM32WBX)
#define STM32_FLASH_MAX_ERASE_TIME	25
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
/* STM32G0: maximum erase time of 40ms for a 2K sector */
#define STM32_FLASH_MAX_ERASE_TIME	40
/* STM32G4: maximum erase time of 24.47ms for a 2K sector */
#elif defined(CONFIG_SOC_SERIES_STM32G4X)
#define STM32_FLASH_MAX_ERASE_TIME	25
#endif

/* Let's wait for double the max erase time to be sure that the operation is
 * completed.
 */
#define STM32_FLASH_TIMEOUT	(2 * STM32_FLASH_MAX_ERASE_TIME)

static const struct flash_parameters flash_stm32_parameters = {
	.write_block_size = FLASH_STM32_WRITE_BLOCK_SIZE,
	/* Some SoCs (L0/L1) use an EEPROM under the hood. Distinguish
	 * between them based on the presence of the PECR register. */
#if defined(FLASH_PECR_ERASE)
	.erase_value = 0,
#else
	.erase_value = 0xff,
#endif
};

#if defined(CONFIG_MULTITHREADING)
/*
 * This is named flash_stm32_sem_take instead of flash_stm32_lock (and
 * similarly for flash_stm32_sem_give) to avoid confusion with locking
 * actual flash pages.
 */
static inline void _flash_stm32_sem_take(const struct device *dev)
{
	k_sem_take(&FLASH_STM32_PRIV(dev)->sem, K_FOREVER);
	z_stm32_hsem_lock(CFG_HW_FLASH_SEMID, HSEM_LOCK_WAIT_FOREVER);
}

static inline void _flash_stm32_sem_give(const struct device *dev)
{
	z_stm32_hsem_unlock(CFG_HW_FLASH_SEMID);
	k_sem_give(&FLASH_STM32_PRIV(dev)->sem);
}

#define flash_stm32_sem_init(dev) k_sem_init(&FLASH_STM32_PRIV(dev)->sem, 1, 1)
#define flash_stm32_sem_take(dev) _flash_stm32_sem_take(dev)
#define flash_stm32_sem_give(dev) _flash_stm32_sem_give(dev)
#else
#define flash_stm32_sem_init(dev)
#define flash_stm32_sem_take(dev)
#define flash_stm32_sem_give(dev)
#endif

#if !defined(CONFIG_SOC_SERIES_STM32WBX)
static int flash_stm32_check_status(const struct device *dev)
{
	uint32_t const error =
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

	if (FLASH_STM32_REGS(dev)->SR & error) {
		LOG_DBG("Status: 0x%08x", FLASH_STM32_REGS(dev)->SR & error);
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_SOC_SERIES_STM32WBX */

int flash_stm32_wait_flash_idle(const struct device *dev)
{
	int64_t timeout_time = k_uptime_get() + STM32_FLASH_TIMEOUT;
	int rc;

	rc = flash_stm32_check_status(dev);
	if (rc < 0) {
		return -EIO;
	}
#if defined(CONFIG_SOC_SERIES_STM32G0X)
	while ((FLASH_STM32_REGS(dev)->SR & FLASH_SR_BSY1)) {
#else
	while ((FLASH_STM32_REGS(dev)->SR & FLASH_SR_BSY)) {
#endif
		if (k_uptime_get() > timeout_time) {
			LOG_ERR("Timeout! val: %d", STM32_FLASH_TIMEOUT);
			return -EIO;
		}
	}

	return 0;
}

static void flash_stm32_flush_caches(const struct device *dev,
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

	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);

	if (regs->ACR & FLASH_ACR_DCEN) {
		regs->ACR &= ~FLASH_ACR_DCEN;
		regs->ACR |= FLASH_ACR_DCRST;
		regs->ACR &= ~FLASH_ACR_DCRST;
		regs->ACR |= FLASH_ACR_DCEN;
	}
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
	SCB_InvalidateDCache_by_Addr((uint32_t *)(CONFIG_FLASH_BASE_ADDRESS
						  + offset), len);
#endif
}

static int flash_stm32_read(const struct device *dev, off_t offset,
			    void *data,
			    size_t len)
{
	if (!flash_stm32_valid_range(dev, offset, len, false)) {
		LOG_ERR("Read range invalid. Offset: %ld, len: %zu",
			(long int) offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("Read offset: %ld, len: %zu", (long int) offset, len);

	memcpy(data, (uint8_t *) CONFIG_FLASH_BASE_ADDRESS + offset, len);

	return 0;
}

static int flash_stm32_erase(const struct device *dev, off_t offset,
			     size_t len)
{
	int rc;

	if (!flash_stm32_valid_range(dev, offset, len, true)) {
		LOG_ERR("Erase range invalid. Offset: %ld, len: %zu",
			(long int) offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	flash_stm32_sem_take(dev);

	LOG_DBG("Erase offset: %ld, len: %zu", (long int) offset, len);

	rc = flash_stm32_block_erase_loop(dev, offset, len);

	flash_stm32_flush_caches(dev, offset, len);

	flash_stm32_sem_give(dev);

	return rc;
}

static int flash_stm32_write(const struct device *dev, off_t offset,
			     const void *data, size_t len)
{
	int rc;

	if (!flash_stm32_valid_range(dev, offset, len, true)) {
		LOG_ERR("Write range invalid. Offset: %ld, len: %zu",
			(long int) offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	flash_stm32_sem_take(dev);

	LOG_DBG("Write offset: %ld, len: %zu", (long int) offset, len);

	rc = flash_stm32_write_range(dev, offset, data, len);

	flash_stm32_sem_give(dev);

	return rc;
}

static int flash_stm32_write_protection(const struct device *dev, bool enable)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);

	int rc = 0;

	flash_stm32_sem_take(dev);

	if (enable) {
		rc = flash_stm32_wait_flash_idle(dev);
		if (rc) {
			flash_stm32_sem_give(dev);
			return rc;
		}
	}

#if defined(FLASH_CR_LOCK)
	if (enable) {
		regs->CR |= FLASH_CR_LOCK;
	} else {
		if (regs->CR & FLASH_CR_LOCK) {
			regs->KEYR = FLASH_KEY1;
			regs->KEYR = FLASH_KEY2;
		}
	}
#else
	if (enable) {
		regs->PECR |= FLASH_PECR_PRGLOCK;
		regs->PECR |= FLASH_PECR_PELOCK;
	} else {
		if (regs->PECR & FLASH_PECR_PRGLOCK) {
			LOG_DBG("Disabling write protection");
			regs->PEKEYR = FLASH_PEKEY1;
			regs->PEKEYR = FLASH_PEKEY2;
			regs->PRGKEYR = FLASH_PRGKEY1;
			regs->PRGKEYR = FLASH_PRGKEY2;
		}
		if (FLASH->PECR & FLASH_PECR_PRGLOCK) {
			LOG_ERR("Unlock failed");
			rc = -EIO;
		}
	}
#endif

	if (enable) {
		LOG_DBG("Enable write protection");
	} else {
		LOG_DBG("Disable write protection");
	}

	flash_stm32_sem_give(dev);

	return rc;
}

static const struct flash_parameters *
flash_stm32_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_stm32_parameters;
}

static struct flash_stm32_priv flash_data = {
	.regs = (FLASH_TypeDef *) DT_INST_REG_ADDR(0),
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_FLASH },
#endif
};

static const struct flash_driver_api flash_stm32_api = {
	.write_protection = flash_stm32_write_protection,
	.erase = flash_stm32_erase,
	.write = flash_stm32_write,
	.read = flash_stm32_read,
	.get_parameters = flash_stm32_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_stm32_page_layout,
#endif
};

static int stm32_flash_init(const struct device *dev)
{
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X)
	struct flash_stm32_priv *p = FLASH_STM32_PRIV(dev);
	const struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	/*
	 * On STM32F0, Flash interface clock source is always HSI,
	 * so statically enable HSI here.
	 */
#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X)
	LL_RCC_HSI_Enable();

	while (!LL_RCC_HSI_IsReady()) {
	}
#endif

	/* enable clock */
	if (clock_control_on(clk, (clock_control_subsys_t *)&p->pclken) != 0) {
		LOG_ERR("Failed to enable clock");
		return -EIO;
	}
#endif

#ifdef CONFIG_SOC_SERIES_STM32WBX
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_HSEM);
#endif /* CONFIG_SOC_SERIES_STM32WBX */

	flash_stm32_sem_init(dev);

	LOG_DBG("Flash initialized. BS: %zu",
		flash_stm32_parameters.write_block_size);

#if ((CONFIG_FLASH_LOG_LEVEL >= LOG_LEVEL_DBG) && CONFIG_FLASH_PAGE_LAYOUT)
	const struct flash_pages_layout *layout;
	size_t layout_size;

	flash_stm32_page_layout(dev, &layout, &layout_size);
	for (size_t i = 0; i < layout_size; i++) {
		LOG_DBG("Block %zu: bs: %zu count: %zu", i,
			layout[i].pages_size, layout[i].pages_count);
	}
#endif

	return flash_stm32_write_protection(dev, false);
}

DEVICE_AND_API_INIT(stm32_flash, DT_INST_LABEL(0),
		    stm32_flash_init, &flash_data, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_stm32_api);
