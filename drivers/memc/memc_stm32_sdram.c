/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_fmc_sdram

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_stm32_sdram, CONFIG_MEMC_LOG_LEVEL);

/** SDRAM controller register offset. */
#define SDRAM_OFFSET 0x140U

/** Upper bound for the FMC to take a command, in ms (only the F4 HAL waits). */
#define SDRAM_CMD_TIMEOUT_MS 1U

/** Upper bound for a bank to report the mode selected by a command. */
#define SDRAM_MODE_TIMEOUT_US 1000U

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

static uint32_t memc_stm32_sdram_cmd_target(const struct memc_stm32_sdram_config *config)
{
	if (config->banks_len == 2U) {
		return FMC_SDRAM_CMD_TARGET_BANK1_2;
	}

	if (config->banks[0].init.SDBank == FMC_SDRAM_BANK1) {
		return FMC_SDRAM_CMD_TARGET_BANK1;
	}

	return FMC_SDRAM_CMD_TARGET_BANK2;
}

static int memc_stm32_sdram_init(const struct device *dev)
{
	const struct memc_stm32_sdram_config *config = dev->config;

	SDRAM_HandleTypeDef sdram = { 0 };
	FMC_SDRAM_CommandTypeDef sdram_cmd = { 0 };

	sdram.Instance = config->sdram;

	for (size_t i = 0U; i < config->banks_len; i++) {
		sdram.State = HAL_SDRAM_STATE_RESET;
		memcpy(&sdram.Init, &config->banks[i].init, sizeof(sdram.Init));

		if (HAL_SDRAM_Init(&sdram,
				   (FMC_SDRAM_TimingTypeDef *)&config->banks[i].timing) != HAL_OK) {
			return -EIO;
		}
	}

	/* SDRAM initialization sequence */
	sdram_cmd.CommandTarget = memc_stm32_sdram_cmd_target(config);
	sdram_cmd.AutoRefreshNumber = config->num_auto_refresh;
	sdram_cmd.ModeRegisterDefinition = config->mode_register;

	/* enable clock */
	sdram_cmd.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
	if (HAL_SDRAM_SendCommand(&sdram, &sdram_cmd, 0U) != HAL_OK) {
		return -EIO;
	}

	k_usleep(config->power_up_delay);

	/* pre-charge all */
	sdram_cmd.CommandMode = FMC_SDRAM_CMD_PALL;
	if (HAL_SDRAM_SendCommand(&sdram, &sdram_cmd, 0U) != HAL_OK) {
		return -EIO;
	}

	/* auto-refresh */
	sdram_cmd.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	if (HAL_SDRAM_SendCommand(&sdram, &sdram_cmd, 0U) != HAL_OK) {
		return -EIO;
	}

	/* load mode */
	sdram_cmd.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	if (HAL_SDRAM_SendCommand(&sdram, &sdram_cmd, 0U) != HAL_OK) {
		return -EIO;
	}

	/* program refresh count */
	if (HAL_SDRAM_ProgramRefreshRate(&sdram, config->refresh_rate) != HAL_OK) {
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int memc_stm32_sdram_set_mode(const struct memc_stm32_sdram_config *config, uint32_t command,
				     uint32_t mode)
{
	const FMC_SDRAM_CommandTypeDef sdram_cmd = {
		.CommandMode = command,
		.CommandTarget = memc_stm32_sdram_cmd_target(config),
		.AutoRefreshNumber = 1U,
		.ModeRegisterDefinition = 0U,
	};

	if (FMC_SDRAM_SendCommand(config->sdram, &sdram_cmd, SDRAM_CMD_TIMEOUT_MS) != HAL_OK) {
		return -EIO;
	}

	for (size_t i = 0U; i < config->banks_len; i++) {
		const uint32_t bank = config->banks[i].init.SDBank;

		if (!WAIT_FOR(FMC_SDRAM_GetModeStatus(config->sdram, bank) == mode,
			      SDRAM_MODE_TIMEOUT_US, k_busy_wait(1))) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int memc_stm32_sdram_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct memc_stm32_sdram_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/*
		 * Low-power modes such as Stop gate the FMC clock, so the FMC
		 * stops refreshing the SDRAM. In self-refresh the SDRAM keeps
		 * its content without that clock.
		 */
		return memc_stm32_sdram_set_mode(config, FMC_SDRAM_CMD_SELFREFRESH_MODE,
						 FMC_SDRAM_SELF_REFRESH_MODE);
	case PM_DEVICE_ACTION_RESUME:
		return memc_stm32_sdram_set_mode(config, FMC_SDRAM_CMD_NORMAL_MODE,
						 FMC_SDRAM_NORMAL_MODE);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

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

PM_DEVICE_DT_INST_DEFINE(0, memc_stm32_sdram_pm_action);

DEVICE_DT_INST_DEFINE(0, memc_stm32_sdram_init, PM_DEVICE_DT_INST_GET(0), NULL, &config,
		      POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);
