/*
 * Copyright (c) 2022 Georgij Cernysiov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_fmc_nor_psram

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_stm32_nor_psram, CONFIG_MEMC_LOG_LEVEL);

/** FMC NOR/PSRAM controller bank configuration fields. */
struct memc_stm32_nor_psram_bank_config {
	FMC_NORSRAM_InitTypeDef init;
	FMC_NORSRAM_TimingTypeDef timing;
	FMC_NORSRAM_TimingTypeDef timing_ext;
};

/** FMC NOR/PSRAM controller configuration fields. */
struct memc_stm32_nor_psram_config {
	const struct memc_stm32_nor_psram_bank_config *banks;
	size_t banks_len;
};

static int memc_stm32_nor_init(const struct memc_stm32_nor_psram_config *config,
			       const struct memc_stm32_nor_psram_bank_config *bank_config)
{
	FMC_NORSRAM_TimingTypeDef *ext_timing;
	NOR_HandleTypeDef hnor = {
		.Instance = FMC_NORSRAM_DEVICE,
		.Extended = FMC_NORSRAM_EXTENDED_DEVICE,
		.Init = bank_config->init
	};

	if (bank_config->init.ExtendedMode == FMC_EXTENDED_MODE_ENABLE) {
		ext_timing = (FMC_NORSRAM_TimingTypeDef *)&bank_config->timing_ext;
	} else {
		ext_timing = NULL;
	}

	if (HAL_NOR_Init(&hnor,
			 (FMC_NORSRAM_TimingTypeDef *)&bank_config->timing,
			 ext_timing) != HAL_OK) {
		return -ENODEV;
	}

	return 0;
}

static int memc_stm32_psram_init(const struct memc_stm32_nor_psram_config *config,
				 const struct memc_stm32_nor_psram_bank_config *bank_config)
{
	FMC_NORSRAM_TimingTypeDef *ext_timing;
	SRAM_HandleTypeDef hsram = {
		.Instance = FMC_NORSRAM_DEVICE,
		.Extended = FMC_NORSRAM_EXTENDED_DEVICE,
		.Init = bank_config->init
	};

	if (bank_config->init.ExtendedMode == FMC_EXTENDED_MODE_ENABLE) {
		ext_timing = (FMC_NORSRAM_TimingTypeDef *)&bank_config->timing_ext;
	} else {
		ext_timing = NULL;
	}

	if (HAL_SRAM_Init(&hsram,
			  (FMC_NORSRAM_TimingTypeDef *)&bank_config->timing,
			  ext_timing) != HAL_OK) {
		return -ENODEV;
	}

	return 0;
}

static int memc_stm32_nor_psram_init(const struct device *dev)
{
	const struct memc_stm32_nor_psram_config *config = dev->config;
	const struct memc_stm32_nor_psram_bank_config *bank_config;
	size_t bank_idx;
	int ret;

	for (bank_idx = 0U; bank_idx < config->banks_len; ++bank_idx) {
		bank_config = &config->banks[bank_idx];

		switch (bank_config->init.MemoryType) {
		case FMC_MEMORY_TYPE_NOR:
			ret = memc_stm32_nor_init(config, bank_config);
			break;
		case FMC_MEMORY_TYPE_PSRAM:
			__fallthrough;
		case FMC_MEMORY_TYPE_SRAM:
			ret = memc_stm32_psram_init(config, bank_config);
			break;
		default:
			ret = -ENOTSUP;
			break;
		}

		if (ret < 0) {
			LOG_ERR("Unable to initialize memory type: "
				"0x%08X, NSBank: %d, err: %d",
				bank_config->init.MemoryType,
				bank_config->init.NSBank, ret);
			return ret;
		}
	}

	return 0;
}

/** SDRAM bank/s configuration initialization macro. */
#define BANK_CONFIG(node_id)                                                    \
	{ .init = {                                                             \
	    .NSBank = DT_REG_ADDR(node_id),                                     \
	    .DataAddressMux = DT_PROP_BY_IDX(node_id, st_control, 0),           \
	    .MemoryType = DT_PROP_BY_IDX(node_id, st_control, 1),               \
	    .MemoryDataWidth = DT_PROP_BY_IDX(node_id, st_control, 2),          \
	    .BurstAccessMode = DT_PROP_BY_IDX(node_id, st_control, 3),          \
	    .WaitSignalPolarity = DT_PROP_BY_IDX(node_id, st_control, 4),       \
	    .WaitSignalActive = DT_PROP_BY_IDX(node_id, st_control, 5),         \
	    .WriteOperation = DT_PROP_BY_IDX(node_id, st_control, 6),           \
	    .WaitSignal = DT_PROP_BY_IDX(node_id, st_control, 7),               \
	    .ExtendedMode = DT_PROP_BY_IDX(node_id, st_control, 8),             \
	    .AsynchronousWait = DT_PROP_BY_IDX(node_id, st_control, 9),         \
	    .WriteBurst = DT_PROP_BY_IDX(node_id, st_control, 10),              \
	    .ContinuousClock = DT_PROP_BY_IDX(node_id, st_control, 11),         \
	    .WriteFifo = DT_PROP_BY_IDX(node_id, st_control, 12),               \
	    .PageSize = DT_PROP_BY_IDX(node_id, st_control, 13)                 \
	  },                                                                    \
	  .timing = {                                                           \
	    .AddressSetupTime = DT_PROP_BY_IDX(node_id, st_timing, 0),          \
	    .AddressHoldTime =  DT_PROP_BY_IDX(node_id, st_timing, 1),          \
	    .DataSetupTime = DT_PROP_BY_IDX(node_id, st_timing, 2),             \
	    .BusTurnAroundDuration = DT_PROP_BY_IDX(node_id, st_timing, 3),     \
	    .CLKDivision = DT_PROP_BY_IDX(node_id, st_timing, 4),               \
	    .DataLatency = DT_PROP_BY_IDX(node_id, st_timing, 5),               \
	    .AccessMode = DT_PROP_BY_IDX(node_id, st_timing, 6),                \
	  },                                                                    \
	  .timing_ext = {                                                       \
	    .AddressSetupTime = DT_PROP_BY_IDX(node_id, st_timing_ext, 0),      \
	    .AddressHoldTime =  DT_PROP_BY_IDX(node_id, st_timing_ext, 1),      \
	    .DataSetupTime = DT_PROP_BY_IDX(node_id, st_timing_ext, 2),         \
	    .BusTurnAroundDuration = DT_PROP_BY_IDX(node_id, st_timing_ext, 3), \
	    .AccessMode = DT_PROP_BY_IDX(node_id, st_timing_ext, 4),            \
	  }                                                                     \
	},

#define BUILD_ASSERT_BANK_CONFIG(node_id)                                       \
	BUILD_ASSERT(IS_FMC_NORSRAM_BANK(DT_REG_ADDR(node_id)),                 \
		     "NSBank " STRINGIFY(DT_REG_ADDR(node_id)) " is not a NORSRAM bank");

DT_INST_FOREACH_CHILD(0, BUILD_ASSERT_BANK_CONFIG);

/** SRAM bank/s configuration. */
static const struct memc_stm32_nor_psram_bank_config bank_config[] = {
	DT_INST_FOREACH_CHILD(0, BANK_CONFIG)
};

/** SRAM configuration. */
static const struct memc_stm32_nor_psram_config config = {
	.banks = bank_config,
	.banks_len = ARRAY_SIZE(bank_config),
};

DEVICE_DT_INST_DEFINE(0, memc_stm32_nor_psram_init, NULL,
		      NULL, &config,
		      POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY,
		      NULL);
