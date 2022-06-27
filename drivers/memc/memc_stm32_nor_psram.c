/*
 * Copyright (c) 2022 Georgij Cernysiov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_fmc_nor_psram

#include <zephyr/device.h>
#include <soc.h>
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_stm32_nor_psram, CONFIG_MEMC_LOG_LEVEL);

/** SRAM base register offset, see FMC_Bank1_R_BASE */
#define SRAM_OFFSET 0x0000UL
/** SRAM extended mode register offset, see FMC_Bank1E_R_BASE */
#define SRAM_EXT_OFFSET 0x0104UL

/** FMC NOR/PSRAM controller bank configuration fields. */
struct memc_stm32_nor_psram_bank_config {
	FMC_NORSRAM_InitTypeDef init;
	FMC_NORSRAM_TimingTypeDef timing;
	FMC_NORSRAM_TimingTypeDef timing_ext;
};

/** FMC NOR/PSRAM controller configuration fields. */
struct memc_stm32_nor_psram_config {
	FMC_NORSRAM_TypeDef *nor_psram;
	FMC_NORSRAM_EXTENDED_TypeDef *extended;
	const struct memc_stm32_nor_psram_bank_config *banks;
	size_t banks_len;
};

static int memc_stm32_nor_init(const struct memc_stm32_nor_psram_config *config,
			       const struct memc_stm32_nor_psram_bank_config *bank_config)
{
	FMC_NORSRAM_TimingTypeDef *ext_timing;
	NOR_HandleTypeDef hnor = { 0 };

	hnor.Instance = config->nor_psram;
	hnor.Extended = config->extended;

	memcpy(&hnor.Init, &bank_config->init, sizeof(hnor.Init));

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
	SRAM_HandleTypeDef hsram = { 0 };

	hsram.Instance = config->nor_psram;
	hsram.Extended = config->extended;

	memcpy(&hsram.Init, &bank_config->init, sizeof(hsram.Init));

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
	uint32_t memory_type;
	size_t bank_idx;
	int ret = 0;

	for (bank_idx = 0U; bank_idx < config->banks_len; ++bank_idx) {
		memory_type = config->banks[bank_idx].init.MemoryType;

		switch (memory_type) {
		case FMC_MEMORY_TYPE_NOR:
			ret = memc_stm32_nor_init(config, &config->banks[bank_idx]);
			break;
		case FMC_MEMORY_TYPE_PSRAM:
			__fallthrough;
		case FMC_MEMORY_TYPE_SRAM:
			ret = memc_stm32_psram_init(config, &config->banks[bank_idx]);
			break;
		default:
			ret = -ENOTSUP;
			break;
		}

		if (ret < 0) {
			LOG_ERR("Unable to initialize memory type: "
				"0x%08X, NSBank: %d, err: %d",
				memory_type, config->banks[bank_idx].init.NSBank, ret);
			goto end;
		}
	}

end:
	return ret;
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

/** SRAM bank/s configuration. */
static const struct memc_stm32_nor_psram_bank_config bank_config[] = {
	DT_INST_FOREACH_CHILD(0, BANK_CONFIG)
};

/** SRAM configuration. */
static const struct memc_stm32_nor_psram_config config = {
	.nor_psram = (FMC_NORSRAM_TypeDef *)(DT_REG_ADDR(DT_INST_PARENT(0)) + SRAM_OFFSET),
	.extended = (FMC_NORSRAM_EXTENDED_TypeDef *)(DT_REG_ADDR(DT_INST_PARENT(0))
								 + SRAM_EXT_OFFSET),
	.banks = bank_config,
	.banks_len = ARRAY_SIZE(bank_config),
};

DEVICE_DT_INST_DEFINE(0, memc_stm32_nor_psram_init, NULL,
		      NULL, &config,
		      POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY,
		      NULL);
