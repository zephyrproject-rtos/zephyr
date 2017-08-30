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

#include <flash_stm32.h>

bool flash_stm32_valid_range(struct device *dev, off_t offset, u32_t len,
			     bool write)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(write);
	return offset >= 0 && (offset + len - 1 <= STM32F4X_FLASH_END);
}

static int write_byte(struct device *dev, off_t offset, u8_t val)
{
	struct stm32f4x_flash *regs = FLASH_STM32_REGS(dev);
	u32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->cr & FLASH_CR_LOCK) {
		return -EIO;
	}

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	regs->cr &= ~CR_PSIZE_MASK;
	regs->cr |= FLASH_PSIZE_BYTE;
	regs->cr |= FLASH_CR_PG;

	/* flush the register write */
	tmp = regs->cr;

	*((u8_t *) offset + CONFIG_FLASH_BASE_ADDRESS) = val;

	rc = flash_stm32_wait_flash_idle(dev);
	regs->cr &= (~FLASH_CR_PG);

	return rc;
}

static int erase_sector(struct device *dev, u16_t sector)
{
	struct stm32f4x_flash *regs = FLASH_STM32_REGS(dev);
	u32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->cr & FLASH_CR_LOCK) {
		return -EIO;
	}

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	regs->cr &= STM32F4X_SECTOR_MASK;
	regs->cr |= FLASH_CR_SER | (sector << 3);
	regs->cr |= FLASH_CR_STRT;

	/* flush the register write */
	tmp = regs->cr;

	rc = flash_stm32_wait_flash_idle(dev);
	regs->cr &= ~(FLASH_CR_SER | FLASH_CR_SNB);

	return rc;
}

int flash_stm32_block_erase_loop(struct device *dev, unsigned int offset,
				 unsigned int len)
{
	int i, rc = 0;

	i = stm32f4x_get_sector(offset);
	for (; i <= stm32f4x_get_sector(offset + len - 1); i++) {
		rc = erase_sector(dev, i);
		if (rc < 0) {
			break;
		}
	}

	return rc;
}


int flash_stm32_write_range(struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;

	for (i = 0; i < len; i++, offset++) {
		rc = write_byte(dev, offset, ((const u8_t *) data)[i]);
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}
