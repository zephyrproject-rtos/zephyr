/*
 * Copyright 2022-2023 NXP
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

#define MEGA (1000000U)

/* Core frequency levels number. */
#define POWER_FREQ_LEVELS_NUM (5U)

/* Invalid voltage level. */
#define POWER_INVALID_VOLT_LEVEL (0xFFFFFFFFU)

static const uint32_t power_freq_level[POWER_FREQ_LEVELS_NUM] = {
	275U * MEGA,
	230U * MEGA,
	192U * MEGA,
	100U * MEGA,
	60U * MEGA
};

/* System clock frequency. */
extern uint32_t SystemCoreClock;

static const int32_t sw1_volt[] = {1100000, 1000000, 900000, 800000, 700000};

static int32_t board_calc_volt_level(void)
{
	uint32_t i;
	uint32_t volt;

	for (i = 0U; i < POWER_FREQ_LEVELS_NUM; i++) {
		if (SystemCoreClock > power_freq_level[i]) {
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

#ifdef CONFIG_I2S

	/* Set shared signal set 0 SCK, WS from Transmit I2S - Flexcomm3 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(3) |
				SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(3);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Data in from Transmit I2S - Flexcomm 3 */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_SHAREDDATASEL(3);
	/* Enable Transmit I2S - Flexcomm 3 for Shared Data Out */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_FC3DATAOUTEN(1);
#endif

	/* Set Receive I2S - Flexcomm 1 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

	/* Set Transmit I2S - Flexcomm 3 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[3] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Receive I2S - Flexcomm 1 Data in from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] |= SYSCTL1_FCCTRLSEL_DATAINSEL(1);
	/* Select Transmit I2S - Flexcomm 3 Data out to shared signal set 0 */
	SYSCTL1->FCCTRLSEL[3] |= SYSCTL1_FCCTRLSEL_DATAOUTSEL(1);
#endif

#endif


#ifdef CONFIG_REBOOT
	/*
	 * The sys_reboot API calls NVIC_SystemReset. On the RT595, the warm
	 * reset will not complete correctly unless the ROM toggles the
	 * flash reset pin. We can control this behavior using the OTP shadow
	 * register for OPT word BOOT_CFG1
	 *
	 * Set FLEXSPI_RESET_PIN_ENABLE=1, FLEXSPI_RESET_PIN= PIO4_5
	 */
	 OCOTP0->OTP_SHADOW[97] = 0x164000;
#endif /* CONFIG_REBOOT */
	return 0;
}


#ifdef CONFIG_LV_Z_VBD_CUSTOM_SECTION
extern char __flexspi2_start[];
extern char __flexspi2_end[];

static int init_psram_framebufs(void)
{
	/*
	 * Framebuffers will be stored in PSRAM, within FlexSPI2 linker
	 * section. Zero out BSS section.
	 */
	memset(__flexspi2_start, 0, __flexspi2_end - __flexspi2_start);
	return 0;
}

#endif /* CONFIG_LV_Z_VBD_CUSTOM_SECTION */

#if CONFIG_REGULATOR
/* PMIC setup is dependent on the regulator API */
SYS_INIT(board_config_pmic, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

#ifdef CONFIG_LV_Z_VBD_CUSTOM_SECTION
/* Framebuffers should be setup after PSRAM is initialized but before
 * Graphics framework init
 */
SYS_INIT(init_psram_framebufs, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

SYS_INIT(mimxrt595_evk_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
