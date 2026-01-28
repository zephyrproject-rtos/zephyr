/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/nvmem.h>
#include <zephyr/sys/__assert.h>

#ifdef CONFIG_NVMEM_MMIO

#include <zephyr/sys/device_mmio.h>

static int nvmem_mmio_read(const struct nvmem_cell *cell, void *buf, off_t off, size_t len)
{
	mm_reg_t addr;

	device_map(&addr, cell->phys_addr + cell->offset, cell->size, K_MEM_CACHE_NONE);
	memcpy(buf, (const void *)(addr + off), len);
	device_unmap(addr, cell->size);

	return 0;
}

static int nvmem_mmio_write(const struct nvmem_cell *cell, const void *buf, off_t off, size_t len)
{
	mm_reg_t addr;

	device_map(&addr, cell->phys_addr + cell->offset, cell->size, K_MEM_CACHE_NONE);
	memcpy((void *)(addr + off), buf, len);
	device_unmap(addr, cell->size);

	return 0;
}

#else

#define nvmem_mmio_read(...) -ENOTSUP
#define nvmem_mmio_write(...) -ENOTSUP

#endif

int nvmem_cell_read(const struct nvmem_cell *cell, void *buf, off_t off, size_t len)
{
	__ASSERT_NO_MSG(cell != NULL);

	if (off < 0 || cell->size < off + len) {
		return -EINVAL;
	}

	if (cell->type == NVMEM_CELL_TYPE_MMIO) {
		return nvmem_mmio_read(cell, buf, off, len);
	}

	if (cell->type == NVMEM_CELL_TYPE_DEVICE) {
		if (!device_is_ready(cell->dev)) {
			return -ENODEV;
		}

		if (IS_ENABLED(CONFIG_NVMEM_BBRAM) && DEVICE_API_IS(bbram, cell->dev)) {
			return bbram_read(cell->dev, cell->offset + off, len, buf);
		}

		if (IS_ENABLED(CONFIG_NVMEM_EEPROM) && DEVICE_API_IS(eeprom, cell->dev)) {
			return eeprom_read(cell->dev, cell->offset + off, buf, len);
		}

		if (IS_ENABLED(CONFIG_NVMEM_FLASH) && DEVICE_API_IS(flash, cell->dev)) {
			return flash_read(cell->dev, cell->offset + off, buf, len);
		}

		if (IS_ENABLED(CONFIG_NVMEM_OTP) && DEVICE_API_IS(otp, cell->dev)) {
			return otp_read(cell->dev, cell->offset + off, buf, len);
		}
	}

	return -ENXIO;
}

int nvmem_cell_write(const struct nvmem_cell *cell, const void *buf, off_t off, size_t len)
{
	__ASSERT_NO_MSG(cell != NULL);

	if (off < 0 || cell->size < off + len) {
		return -EINVAL;
	}

	if (cell->read_only) {
		return -EROFS;
	}

	if (cell->type == NVMEM_CELL_TYPE_MMIO) {
		return nvmem_mmio_write(cell, buf, off, len);
	}

	if (cell->type == NVMEM_CELL_TYPE_DEVICE) {
		if (!device_is_ready(cell->dev)) {
			return -ENODEV;
		}

		if (IS_ENABLED(CONFIG_NVMEM_BBRAM) && DEVICE_API_IS(bbram, cell->dev)) {
			return bbram_write(cell->dev, cell->offset + off, len, buf);
		}

		if (IS_ENABLED(CONFIG_NVMEM_EEPROM) && DEVICE_API_IS(eeprom, cell->dev)) {
			return eeprom_write(cell->dev, cell->offset + off, buf, len);
		}

		if (IS_ENABLED(CONFIG_NVMEM_FLASH_WRITE) && DEVICE_API_IS(flash, cell->dev)) {
			return flash_write(cell->dev, cell->offset + off, buf, len);
		}

		if (IS_ENABLED(CONFIG_NVMEM_OTP_WRITE) && DEVICE_API_IS(otp, cell->dev)) {
			return otp_program(cell->dev, cell->offset + off, buf, len);
		}
	}

	return -ENXIO;
}
