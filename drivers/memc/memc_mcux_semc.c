/*
 * Copyright 2022 Macronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT   nxp_imx_semc

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif
#include <zephyr/drivers/onfi.h>
#include "memc_mcux_semc.h"
#include <stdio.h>

#define SEMC_NAND_AXI_START_ADDRESS        (0x9E000000U)
#define SEMC_NAND_IPG_START_ADDRESS        (0x00000000U)

uint32_t clkSrc_Hz;

LOG_MODULE_REGISTER(nxp_imx_semc, CONFIG_FLASH_LOG_LEVEL);

struct memc_semc_data {
	SEMC_Type *base;
	bool queuebenable;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
};

semc_nand_config_t nand_config = {
	.cePinMux          = kSEMC_MUXCSX0,                     /*!< The CE# pin mux setting. */
	.axiAddress        = SEMC_NAND_AXI_START_ADDRESS,       /*!< The base address. */
	.axiMemsize_kbytes = 2 * 1024 * 1024,                   /*!< The memory size is 8*1024*2*1024*1024 = 16Gb. */
	.ipgAddress        = SEMC_NAND_IPG_START_ADDRESS,       /*!< The base address. */
	.ipgMemsize_kbytes = 2 * 1024 * 1024,                   /*!< The memory size is 8*1024*2*1024*1024 = 16Gb. */
	.rdyactivePolarity = kSEMC_RdyActiveLow,                /*!< Wait ready polarity. */
	.arrayAddrOption   = kSEMC_NandAddrOption_5byte_CA2RA3,
	.edoModeEnabled    = false,                             /*!< Address mode. */
	.columnAddrBitNum  = kSEMC_NandColum_12bit,             /*!< 12bit + 1bit to access the spare area. */
	.burstLen          = kSEMC_Nand_BurstLen64,             /*!< Burst length. */
	.portSize          = kSEMC_PortSize8Bit,                /*!< Port size. */
	.timingConfig      = NULL,
};

static int memc_semc_init(const struct device *dev)
{
	struct memc_semc_data *data = dev->data;

#if defined(CONFIG_PINCTRL)
	int ret;

	ret = pinctrl_apply_state(data->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
#endif
	clock_root_config_t clock_config = {0};
	clock_config.mux                 = 7;
	clock_config.div                 = 4;

	CLOCK_SetRootClock(kCLOCK_Root_Semc, &clock_config);

	clkSrc_Hz = CLOCK_GetRootClockFreq(kCLOCK_Root_Semc);

	semc_config_t config;

	/* Initializes the MAC configure structure to zero. */
	memset(&config, 0, sizeof(semc_config_t));

	/*
   	Get default configuration:
   	config->queueWeight.queueaEnable = true;
   	config->queueWeight.queuebEnable = true;
	*/
	SEMC_GetDefaultConfig(&config);
	/* Disable queue B weight, which is for AXI bus access to SDRAM slave. */
	config.queueWeight.queuebEnable = data->queuebenable;
	/* Initialize SEMC. */
	SEMC_Init(data->base, &config);

	return 0;
}

bool memc_semc_is_nand_ready(const struct device *dev)
{
	struct memc_semc_data *data = dev->data;

	SEMC_IsNandReady(data->base);

	return 0;
}

status_t memc_semc_send_ipcommand(const struct device *dev, uint32_t address, uint32_t command, uint32_t write, uint32_t *read)
{
	struct memc_semc_data *data = dev->data;

	SEMC_SendIPCommand(data->base, kSEMC_MemType_NAND, address, command, write, read);

	return 0;
}

status_t memc_semc_ipcommand_nand_write(const struct device *dev, uint32_t address, uint8_t *buffer, uint32_t size_bytes)
{
	struct memc_semc_data *data = dev->data;

	SEMC_IPCommandNandWrite(data->base, address, buffer, size_bytes);

	return 0;
}

status_t memc_semc_ipcommand_nand_read(const struct device *dev, uint32_t address, uint8_t *buffer, uint32_t size_bytes)
{
	struct memc_semc_data *data = dev->data;

	SEMC_IPCommandNandRead(data->base, address, buffer, size_bytes);

	return 0;
}

status_t memc_semc_configure_nand(const struct device *dev, onfi_nand_config_t *config)
{
	struct memc_semc_data *data = dev->data;

	nand_config.edoModeEnabled = config->edoModeEnabled;
	nand_config.timingConfig = config->timingConfig;

	switch (config->addressCycle) {
		case 0x22u:
			nand_config.arrayAddrOption = kSEMC_NandAddrOption_4byte_CA2RA2;
			break;
		case 0x21u:
			nand_config.arrayAddrOption = kSEMC_NandAddrOption_3byte_CA2RA1;
			break;
		case 0x13u:
			nand_config.arrayAddrOption = kSEMC_NandAddrOption_4byte_CA1RA3;
			break;
		case 0x12u:
			nand_config.arrayAddrOption = kSEMC_NandAddrOption_3byte_CA1RA2;
			break;
		case 0x11u:
			nand_config.arrayAddrOption = kSEMC_NandAddrOption_2byte_CA1RA1;
			break;
		case 0x23u:
		default:
			nand_config.arrayAddrOption = kSEMC_NandAddrOption_5byte_CA2RA3;
			break;
	}

	SEMC_ConfigureNAND(data->base, &nand_config, clkSrc_Hz);

	return 0;
}

static const struct onfi_api semc_nand_api = {
	.is_nand_ready = memc_semc_is_nand_ready,
	.send_command = memc_semc_send_ipcommand,
	.write = memc_semc_ipcommand_nand_write,
	.read = memc_semc_ipcommand_nand_read,
	.configure_nand = memc_semc_configure_nand,
};

PINCTRL_DT_INST_DEFINE(0);

static struct memc_semc_data memc_semc_data_0 = {
	.base = (SEMC_Type *) DT_INST_REG_ADDR(0),
	.queuebenable = false,
#ifdef CONFIG_PINCTRL
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
#endif
};

DEVICE_DT_INST_DEFINE(0, &memc_semc_init,  NULL, &memc_semc_data_0, NULL, POST_KERNEL, 80, &semc_nand_api);
