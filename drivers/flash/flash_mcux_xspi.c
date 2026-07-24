/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_xspi_nor

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include "memc_mcux_xspi.h"
#include "spi_nor.h"

LOG_MODULE_REGISTER(flash_mcux_xspi);

#define FLASH_MCUX_XSPI_HAS_RESET_GPIOS DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define FLASH_MCUX_XSPI_HAS_SOFT_RESET  DT_ANY_INST_HAS_BOOL_STATUS_OKAY(initial_soft_reset)

#if FLASH_MCUX_XSPI_HAS_RESET_GPIOS
#include <zephyr/drivers/gpio.h>
#endif

#define FLASH_MCUX_XSPI_HAS_RESET \
	(FLASH_MCUX_XSPI_HAS_RESET_GPIOS || FLASH_MCUX_XSPI_HAS_SOFT_RESET)

#ifdef CONFIG_FLASH_MCUX_XSPI_WRITE_BUFFER
static uint8_t flash_xspi_write_buf[SPI_NOR_PAGE_SIZE];
#endif

#define FLASH_MCUX_XSPI_LUT_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0][0]))

#define FLASH_BUSY_STATUS_OFFSET 0

/*
 * Unified LUT sequence layout shared by every supported chip.
 *
 * FLASH_CMD_MEM_READ MUST stay at index 0: the XSPI controller AHB read descriptor is wired
 * to sequence index 0 (memc_mcux_xspi.c, ARDSeqIndex = 0).
 */
enum {
	FLASH_CMD_MEM_READ = 0,
	FLASH_CMD_READ_STATUS,
	FLASH_CMD_WRITE_ENABLE,
	FLASH_CMD_PAGE_PROGRAM,
	FLASH_CMD_ERASE_SECTOR,
	FLASH_CMD_READ_ID,
	FLASH_CMD_MODE_ENABLE,
	FLASH_CMD_ENTER_4BYTE,
	FLASH_CMD_RESET_ENABLE,
	FLASH_CMD_RESET_MEMORY,
	FLASH_CMD_SEQ_MAX
};

struct flash_xspi_device {
	struct memc_xspi_dev_config memc;
	bool mode_enable;
	/* Operand written by the MODE_ENABLE sequence (chip-specific register). */
	uint8_t mode_enable_value;
	bool addr_4byte;
	uint8_t status_read_size;
	uint16_t reset_assert_us;
	uint32_t reset_recovery_us;
};

struct flash_mcux_xspi_config {
	bool enable_differential_clk;
	xspi_sample_clk_config_t sample_clk_config;
#if FLASH_MCUX_XSPI_HAS_RESET_GPIOS
	struct gpio_dt_spec reset_gpio;
#endif
#if FLASH_MCUX_XSPI_HAS_SOFT_RESET
	bool initial_soft_reset;
#endif
};

struct flash_mcux_xspi_data {
	xspi_config_t xspi_config;
	const struct device *xspi_dev;
	const char *dev_name;
	uint32_t amba_address;
	const struct flash_xspi_device *chip;
	struct flash_parameters flash_param;
	uint64_t flash_size;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

/*
 * Errata ERR052528: Limitation on LUT-Data Size < 8byte in xspi.
 * Description: Read command including RDSR command can't work if LUT data size in read status is
 * less than 8. Workaround: Use LUT data size of minimum 8 byte for read commands including RDSR.
 */
static const uint32_t flash_xspi_lut_mx25um[][5] = {
	[FLASH_CMD_MEM_READ] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0xEE, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0x11),
			XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20,
					 kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x12),
			XSPI_LUT_SEQ(kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x2,
					 kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x8),
			XSPI_LUT_SEQ(kXSPI_Command_STOP, kXSPI_8PAD, 0x0, 0, 0, 0),
		},
	[FLASH_CMD_READ_STATUS] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x05, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0xFA),
			XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20,
					 kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x12),
			XSPI_LUT_SEQ(kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x2,
					 kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x8),
			XSPI_LUT_SEQ(kXSPI_Command_STOP, kXSPI_8PAD, 0x0, 0, 0, 0),
		},
	[FLASH_CMD_WRITE_ENABLE] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x06, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0xF9),
		},
	[FLASH_CMD_PAGE_PROGRAM] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x12, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0xED),
			XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20,
					 kXSPI_Command_WRITE_DDR, kXSPI_8PAD, 0x8),
		},
	[FLASH_CMD_ERASE_SECTOR] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x21, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0xDE),
			XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20, kXSPI_Command_STOP,
					 kXSPI_8PAD, 0x0),
		},
	[FLASH_CMD_READ_ID] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x9F, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0x60),
			XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20,
					 kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x04),
			XSPI_LUT_SEQ(kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x08, kXSPI_Command_STOP,
					 kXSPI_1PAD, 0x0),
		},
	[FLASH_CMD_MODE_ENABLE] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x72, kXSPI_Command_SDR,
					 kXSPI_1PAD, 0x00),
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x00, kXSPI_Command_SDR,
					 kXSPI_1PAD, 0x00),
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x00, kXSPI_Command_WRITE_SDR,
					 kXSPI_1PAD, 0x01),
		},
	[FLASH_CMD_RESET_ENABLE] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, SPI_NOR_CMD_RESET_EN,
					 kXSPI_Command_STOP, kXSPI_1PAD, 0x00),
		},
	[FLASH_CMD_RESET_MEMORY] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, SPI_NOR_CMD_RESET_MEM,
					 kXSPI_Command_STOP, kXSPI_1PAD, 0x00),
		},
};

static const uint32_t flash_xspi_lut_w25q512nw[][5] = {
	[FLASH_CMD_MEM_READ] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0xEC,
					 kXSPI_Command_RADDR_SDR, kXSPI_4PAD, 0x20),
			XSPI_LUT_SEQ(kXSPI_Command_MODE_SDR, kXSPI_4PAD, 0x00,
					 kXSPI_Command_DUMMY_SDR, kXSPI_4PAD, 0x04),
			XSPI_LUT_SEQ(kXSPI_Command_READ_SDR, kXSPI_4PAD, 0x04,
					 kXSPI_Command_STOP, kXSPI_1PAD, 0x00),
		},
	/* Read Status Register-1 (0x05). */
	[FLASH_CMD_READ_STATUS] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x05,
					 kXSPI_Command_READ_SDR, kXSPI_1PAD, 0x08),
		},
	[FLASH_CMD_WRITE_ENABLE] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x06,
					 kXSPI_Command_STOP, kXSPI_1PAD, 0x00),
		},
	/* 4-byte quad input page program (0x34, 1-1-4) with 32-bit address. */
	[FLASH_CMD_PAGE_PROGRAM] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x34,
					 kXSPI_Command_RADDR_SDR, kXSPI_1PAD, 0x20),
			XSPI_LUT_SEQ(kXSPI_Command_WRITE_SDR, kXSPI_4PAD, 0x04,
					 kXSPI_Command_STOP, kXSPI_1PAD, 0x00),
		},
	/* 4KB sector erase (0x20) with 32-bit address (4-byte mode entered at init). */
	[FLASH_CMD_ERASE_SECTOR] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x20,
					 kXSPI_Command_RADDR_SDR, kXSPI_1PAD, 0x20),
			XSPI_LUT_SEQ(kXSPI_Command_STOP, kXSPI_1PAD, 0x00, 0, 0, 0),
		},
	[FLASH_CMD_READ_ID] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x9F,
					 kXSPI_Command_READ_SDR, kXSPI_1PAD, 0x08),
		},
	/* Write Status Register-2 (0x31) — sets QE (bit 1). */
	[FLASH_CMD_MODE_ENABLE] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x31,
					 kXSPI_Command_WRITE_SDR, kXSPI_1PAD, 0x01),
		},
	[FLASH_CMD_ENTER_4BYTE] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0xB7,
					 kXSPI_Command_STOP, kXSPI_1PAD, 0x00),
		},
	[FLASH_CMD_RESET_ENABLE] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, SPI_NOR_CMD_RESET_EN,
					 kXSPI_Command_STOP, kXSPI_1PAD, 0x00),
		},
	[FLASH_CMD_RESET_MEMORY] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, SPI_NOR_CMD_RESET_MEM,
					 kXSPI_Command_STOP, kXSPI_1PAD, 0x00),
		},
};

static struct flash_xspi_device device_configs[] = {
	{
		.memc = {
			.name_prefix = "mx25um51345g",
			.xspi_dev_config = {
				.deviceInterface = kXSPI_StrandardExtendedSPI,
				.interfaceSettings.strandardExtendedSPISettings.pageSize = 256,
				.CSHoldTime = 2,
				.CSSetupTime = 2,
				.addrMode = kXSPI_DeviceByteAddressable,
				.columnAddrWidth = 0,
				.enableCASInterleaving = false,
				.ptrDeviceDdrConfig =
					&(xspi_device_ddr_config_t){
						.ddrDataAlignedClk =
							kXSPI_DDRDataAlignedWith2xInternalRefClk,
						.enableByteSwapInOctalMode = false,
						.enableDdr = true,
					},
				.deviceSize = {64 * 1024, 64 * 1024},
			},
			.lut_array = &flash_xspi_lut_mx25um[0][0],
			.lut_count = FLASH_MCUX_XSPI_LUT_ARRAY_SIZE(flash_xspi_lut_mx25um),
		},
		.mode_enable = true,
		.mode_enable_value = 0x02, /* CR2: select DTR-OPI. */
		.addr_4byte = false,
		.status_read_size = 2,
		.reset_assert_us = 10, /* tRLRH: Reset# low pulse width. */
		.reset_recovery_us = 35, /* tREADY1: reset recovery time from standby. */
	},
	{
		.memc = {
			.name_prefix = "w25q512nw",
			.xspi_dev_config = {
				.deviceInterface = kXSPI_StrandardExtendedSPI,
				.interfaceSettings.strandardExtendedSPISettings.pageSize = 256,
				.CSHoldTime = 3,
				.CSSetupTime = 3,
				.addrMode = kXSPI_DeviceByteAddressable,
				.columnAddrWidth = 0,
				.enableCASInterleaving = false,
				.ptrDeviceDdrConfig = NULL,
				.deviceSize = {64 * 1024, 0},
			},
			.lut_array = &flash_xspi_lut_w25q512nw[0][0],
			.lut_count = FLASH_MCUX_XSPI_LUT_ARRAY_SIZE(flash_xspi_lut_w25q512nw),
		},
		.mode_enable = true,
		.mode_enable_value = 0x02, /* SR2: set QE (bit 1) */
		.addr_4byte = true,
		.status_read_size = 1,
		.reset_assert_us = 0,
		.reset_recovery_us = 30, /* Soft reset tRST=30us. Not support hardware reset. */
	},
};

static int flash_xspi_nor_wait_bus_busy(const struct device *dev)
{
	struct flash_mcux_xspi_data *devData = dev->data;
	const struct device *xspi_dev = devData->xspi_dev;
	xspi_transfer_t flashXfer = {0};
	uint32_t readValue;
	bool isBusy;
	int ret;

	flashXfer.deviceAddress = devData->amba_address;
	flashXfer.cmdType = kXSPI_Read;
	flashXfer.data = &readValue;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.dataSize = devData->chip->status_read_size;
	flashXfer.seqIndex = FLASH_CMD_READ_STATUS;
	flashXfer.lockArbitration = false;

	do {
		ret = memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
		if (ret < 0) {
			break;
		}

		isBusy = (readValue & (1U << FLASH_BUSY_STATUS_OFFSET)) ? true : false;
	} while (isBusy);

	return ret;
}

static int flash_mcux_xspi_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_mcux_xspi_data *devData = dev->data;
	uint8_t *src = (uint8_t *)devData->amba_address + offset;

	if (len == 0) {
		return 0;
	}

	if (!data) {
		return -EINVAL;
	}

	if ((offset < 0) || (offset >= devData->flash_size) ||
		((devData->flash_size - offset) < len)) {
		return -EINVAL;
	}

	XSPI_Cache64_InvalidateCacheByRange((uint32_t)src, len);

	(void)memcpy(data, src, len);

	return 0;
}

static int flash_mcux_xspi_write_enable(const struct device *dev)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	xspi_transfer_t flashXfer = {0};

	flashXfer.deviceAddress = data->amba_address;
	flashXfer.cmdType = kXSPI_Command;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.data = NULL;
	flashXfer.dataSize = 0;
	flashXfer.lockArbitration = false;
	flashXfer.seqIndex = FLASH_CMD_WRITE_ENABLE;

	return memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
}

static int flash_mcux_xspi_write(const struct device *dev, off_t offset, const void *data,
				 size_t len)
{
	struct flash_mcux_xspi_data *devData = dev->data;
	const struct device *xspi_dev = devData->xspi_dev;
	uint8_t *p_data = (uint8_t *)(uintptr_t)data;
	xspi_transfer_t flashXfer = {0};
	size_t write_size;
	uint32_t key = 0;
	int ret = 0;

#if !defined(CONFIG_FLASH_MCUX_XSPI_WRITE_BUFFER)
	/* Without write buffer, offset and len must be 4-byte aligned.
	 * XSPI IP TX FIFO only transfers full 32-bit words, and a
	 * non-4-aligned offset causes the page boundary split to
	 * produce non-4-aligned chunk sizes.
	 */
	if ((len & 3U) || (offset & 3U)) {
		return -EINVAL;
	}
#endif

	if (memc_xspi_is_running_xip(xspi_dev)) {
		key = irq_lock();
		memc_xspi_wait_bus_idle(xspi_dev);
	}

	while (len > 0) {
		write_size = MIN(len, SPI_NOR_PAGE_SIZE - (offset % SPI_NOR_PAGE_SIZE));

		ret = flash_mcux_xspi_write_enable(dev);
		if (ret < 0) {
			break;
		}

		flashXfer.deviceAddress = devData->amba_address + offset;
		flashXfer.cmdType = kXSPI_Write;
		flashXfer.seqIndex = FLASH_CMD_PAGE_PROGRAM;
		flashXfer.targetGroup = kXSPI_TargetGroup0;
#ifdef CONFIG_FLASH_MCUX_XSPI_WRITE_BUFFER
		/*
		 * Copy to RAM buffer to prevent faults when source
		 * data resides in flash, and pad to 4-byte boundary
		 * because XSPI IP TX FIFO only transfers full words.
		 */
		size_t aligned_size = ROUND_UP(write_size, 4);

		memcpy(flash_xspi_write_buf, p_data, write_size);
		if (aligned_size != write_size) {
			memset(flash_xspi_write_buf + write_size, 0xFF,
			       aligned_size - write_size);
		}
		flashXfer.data = (uint32_t *)flash_xspi_write_buf;
		flashXfer.dataSize = aligned_size;
#else
		flashXfer.data = (uint32_t *)p_data;
		flashXfer.dataSize = write_size;
#endif
		flashXfer.lockArbitration = false;

		ret = memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
		if (ret < 0) {
			break;
		}

		ret = flash_xspi_nor_wait_bus_busy(dev);
		if (ret < 0) {
			break;
		}

		len -= write_size;
		p_data = p_data + write_size;
		offset += write_size;
	}

	/*
	 * Invalidate the controller's AHB read/prefetch buffer so a memory-mapped
	 * read issued right after this program returns the written data instead of
	 * stale data left in the prefetch buffer.
	 */
	memc_xspi_reset(xspi_dev);

	if (memc_xspi_is_running_xip(xspi_dev)) {
		irq_unlock(key);
	}

	return ret;
}

static int flash_mcux_xspi_erase_sector(const struct device *dev, off_t offset)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	xspi_transfer_t flashXfer = {0};

	flashXfer.deviceAddress = data->amba_address + offset;
	flashXfer.cmdType = kXSPI_Command;
	flashXfer.seqIndex = FLASH_CMD_ERASE_SECTOR;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.lockArbitration = false;
	flashXfer.dataSize = 0;
	flashXfer.data = NULL;

	return memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
}

static int flash_mcux_xspi_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	uint32_t key = 0;
	int ret = 0;

	if (0 != (offset % SPI_NOR_SECTOR_SIZE)) {
		LOG_ERR("Invalid offset");
		return -EINVAL;
	}

	if (size % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	if (memc_xspi_is_running_xip(xspi_dev)) {
		key = irq_lock();
		memc_xspi_wait_bus_idle(xspi_dev);
	}

	for (int i = 0; i < size / SPI_NOR_SECTOR_SIZE; i++) {
		ret = flash_mcux_xspi_write_enable(dev);
		if (ret < 0) {
			break;
		}

		ret = flash_mcux_xspi_erase_sector(dev, offset + i * SPI_NOR_SECTOR_SIZE);
		if (ret < 0) {
			break;
		}

		ret = flash_xspi_nor_wait_bus_busy(dev);
		if (ret < 0) {
			break;
		}
	}

	/*
	 * Invalidate the controller's AHB read/prefetch buffer so a memory-mapped
	 * read issued right after this erase returns the erased data instead of
	 * stale data left in the prefetch buffer.
	 */
	memc_xspi_reset(xspi_dev);

	if (memc_xspi_is_running_xip(xspi_dev)) {
		irq_unlock(key);
	}

	return ret;
}

static int flash_mcux_xspi_mode_enable(const struct device *dev)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	uint32_t value = data->chip->mode_enable_value;
	xspi_transfer_t flashXfer = {0};
	int ret;

	ret = flash_mcux_xspi_write_enable(dev);
	if (ret < 0) {
		return ret;
	}

	flashXfer.deviceAddress = data->amba_address;
	flashXfer.cmdType = kXSPI_Write;
	flashXfer.seqIndex = FLASH_CMD_MODE_ENABLE;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.data = &value;
	flashXfer.dataSize = 1;
	flashXfer.lockArbitration = false;

	ret = memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
	if (ret < 0) {
		return ret;
	}

	return flash_xspi_nor_wait_bus_busy(dev);
}

static int flash_mcux_xspi_enter_4byte(const struct device *dev)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	xspi_transfer_t flashXfer = {0};

	flashXfer.deviceAddress = data->amba_address;
	flashXfer.cmdType = kXSPI_Command;
	flashXfer.seqIndex = FLASH_CMD_ENTER_4BYTE;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.data = NULL;
	flashXfer.dataSize = 0;
	flashXfer.lockArbitration = false;

	return memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
}

static int flash_mcux_xspi_configure_mode(const struct device *dev)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct flash_xspi_device *chip = data->chip;
	int ret;

	if (chip->mode_enable) {
		ret = flash_mcux_xspi_mode_enable(dev);
		if (ret < 0) {
			return ret;
		}
	}

	if (chip->addr_4byte) {
		ret = flash_mcux_xspi_enter_4byte(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static const struct flash_parameters *flash_mcux_xspi_get_parameters(const struct device *dev)
{
	return &((const struct flash_mcux_xspi_data *)dev->data)->flash_param;
}

static int flash_mcux_xspi_get_size(const struct device *dev, uint64_t *size)
{
	*size = ((const struct flash_mcux_xspi_data *)dev->data)->flash_size;

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_mcux_xspi_pages_layout(const struct device *dev,
					 const struct flash_pages_layout **layout,
					 size_t *layout_size)
{
	struct flash_mcux_xspi_data *data = dev->data;

	*layout = &data->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if defined(CONFIG_FLASH_JESD216_API)
static int flash_mcux_xspi_sfdp_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	return -EOPNOTSUPP;
}

static int flash_mcux_xspi_read_jedec_id(const struct device *dev, uint8_t *id)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	xspi_transfer_t flashXfer = {0};

	flashXfer.deviceAddress = data->amba_address;
	flashXfer.cmdType = kXSPI_Read;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.seqIndex = FLASH_CMD_READ_ID;
	flashXfer.data = (uint32_t *)id;
	flashXfer.dataSize = SPI_NOR_MAX_ID_LEN;
	flashXfer.lockArbitration = false;

	return memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
}
#endif /* CONFIG_FLASH_JESD216_API */

#if FLASH_MCUX_XSPI_HAS_RESET
#if FLASH_MCUX_XSPI_HAS_SOFT_RESET
static int flash_mcux_xspi_soft_reset(const struct device *dev)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	xspi_transfer_t flashXfer = {0};
	int ret;

	flashXfer.deviceAddress = data->amba_address;
	flashXfer.cmdType = kXSPI_Command;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.data = NULL;
	flashXfer.dataSize = 0;
	flashXfer.lockArbitration = false;

	flashXfer.seqIndex = FLASH_CMD_RESET_ENABLE;
	ret = memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
	if (ret < 0) {
		return ret;
	}

	flashXfer.seqIndex = FLASH_CMD_RESET_MEMORY;
	return memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
}
#endif /* FLASH_MCUX_XSPI_HAS_SOFT_RESET */

#if FLASH_MCUX_XSPI_HAS_RESET_GPIOS
static int flash_mcux_xspi_hw_reset(const struct device *dev)
{
	const struct flash_mcux_xspi_config *config = dev->config;
	const struct flash_xspi_device *chip = ((struct flash_mcux_xspi_data *)dev->data)->chip;
	int ret;

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}

	/* Drive RESET# to its asserted (active) level. */
	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	if (chip->reset_assert_us != 0) {
		k_busy_wait(chip->reset_assert_us);
	}

	/* Deassert RESET#. */
	return gpio_pin_set_dt(&config->reset_gpio, 0);
}
#endif /* FLASH_MCUX_XSPI_HAS_RESET_GPIOS */

static int flash_mcux_xspi_reset(const struct device *dev)
{
	const struct flash_mcux_xspi_config *config = dev->config;
	const struct flash_xspi_device *chip = ((struct flash_mcux_xspi_data *)dev->data)->chip;
	bool did_reset = false;
	int ret;

#if FLASH_MCUX_XSPI_HAS_RESET_GPIOS
	if (config->reset_gpio.port != NULL) {
		ret = flash_mcux_xspi_hw_reset(dev);
		if (ret < 0) {
			return ret;
		}
		did_reset = true;
	}
#endif

#if FLASH_MCUX_XSPI_HAS_SOFT_RESET
	if (config->initial_soft_reset) {
		ret = flash_mcux_xspi_soft_reset(dev);
		if (ret < 0) {
			return ret;
		}
		did_reset = true;
	}
#endif

	if (did_reset && chip->reset_recovery_us != 0) {
		k_busy_wait(chip->reset_recovery_us);
	}

	return 0;
}
#endif /* FLASH_MCUX_XSPI_HAS_RESET */

static int flash_mcux_xspi_probe(const struct device *dev)
{
	const struct flash_mcux_xspi_config *flash_config =
		(const struct flash_mcux_xspi_config *)dev->config;
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	struct flash_xspi_device *flash_dev_config = NULL;
	xspi_device_config_t *dev_config = NULL;
	uint32_t key = 0;
	int ret;

	/* Setup the specific flash parameters. */
	for (uint32_t i = 0; i < ARRAY_SIZE(device_configs); i++) {
		if (strncmp(device_configs[i].memc.name_prefix, data->dev_name,
				strlen(device_configs[i].memc.name_prefix)) == 0) {
			flash_dev_config = &device_configs[i];
			break;
		}
	}

	do {
		if (flash_dev_config == NULL) {
			LOG_ERR("Unsupported device: %s", data->dev_name);
			ret = -ENOTSUP;
			break;
		}

		data->chip = flash_dev_config;

		/* Set special device configurations. */
		dev_config = &flash_dev_config->memc.xspi_dev_config;
		dev_config->enableCknPad = flash_config->enable_differential_clk;
		dev_config->sampleClkConfig = flash_config->sample_clk_config;

		ret = memc_mcux_xspi_get_root_clock(xspi_dev, &dev_config->xspiRootClk);
		if (ret < 0) {
			break;
		}

		data->amba_address = memc_mcux_xspi_get_ahb_address(xspi_dev);

		/* Block potential AHB access while (re)configuring the XSPI under XIP. */
		if (memc_xspi_is_running_xip(xspi_dev)) {
			key = irq_lock();
			memc_xspi_wait_bus_idle(xspi_dev);
		}

		ret = memc_xspi_set_device_config(xspi_dev, dev_config,
						  flash_dev_config->memc.lut_array,
						  flash_dev_config->memc.lut_count);
#if FLASH_MCUX_XSPI_HAS_RESET
		if (ret == 0 && !memc_xspi_is_running_xip(xspi_dev)) {
			ret = flash_mcux_xspi_reset(dev);
		}
#endif
		if (ret == 0) {
			ret = flash_mcux_xspi_configure_mode(dev);
		}

		if (memc_xspi_is_running_xip(xspi_dev)) {
			irq_unlock(key);
		}
	} while (0);

	return ret;
}

static int flash_mcux_xspi_init(const struct device *dev)
{
	const struct device *xspi_dev = ((struct flash_mcux_xspi_data *)dev->data)->xspi_dev;

	if (!device_is_ready(xspi_dev)) {
		LOG_ERR("XSPI device is not ready");
		return -ENODEV;
	}

	return flash_mcux_xspi_probe(dev);
}

static DEVICE_API(flash, flash_mcux_xspi_api) = {
	.read = flash_mcux_xspi_read,
	.write = flash_mcux_xspi_write,
	.erase = flash_mcux_xspi_erase,
	.get_parameters = flash_mcux_xspi_get_parameters,
	.get_size = flash_mcux_xspi_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mcux_xspi_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = flash_mcux_xspi_sfdp_read,
	.read_jedec_id = flash_mcux_xspi_read_jedec_id,
#endif
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
#define FLASH_MCUX_XSPI_LAYOUT(n)	\
	.layout.pages_size = SPI_NOR_SECTOR_SIZE,	\
	.layout.pages_count = DT_INST_PROP(n, size) / SPI_NOR_SECTOR_SIZE,
#else
#define FLASH_MCUX_XSPI_LAYOUT(n)
#endif

#if FLASH_MCUX_XSPI_HAS_RESET_GPIOS
#define FLASH_MCUX_XSPI_RESET_GPIO(n)	\
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define FLASH_MCUX_XSPI_RESET_GPIO(n)
#endif

#if FLASH_MCUX_XSPI_HAS_SOFT_RESET
#define FLASH_MCUX_XSPI_SOFT_RESET(n)	\
	.initial_soft_reset = DT_INST_PROP(n, initial_soft_reset),
#else
#define FLASH_MCUX_XSPI_SOFT_RESET(n)
#endif

#define FLASH_MCUX_XSPI_INIT(n)	\
	static const struct flash_mcux_xspi_config flash_mcux_xspi_config_##n = {	\
		.enable_differential_clk = DT_INST_PROP(n, enable_differential_clk),	\
		.sample_clk_config.sampleClkSource = DT_INST_PROP(n, sample_clk_source),	\
		.sample_clk_config.enableDQSLatency = DT_INST_PROP(n, enable_dqs_latency),	\
		.sample_clk_config.dllConfig = {	\
				.dllMode = kXSPI_AutoUpdateMode,	\
				.useRefValue = true,	\
				.enableCdl8 = true,	\
			},	\
		FLASH_MCUX_XSPI_RESET_GPIO(n)	\
		FLASH_MCUX_XSPI_SOFT_RESET(n)	\
	};	\
	static struct flash_mcux_xspi_data flash_mcux_xspi_data_##n = {	\
		.xspi_dev = DEVICE_DT_GET(DT_INST_BUS(n)),	\
		.dev_name = DT_INST_PROP(n, device_name),	\
		.flash_param = {	\
				.write_block_size = DT_INST_PROP(n, write_block_size),	\
				.erase_value = 0xFF,	\
				.caps = {	\
						.no_explicit_erase = false,	\
					},	\
			},	\
		.flash_size = DT_INST_PROP(n, size),	\
		FLASH_MCUX_XSPI_LAYOUT(n)	\
	};	\
	DEVICE_DT_INST_DEFINE(n, &flash_mcux_xspi_init, NULL, &flash_mcux_xspi_data_##n,	\
				&flash_mcux_xspi_config_##n, POST_KERNEL,	\
				CONFIG_FLASH_INIT_PRIORITY, &flash_mcux_xspi_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MCUX_XSPI_INIT)
