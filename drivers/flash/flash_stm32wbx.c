/*
 * Copyright (c) 2019 Linaro Limited
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32wb
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <init.h>
#include <soc.h>
#include <sys/__assert.h>

#include "flash_stm32.h"
#include "stm32_hsem.h"
#if defined(CONFIG_BT)
#include "shci.h"
#endif

#define STM32WBX_PAGE_SHIFT	12

/* offset and len must be aligned on 8 for write,
 * positive and not beyond end of flash
 */
bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
	return (!write || (offset % 8 == 0 && len % 8 == 0U)) &&
	       flash_stm32_range_exists(dev, offset, len);
}

/*
 * Up to 255 4K pages
 */
static uint32_t get_page(off_t offset)
{
	return offset >> STM32WBX_PAGE_SHIFT;
}

static int write_dword(const struct device *dev, off_t offset, uint64_t val)
{
	volatile uint32_t *flash = (uint32_t *)(offset + CONFIG_FLASH_BASE_ADDRESS);
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int ret, rc;
	uint32_t cpu1_sem_status;
	uint32_t cpu2_sem_status = 0;
	uint32_t key;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check if this double word is erased */
	if (flash[0] != 0xFFFFFFFFUL ||
	    flash[1] != 0xFFFFFFFFUL) {
		return -EIO;
	}

	ret = flash_stm32_check_status(dev);
	if (ret < 0) {
		return -EIO;
	}

	/* Implementation of STM32 AN5289, proposed in STM32WB Cube Application
	 * BLE_RfWithFlash
	 * https://github.com/STMicroelectronics/STM32CubeWB/tree/master/Projects/P-NUCLEO-WB55.Nucleo/Applications/BLE/BLE_RfWithFlash
	 */

	do {
		/**
		 * When the PESD bit mechanism is used by CPU2 to protect its
		 * timing, the PESD bit should be polled here.
		 * If the PESD is set, the CPU1 will be stalled when reading
		 * literals from an ISR that may occur after the flash
		 * processing has been requested but suspended due to the PESD
		 * bit.
		 *
		 * Note: This code is required only when the PESD mechanism is
		 * used to protect the CPU2 timing.
		 * However, keeping that code make it compatible with both
		 * mechanisms.
		 */
		while (LL_FLASH_IsActiveFlag_OperationSuspended())
			;

		/* Enter critical section */
		key = irq_lock();

		/**
		 *  Depending on the application implementation, in case a
		 *  multitasking is possible with an OS, it should be checked
		 *  here if another task in the application disallowed flash
		 *  processing to protect some latency in critical code
		 *  execution.
		 *  When flash processing is ongoing, the CPU cannot access the
		 *  flash anymore.Trying to access the flash during that time
		 *  stalls the CPU.
		 *  The only way for CPU1 to disallow flash processing is to
		 *  take CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID.
		 */
		cpu1_sem_status = LL_HSEM_GetStatus(HSEM,
			CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID);
		if (cpu1_sem_status == 0) {
			/**
			 *  Check now if the CPU2 disallows flash processing to
			 *  protect its timing. If the semaphore is locked, the
			 *  CPU2 does not allow flash processing
			 *
			 *  Note: By default, the CPU2 uses the PESD mechanism
			 *  to protect its timing, therefore, it is useless to
			 *  get/release the semaphore.
			 *
			 *  However, keeping that code make it compatible with
			 *  bothmechanisms.
			 *  The protection by semaphore is enabled on CPU2 side
			 *  with the command SHCI_C2_SetFlashActivityControl()
			 *
			 */
			cpu2_sem_status = LL_HSEM_1StepLock(HSEM,
				CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID);
			if (cpu2_sem_status == 0) {
				/**
				 * When CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID is
				 * taken, it is allowed to only write one
				 * single 64bits data.
				 * When several 64bits data need to be erased,
				 * the application shall first exit from the
				 * critical section and try again.
				 */
				/* Set the PG bit */
				regs->CR |= FLASH_CR_PG;

				/* Flush the register write */
				tmp = regs->CR;

				/* Perform the data write operation at desired
				 * memory address
				 */
				flash[0] = (uint32_t)val;
				flash[1] = (uint32_t)(val >> 32);

				/**
				 *  Release the semaphore to give the
				 *  opportunity to CPU2 to protect its timing
				 *  versus the next flash operation by taking
				 *  this semaphore.
				 *  Note that the CPU2 is polling on this
				 *  semaphore so CPU1 shall release it as fast
				 *  as possible.
				 *  This is why this code is protected by a
				 *  critical section.
				 */
				LL_HSEM_ReleaseLock(HSEM,
					CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID,
					0);
			}
		}

		/* Exit critical section */
		irq_unlock(key);

	} while (cpu2_sem_status || cpu1_sem_status);

	/* Wait until the BSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(dev);

	/* Clear the PG bit */
	regs->CR &= (~FLASH_CR_PG);

	return rc;
}

static int erase_page(const struct device *dev, uint32_t page)
{
	uint32_t cpu1_sem_status;
	uint32_t cpu2_sem_status = 0;
	uint32_t key;

	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Implementation of STM32 AN5289, proposed in STM32WB Cube Application
	 * BLE_RfWithFlash
	 * https://github.com/STMicroelectronics/STM32CubeWB/tree/master/Projects/P-NUCLEO-WB55.Nucleo/Applications/BLE/BLE_RfWithFlash
	 */

	do {
		/**
		 * When the PESD bit mechanism is used by CPU2 to protect its
		 * timing, the PESD bit should be polled here.
		 * If the PESD is set, the CPU1 will be stalled when reading
		 * literals from an ISR that may occur after the flash
		 * processing has been requested but suspended due to the PESD
		 * bit.
		 *
		 * Note: This code is required only when the PESD mechanism is
		 * used to protect the CPU2 timing.
		 * However, keeping that code make it compatible with both
		 * mechanisms.
		 */
		while (LL_FLASH_IsActiveFlag_OperationSuspended())
			;

		/* Enter critical section */
		key = irq_lock();

		/**
		 *  Depending on the application implementation, in case a
		 *  multitasking is possible with an OS, it should be checked
		 *  here if another task in the application disallowed flash
		 *  processing to protect some latency in critical code
		 *  execution.
		 *  When flash processing is ongoing, the CPU cannot access the
		 *  flash anymore.Trying to access the flash during that time
		 *  stalls the CPU.
		 *  The only way for CPU1 to disallow flash processing is to
		 *  take CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID.
		 */
		cpu1_sem_status = LL_HSEM_GetStatus(HSEM,
			CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID);
		if (cpu1_sem_status == 0) {
			/**
			 *  Check now if the CPU2 disallows flash processing to
			 *  protect its timing. If the semaphore is locked, the
			 *  CPU2 does not allow flash processing
			 *
			 *  Note: By default, the CPU2 uses the PESD mechanism
			 *  to protect its timing, therefore, it is useless to
			 *  get/release the semaphore.
			 *
			 *  However, keeping that code make it compatible with
			 *  bothmechanisms.
			 *  The protection by semaphore is enabled on CPU2 side
			 *  with the command SHCI_C2_SetFlashActivityControl()
			 *
			 */
			cpu2_sem_status = LL_HSEM_1StepLock(HSEM,
				CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID);
			if (cpu2_sem_status == 0) {
				/**
				 * When CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID is
				 * taken, it is allowed to only erase one
				 * sector.
				 * When several sectors need to be erased,
				 * the application shall first exit from the
				 * critical section and try again.
				 */
				regs->CR |= FLASH_CR_PER;
				regs->CR &= ~FLASH_CR_PNB_Msk;
				regs->CR |= page << FLASH_CR_PNB_Pos;

				regs->CR |= FLASH_CR_STRT;

				/**
				 *  Release the semaphore to give the
				 *  opportunity to CPU2 to protect its timing
				 *  versus the next flash operation by taking
				 *  this semaphore.
				 *  Note that the CPU2 is polling on this
				 *  semaphore so CPU1 shall release it as fast
				 *  as possible.
				 *  This is why this code is protected by a
				 *  critical section.
				 */
				LL_HSEM_ReleaseLock(HSEM,
					CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID,
					0);
			}
		}

		/* Exit critical section */
		irq_unlock(key);

	} while (cpu2_sem_status || cpu1_sem_status);


	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	regs->CR &= ~FLASH_CR_PER;

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	int i, rc = 0;

#if defined(CONFIG_BT)
	/**
	 *  Notify the CPU2 that some flash erase activity may be executed
	 *  On reception of this command, the CPU2 enables the BLE timing
	 *  protection versus flash erase processing.
	 *  The Erase flash activity will be executed only when the BLE RF is
	 *  idle for at least 25ms.
	 *  The CPU2 will prevent all flash activity (write or erase) in all
	 *  cases when the BL RF Idle is shorter than 25ms.
	 */
	SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON);
#endif /* CONFIG_BT */

	i = get_page(offset);
	for (; i <= get_page(offset + len - 1) ; ++i) {
		rc = erase_page(dev, i);
		if (rc < 0) {
			break;
		}
	}

#if defined(CONFIG_BT)
	/**
	 *  Notify the CPU2 there will be no request anymore to erase the flash
	 *  On reception of this command, the CPU2 disables the BLE timing
	 *  protection versus flash erase processing
	 */
	SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);
#endif /* CONFIG_BT */

	return rc;
}

int flash_stm32_write_range(const struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;

	for (i = 0; i < len; i += 8, offset += 8U) {
		rc = write_dword(dev, offset,
				UNALIGNED_GET((const uint64_t *) data + (i >> 3)));
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout stm32wb_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (stm32wb_flash_layout.pages_count == 0) {
		stm32wb_flash_layout.pages_count = FLASH_SIZE / FLASH_PAGE_SIZE;
		stm32wb_flash_layout.pages_size = FLASH_PAGE_SIZE;
	}

	*layout = &stm32wb_flash_layout;
	*layout_size = 1;
}

int flash_stm32_check_status(const struct device *dev)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t error = 0;

	/* Save Flash errors */
	error = (regs->SR & FLASH_FLAG_SR_ERRORS);
	error |= (regs->ECCR & FLASH_FLAG_ECCC);

	/* Clear systematic Option and Enginneering bits validity error */
	if (error & FLASH_FLAG_OPTVERR) {
		regs->SR |= FLASH_FLAG_SR_ERRORS;
		return 0;
	}

	if (error) {
		return -EIO;
	}

	return 0;
}
