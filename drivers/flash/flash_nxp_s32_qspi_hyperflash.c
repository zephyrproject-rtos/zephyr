/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_qspi_hyperflash

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include <Qspi_Ip.h>

#include "memc_nxp_s32_qspi.h"
#include "flash_nxp_s32_qspi.h"

LOG_MODULE_REGISTER(nxp_s32_qspi_hyperflash, CONFIG_FLASH_LOG_LEVEL);

/* Use the fixed command sets from Qspi_Ip_Hyperflash.c */
extern Qspi_Ip_InstrOpType QSPI_IP_HF_LUT_NAME[QSPI_IP_HF_LUT_SIZE];

static int nxp_s32_qspi_init(const struct device *dev)
{
	struct nxp_s32_qspi_data *data = dev->data;
	const struct nxp_s32_qspi_config *config = dev->config;
	Qspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);
	uint8_t dev_id[memory_cfg->readIdSettings.readIdSize];
	Qspi_Ip_StatusType status;
	int ret = 0;

	/* Used by the HAL to retrieve the internal driver state */
	data->instance = nxp_s32_qspi_register_device();
	__ASSERT_NO_MSG(data->instance < QSPI_IP_MEM_INSTANCE_COUNT);
	data->memory_conn_cfg.qspiInstance = memc_nxp_s32_qspi_get_instance(config->controller);

#if defined(CONFIG_MULTITHREADING)
	k_sem_init(&data->sem, 1, 1);
#endif

	if (!device_is_ready(config->controller)) {
		LOG_ERR("Memory control device not ready");
		return -ENODEV;
	}

	status = Qspi_Ip_Init(data->instance, (const Qspi_Ip_MemoryConfigType *)memory_cfg,
			      (const Qspi_Ip_MemoryConnectionType *)&data->memory_conn_cfg);
	if (status != STATUS_QSPI_IP_SUCCESS) {
		LOG_ERR("Fail to init memory device %d (%d)", data->instance, status);
		return -EIO;
	}

	/* Verify connectivity by reading the device ID */
	ret = nxp_s32_qspi_read_id(dev, dev_id);
	if (ret != 0) {
		LOG_ERR("Device ID read failed (%d)", ret);
		return -ENODEV;
	}

	if (memcmp(dev_id, memory_cfg->readIdSettings.readIdExpected, sizeof(dev_id))) {
		LOG_ERR("Device id does not match config");
		return -EINVAL;
	}

	return ret;
}

static DEVICE_API(flash, nxp_s32_qspi_api) = {
	.erase = nxp_s32_qspi_erase,
	.write = nxp_s32_qspi_write,
	.read = nxp_s32_qspi_read,
	.get_parameters = nxp_s32_qspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = nxp_s32_qspi_pages_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

#define QSPI_PAGE_LAYOUT(n)							\
	.layout = {								\
		.pages_count = (DT_INST_PROP(n, size) / 8)			\
			/ CONFIG_FLASH_NXP_S32_QSPI_LAYOUT_PAGE_SIZE,		\
		.pages_size = CONFIG_FLASH_NXP_S32_QSPI_LAYOUT_PAGE_SIZE,	\
	}

#define QSPI_READ_ID_CFG(n)							\
	{									\
		.readIdLut = QSPI_IP_HF_LUT_READ,				\
		.readIdSize = DT_INST_PROP_LEN(n, jedec_id),			\
		.readIdExpected = DT_INST_PROP(n, jedec_id),			\
	}

#define QSPI_MEMORY_CONN_CFG(n)							\
	{									\
		.connectionType = (Qspi_Ip_ConnectionType)DT_INST_REG_ADDR(n),	\
		.memAlignment = DT_INST_PROP(n, write_block_size)		\
	}

#define QSPI_ERASE_CFG(n)							\
	{									\
		.eraseTypes = {							\
			{							\
				.eraseLut = QSPI_IP_HF_LUT_SE,			\
				.size = 12, /* 4 KB */				\
			},							\
			{							\
				.eraseLut = QSPI_IP_HF_LUT_SE,			\
				.size = 18, /* 256 KB */			\
			},							\
			{							\
				.eraseLut = QSPI_IP_LUT_INVALID,		\
				.size = 0,					\
			},							\
			{							\
				.eraseLut = QSPI_IP_LUT_INVALID,		\
				.size = 0,					\
			},							\
		},								\
		.chipEraseLut = QSPI_IP_HF_LUT_CE,				\
	}

#define QSPI_RESET_CFG(n)							\
	{									\
		.resetCmdLut = QSPI_IP_HF_LUT_RST,				\
		.resetCmdCount = QSPI_IP_HF_RST_CNT,				\
	}

#define QSPI_STATUS_REG_CFG(n)							\
	{									\
		.statusRegInitReadLut = QSPI_IP_HF_LUT_RDSR,			\
		.statusRegReadLut = QSPI_IP_HF_LUT_RDSR,			\
		.statusRegWriteLut = QSPI_IP_LUT_INVALID,			\
		.writeEnableSRLut = QSPI_IP_LUT_INVALID,			\
		.writeEnableLut = QSPI_IP_LUT_INVALID,				\
		.regSize = 1U,							\
		.busyOffset = 0U,						\
		.busyValue = 1U,						\
		.writeEnableOffset = 1U,					\
	}

#define QSPI_INIT_CFG(n)							\
	{									\
		.opCount = 0U,							\
		.operations = NULL,						\
	}

#define QSPI_LUT_CFG(n)								\
	{									\
		.opCount = QSPI_IP_HF_LUT_SIZE,					\
		.lutOps = (Qspi_Ip_InstrOpType *)QSPI_IP_HF_LUT_NAME,		\
	}

#define QSPI_SUSPEND_CFG(n)							\
	{									\
		.eraseSuspendLut = QSPI_IP_HF_LUT_ES,				\
		.eraseResumeLut = QSPI_IP_HF_LUT_ER,				\
		.programSuspendLut = QSPI_IP_HF_LUT_PS,				\
		.programResumeLut = QSPI_IP_HF_LUT_PR,				\
	}

#define QSPI_MEMORY_CFG(n)							\
	{									\
		.memType = QSPI_IP_HYPER_FLASH,					\
		.hfConfig = &hyperflash_config_##n,				\
		.memSize = DT_INST_PROP(n, size) / 8,				\
		.pageSize = DT_INST_PROP(n, max_program_buffer_size),		\
		.writeLut = QSPI_IP_HF_LUT_WRITE,				\
		.readLut = QSPI_IP_HF_LUT_READ,					\
		.read0xxLut = QSPI_IP_LUT_INVALID,				\
		.read0xxLutAHB = QSPI_IP_LUT_INVALID,				\
		.eraseSettings = QSPI_ERASE_CFG(n),				\
		.statusConfig = QSPI_STATUS_REG_CFG(n),				\
		.resetSettings = QSPI_RESET_CFG(n),				\
		.initResetSettings = QSPI_RESET_CFG(n),				\
		.initConfiguration = QSPI_INIT_CFG(n),				\
		.lutSequences = QSPI_LUT_CFG(n),				\
		.readIdSettings = QSPI_READ_ID_CFG(n),				\
		.suspendSettings = QSPI_SUSPEND_CFG(n),				\
		.initCallout = NULL,						\
		.resetCallout = NULL,						\
		.errorCheckCallout = NULL,					\
		.eccCheckCallout = NULL,					\
		.ctrlAutoCfgPtr = NULL,						\
	}

#define FLASH_NXP_S32_QSPI_DRV_STRENGTH(n)							\
	COND_CODE_1(DT_INST_ENUM_IDX(n, vcc_mv),						\
		(DT_INST_PROP(n, drive_strength_ohm) == 12 ? QSPI_IP_HF_DRV_STRENGTH_007 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 14 ? QSPI_IP_HF_DRV_STRENGTH_006 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 16 ? QSPI_IP_HF_DRV_STRENGTH_005 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 20 ? QSPI_IP_HF_DRV_STRENGTH_000 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 27 ? QSPI_IP_HF_DRV_STRENGTH_003 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 40 ? QSPI_IP_HF_DRV_STRENGTH_002 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 71 ? QSPI_IP_HF_DRV_STRENGTH_001 :	\
		QSPI_IP_HF_DRV_STRENGTH_000))))))),						\
		(DT_INST_PROP(n, drive_strength_ohm) == 20 ? QSPI_IP_HF_DRV_STRENGTH_007 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 24 ? QSPI_IP_HF_DRV_STRENGTH_006 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 27 ? QSPI_IP_HF_DRV_STRENGTH_000 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 34 ? QSPI_IP_HF_DRV_STRENGTH_004 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 45 ? QSPI_IP_HF_DRV_STRENGTH_003 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 68 ? QSPI_IP_HF_DRV_STRENGTH_002 :	\
		(DT_INST_PROP(n, drive_strength_ohm) == 117 ? QSPI_IP_HF_DRV_STRENGTH_001 :	\
		QSPI_IP_HF_DRV_STRENGTH_000))))))))

#define FLASH_NXP_S32_QSPI_SECTOR_MAP(n)							\
	COND_CODE_1(DT_INST_PROP(n, support_only_uniform_sectors),				\
		(DT_INST_ENUM_IDX(n, ppw_sectors_addr_mapping) ?				\
			QSPI_IP_HF_UNIFORM_SECTORS_READ_PASSWORD_HIGH :				\
			QSPI_IP_HF_UNIFORM_SECTORS_READ_PASSWORD_LOW),				\
		(DT_INST_ENUM_IDX(n, ppw_sectors_addr_mapping) ?				\
			QSPI_IP_HF_PARAM_AND_PASSWORD_MAP_HIGH :				\
			QSPI_IP_HF_PARAM_AND_PASSWORD_MAP_LOW))

#define FLASH_NXP_S32_QSPI_INIT_DEVICE(n)					\
	static Qspi_Ip_HyperFlashConfigType hyperflash_config_##n =		\
	{									\
		.outputDriverStrength = FLASH_NXP_S32_QSPI_DRV_STRENGTH(n),	\
		.RWDSLowOnDualError = DT_INST_PROP(n, rwds_low_dual_error),	\
		.secureRegionUnlocked = !DT_INST_PROP(n, secure_region_locked),	\
		.readLatency = DT_INST_ENUM_IDX(n, read_latency_cycles),	\
		.paramSectorMap = FLASH_NXP_S32_QSPI_SECTOR_MAP(n),		\
		.deviceIdWordAddress = DT_INST_PROP(n, device_id_word_addr),	\
	};									\
	static const struct nxp_s32_qspi_config nxp_s32_qspi_config_##n = {	\
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),			\
		.flash_parameters = {						\
			.write_block_size = DT_INST_PROP(n, write_block_size),	\
			.erase_value = QSPI_ERASE_VALUE,			\
		},								\
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,				\
			(QSPI_PAGE_LAYOUT(n),))					\
		.memory_cfg = QSPI_MEMORY_CFG(n),				\
	};									\
										\
	static struct nxp_s32_qspi_data nxp_s32_qspi_data_##n = {		\
		.memory_conn_cfg = QSPI_MEMORY_CONN_CFG(n),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      nxp_s32_qspi_init,				\
			      NULL,						\
			      &nxp_s32_qspi_data_##n,				\
			      &nxp_s32_qspi_config_##n,				\
			      POST_KERNEL,					\
			      CONFIG_FLASH_INIT_PRIORITY,			\
			      &nxp_s32_qspi_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_NXP_S32_QSPI_INIT_DEVICE)
