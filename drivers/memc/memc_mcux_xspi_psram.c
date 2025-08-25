/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_xspi_psram

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>
#include <soc.h>
#include "memc_mcux_xspi.h"

LOG_MODULE_REGISTER(memc_mcux_xspi_psram, CONFIG_MEMC_LOG_LEVEL);

struct memc_mcux_xspi_psram_config {
    xspi_device_config_t xspi_dev_config;
};

struct memc_mcux_xspi_psram_data {
	const struct device *xspi_dev;
    uint32_t amba_address;
    uint32_t size;
};

enum {
	PSRAM_CMD_MEM_READ,
    PSRAM_CMD_MEM_WRITE,
	PSRAM_CMD_REG_READ,
	PSRAM_CMD_REG_WRITE,
};

const uint32_t psram_xspi_lut[][5] = {
    /* Memory Read */
    [PSRAM_CMD_MEM_READ] = {
        XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0xA0, kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x18),
        XSPI_LUT_SEQ(kXSPI_Command_CADDR_DDR, kXSPI_8PAD, 0x10, kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 13),
        XSPI_LUT_SEQ(kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x08, kXSPI_Command_STOP, kXSPI_1PAD, 0x0),
    },

    /* Memory Write */
    [PSRAM_CMD_MEM_WRITE] = {
        XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x20, kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x18),
        XSPI_LUT_SEQ(kXSPI_Command_CADDR_DDR, kXSPI_8PAD, 0x10, kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 13),
        XSPI_LUT_SEQ(kXSPI_Command_WRITE_DDR, kXSPI_8PAD, 0x08, kXSPI_Command_STOP, kXSPI_1PAD, 0X0),
    },

    /* Register Read */
    [PSRAM_CMD_REG_READ] = {
        XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0xE0, kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x18),
        XSPI_LUT_SEQ(kXSPI_Command_CADDR_DDR, kXSPI_8PAD, 0x10, kXSPI_Command_DUMMY_SDR, kXSPI_8PAD, 13),  /* Dummy cycle: 13 + 1 */
        XSPI_LUT_SEQ(kXSPI_Command_READ_DDR, kXSPI_8PAD, 0x08, kXSPI_Command_STOP, kXSPI_1PAD, 0x0),
    },

    /* Register write */
    [PSRAM_CMD_REG_WRITE] = {
        XSPI_LUT_SEQ(kXSPI_Command_DDR, kXSPI_8PAD, 0x60, kXSPI_Command_RADDR_DDR, kXSPI_8PAD, 0x18),
        XSPI_LUT_SEQ(kXSPI_Command_CADDR_DDR, kXSPI_8PAD, 0x10, kXSPI_Command_WRITE_DDR, kXSPI_8PAD, 0x08),
        XSPI_LUT_SEQ(kXSPI_Command_STOP, kXSPI_1PAD, 0x0, kXSPI_Command_STOP, kXSPI_1PAD, 0x0),
    },
};

int xspi_psram_write_reg(const struct device *dev, uint32_t regAddr, uint8_t *data, uint32_t size)
{
    struct memc_mcux_xspi_psram_data *psram_data = dev->data;
    xspi_transfer_t flashXfer;

    flashXfer.deviceAddress   = psram_data->amba_address + regAddr;
    flashXfer.cmdType         = kXSPI_Write;
    flashXfer.seqIndex        = PSRAM_CMD_REG_WRITE;
    flashXfer.targetGroup     = kXSPI_TargetGroup0;
    flashXfer.data            = (uint32_t *)data;
    flashXfer.dataSize        = size;
    flashXfer.lockArbitration = false;

    return memc_mcux_xspi_transfer(dev, &flashXfer);
}

int xspi_psram_read_reg(const struct device *dev, uint32_t regAddr, uint8_t *data, uint32_t size)
{
    struct memc_mcux_xspi_psram_data *psram_data = dev->data;
    xspi_transfer_t flashXfer;

    flashXfer.deviceAddress   = psram_data->amba_address + regAddr;
    flashXfer.cmdType         = kXSPI_Read;
    flashXfer.seqIndex        = PSRAM_CMD_REG_READ;
    flashXfer.targetGroup     = kXSPI_TargetGroup0;
    flashXfer.data            = (uint32_t *)data;
    flashXfer.dataSize        = size;
    flashXfer.lockArbitration = false;

    return memc_mcux_xspi_transfer(dev, &flashXfer);
}

static int memc_mcux_xspi_psram_init(const struct device *dev)
{
	struct memc_mcux_xspi_psram_config *psram_config = (struct memc_mcux_xspi_psram_config *)dev->config;
    xspi_device_config_t dev_config = psram_config->xspi_dev_config;
    struct memc_mcux_xspi_psram_data *psram_data = dev->data;
    const struct device *xspi_dev = psram_data->xspi_dev;
    int ret;

	if (!device_is_ready(xspi_dev)) {
		LOG_ERR("XSPI device is not ready");
		return -ENODEV;
	}

    psram_data->amba_address = memc_mcux_xspi_get_ahb_address(xspi_dev);

    if (dev_config.deviceInterface == kXSPI_StrandardExtendedSPI) {
        uint32_t page_size = dev_config.interfaceSettings.hyperBusSettings.pageSize;
        dev_config.interfaceSettings.strandardExtendedSPISettings.pageSize = page_size;
    }

    ret = memc_mcux_xspi_get_root_clock(xspi_dev, &dev_config.xspiRootClk);
    if (ret < 0) {
        return ret;
    }

    return memc_xspi_set_device_config(xspi_dev, &dev_config, &psram_xspi_lut[0][0], sizeof(psram_xspi_lut) / sizeof(psram_xspi_lut[0][0]));
}

#define MEMC_MCUX_XSPI_PSRAM_INIT(n)                                                      \
	static const struct memc_mcux_xspi_psram_config memc_mcux_xspi_psram_config_##n = {   \
        .xspi_dev_config = \
        { \
            .enableCknPad = DT_INST_PROP(n, enable_ckn), \
            .deviceInterface = DT_INST_ENUM_IDX(n, device_interface), \
            .interfaceSettings = { \
                    .hyperBusSettings = { \
                        .x16Mode = DT_INST_PROP(n, hyperbus_x16_mode), \
                        .enableVariableLatency = DT_INST_PROP(n, hyperbus_enable_latency), \
                        .forceBit10To1 = false, \
                        .pageSize = DT_INST_PROP(n, page_size), \
                    } \
                }, \
            .CSHoldTime = DT_INST_PROP(n, cs_hold_time), \
            .CSSetupTime = DT_INST_PROP(n, cs_setup_time), \
            .sampleClkConfig = { \
                .sampleClkSource = DT_INST_PROP(n, sample_clk_source), \
                .enableDQSLatency = DT_INST_PROP(n, enable_dqs_latency), \
                .dllConfig = { \
                    .dllMode = kXSPI_AutoUpdateMode, \
                    .useRefValue = true, \
                    .enableCdl8 = true, \
                }, \
            }, \
            .addrMode = DT_INST_ENUM_IDX(n, address_mode), \
            .columnAddrWidth = DT_INST_PROP(n, column_space), \
            .enableCASInterleaving = DT_INST_PROP(n, enable_cas_interleaving), \
            .deviceSize = { DT_INST_PROP(n, size) / 1024U, DT_INST_PROP(n, size) / 1024U }, \
            .ptrDeviceRegInfo = NULL, \
            .ptrDeviceDdrConfig = DT_INST_PROP(n, enable_ddr) ? \
                &(xspi_device_ddr_config_t){ \
                    .enableDdr = true, \
                    .ddrDataAlignedClk = DT_INST_PROP(n, ddr_data_aligned_clk), \
                    .enableByteSwapInOctalMode = DT_INST_PROP(n, ddr_enable_byte_swap), \
                } : NULL, \
        } \
	}; \
	static struct memc_mcux_xspi_psram_data memc_mcux_xspi_psram_data_##n = {                  \
        .xspi_dev = DEVICE_DT_GET(DT_INST_BUS(n)),                                 \
        .size = DT_INST_PROP(n, size),                                               \
    };                                                                               \
	DEVICE_DT_INST_DEFINE(n, &memc_mcux_xspi_psram_init, NULL, &memc_mcux_xspi_psram_data_##n, \
			      &memc_mcux_xspi_psram_config_##n, POST_KERNEL, CONFIG_MEMC_MCUX_XSPI_PSRAM, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MCUX_XSPI_PSRAM_INIT)
