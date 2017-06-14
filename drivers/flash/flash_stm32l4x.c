/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS
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

#define STM32L4X_BANK_SIZE_MAX	512
#define STM32L4X_PAGE_SHIFT	11

#define STM32L4X_FLASH_END \
	((u32_t)(STM32L4X_BANK_SIZE_MAX << STM32L4X_PAGE_SHIFT) - 1)

/* offset and len must be aligned on 8 for write
 * , positive and not beyond end of flash */
bool flash_stm32_valid_range(off_t offset, u32_t len, bool write)
{
	return (!write || (offset % 8 == 0 && len % 8 == 0)) &&
		offset >= 0 &&
		(offset + len - 1 <= STM32L4X_FLASH_END);
}

/* STM32L4xx devices can have up to 512 2K pages on two 256x2K pages banks */
static unsigned int get_page(off_t offset)
{
	return offset >> STM32L4X_PAGE_SHIFT;
}

static int write_dword(off_t offset, u64_t val, struct flash_stm32_priv *p)
{
	volatile u32_t *flash = (u32_t *)(offset + CONFIG_FLASH_BASE_ADDRESS);
	struct stm32l4x_flash *regs = p->regs;
	u32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->cr & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check that no Flash main memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(p);
	if (rc < 0) {
		return rc;
	}

	/* Check if this double word is erased */
	if (flash[0] != 0xFFFFFFFFUL ||
	    flash[1] != 0xFFFFFFFFUL) {
		return -EIO;
	}

	/* Set the PG bit */
	regs->cr |= FLASH_CR_PG;

	/* Flush the register write */
	tmp = regs->cr;

	/* Perform the data write operation at the desired memory address */
	flash[0] = (uint32_t)val;
	flash[1] = (uint32_t)(val >> 32);

	/* Wait until the BSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(p);

	/* Clear the PG bit */
	regs->cr &= (~FLASH_CR_PG);

	return rc;
}

static int erase_page(unsigned int page, struct flash_stm32_priv *p)
{
	struct stm32l4x_flash *regs = p->regs;
	u32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->cr & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(p);
	if (rc < 0) {
		return rc;
	}

	/* Set the PER bit and select the page you wish to erase */
	regs->cr |= FLASH_CR_PER;
#ifdef FLASH_CR_BKER
	regs->cr &= ~FLASH_CR_BKER_Msk;
	/* Select bank, only for DUALBANK devices */
	if (page >= 256)
		regs->cr |= FLASH_CR_BKER;
#endif
	regs->cr &= ~FLASH_CR_PNB_Msk;
	regs->cr |= ((page % 256) << 3);

	/* Set the STRT bit */
	regs->cr |= FLASH_CR_STRT;

	/* flush the register write */
	tmp = regs->cr;

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(p);

	regs->cr &= ~FLASH_CR_PER;

	return rc;
}

int flash_stm32_block_erase_loop(unsigned int offset, unsigned int len,
				 struct flash_stm32_priv *p)
{
	int i, rc = 0;

	i = get_page(offset);
	for (; i <= get_page(offset + len - 1) ; ++i) {
		rc = erase_page(i, p);
		if (rc < 0) {
			break;
		}
	}

	return rc;
}

int flash_stm32_write_range(unsigned int offset, const void *data,
			    unsigned int len, struct flash_stm32_priv *p)
{
	int i, rc = 0;

	for (i = 0; i < len; i += 8, offset += 8) {
		rc = write_dword(offset, ((const u64_t *) data)[i>>3], p);
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}
