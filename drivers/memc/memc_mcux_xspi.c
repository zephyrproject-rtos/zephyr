/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_xspi

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <soc.h>
#include "memc_mcux_xspi.h"

LOG_MODULE_REGISTER(memc_mcux_xspi, CONFIG_MEMC_LOG_LEVEL);

#define MEMC_XSPI_TARGET_GROUP_COUNT 2
#define MEMC_XSPI_SFP_FRAD_COUNT     8

struct memc_mcux_xspi_config {
	const struct pinctrl_dev_config *pincfg;
	xspi_config_t xspi_config;
	xspi_sfp_mdad_config_t mdad_configs;
	bool mdad_valid;
	xspi_sfp_frad_config_t frad_configs;
	bool frad_valid;
};

struct memc_mcux_xspi_data {
	XSPI_Type *base;
	bool xip;
	uint32_t amba_address;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

void memc_mcux_xspi_update_device_addr_mode(const struct device *dev,
						xspi_device_addr_mode_t addr_mode)
{
	XSPI_Type *base = ((struct memc_mcux_xspi_data *)dev->data)->base;

	XSPI_UpdateDeviceAddrMode(base, addr_mode);
}

int memc_mcux_xspi_get_root_clock(const struct device *dev, uint32_t *clock_rate)
{
	struct memc_mcux_xspi_data *data = dev->data;

	return clock_control_get_rate(data->clock_dev, data->clock_subsys, clock_rate);
}

void memc_xspi_wait_bus_idle(const struct device *dev)
{
	struct memc_mcux_xspi_data *data = dev->data;

	while (!XSPI_GetBusIdleStatus(data->base)) {
	}
}

int memc_xspi_set_device_config(const struct device *dev, const xspi_device_config_t *device_config,
				const uint32_t *lut_array, uint8_t lut_count)
{
	struct memc_mcux_xspi_data *data = dev->data;
	XSPI_Type *base = data->base;
	status_t status;

	/* Configure flash settings according to serial flash feature. */
	status = XSPI_SetDeviceConfig(base, (xspi_device_config_t *)device_config);
	if (status != kStatus_Success) {
		LOG_ERR("XSPI_SetDeviceConfig failed with status %u\n", status);
		return -ENODEV;
	}

	XSPI_UpdateLUT(base, 0, lut_array, lut_count);

	return 0;
}

uint32_t memc_mcux_xspi_get_ahb_address(const struct device *dev)
{
	return ((const struct memc_mcux_xspi_data *)dev->data)->amba_address;
}

int memc_mcux_xspi_transfer(const struct device *dev, xspi_transfer_t *xfer)
{
	XSPI_Type *base = ((struct memc_mcux_xspi_data *)dev->data)->base;
	status_t status;

	status = XSPI_TransferBlocking(base, xfer);

	return (status == kStatus_Success) ? 0 : -EIO;
}

bool memc_xspi_is_running_xip(const struct device *dev)
{
	return ((const struct memc_mcux_xspi_data *)dev->data)->xip;
}

static int memc_mcux_xspi_init(const struct device *dev)
{
	const struct memc_mcux_xspi_config *memc_xspi_config = dev->config;
	const struct pinctrl_dev_config *pincfg = memc_xspi_config->pincfg;
	xspi_config_t config = memc_xspi_config->xspi_config;
	XSPI_Type *base = ((struct memc_mcux_xspi_data *)dev->data)->base;
	int ret;

	if ((memc_xspi_is_running_xip(dev)) && (!IS_ENABLED(CONFIG_MEMC_MCUX_XSPI_INIT_XIP))) {
		LOG_DBG("XIP active on %s, skipping init\n", dev->name);
		return 0;
	}

	ret = pinctrl_apply_state(pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d", ret);
		return ret;
	}

	config.ptrAhbAccessConfig->ahbAlignment = kXSPI_AhbAlignmentNoLimit;
	config.ptrAhbAccessConfig->ahbSplitSize = kXSPI_AhbSplitSizeDisabled;

	for (uint8_t i = 0U; i < XSPI_BUFCR_COUNT; i++) {
		config.ptrAhbAccessConfig->buffer[i].masterId = i;
		if (i == (XSPI_BUFCR_COUNT - 1U)) {
			config.ptrAhbAccessConfig->buffer[i].enaPri.enableAllMaster = true;
		} else {
			config.ptrAhbAccessConfig->buffer[i].enaPri.enablePriority = false;
		}
		config.ptrAhbAccessConfig->buffer[i].bufferSize = 0x80U;
		config.ptrAhbAccessConfig->buffer[i].ptrSubBuffer0Config = NULL;
		config.ptrAhbAccessConfig->buffer[i].ptrSubBuffer1Config = NULL;
		config.ptrAhbAccessConfig->buffer[i].ptrSubBuffer2Config = NULL;
		config.ptrAhbAccessConfig->buffer[i].ptrSubBuffer3Config = NULL;
	}

	if (memc_xspi_config->mdad_valid) {
		config.ptrIpAccessConfig->ptrSfpMdadConfig =
			(xspi_sfp_mdad_config_t *)&memc_xspi_config->mdad_configs;
	}
	if (memc_xspi_config->frad_valid) {
		config.ptrIpAccessConfig->ptrSfpFradConfig =
			(xspi_sfp_frad_config_t *)&memc_xspi_config->frad_configs;
	}

	XSPI_Init(base, &config);

	return 0;
}

#if defined(CONFIG_XIP) && defined(CONFIG_FLASH_MCUX_XSPI_XIP)
/* Checks if image flash base address is in the XSPI AHB base region */
#define MEMC_XSPI_CFG_XIP(n) \
	((CONFIG_FLASH_BASE_ADDRESS) >= DT_INST_REG_ADDR_BY_IDX(n, 1)) && \
		((CONFIG_FLASH_BASE_ADDRESS) < \
		 (DT_INST_REG_ADDR_BY_IDX(n, 1) + DT_INST_REG_SIZE_BY_IDX(n, 1)))

#else
#define MEMC_XSPI_CFG_XIP(node_id) false
#endif

#define MCUX_XSPI_GET_MDAD(n, idx, x)	\
	DT_PROP(DT_CHILD(DT_DRV_INST(n), mdad_tg ## idx), x)
#define MCUX_XSPI_GET_FRAD(n, idx, x)	\
	DT_PROP(DT_CHILD(DT_DRV_INST(n), frad_region ## idx), x)

#define MCUX_XSPI_MDAD_INIT(idx, n)	\
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(n), mdad_tg ## idx)),	\
		({	\
			.enableDescriptorLock =	\
				MCUX_XSPI_GET_MDAD(n, idx, enable_descriptor_lock),	\
			.maskType = MCUX_XSPI_GET_MDAD(n, idx, mask_type),	\
			.mask = MCUX_XSPI_GET_MDAD(n, idx, mask),	\
			.masterIdReference =	\
				MCUX_XSPI_GET_MDAD(n, idx, master_id_reference),	\
			.secureAttribute =	\
				MCUX_XSPI_GET_MDAD(n, idx, secure_attribute),	\
		}),	\
		({	\
			0	\
		}))

#define MCUX_XSPI_FRAD_INIT(idx, n)	\
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(n), frad_region ## idx)),	\
		({	\
			.startAddress = MCUX_XSPI_GET_FRAD(n, idx, start_address), \
			.endAddress = MCUX_XSPI_GET_FRAD(n, idx, end_address),	\
			.tg0MasterAccess =	\
				MCUX_XSPI_GET_FRAD(n, idx, tg0_master_access),	\
			.tg1MasterAccess =	\
				MCUX_XSPI_GET_FRAD(n, idx, tg1_master_access),	\
			.assignIsValid = true,	\
			.descriptorLock =	\
				MCUX_XSPI_GET_FRAD(n, idx, descriptor_lock),	\
			.exclusiveAccessLock =	\
				MCUX_XSPI_GET_FRAD(n, idx, exclusive_access_lock),	\
		}),	\
		({	\
			0	\
		}))

#define MCUX_XSPI_INIT(n)	\
	PINCTRL_DT_INST_DEFINE(n);	\
	static const struct memc_mcux_xspi_config memc_mcux_xspi_config_##n = {	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),	\
		.xspi_config = {	\
			.byteOrder = DT_INST_PROP(n, byte_order),	\
			.enableDoze = false,	\
			.ptrAhbAccessConfig = &(xspi_ahb_access_config_t){	\
				.ahbErrorPayload = {	\
					.highPayload = 0x5A5A5A5A,	\
					.lowPayload = 0x5A5A5A5A,	\
				},	\
				.ARDSeqIndex = 0,	\
				.enableAHBBufferWriteFlush =	\
					DT_INST_PROP(n, ahb_buffer_write_flush),	\
				.enableAHBPrefetch = DT_INST_PROP(n, ahb_prefetch),	\
				.ptrAhbWriteConfig = DT_INST_PROP(n, enable_ahb_write) ?	\
					&(xspi_ahb_write_config_t){	\
						.AWRSeqIndex = 1,	\
						.ARDSRSeqIndex = 0,	\
						.blockRead = false,	\
						.blockSequenceWrite = false,	\
					} : NULL,	\
			},	\
			.ptrIpAccessConfig = &(xspi_ip_access_config_t){	\
				.ipAccessTimeoutValue = 0xFFFFFFFF,	\
				.ptrSfpFradConfig = NULL,	\
				.ptrSfpMdadConfig = NULL,	\
				.sfpArbitrationLockTimeoutValue = 0xFFFFFF,	\
			},	\
		},	\
		.mdad_configs = {	\
			.tgMdad = {	\
				LISTIFY(MEMC_XSPI_TARGET_GROUP_COUNT,	\
					MCUX_XSPI_MDAD_INIT, (,), n)	\
			},	\
		},	\
		.mdad_valid = DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(n), mdad_tg0)),	\
		.frad_configs = {	\
			.fradConfig = {	\
				LISTIFY(MEMC_XSPI_SFP_FRAD_COUNT, MCUX_XSPI_FRAD_INIT, (,), n)	\
			},	\
		},	\
		.frad_valid = DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(n), frad_region0)),	\
	};	\
	static struct memc_mcux_xspi_data memc_mcux_xspi_data_##n = {	\
		.base = (XSPI_Type *)DT_INST_REG_ADDR(n),	\
		.xip = MEMC_XSPI_CFG_XIP(n),	\
		.amba_address = DT_INST_REG_ADDR_BY_IDX(n, 1),	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
	};	\
	DEVICE_DT_INST_DEFINE(n, &memc_mcux_xspi_init, NULL,	\
		&memc_mcux_xspi_data_##n, &memc_mcux_xspi_config_##n,	\
		POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MCUX_XSPI_INIT)
