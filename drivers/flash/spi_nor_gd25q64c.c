#include <errno.h>
#include <drivers/flash.h>
#include <drivers/spi.h>
#include <string.h>
#include <logging/log.h>

#include "spi_nor.h"
#include "jesd216.h"
#include "flash_priv.h"
#include "spi_nor_priv.h"

LOG_MODULE_REGISTER(gd25q64c, CONFIG_FLASH_LOG_LEVEL);

#define GD25Q64C_CMD_PROGRAM_SECURITY_REGISTERS 0x42
#define GD25Q64C_CMD_ERASE_SECURITY_REGISTERS   0x44
#define GD25Q64C_CMD_READ_SECURITY_REGISTERS    0x48

/* Read Status Register, bytes 2:3 */
#define GD25Q64C_CMD_RDSRS2			0x35 // 8:15
#define GD25Q64C_CMD_RDSRS3			0x15 // 16:23

/* Write Status Register 2 */
#define GD25Q64C_CMD_WRSR2			0x31 // 8:15

#define GD25Q64C_OTP_PAGE_SIZE 256

uint32_t otp_idx_mask[3] = {
	0b00000000000000,
	0b10000000000000,
	0b11000000000000,
};

static uint32_t gd25q_otp_register_address(uint8_t idx, uint32_t offset)
{
	__ASSERT_NO_MSG(offset < 1024);
	__ASSERT_NO_MSG(idx < ARRAY_SIZE(otp_idx_mask));

	uint32_t otp_addr = otp_idx_mask[idx] | offset;
	if (otp_addr < offset) {
		LOG_ERR("offset = %u, otp_addr = 0x%X", offset, otp_addr);
		k_sleep(K_SECONDS(10));
	}
	return otp_addr;
}

int gd25q64c_read_otp_register(const struct device *dev, uint8_t reg_idx,
			      uint32_t addr, uint8_t *buf, size_t size)
{
	int ret;

	if (!dev || reg_idx > 2 || addr > 1024 || size > 1024 || addr + size > 1024) {
		return -EINVAL;
	}

	spi_nor_acquire_device(dev);
	spi_nor_wait_until_ready(dev);

	/* Got to use spi_nor_access instead of spi_nor_cmd_addr_read because
	 * this operation requires shifting in an additional dummy rx byte
	 */
	ret = spi_nor_access(dev, GD25Q64C_CMD_READ_SECURITY_REGISTERS, true,
		       gd25q_otp_register_address(reg_idx, addr), buf, size,
		       false, true);

	spi_nor_release_device(dev);
	return ret;
}

int gd25q64c_program_otp_register(const struct device *dev, uint8_t reg_idx,
				 size_t addr, uint8_t *data, size_t size)
{
	int ret = 0;
	size_t page_size = GD25Q64C_OTP_PAGE_SIZE;

	spi_nor_acquire_device(dev);

	while (size > 0) {
		size_t to_write = size;
		if (to_write > page_size) {
			to_write = page_size;
		}

		/* Don't write across a page boundary */
		if (((addr + to_write - 1U) / page_size)
		    != (addr / page_size)) {
			to_write = page_size - (addr % page_size);
		}

		spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);
		ret = spi_nor_cmd_addr_write(
			dev, GD25Q64C_CMD_PROGRAM_SECURITY_REGISTERS,
			gd25q_otp_register_address(reg_idx, addr),
			data, to_write);
		if (ret) {
			goto out;
		}

		size -= to_write;
		data += to_write;
		addr += to_write;

		spi_nor_wait_until_ready(dev);
	}

out:
	spi_nor_release_device(dev);
	return ret;
}

int gd25q64c_erase_otp_register(const struct device *dev, uint8_t reg_idx)
{
	int ret = 0;

	spi_nor_acquire_device(dev);
	spi_nor_wait_until_ready(dev);
	spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);

	ret = spi_nor_cmd_addr_write(
		dev, GD25Q64C_CMD_ERASE_SECURITY_REGISTERS,
		gd25q_otp_register_address(reg_idx, 0), NULL, 0);

	spi_nor_wait_until_ready(dev);
	spi_nor_release_device(dev);

	return ret;
}

int gd25q64c_lock_otp_register(const struct device *dev, uint8_t reg_idx)
{
	uint8_t reg;
	int ret;

	spi_nor_acquire_device(dev);
	spi_nor_wait_until_ready(dev);

	/* read status reg 2 */
	ret = spi_nor_cmd_read(dev, GD25Q64C_CMD_RDSRS2, &reg, 1);

	/* Mask read value with OTP protection bit */
	reg |= BIT(3 + reg_idx);

	/* Write enable */
	spi_nor_cmd_write(dev, SPI_NOR_CMD_WREN);

	/* Write new value */
	ret = spi_nor_cmd_write_data(dev, GD25Q64C_CMD_WRSR2, &reg, 1);

	spi_nor_wait_until_ready(dev);
	spi_nor_release_device(dev);
	return ret;
}
