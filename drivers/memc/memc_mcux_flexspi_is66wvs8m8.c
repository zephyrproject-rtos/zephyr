/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_flexspi_is66wvs8m8

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "memc_mcux_flexspi.h"

LOG_MODULE_REGISTER(memc_flexspi_is66wvs8m8, CONFIG_MEMC_LOG_LEVEL);

/*
 * Example PSRAM and FlexSPI Controller DTS setting.
 *
	/ {
		// Add External PSRAM to the Linker Map.
		psram0: psram_region@90800000 {
			compatible = "zephyr,memory-region", "mmio-sram";
			zephyr,memory-region = "EXT_PSRAM";
			device_type = "memory";
			reg = <0x90800000 0x800000>;
		};
	};

	&flexspi {
		status = "okay";
		pinctrl-0 = <&pinmux_flexspi>;
		pinctrl-names = "default";
		rx-clock-source = <1>;
		/delete-property/ combination-mode;
		/delete-property/ ahb-cacheable;
		/delete-property/ ahb-bufferable;
		/delete-property/ ahb-prefetch;
		/delete-property/ ahb-read-addr-opt;

		// Account for both the memories (w25q64jvssiq & is66wvs8m8)
		// on the FlexSPI controller.
		reg = <0x500c8000 0x1000>,
		<0x90000000 DT_SIZE_M(16)>;

		w25q64jvssiq: w25q64jvssiq@0 {
			........
		}

		is66wvs8m8: is66wvs8m8@2 {
			compatible = "nxp,imx-flexspi-is66wvs8m8";
			// IS66WVS8M8 is 8MB, 64MBit SerialRAM
			size = <DT_SIZE_M(64)>;
			reg = <2>;
			spi-max-frequency = <100000000>;
			// PSRAM cannot be enabled while board is in default XIP
			// configuration, as it will conflict with flash chip.
			//
			status = "okay";
			cs-interval-unit = <1>;
			cs-interval = <3>;
			cs-hold-time = <3>;
			cs-setup-time = <3>;
			data-valid-time = <1>;
			column-space = <0>;
			ahb-write-wait-unit = <2>;
			ahb-write-wait-interval = <1>;
		};
	};
*/

/* Vendor ID for ISSI device */
#define ISSI_VENDOR_ID 0x9D

enum {
	READ_DATA = 0,
	WRITE_DATA,
	READ_ID,
	DPD_ENTRY,
	SET_BURST_LENGTH,
};

struct memc_flexspi_is66wvs8m8_config {
	flexspi_port_t port;
	flexspi_device_config_t config;
};

/* Device variables used in critical sections should be in this structure */
struct memc_flexspi_is66wvs8m8_data {
	const struct device *controller;
};

/* is66wvs8m8 configuration register constants */
#define is66wvs8m8_LATENCY_MASK  BIT(3)
#define is66wvs8m8_LATENCY_FIXED BIT(3)

static const uint32_t memc_flexspi_is66wvs8m8_lut[][4] = {
	/* Read Data (Quad IO read) */
	[READ_DATA] = {
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xEB,
					kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 24),
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_4PAD, 6,
					kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x0),
		},
	/* Write Data (Quad IO Write) */
	[WRITE_DATA] = {
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x38,
					kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 24),
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0x0,
					kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),
		},
	/* Read Identification register */
	[READ_ID] = {
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x9F,
					kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 24),
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x00,
					kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),
		},
	[DPD_ENTRY] = {
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xB9,
					kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),
		},
	[SET_BURST_LENGTH] = {
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xC0,
					kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),
		},
};

/* Read vendor ID from identification register */
static int memc_flexspi_is66wvs8m8_get_vendor_id(const struct device *dev, uint8_t *vendor_id)
{
	const struct memc_flexspi_is66wvs8m8_config *config = dev->config;
	struct memc_flexspi_is66wvs8m8_data *data = dev->data;
	uint8_t buffer[8] = {0};
	int ret;

	flexspi_transfer_t transfer = {
		.deviceAddress = 0x00, /* Not used by this command */
		.port = config->port,
		.cmdType = kFLEXSPI_Read,
		.seqIndex = READ_ID,
		.SeqNumber = 1,
		.data = (uint32_t *)&buffer,
		.dataSize = 8,
	};

	ret = memc_flexspi_transfer(data->controller, &transfer);

	*vendor_id = buffer[0];

	return ret;
}

static int memc_flexspi_is66wvs8m8_init(const struct device *dev)
{
	const struct memc_flexspi_is66wvs8m8_config *config = dev->config;
	struct memc_flexspi_is66wvs8m8_data *data = dev->data;
	uint8_t vendor_id;

	if (!device_is_ready(data->controller)) {
		LOG_ERR("Controller device not ready");
		return -ENODEV;
	}

	if (memc_flexspi_set_device_config(
		    data->controller, &config->config,
		    (const uint32_t *)memc_flexspi_is66wvs8m8_lut,
		    sizeof(memc_flexspi_is66wvs8m8_lut) / MEMC_FLEXSPI_CMD_SIZE, config->port)) {
		LOG_ERR("Could not set device configuration");
		return -EINVAL;
	}

	if (memc_flexspi_is66wvs8m8_get_vendor_id(dev, &vendor_id)) {
		LOG_ERR("Could not read vendor id");
		return -EIO;
	}

	if (vendor_id != ISSI_VENDOR_ID) {
		LOG_WRN("Vendor ID does not match expected value of 0x%0x", ISSI_VENDOR_ID);
	}

	return 0;
}

#define CONCAT3(x, y, z) x##y##z

#define CS_INTERVAL_UNIT(unit) CONCAT3(kFLEXSPI_CsIntervalUnit, unit, SckCycle)

#define AHB_WRITE_WAIT_UNIT(unit) CONCAT3(kFLEXSPI_AhbWriteWaitUnit, unit, AhbCycle)

#define MEMC_FLEXSPI_DEVICE_CONFIG(n)                                                              \
	{                                                                                          \
		.flexspiRootClk = DT_INST_PROP(n, spi_max_frequency),                              \
		.isSck2Enabled = false,                                                            \
		.flashSize = DT_INST_PROP(n, size) / 8 / KB(1),                                    \
		.CSIntervalUnit = CS_INTERVAL_UNIT(DT_INST_PROP(n, cs_interval_unit)),             \
		.CSInterval = DT_INST_PROP(n, cs_interval),                                        \
		.CSHoldTime = DT_INST_PROP(n, cs_hold_time),                                       \
		.CSSetupTime = DT_INST_PROP(n, cs_setup_time),                                     \
		.dataValidTime = DT_INST_PROP(n, data_valid_time),                                 \
		.columnspace = DT_INST_PROP(n, column_space),                                      \
		.enableWordAddress = DT_INST_PROP(n, word_addressable),                            \
		.AWRSeqIndex = WRITE_DATA,                                                         \
		.AWRSeqNumber = 1,                                                                 \
		.ARDSeqIndex = READ_DATA,                                                          \
		.ARDSeqNumber = 1,                                                                 \
		.AHBWriteWaitUnit = AHB_WRITE_WAIT_UNIT(DT_INST_PROP(n, ahb_write_wait_unit)),     \
		.AHBWriteWaitInterval = DT_INST_PROP(n, ahb_write_wait_interval),                  \
		.enableWriteMask = false,                                                          \
	}

#define MEMC_FLEXSPI_ISS66VWS8M8(n)                                                                \
	static const struct memc_flexspi_is66wvs8m8_config memc_flexspi_is66wvs8m8_config_##n = {  \
		.port = DT_INST_REG_ADDR(n),                                                       \
		.config = MEMC_FLEXSPI_DEVICE_CONFIG(n),                                           \
	};                                                                                         \
                                                                                                   \
	static struct memc_flexspi_is66wvs8m8_data memc_flexspi_is66wvs8m8_data_##n = {            \
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, memc_flexspi_is66wvs8m8_init, NULL,                               \
			      &memc_flexspi_is66wvs8m8_data_##n,                                   \
			      &memc_flexspi_is66wvs8m8_config_##n, POST_KERNEL,                    \
			      CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_FLEXSPI_ISS66VWS8M8)
