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
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

static int nvmem_dev_read(const struct device *dev, off_t off, void *buf, size_t len)
{
	if (IS_ENABLED(CONFIG_NVMEM_BBRAM) && DEVICE_API_IS(bbram, dev)) {
		return bbram_read(dev, off, len, buf);
	}

	if (IS_ENABLED(CONFIG_NVMEM_EEPROM) && DEVICE_API_IS(eeprom, dev)) {
		return eeprom_read(dev, off, buf, len);
	}

	if (IS_ENABLED(CONFIG_NVMEM_FLASH) && DEVICE_API_IS(flash, dev)) {
		return flash_read(dev, off, buf, len);
	}

	if (IS_ENABLED(CONFIG_NVMEM_OTP) && DEVICE_API_IS(otp, dev)) {
		return otp_read(dev, off, buf, len);
	}

	return -ENXIO;
}

static int nvmem_dev_write(const struct device *dev, off_t off, const void *buf, size_t len)
{
	if (IS_ENABLED(CONFIG_NVMEM_BBRAM) && DEVICE_API_IS(bbram, dev)) {
		return bbram_write(dev, off, len, buf);
	}

	if (IS_ENABLED(CONFIG_NVMEM_EEPROM) && DEVICE_API_IS(eeprom, dev)) {
		return eeprom_write(dev, off, buf, len);
	}

	if (IS_ENABLED(CONFIG_NVMEM_FLASH_WRITE) && DEVICE_API_IS(flash, dev)) {
		return flash_write(dev, off, buf, len);
	}

	if (IS_ENABLED(CONFIG_NVMEM_OTP_WRITE) && DEVICE_API_IS(otp, dev)) {
		return otp_program(dev, off, buf, len);
	}

	return -ENXIO;
}

/*
 * Byte i of a reversed cell is stored at offset size - 1 - i, the range
 * [off, off + len) is stored mirrored in [size - off - len, size - off).
 */
static int nvmem_dev_write_reversed(const struct device *dev, off_t off, const uint8_t *buf,
				    size_t len)
{
	uint8_t tmp[CONFIG_NVMEM_REVERSED_WRITE_BUF_SIZE];
	int ret;

	while (len > 0) {
		size_t chunk = MIN(len, sizeof(tmp));

		sys_memcpy_swap(tmp, &buf[len - chunk], chunk);

		ret = nvmem_dev_write(dev, off, tmp, chunk);
		if (ret != 0) {
			return ret;
		}

		off += chunk;
		len -= chunk;
	}

	return 0;
}

int nvmem_cell_read(const struct nvmem_cell *cell, void *buf, off_t off, size_t len)
{
	int ret;

	__ASSERT_NO_MSG(cell != NULL);

	if (off < 0 || cell->size < off + len) {
		return -EINVAL;
	}

	if (!device_is_ready(cell->dev)) {
		return -ENODEV;
	}

	if ((cell->flags & NVMEM_CELL_FLAG_REVERSED) == 0U) {
		return nvmem_dev_read(cell->dev, cell->offset + off, buf, len);
	}

	ret = nvmem_dev_read(cell->dev, cell->offset + cell->size - off - len, buf, len);
	if (ret == 0) {
		sys_mem_swap(buf, len);
	}

	return ret;
}

int nvmem_cell_write(const struct nvmem_cell *cell, const void *buf, off_t off, size_t len)
{
	__ASSERT_NO_MSG(cell != NULL);

	if (off < 0 || cell->size < off + len) {
		return -EINVAL;
	}

	if (nvmem_cell_is_read_only(cell)) {
		return -EROFS;
	}

	if (!device_is_ready(cell->dev)) {
		return -ENODEV;
	}

	if ((cell->flags & NVMEM_CELL_FLAG_REVERSED) != 0U) {
		return nvmem_dev_write_reversed(cell->dev, cell->offset + cell->size - off - len,
						buf, len);
	}

	return nvmem_dev_write(cell->dev, cell->offset + off, buf, len);
}
