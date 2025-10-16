/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_xspi_nor

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include "memc_mcux_xspi.h"
#include "spi_nor.h"

LOG_MODULE_REGISTER(flash_mcux_xspi);

#define FLASH_MCUX_XSPI_LUT_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0][0]))

#define FLASH_BUSY_STATUS_OFFSET 0
#define FLASH_WE_STATUS_OFFSET   7

#define FLASH_MX25_WRCR2_DTR_OPI_ENABLE_OFFSET (1U << 1)

enum {
	FLASH_CMD_MEM_READ,
	FLASH_CMD_READ_STATUS,
	FLASH_CMD_READ_STATUS_OPI,
	FLASH_CMD_WRITE_ENABLE,
	FLASH_CMD_WRITE_ENABLE_OPI,
	FLASH_CMD_PAGEPROGRAM_OCTAL,
	FLASH_CMD_ERASE_SECTOR,
	FLASH_CMD_READ_ID_OPI,
	FLASH_CMD_ENTER_OPI,
};

struct flash_mcux_xspi_config {
	bool enable_differential_clk;
	xspi_sample_clk_config_t sample_clk_config;
};

struct flash_mcux_xspi_data {
	xspi_config_t xspi_config;
	const struct device *xspi_dev;
	const char *dev_name;
	uint32_t amba_address;
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
static const uint32_t flash_xspi_lut[][5] = {
	/* Memory read. */
	[FLASH_CMD_MEM_READ] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0xEE, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0x11),
			XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20,
					 kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x12),
			XSPI_LUT_SEQ(kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x2,
					 kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x8),
			XSPI_LUT_SEQ(kXSPI_Command_STOP, kXSPI_8PAD, 0x0, 0, 0, 0),
		},

	/* Read status SPI. */
	[FLASH_CMD_READ_STATUS] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x05, kXSPI_Command_READ_SDR,
					 kXSPI_1PAD, 0x08),
		},

	/* Read Status OPI. */
	[FLASH_CMD_READ_STATUS_OPI] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x05, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0xFA),
			XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20,
					 kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x12),
			XSPI_LUT_SEQ(kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x2,
					 kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x8),
			XSPI_LUT_SEQ(kXSPI_Command_STOP, kXSPI_8PAD, 0x0, 0, 0, 0),
		},

	/* Write enable. */
	[FLASH_CMD_WRITE_ENABLE] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x06, kXSPI_Command_STOP,
					 kXSPI_1PAD, 0x04),
		},

	/* Write Enable - OPI. */
	[FLASH_CMD_WRITE_ENABLE_OPI] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x06, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0xF9),
		},

	/* Read ID. */
	[FLASH_CMD_READ_ID_OPI] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x9F, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0x60),
			XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20,
					 kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 0x04),
			XSPI_LUT_SEQ(kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x08, kXSPI_Command_STOP,
					 kXSPI_1PAD, 0x0),
		},

	/* Erase Sector. */
	[FLASH_CMD_ERASE_SECTOR] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x21, kXSPI_Command_DDR,
					 kXSPI_8PAD, 0xDE),
			XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20, kXSPI_Command_STOP,
					 kXSPI_8PAD, 0x0),
		},

	/* Enable OPI DDR mode. */
	[FLASH_CMD_ENTER_OPI] = {
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x72, kXSPI_Command_SDR,
					 kXSPI_1PAD, 0x00),
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x00, kXSPI_Command_SDR,
					 kXSPI_1PAD, 0x00),
			XSPI_LUT_SEQ(kXSPI_Command_SDR, kXSPI_1PAD, 0x00, kXSPI_Command_WRITE_SDR,
					 kXSPI_1PAD, 0x01),
		},

	/* Page program. */
	[FLASH_CMD_PAGEPROGRAM_OCTAL] = {
		XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x12, kXSPI_Command_DDR, kXSPI_8PAD,
				 0xED),
		XSPI_LUT_SEQ(kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x20, kXSPI_Command_WRITE_DDR,
				 kXSPI_8PAD, 0x8),
	}};

/* Memory devices table. */
static struct memc_xspi_dev_config device_configs[] = {
	{
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
		.lut_array = &flash_xspi_lut[0][0],
		.lut_count = FLASH_MCUX_XSPI_LUT_ARRAY_SIZE(flash_xspi_lut),
	},
};

static int flash_xspi_nor_wait_bus_busy(const struct device *dev, bool enableOctal)
{
	struct flash_mcux_xspi_data *devData = dev->data;
	const struct device *xspi_dev = devData->xspi_dev;
	xspi_transfer_t flashXfer;
	uint32_t readValue;
	bool isBusy;
	int ret;

	flashXfer.deviceAddress = devData->amba_address;
	flashXfer.cmdType = kXSPI_Read;
	flashXfer.data = &readValue;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.dataSize = enableOctal ? 2 : 1;
	flashXfer.seqIndex = enableOctal ? FLASH_CMD_READ_STATUS_OPI : FLASH_CMD_READ_STATUS;
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

static int flash_mcux_xspi_write_enable(const struct device *dev, uint32_t baseAddr,
					bool enableOctal)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	xspi_transfer_t flashXfer;

	flashXfer.deviceAddress = data->amba_address + baseAddr;
	flashXfer.cmdType = kXSPI_Command;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.data = NULL;
	flashXfer.dataSize = 0;
	flashXfer.lockArbitration = false;
	flashXfer.seqIndex = enableOctal ? FLASH_CMD_WRITE_ENABLE_OPI : FLASH_CMD_WRITE_ENABLE;

	return memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
}

static int flash_mcux_xspi_write(const struct device *dev, off_t offset, const void *data,
				 size_t len)
{
	struct flash_mcux_xspi_data *devData = dev->data;
	const struct device *xspi_dev = devData->xspi_dev;
	uint8_t *p_data = (uint8_t *)(uintptr_t)data;
	xspi_transfer_t flashXfer;
	size_t write_size;
	uint32_t key = 0;
	int ret = 0;

	if (memc_xspi_is_running_xip(xspi_dev)) {
		key = irq_lock();
		memc_xspi_wait_bus_idle(xspi_dev);
	}

	while (len > 0) {
		write_size = MIN(len, SPI_NOR_PAGE_SIZE);

		ret = flash_mcux_xspi_write_enable(dev, 0, true);
		if (ret < 0) {
			break;
		}

		flashXfer.deviceAddress = devData->amba_address + offset;
		flashXfer.cmdType = kXSPI_Write;
		flashXfer.seqIndex = FLASH_CMD_PAGEPROGRAM_OCTAL;
		flashXfer.targetGroup = kXSPI_TargetGroup0;
		flashXfer.data = (uint32_t *)p_data;
		flashXfer.dataSize = write_size;
		flashXfer.lockArbitration = false;

		ret = memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
		if (ret < 0) {
			break;
		}

		ret = flash_xspi_nor_wait_bus_busy(dev, true);
		if (ret < 0) {
			break;
		}

		len -= write_size;
		p_data = p_data + write_size;
		offset += write_size;
	}

	if (memc_xspi_is_running_xip(xspi_dev)) {
		irq_unlock(key);
	}

	return ret;
}

static int flash_mcux_xspi_erase_sector(const struct device *dev, off_t offset)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	xspi_transfer_t flashXfer;

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
		ret = flash_mcux_xspi_write_enable(dev, 0, true);
		if (ret < 0) {
			break;
		}

		ret = flash_mcux_xspi_erase_sector(dev, offset + i * SPI_NOR_SECTOR_SIZE);
		if (ret < 0) {
			break;
		}

		ret = flash_xspi_nor_wait_bus_busy(dev, true);
		if (ret < 0) {
			break;
		}
	}

	if (memc_xspi_is_running_xip(xspi_dev)) {
		irq_unlock(key);
	}

	return ret;
}

static int flash_mcux_xspi_enable_opi(const struct device *dev)
{
	uint32_t value = FLASH_MX25_WRCR2_DTR_OPI_ENABLE_OFFSET;
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	xspi_transfer_t flashXfer;
	int ret;

	ret = flash_mcux_xspi_write_enable(dev, 0, true);
	if (ret < 0) {
		return ret;
	}

	flashXfer.deviceAddress = data->amba_address;
	flashXfer.cmdType = kXSPI_Write;
	flashXfer.seqIndex = FLASH_CMD_ENTER_OPI;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.data = &value;
	flashXfer.dataSize = 1;
	flashXfer.lockArbitration = false;

	ret = memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
	if (ret < 0) {
		return ret;
	}

	return flash_xspi_nor_wait_bus_busy(dev, true);
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
	xspi_transfer_t flashXfer;

	flashXfer.deviceAddress = data->amba_address;
	flashXfer.cmdType = kXSPI_Read;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.seqIndex = FLASH_CMD_READ_ID_OPI;
	flashXfer.data = id;
	flashXfer.dataSize = 1;
	flashXfer.lockArbitration = false;

	return memc_mcux_xspi_transfer(xspi_dev, &flashXfer);
}
#endif /* CONFIG_FLASH_JESD216_API */

static int flash_mcux_xspi_probe(const struct device *dev)
{
	const struct flash_mcux_xspi_config *flash_config =
		(const struct flash_mcux_xspi_config *)dev->config;
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	struct memc_xspi_dev_config *flash_dev_config = NULL;
	xspi_device_config_t *dev_config = NULL;
	uint32_t key = 0;
	int ret;

	if (memc_xspi_is_running_xip(xspi_dev)) {
		key = irq_lock();
		memc_xspi_wait_bus_idle(xspi_dev);
	}

	/* Setup the specific flash parameters. */
	for (uint32_t i = 0; i < ARRAY_SIZE(device_configs); i++) {
		if (strncmp(device_configs[i].name_prefix, data->dev_name,
				strlen(device_configs[i].name_prefix)) == 0) {
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

		/* Set special device configurations. */
		dev_config = &flash_dev_config->xspi_dev_config;
		dev_config->enableCknPad = flash_config->enable_differential_clk;
		dev_config->sampleClkConfig = flash_config->sample_clk_config;

		ret = memc_mcux_xspi_get_root_clock(xspi_dev, &dev_config->xspiRootClk);
		if (ret < 0) {
			break;
		}

		ret = memc_xspi_set_device_config(xspi_dev, dev_config, flash_dev_config->lut_array,
						  flash_dev_config->lut_count);
	} while (0);

	if (memc_xspi_is_running_xip(xspi_dev)) {
		irq_unlock(key);
	}

	return ret;
}

static int flash_mcux_xspi_init(const struct device *dev)
{
	struct flash_mcux_xspi_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	int ret;

	if (!device_is_ready(xspi_dev)) {
		LOG_ERR("XSPI device is not ready");
		return -ENODEV;
	}

	ret = flash_mcux_xspi_probe(dev);
	if (ret < 0) {
		return ret;
	}

	data->amba_address = memc_mcux_xspi_get_ahb_address(xspi_dev);

	return flash_mcux_xspi_enable_opi(dev);
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

#define FLASH_MCUX_XSPI_INIT(n)	\
	static const struct flash_mcux_xspi_config flash_mcux_xspi_config_##n = {	\
		.sample_clk_config.sampleClkSource = DT_INST_PROP(n, sample_clk_source),	\
		.sample_clk_config.enableDQSLatency = DT_INST_PROP(n, enable_dqs_latency),	\
		.sample_clk_config.dllConfig = {	\
				.dllMode = kXSPI_AutoUpdateMode,	\
				.useRefValue = true,	\
				.enableCdl8 = true,	\
			},	\
	};	\
	static struct flash_mcux_xspi_data flash_mcux_xspi_data_##n = {	\
		.xspi_dev = DEVICE_DT_GET(DT_INST_BUS(n)),	\
		.dev_name = DT_INST_PROP(n, device_name),	\
		.flash_param = {	\
				.write_block_size = 1,	\
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
