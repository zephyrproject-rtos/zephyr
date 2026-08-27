/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_xspi_psram

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>
#include <soc.h>
#include "memc_mcux_xspi.h"

LOG_MODULE_REGISTER(memc_mcux_xspi_psram, CONFIG_MEMC_LOG_LEVEL);

#define MEMC_MCUX_XSPI_LUT_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0][0]))

#define ID0_REG_ADDR (0U)
#define CR0_REG_ADDR (0x800U)
#define CR1_REG_ADDR (0x801U)

#define ID0_REG_ID_MASK (0xFU)

#define CR0_REG_DRIVE_STRENGTH_SHIFT  4U
#define CR0_REG_DRIVE_STRENGTH_MASK   (0x07U << CR0_REG_DRIVE_STRENGTH_SHIFT)
#define CR0_REG_DRIVE_STRENGTH_46OHMS 3U
#define CR0_REG_VARIABLE_LATENCY_MASK (1U << 3)

#define CR1_DIFFERENTIAL_CLOCK_SHIFT 6U
#define CR1_DIFFERENTIAL_CLOCK_MASK  (1U << CR1_DIFFERENTIAL_CLOCK_SHIFT)

enum {
	PSRAM_MANUFACTURER_ID_WINBOND = 0x6U,
};

enum {
	PSRAM_CMD_MEM_READ,
	PSRAM_CMD_MEM_WRITE,
	PSRAM_CMD_REG_READ,
	PSRAM_CMD_REG_WRITE,
};

struct memc_mcux_xspi_psram_config {
	bool enable_differential_clk;
	xspi_sample_clk_config_t sample_clk_config;
};

struct memc_mcux_xspi_psram_data {
	const struct device *xspi_dev;
	const char *dev_name;
	uint32_t amba_address;
	uint32_t size;
};

const uint32_t memc_xspi_w958d6nbkx_lut[][5] = {
	/* Memory Read */
	[PSRAM_CMD_MEM_READ] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0xA0, kXSPI_Command_RADDR_DDR,
					 kXSPI_8PAD, 0x18),
			XSPI_LUT_SEQ(kXSPI_Command_CADDR_DDR, kXSPI_8PAD, 0x10,
					 kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 6),
			XSPI_LUT_SEQ(kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x08, kXSPI_Command_STOP,
					 kXSPI_1PAD, 0x0),
		},

	/* Memory Write */
	[PSRAM_CMD_MEM_WRITE] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x20, kXSPI_Command_RADDR_DDR,
					 kXSPI_8PAD, 0x18),
			XSPI_LUT_SEQ(kXSPI_Command_CADDR_DDR, kXSPI_8PAD, 0x10,
					 kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 6),
			XSPI_LUT_SEQ(kXSPI_Command_WRITE_DDR, kXSPI_8PAD, 0x08, kXSPI_Command_STOP,
					 kXSPI_1PAD, 0X0),
		},

	/* Register Read */
	[PSRAM_CMD_REG_READ] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0xE0, kXSPI_Command_RADDR_DDR,
					 kXSPI_8PAD, 0x18),
			XSPI_LUT_SEQ(kXSPI_Command_CADDR_DDR, kXSPI_8PAD, 0x10,
					 kXSPI_Command_DUMMY_SDR, kXSPI_8PAD,
					 6), /* Dummy cycle: 2 * 6 + 2 */
			XSPI_LUT_SEQ(kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x08, kXSPI_Command_STOP,
					 kXSPI_1PAD, 0x0),
		},

	/* Register write */
	[PSRAM_CMD_REG_WRITE] = {
			XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x60, kXSPI_Command_RADDR_DDR,
					 kXSPI_8PAD, 0x18),
			XSPI_LUT_SEQ(kXSPI_Command_CADDR_DDR, kXSPI_8PAD, 0x10,
					 kXSPI_Command_WRITE_DDR, kXSPI_8PAD, 0x08),
			XSPI_LUT_SEQ(kXSPI_Command_STOP, kXSPI_1PAD, 0x0, kXSPI_Command_STOP,
					 kXSPI_1PAD, 0x0),
		},
};

/* Memory devices table. */
static struct memc_xspi_dev_config dev_configs[] = {
	{
		.name_prefix = "w958d6nbkx",
		.xspi_dev_config = {
			.deviceInterface = kXSPI_HyperBus,
			.interfaceSettings.hyperBusSettings.x16Mode = kXSPI_x16ModeEnabledOnlyData,
			.interfaceSettings.hyperBusSettings.enableVariableLatency = true,
			.interfaceSettings.hyperBusSettings.forceBit10To1 = false,
			.interfaceSettings.hyperBusSettings.pageSize = 1024,
			.CSHoldTime = 2,
			.CSSetupTime = 2,
			.addrMode = kXSPI_Device4ByteAddressable,
			.columnAddrWidth = 3,
			.enableCASInterleaving = false,
			.ptrDeviceDdrConfig =
				&(xspi_device_ddr_config_t){
					.ddrDataAlignedClk =
						kXSPI_DDRDataAlignedWith2xInternalRefClk,
					.enableByteSwapInOctalMode = false,
					.enableDdr = true,
				},
			.deviceSize = {32 * 1024, 32 * 1024},
			},
		.lut_array = &memc_xspi_w958d6nbkx_lut[0][0],
		.lut_count = MEMC_MCUX_XSPI_LUT_ARRAY_SIZE(memc_xspi_w958d6nbkx_lut),
	},
};

static int xspi_psram_write_reg(const struct device *dev, uint32_t regAddr,
				uint8_t *data, uint32_t size);
static int xspi_psram_read_reg(const struct device *dev, uint32_t regAddr,
				uint8_t *data, uint32_t size);

static int memc_mcux_xspi_w958d6nbkx_enable_clk(const struct device *dev)
{
	uint16_t reg[2] = {0x0U, 0x0U};
	int ret = 0;

	do {
		ret = xspi_psram_read_reg(dev, CR1_REG_ADDR, (uint8_t *)reg, 4);
		if (ret < 0) {
			break;
		}

		reg[1] &= ~CR1_DIFFERENTIAL_CLOCK_MASK;

		ret = xspi_psram_write_reg(dev, CR1_REG_ADDR, (uint8_t *)reg, 4);
		if (ret < 0) {
			break;
		}

		ret = xspi_psram_read_reg(dev, CR1_REG_ADDR, (uint8_t *)reg, 4);
		if (ret < 0) {
			break;
		}

		if ((reg[1] & CR1_DIFFERENTIAL_CLOCK_MASK) != 0U) {
			ret = -EIO;
			break;
		}
	} while (0);

	return ret;
}

static int memc_mcux_xspi_w958d6nbkx_enable_variable_latency(const struct device *dev)
{
	uint16_t reg[2] = {0x0U, 0x0U};
	int ret = 0;

	do {
		ret = xspi_psram_read_reg(dev, CR0_REG_ADDR, (uint8_t *)reg, 4);
		if (ret < 0) {
			break;
		}

		reg[1] &= ~CR0_REG_VARIABLE_LATENCY_MASK;
		reg[0] &= ~CR0_REG_DRIVE_STRENGTH_MASK;
		reg[0] |= (CR0_REG_DRIVE_STRENGTH_46OHMS << CR0_REG_DRIVE_STRENGTH_SHIFT);

		ret = xspi_psram_write_reg(dev, CR0_REG_ADDR, (uint8_t *)reg, 4);
		if (ret < 0) {
			break;
		}

		ret = xspi_psram_read_reg(dev, CR0_REG_ADDR, (uint8_t *)reg, 4);
		if (ret < 0) {
			break;
		}

		if ((reg[1] & CR0_REG_VARIABLE_LATENCY_MASK) != 0U) {
			ret = -EIO;
			break;
		}
	} while (0);

	return ret;
}

static int memc_mcux_xspi_w958d6nbkx_setup(const struct device *dev,
				xspi_device_config_t *config)
{
	struct memc_mcux_xspi_psram_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	uint16_t reg[2] = {0x0U, 0x0U};
	int ret = 0;
	uint8_t id;

	memc_mcux_xspi_update_device_addr_mode(xspi_dev, kXSPI_DeviceByteAddressable);

	do {
		ret = xspi_psram_read_reg(dev, ID0_REG_ADDR, (uint8_t *)reg, 4);
		if (ret < 0) {
			break;
		}

		id = reg[1] & ID0_REG_ID_MASK;
		if (id != PSRAM_MANUFACTURER_ID_WINBOND) {
			LOG_ERR("Wrong manufacturer ID: 0x%X, expected: 0x%X", id,
				PSRAM_MANUFACTURER_ID_WINBOND);
			ret = -ENODEV;
			break;
		}

		if (config->enableCknPad) {
			ret = memc_mcux_xspi_w958d6nbkx_enable_clk(dev);
			if (ret < 0) {
				break;
			}
		}

		if (config->interfaceSettings.hyperBusSettings.enableVariableLatency) {
			ret = memc_mcux_xspi_w958d6nbkx_enable_variable_latency(dev);
			if (ret < 0) {
				break;
			}
		}
	} while (0);

	memc_mcux_xspi_update_device_addr_mode(xspi_dev, kXSPI_Device4ByteAddressable);

	return ret;
}

static int xspi_psram_write_reg(const struct device *dev, uint32_t regAddr,
				uint8_t *data, uint32_t size)
{
	struct memc_mcux_xspi_psram_data *psram_data = dev->data;
	xspi_transfer_t flashXfer;

	flashXfer.deviceAddress = psram_data->amba_address + regAddr;
	flashXfer.cmdType = kXSPI_Write;
	flashXfer.seqIndex = PSRAM_CMD_REG_WRITE;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.data = (uint32_t *)data;
	flashXfer.dataSize = size;
	flashXfer.lockArbitration = false;

	return memc_mcux_xspi_transfer(psram_data->xspi_dev, &flashXfer);
}

static int xspi_psram_read_reg(const struct device *dev, uint32_t regAddr,
					uint8_t *data, uint32_t size)
{
	struct memc_mcux_xspi_psram_data *psram_data = dev->data;
	xspi_transfer_t flashXfer;

	flashXfer.deviceAddress = psram_data->amba_address + regAddr;
	flashXfer.cmdType = kXSPI_Read;
	flashXfer.seqIndex = PSRAM_CMD_REG_READ;
	flashXfer.targetGroup = kXSPI_TargetGroup0;
	flashXfer.data = (uint32_t *)data;
	flashXfer.dataSize = size;
	flashXfer.lockArbitration = false;

	return memc_mcux_xspi_transfer(psram_data->xspi_dev, &flashXfer);
}

static int memc_mcux_xspi_psram_setup(const struct device *dev,
		const char *dev_name_prefix, xspi_device_config_t *config)
{
	int ret = 0;

	if (strcmp(dev_name_prefix, "w958d6nbkx") == 0) {
		ret = memc_mcux_xspi_w958d6nbkx_setup(dev, config);
	}

	return ret;
}

static int memc_mcux_xspi_psram_probe(const struct device *dev)
{
	const struct memc_mcux_xspi_psram_config *config =
		(const struct memc_mcux_xspi_psram_config *)dev->config;
	struct memc_mcux_xspi_psram_data *data = dev->data;
	const struct device *xspi_dev = data->xspi_dev;
	struct memc_xspi_dev_config *xspi_psram_config = NULL;
	xspi_device_config_t *dev_config = NULL;
	int ret;

	/* Get the specific memory parameters. */
	for (uint32_t i = 0; i < ARRAY_SIZE(dev_configs); i++) {
		if (strncmp(dev_configs[i].name_prefix, data->dev_name,
				strlen(dev_configs[i].name_prefix)) == 0) {
			xspi_psram_config = (struct memc_xspi_dev_config *)&dev_configs[i];
			break;
		}
	}

	if (xspi_psram_config == NULL) {
		LOG_ERR("Unsupported device: %s", data->dev_name);
		return -ENOTSUP;
	}

	/* Set special device configurations. */
	dev_config = &xspi_psram_config->xspi_dev_config;
	dev_config->enableCknPad = config->enable_differential_clk;
	dev_config->sampleClkConfig = config->sample_clk_config;

	ret = memc_mcux_xspi_get_root_clock(xspi_dev, &dev_config->xspiRootClk);
	if (ret < 0) {
		return ret;
	}

	ret = memc_xspi_set_device_config(xspi_dev, dev_config,
		xspi_psram_config->lut_array, xspi_psram_config->lut_count);
	if (ret < 0) {
		return ret;
	}

	return memc_mcux_xspi_psram_setup(dev, xspi_psram_config->name_prefix, dev_config);
}

static int memc_mcux_xspi_psram_init(const struct device *dev)
{
	struct memc_mcux_xspi_psram_data *psram_data = dev->data;
	const struct device *xspi_dev = psram_data->xspi_dev;

	if (!device_is_ready(xspi_dev)) {
		LOG_ERR("XSPI device is not ready");
		return -ENODEV;
	}

	psram_data->amba_address = memc_mcux_xspi_get_ahb_address(xspi_dev);

	return memc_mcux_xspi_psram_probe(dev);
}

#define MEMC_MCUX_XSPI_PSRAM_INIT(n)	\
	static const struct memc_mcux_xspi_psram_config \
		memc_mcux_xspi_psram_config_##n = {	\
		.enable_differential_clk = DT_INST_PROP(n, enable_differential_clk),	\
		.sample_clk_config = {	\
			.sampleClkSource = DT_INST_PROP(n, sample_clk_source),	\
			.enableDQSLatency = DT_INST_PROP(n, enable_dqs_latency),	\
			.dllConfig = {	\
				.dllMode = kXSPI_AutoUpdateMode,	\
				.useRefValue = true,	\
				.enableCdl8 = true,	\
			},	\
		},	\
	};	\
	static struct memc_mcux_xspi_psram_data memc_mcux_xspi_psram_data_##n = {	\
		.xspi_dev = DEVICE_DT_GET(DT_INST_BUS(n)),	\
		.dev_name = DT_INST_PROP(n, device_name),	\
		.size = DT_INST_PROP(n, size),	\
	};	\
	DEVICE_DT_INST_DEFINE(n, &memc_mcux_xspi_psram_init, NULL, \
				&memc_mcux_xspi_psram_data_##n,	\
				&memc_mcux_xspi_psram_config_##n, POST_KERNEL,	\
				CONFIG_MEMC_MCUX_XSPI_PSRAM, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MCUX_XSPI_PSRAM_INIT)
