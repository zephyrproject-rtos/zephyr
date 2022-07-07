/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_fmc_sdram

#include <zephyr/device.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_stm32_sdram, CONFIG_MEMC_LOG_LEVEL);

/** SDRAM controller register offset. */
#define SDRAM_OFFSET 0x140U

/** FMC SDRAM controller bank configuration fields. */
struct memc_stm32_sdram_bank_config {
	FMC_SDRAM_InitTypeDef init;
	FMC_SDRAM_TimingTypeDef timing;
};

/** FMC SDRAM controller configuration fields. */
struct memc_stm32_sdram_config {
	FMC_SDRAM_TypeDef *sdram;
	uint32_t power_up_delay;
	uint8_t num_auto_refresh;
	uint16_t mode_register;
	uint16_t refresh_rate;
	const struct memc_stm32_sdram_bank_config *banks;
	size_t banks_len;
};

static int memc_stm32_sdram_init(const struct device *dev)
{
	const struct memc_stm32_sdram_config *config = dev->config;

	SDRAM_HandleTypeDef sdram = { 0 };
	FMC_SDRAM_CommandTypeDef sdram_cmd = { 0 };

	sdram.Instance = config->sdram;

	for (size_t i = 0U; i < config->banks_len; i++) {
		sdram.State = HAL_SDRAM_STATE_RESET;
		memcpy(&sdram.Init, &config->banks[i].init, sizeof(sdram.Init));

		(void)HAL_SDRAM_Init(
			&sdram,
			(FMC_SDRAM_TimingTypeDef *)&config->banks[i].timing);
	}

	/* SDRAM initialization sequence */
	if (config->banks_len == 2U) {
		sdram_cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1_2;
	} else if (config->banks[0].init.SDBank == FMC_SDRAM_BANK1) {
		sdram_cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	} else {
		sdram_cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
	}

	sdram_cmd.AutoRefreshNumber = config->num_auto_refresh;
	sdram_cmd.ModeRegisterDefinition = config->mode_register;

	/* enable clock */
	sdram_cmd.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
	(void)HAL_SDRAM_SendCommand(&sdram, &sdram_cmd, 0U);

	k_usleep(config->power_up_delay);

	/* pre-charge all */
	sdram_cmd.CommandMode = FMC_SDRAM_CMD_PALL;
	(void)HAL_SDRAM_SendCommand(&sdram, &sdram_cmd, 0U);

	/* auto-refresh */
	sdram_cmd.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	(void)HAL_SDRAM_SendCommand(&sdram, &sdram_cmd, 0U);

	/* load mode */
	sdram_cmd.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	(void)HAL_SDRAM_SendCommand(&sdram, &sdram_cmd, 0U);

	/* program refresh count */
	(void)HAL_SDRAM_ProgramRefreshRate(&sdram, config->refresh_rate);

	return 0;
}

/** SDRAM bank/s configuration initialization macro. */
#define BANK_CONFIG(node_id)                                                   \
	{ .init = {                                                            \
	    .SDBank = DT_REG_ADDR(node_id),                                    \
	    .ColumnBitsNumber = DT_PROP_BY_IDX(node_id, st_sdram_control, 0),  \
	    .RowBitsNumber = DT_PROP_BY_IDX(node_id, st_sdram_control, 1),     \
	    .MemoryDataWidth = DT_PROP_BY_IDX(node_id, st_sdram_control, 2),   \
	    .InternalBankNumber = DT_PROP_BY_IDX(node_id, st_sdram_control, 3),\
	    .CASLatency = DT_PROP_BY_IDX(node_id, st_sdram_control, 4),        \
	    .WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE,             \
	    .SDClockPeriod = DT_PROP_BY_IDX(node_id, st_sdram_control, 5),     \
	    .ReadBurst = DT_PROP_BY_IDX(node_id, st_sdram_control, 6),         \
	    .ReadPipeDelay = DT_PROP_BY_IDX(node_id, st_sdram_control, 7),     \
	  },                                                                   \
	  .timing = {                                                          \
	    .LoadToActiveDelay = DT_PROP_BY_IDX(node_id, st_sdram_timing, 0),  \
	    .ExitSelfRefreshDelay =                                            \
		DT_PROP_BY_IDX(node_id, st_sdram_timing, 1),                   \
	    .SelfRefreshTime = DT_PROP_BY_IDX(node_id, st_sdram_timing, 2),    \
	    .RowCycleDelay = DT_PROP_BY_IDX(node_id, st_sdram_timing, 3),      \
	    .WriteRecoveryTime = DT_PROP_BY_IDX(node_id, st_sdram_timing, 4),  \
	    .RPDelay = DT_PROP_BY_IDX(node_id, st_sdram_timing, 5),            \
	    .RCDDelay = DT_PROP_BY_IDX(node_id, st_sdram_timing, 6),           \
	  }                                                                    \
	},

/** SDRAM bank/s configuration. */
static const struct memc_stm32_sdram_bank_config bank_config[] = {
	DT_INST_FOREACH_CHILD(0, BANK_CONFIG)
};

/** SDRAM configuration. */
static const struct memc_stm32_sdram_config config = {
	.sdram = (FMC_SDRAM_TypeDef *)(DT_REG_ADDR(DT_INST_PARENT(0)) +
				       SDRAM_OFFSET),
	.power_up_delay = DT_INST_PROP(0, power_up_delay),
	.num_auto_refresh = DT_INST_PROP(0, num_auto_refresh),
	.mode_register = DT_INST_PROP(0, mode_register),
	.refresh_rate = DT_INST_PROP(0, refresh_rate),
	.banks = bank_config,
	.banks_len = ARRAY_SIZE(bank_config),
};

DEVICE_DT_INST_DEFINE(0, memc_stm32_sdram_init, NULL,
	      NULL, &config, POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);
