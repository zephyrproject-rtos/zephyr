/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #define DT_DRV_COMPAT nxp_imx_flexspi_is66wvq8m4

 #include <zephyr/kernel.h>
 #include <zephyr/logging/log.h>
 #include <zephyr/sys/util.h>

 #include "memc_mcux_flexspi.h"

LOG_MODULE_REGISTER(memc_flexspi_is66wvq8m4, CONFIG_MEMC_LOG_LEVEL);

/* Vendor ID for ISSI device */
#define ISSI_VENDOR_ID 0x3

enum {
	READ_DATA = 0,
	WRITE_DATA,
	READ_REG,
	WRITE_REG,
	READ_ID,
};

struct memc_flexspi_is66wvq8m4_config {
	flexspi_port_t port;
	flexspi_device_config_t config;
};

/* Device variables used in critical sections should be in this structure */
struct memc_flexspi_is66wvq8m4_data {
	const struct device *controller;
};

/* IS66WVQ8M4 configuration register constants */
#define IS66WVQ8M4_LATENCY_MASK BIT(3)
#define IS66WVQ8M4_LATENCY_FIXED BIT(3)

static const uint32_t memc_flexspi_is66wvq8m4_lut[][4] = {
	/* Read Data (continuous burst) */
	[READ_DATA] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_4PAD, 0xAA,
				kFLEXSPI_Command_DDR, kFLEXSPI_4PAD, 0x00),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_4PAD, 16,
				kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_4PAD, 16),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_4PAD, 28,
				kFLEXSPI_Command_READ_DDR, kFLEXSPI_4PAD, 0x01),
	},
	/* Write Data (continuous burst) */
	[WRITE_DATA] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_4PAD, 0x22,
				kFLEXSPI_Command_DDR, kFLEXSPI_4PAD, 0x00),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_4PAD, 16,
				kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_4PAD, 16),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_4PAD, 28,
				kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_4PAD, 0x01),
	},
	/* Read Register */
	[READ_REG] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_4PAD, 0xCC,
				kFLEXSPI_Command_DDR, kFLEXSPI_4PAD, 0x00),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_4PAD, 16,
				kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_4PAD, 16),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_4PAD, 12,
				kFLEXSPI_Command_READ_DDR, kFLEXSPI_4PAD, 0x01),
	},
	/* Write Register */
	[WRITE_REG] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_4PAD, 0x66,
				kFLEXSPI_Command_DDR, kFLEXSPI_4PAD, 0x00),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_4PAD, 16,
				kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_4PAD, 16),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_4PAD, 0x01,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),
	},
	/* Read Identification register */
	[READ_ID] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_4PAD, 0xE0,
				kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_4PAD, 16),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_4PAD, 16,
				kFLEXSPI_Command_DUMMY_RWDS_DDR, kFLEXSPI_4PAD, 0x08),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_DDR, kFLEXSPI_4PAD, 0x01,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),
	}
};


/* Read vendor ID from identification register */
static int memc_flexspi_is66wvq8m4_get_vendor_id(const struct device *dev,
						uint8_t *vendor_id)
{
	const struct memc_flexspi_is66wvq8m4_config *config = dev->config;
	struct memc_flexspi_is66wvq8m4_data *data = dev->data;
	uint32_t buffer = 0;
	int ret;

	flexspi_transfer_t transfer = {
		.deviceAddress = 0x00, /* Not used by this command */
		.port = config->port,
		.cmdType = kFLEXSPI_Read,
		.SeqNumber = 1,
		.seqIndex = READ_ID,
		.data = &buffer,
		.dataSize = 2,
	};

	ret = memc_flexspi_transfer(data->controller, &transfer);
	*vendor_id = buffer & 0x7;

	return ret;
}

/* Update configuration register */
static int memc_flexspi_is66wvq8m4_update_cfg(const struct device *dev,
					      uint16_t mask, uint16_t set_val)
{
	const struct memc_flexspi_is66wvq8m4_config *config = dev->config;
	struct memc_flexspi_is66wvq8m4_data *data = dev->data;
	uint32_t buffer = 0;
	int ret;

	flexspi_transfer_t transfer = {
		/* Results in 0x4 being written on clock 4 */
		.deviceAddress = (0x4 << 9),
		.port = config->port,
		.cmdType = kFLEXSPI_Read,
		.SeqNumber = 1,
		.seqIndex = READ_REG,
		.data = &buffer,
		.dataSize = 2,
	};

	ret = memc_flexspi_transfer(data->controller, &transfer);
	if (ret < 0) {
		return ret;
	}

	buffer &= (~mask  & GENMASK(15, 0));
	buffer |= set_val;

	LOG_DBG("Setting cfg reg to 0x%0x", buffer);

	transfer.cmdType = kFLEXSPI_Write,
	transfer.seqIndex = WRITE_REG;

	ret = memc_flexspi_transfer(data->controller, &transfer);
	return ret;
}

static int memc_flexspi_is66wvq8m4_init(const struct device *dev)
{
	const struct memc_flexspi_is66wvq8m4_config *config = dev->config;
	struct memc_flexspi_is66wvq8m4_data *data = dev->data;
	uint8_t vendor_id;

	if (!device_is_ready(data->controller)) {
		LOG_ERR("Controller device not ready");
		return -ENODEV;
	}

	if (memc_flexspi_set_device_config(data->controller, &config->config,
	    (const uint32_t *) memc_flexspi_is66wvq8m4_lut,
	    sizeof(memc_flexspi_is66wvq8m4_lut) / MEMC_FLEXSPI_CMD_SIZE,
	    config->port)) {
		LOG_ERR("Could not set device configuration");
		return -EINVAL;
	}

	if (memc_flexspi_is66wvq8m4_get_vendor_id(dev, &vendor_id)) {
		LOG_ERR("Could not read vendor id");
		return -EIO;
	}
	LOG_DBG("Vendor id: 0x%0x", vendor_id);
	if (vendor_id != ISSI_VENDOR_ID) {
		LOG_WRN("Vendor ID does not match expected value of 0x%0x",
			ISSI_VENDOR_ID);
	}

	if (memc_flexspi_is66wvq8m4_update_cfg(dev, IS66WVQ8M4_LATENCY_MASK,
						IS66WVQ8M4_LATENCY_FIXED)) {
		LOG_ERR("Could not set fixed latency mode");
		return -EIO;
	}

	return 0;
}

#define CONCAT3(x, y, z) x ## y ## z

#define CS_INTERVAL_UNIT(unit) \
	CONCAT3(kFLEXSPI_CsIntervalUnit, unit, SckCycle)

#define AHB_WRITE_WAIT_UNIT(unit) \
	CONCAT3(kFLEXSPI_AhbWriteWaitUnit, unit, AhbCycle)

#define MEMC_FLEXSPI_DEVICE_CONFIG(n)					\
	{								\
		.flexspiRootClk = DT_INST_PROP(n, spi_max_frequency),	\
		.isSck2Enabled = false,					\
		.flashSize = DT_INST_PROP(n, size) / 8 / KB(1),		\
		.addressShift = DT_INST_REG_ADDR(n) != 0,		\
		.CSIntervalUnit =					\
			CS_INTERVAL_UNIT(				\
				DT_INST_PROP(n, cs_interval_unit)),	\
		.CSInterval = DT_INST_PROP(n, cs_interval),		\
		.CSHoldTime = DT_INST_PROP(n, cs_hold_time),		\
		.CSSetupTime = DT_INST_PROP(n, cs_setup_time),		\
		.dataValidTime = DT_INST_PROP(n, data_valid_time),	\
		.columnspace = DT_INST_PROP(n, column_space),		\
		.enableWordAddress = DT_INST_PROP(n, word_addressable),	\
		.AWRSeqIndex = WRITE_DATA,				\
		.AWRSeqNumber = 1,					\
		.ARDSeqIndex = READ_DATA,				\
		.ARDSeqNumber = 1,					\
		.AHBWriteWaitUnit =					\
			AHB_WRITE_WAIT_UNIT(				\
				DT_INST_PROP(n, ahb_write_wait_unit)),	\
		.AHBWriteWaitInterval =					\
			DT_INST_PROP(n, ahb_write_wait_interval),	\
		.enableWriteMask = true,				\
	}

#define MEMC_FLEXSPI_IS66WVQ8M4(n)				  \
	static const struct memc_flexspi_is66wvq8m4_config	  \
		memc_flexspi_is66wvq8m4_config_##n = {		  \
		.port = DT_INST_REG_ADDR(n),			  \
		.config = MEMC_FLEXSPI_DEVICE_CONFIG(n),	  \
	};							  \
								  \
	static struct memc_flexspi_is66wvq8m4_data		  \
		memc_flexspi_is66wvq8m4_data_##n = {		  \
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),	  \
	};							  \
								  \
	DEVICE_DT_INST_DEFINE(n,				  \
			      memc_flexspi_is66wvq8m4_init,	  \
			      NULL,				  \
			      &memc_flexspi_is66wvq8m4_data_##n,  \
			      &memc_flexspi_is66wvq8m4_config_##n,\
			      POST_KERNEL,			  \
			      CONFIG_MEMC_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_FLEXSPI_IS66WVQ8M4)
