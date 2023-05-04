/*
 * Copyright 2022,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "fsl_power.h"
#include <zephyr/pm/policy.h>

#if CONFIG_REGULATOR
#include <zephyr/drivers/regulator.h>

#define NODE_SW1	DT_NODELABEL(pca9420_sw1)
#define NODE_SW2	DT_NODELABEL(pca9420_sw2)
#define NODE_LDO1	DT_NODELABEL(pca9420_ldo1)
#define NODE_LDO2	DT_NODELABEL(pca9420_ldo2)
static const struct device *sw1 = DEVICE_DT_GET(NODE_SW1);
static const struct device *sw2 = DEVICE_DT_GET(NODE_SW2);
static const struct device *ldo1 = DEVICE_DT_GET(NODE_LDO1);
static const struct device *ldo2 = DEVICE_DT_GET(NODE_LDO2);

/* System clock frequency. */
extern uint32_t SystemCoreClock;

static const int32_t sw1_volt[] = {1100000, 1000000, 900000, 800000, 700000};

static int32_t board_calc_volt_level(void)
{
	uint32_t i;
	uint32_t volt;

	for (i = 0U; i < POWER_FREQ_LEVELS_NUM; i++) {
		if (SystemCoreClock > powerFreqLevel[i]) {
			break;
		}
	}

	/* Frequency exceed max supported */
	if (i == 0U) {
		volt = POWER_INVALID_VOLT_LEVEL;
	} else {
		volt = sw1_volt[i - 1U];
	}

	return volt;
}

static int board_config_pmic(void)
{
	uint32_t volt;
	int ret = 0;

	volt = board_calc_volt_level();

	ret = regulator_set_voltage(sw1, volt, volt);
	if (ret != 0) {
		return ret;
	}

	ret = regulator_set_voltage(sw2, 1800000, 1800000);
	if (ret != 0) {
		return ret;
	}

	ret = regulator_set_voltage(ldo1, 1800000, 1800000);
	if (ret != 0) {
		return ret;
	}

	ret = regulator_set_voltage(ldo2, 3300000, 3300000);
	if (ret != 0) {
		return ret;
	}

	/* We can enter deep low power modes */
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	return ret;
}
#endif

static int mimxrt595_evk_init(void)
{
	/* Set the correct voltage range according to the board. */
	power_pad_vrange_t vrange = {
		.Vdde0Range = kPadVol_171_198,
		.Vdde1Range = kPadVol_171_198,
		.Vdde2Range = kPadVol_171_198,
		.Vdde3Range = kPadVol_300_360,
		.Vdde4Range = kPadVol_171_198
	};

	POWER_SetPadVolRange(&vrange);

	/* Do not enter deep low power modes until the PMIC modes have been initialized */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	return 0;
}

#if CONFIG_REGULATOR
/* PMIC setup is dependent on the regulator API */
SYS_INIT(board_config_pmic, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

SYS_INIT(mimxrt595_evk_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
