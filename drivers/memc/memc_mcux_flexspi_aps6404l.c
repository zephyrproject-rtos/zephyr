/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /*
  * Based on memc_mcux_flexspi_s27ks0641, which is: Copyright 2021 Basalte bv
  */

 #define DT_DRV_COMPAT nxp_imx_flexspi_aps6404l

 #include <zephyr/kernel.h>
 #include <zephyr/logging/log.h>
 #include <zephyr/sys/util.h>

 #include "memc_mcux_flexspi.h"


/*
 * NOTE: If CONFIG_FLASH_MCUX_FLEXSPI_XIP is selected, Any external functions
 * called while interacting with the flexspi MUST be relocated to SRAM or ITCM
 * at runtime, so that the chip does not access the flexspi to read program
 * instructions while it is being written to
 */
#if defined(CONFIG_FLASH_MCUX_FLEXSPI_XIP) && (CONFIG_MEMC_LOG_LEVEL > 0)
#warning "Enabling memc driver logging and XIP mode simultaneously can cause \
	read-while-write hazards. This configuration is not recommended."
#endif

LOG_MODULE_REGISTER(memc_flexspi_aps6404l, CONFIG_MEMC_LOG_LEVEL);

#define APM_VENDOR_ID 0xD

enum {
	READ_DATA = 0,
	WRITE_DATA,
	RESET_EN,
	RESET,
	READ_ID
};

struct memc_flexspi_aps6404l_config {
	flexspi_port_t port;
	flexspi_device_config_t config;
};

/* Device variables used in critical sections should be in this structure */
struct memc_flexspi_aps6404l_data {
	const struct device *controller;
};


static const uint32_t memc_flexspi_aps6404l_lut[][4] = {
	/* Read Data (Sync read, linear burst) */
	[READ_DATA] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xEB,
			kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 0x18),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_4PAD,
			0x06, kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x04),
	},
	/* Write Data (Sync write, linear burst) */
	[WRITE_DATA] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x38,
			kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 0x18),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD,
			0x00, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),
	},

	[RESET_EN] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x66,
			kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),
	},
	/* Reset (Global reset) */
	[RESET] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x99,
			kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),
	},

	[READ_ID] = {
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x9F,
		kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x08,
		kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	},
};


static int memc_flexspi_aps6404l_get_vendor_id(const struct device *dev,
						uint8_t *vendor_id)
{
	const struct memc_flexspi_aps6404l_config *config = dev->config;
	struct memc_flexspi_aps6404l_data *data = dev->data;
	uint32_t buffer = 0;
	int ret;

	flexspi_transfer_t transfer = {
		.deviceAddress = 0x0,
		.port = config->port,
		.cmdType = kFLEXSPI_Read,
		.SeqNumber = 1,
		.seqIndex = READ_ID,
		.data = &buffer,
		.dataSize = 1,
	};

	ret = memc_flexspi_transfer(data->controller, &transfer);
	*vendor_id = buffer & 0x1f;

	return ret;
}

static int memc_flexspi_aps6404l_reset_enable(const struct device *dev)
{
	const struct memc_flexspi_aps6404l_config *config = dev->config;
	struct memc_flexspi_aps6404l_data *data = dev->data;
	int ret;

	flexspi_transfer_t transfer = {
		.deviceAddress = 0x0,
		.port = config->port,
		.cmdType = kFLEXSPI_Command,
		.SeqNumber = 1,
		.seqIndex = RESET_EN,
		.data = NULL,
		.dataSize = 0,
	};

	LOG_DBG("Enabling reset ram");
	ret = memc_flexspi_transfer(data->controller, &transfer);
	if (ret < 0) {
		return ret;
	}
	/* We need to delay 5 ms to allow APS6404L pSRAM to reinitialize */
	k_msleep(5);

	return ret;
}


static int memc_flexspi_aps6404l_reset(const struct device *dev)
{
	const struct memc_flexspi_aps6404l_config *config = dev->config;
	struct memc_flexspi_aps6404l_data *data = dev->data;
	int ret;

	flexspi_transfer_t transfer = {
		.deviceAddress = 0x0,
		.port = config->port,
		.cmdType = kFLEXSPI_Command,
		.SeqNumber = 1,
		.seqIndex = RESET,
		.data = NULL,
		.dataSize = 0,
	};

	LOG_DBG("Resetting ram");
	ret = memc_flexspi_transfer(data->controller, &transfer);
	if (ret < 0) {
		return ret;
	}
	/* We need to delay 5 ms to allow APS6404L pSRAM to reinitialize */
	k_msleep(5);

	return ret;
}

static int memc_flexspi_aps6404l_init(const struct device *dev)
{
	const struct memc_flexspi_aps6404l_config *config = dev->config;
	struct memc_flexspi_aps6404l_data *data = dev->data;
	uint8_t vendor_id;

	if (!device_is_ready(data->controller)) {
		LOG_ERR("Controller device not ready");
		return -ENODEV;
	}

	if (memc_flexspi_set_device_config(data->controller, &config->config,
	    (const uint32_t *) memc_flexspi_aps6404l_lut,
	    sizeof(memc_flexspi_aps6404l_lut) / MEMC_FLEXSPI_CMD_SIZE,
	    config->port)) {
		LOG_ERR("Could not set device configuration");
		return -EINVAL;
	}

	memc_flexspi_reset(data->controller);

	if (memc_flexspi_aps6404l_reset_enable(dev)) {
		LOG_ERR("Could not enable reset pSRAM");
		return -EIO;
	}

	if (memc_flexspi_aps6404l_reset(dev)) {
		LOG_ERR("Could not reset pSRAM");
		return -EIO;
	}

	if (memc_flexspi_aps6404l_get_vendor_id(dev, &vendor_id)) {
		LOG_ERR("Could not read vendor id");
		return -EIO;
	}
	LOG_DBG("Vendor id: 0x%0x", vendor_id);
	if (vendor_id != APM_VENDOR_ID) {
		LOG_WRN("Vendor ID does not match expected value of 0x%0x",
			APM_VENDOR_ID);
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
		.addressShift = false,					\
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
		.enableWriteMask = false,				\
	}								\

#define MEMC_FLEXSPI_APS6404L(n)				  \
	static const struct memc_flexspi_aps6404l_config	  \
		memc_flexspi_aps6404l_config_##n = {		  \
		.port = DT_INST_REG_ADDR(n),			  \
		.config = MEMC_FLEXSPI_DEVICE_CONFIG(n),	  \
	};							  \
								  \
	static struct memc_flexspi_aps6404l_data		  \
		memc_flexspi_aps6404l_data_##n = {		  \
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),	  \
	};							  \
								  \
	DEVICE_DT_INST_DEFINE(n,				  \
			      memc_flexspi_aps6404l_init,	  \
			      NULL,				  \
			      &memc_flexspi_aps6404l_data_##n,  \
			      &memc_flexspi_aps6404l_config_##n,  \
			      POST_KERNEL,			  \
			      CONFIG_MEMC_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_FLEXSPI_APS6404L)
