/*
 * Copyright 2022-2023 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "fsl_power.h"
#include "fsl_inputmux.h"
#include <zephyr/pm/policy.h>
#include "board.h"

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
#include "flash_clock_setup.h"
#endif

/* OTP fuse index. */
#define FRO_192MHZ_SC_TRIM 0x2C
#define FRO_192MHZ_RD_TRIM 0x2B
#define FRO_96MHZ_SC_TRIM  0x2E
#define FRO_96MHZ_RD_TRIM  0x2D

#define OTP_FUSE_READ_API ((void (*)(uint32_t addr, uint32_t *data))0x1300805D)

#define PMIC_MODE_BOOT			0U
#define PMIC_MODE_DEEP_SLEEP		1U
#define PMIC_MODE_FRO192M_900MV		2U
#define PMIC_MODE_FRO96M_800MV		3U

#define PMIC_SETTLING_TIME		2000U	/* in micro-seconds */

static uint32_t sc_trim_192, rd_trim_192, sc_trim_96, rd_trim_96;

#if CONFIG_REGULATOR
#include <zephyr/drivers/regulator.h>

#define NODE_PCA9420	DT_NODELABEL(pca9420)
#define NODE_SW1	DT_NODELABEL(pca9420_sw1)
#define NODE_SW2	DT_NODELABEL(pca9420_sw2)
#define NODE_LDO1	DT_NODELABEL(pca9420_ldo1)
#define NODE_LDO2	DT_NODELABEL(pca9420_ldo2)
static const struct device *pca9420 = DEVICE_DT_GET(NODE_PCA9420);
static const struct device *sw1 = DEVICE_DT_GET(NODE_SW1);
static const struct device *sw2 = DEVICE_DT_GET(NODE_SW2);
static const struct device *ldo1 = DEVICE_DT_GET(NODE_LDO1);
static const struct device *ldo2 = DEVICE_DT_GET(NODE_LDO2);

static int current_power_profile;

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

static int board_pmic_change_mode(uint8_t pmic_mode)
{
	int ret;

	if (pmic_mode >= 4) {
		return -ERANGE;
	}

	ret = regulator_parent_dvs_state_set(pca9420, pmic_mode);
	if (ret != -EPERM) {
		return ret;
	}

	POWER_SetPmicMode(pmic_mode, kCfg_Run);
	k_busy_wait(PMIC_SETTLING_TIME);

	return 0;
}

/* Changes power-related config to preset profiles, like clocks and VDDCORE voltage */
__ramfunc int32_t power_manager_set_profile(uint32_t power_profile)
{
	bool voltage_changed = false;
	int32_t current_uv, future_uv;
	int ret;

	if (power_profile == current_power_profile) {
		return 0;
	}

	/* Confirm valid power_profile, and read the new VDDCORE voltage */
	switch (power_profile) {
	case POWER_PROFILE_AFTER_BOOT:
		future_uv = DT_PROP(NODE_SW1, nxp_mode0_microvolt);
		break;

	case POWER_PROFILE_FRO192M_900MV:
		future_uv = DT_PROP(NODE_SW1, nxp_mode2_microvolt);
		break;

	case POWER_PROFILE_FRO96M_800MV:
		future_uv = DT_PROP(NODE_SW1, nxp_mode3_microvolt);
		break;

	default:
		return -EINVAL;
	}

	if (current_power_profile == POWER_PROFILE_AFTER_BOOT) {
		/* One-Time optimization after boot */

		POWER_DisableLVD();

		CLOCK_AttachClk(kFRO_DIV1_to_MAIN_CLK);
		/* Set SYSCPUAHBCLKDIV divider to value 1 */
		CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 1U);

		/* Other clock optimizations */
	#ifdef CONFIG_FLASH_MCUX_FLEXSPI_XIP
		flexspi_setup_clock(FLEXSPI0, 0U, 1U);	/* main_clk div by 1 */
	#endif
		/* Disable the PFDs of SYSPLL */
		CLKCTL0->SYSPLL0PFD |=	CLKCTL0_SYSPLL0PFD_PFD0_CLKGATE_MASK |
					CLKCTL0_SYSPLL0PFD_PFD1_CLKGATE_MASK |
					CLKCTL0_SYSPLL0PFD_PFD2_CLKGATE_MASK;

		POWER_EnablePD(kPDRUNCFG_PD_SYSPLL_LDO);
		POWER_EnablePD(kPDRUNCFG_PD_SYSPLL_ANA);
		POWER_EnablePD(kPDRUNCFG_PD_AUDPLL_LDO);
		POWER_EnablePD(kPDRUNCFG_PD_AUDPLL_ANA);
		POWER_EnablePD(kPDRUNCFG_PD_SYSXTAL);

		/* Configure MCU that PMIC supplies will be powered in all
		 * PMIC modes
		 */
		PMC->PMICCFG = 0xFF;

	}

	/* Get current and future PMIC voltages to determine DVFS sequence */
	ret = regulator_get_voltage(sw1, &current_uv);
	if (ret) {
		return ret;
	}

	if (power_profile == POWER_PROFILE_FRO192M_900MV) {
		/* check if voltage or frequency change is first */
		if (future_uv > current_uv) {
			/* Increase voltage first before frequencies */
			ret = board_pmic_change_mode(PMIC_MODE_FRO192M_900MV);
			if (ret) {
				return ret;
			}

			voltage_changed = true;
		}

		/* Trim FRO to 192MHz */
		CLKCTL0->FRO_SCTRIM = sc_trim_192;
		CLKCTL0->FRO_RDTRIM = rd_trim_192;
		/* Reset the EXP_COUNT. */
		CLKCTL0->FRO_CONTROL &= ~CLKCTL0_FRO_CONTROL_EXP_COUNT_MASK;

		CLOCK_AttachClk(kFRO_DIV1_to_MAIN_CLK);
		/* Set SYSCPUAHBCLKDIV divider to value 1 */
		CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 1U);

		if (voltage_changed == false) {
			ret = board_pmic_change_mode(PMIC_MODE_FRO192M_900MV);
			if (ret) {
				return ret;
			}
		}

	} else if (power_profile == POWER_PROFILE_FRO96M_800MV) {
		/* This PMIC mode is the lowest voltage used for DVFS,
		 * Reduce frequency first, and then reduce voltage
		 */

		/* Trim the FRO to 96MHz */
		CLKCTL0->FRO_SCTRIM = sc_trim_96;
		CLKCTL0->FRO_RDTRIM = rd_trim_96;
		/* Reset the EXP_COUNT. */
		CLKCTL0->FRO_CONTROL &= ~CLKCTL0_FRO_CONTROL_EXP_COUNT_MASK;

		CLOCK_AttachClk(kFRO_DIV1_to_MAIN_CLK);
		/* Set SYSCPUAHBCLKDIV divider to value 1 */
		CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 1U);

		ret = board_pmic_change_mode(PMIC_MODE_FRO96M_800MV);
		if (ret) {
			return ret;
		}
	}

	current_power_profile = power_profile;

	return 0;
}

#endif /* CONFIG_REGULATOR */

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
#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Set shared signal set 0 SCK, WS from Transmit I2S - Flexcomm3 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(3) |
				SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(3);
	/* Select Data in from Transmit I2S - Flexcomm 3 */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_SHAREDDATASEL(3);
	/* Enable Transmit I2S - Flexcomm 3 for Shared Data Out */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_FC3DATAOUTEN(1);
#else
	/* Set shared signal set 0: SCK, WS from Flexcomm1 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(1) |
				SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(1);
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
#endif /* CONFIG_I2S_TEST_SEPARATE_DEVICES */
#endif /* CONFIG_I2S */

	/* Configure the DMA requests in the INPUTMUX */
	INPUTMUX_Init(INPUTMUX);
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dma0))
	/* Enable the DMA requests with only 1 mux option.  The other request
	 * choices should be configured for the application
	 */
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm11RxToDmac0Ch32RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm11TxToDmac0Ch33RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm12RxToDmac0Ch34RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm12TxToDmac0Ch35RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm16RxToDmac0Ch28RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm16TxToDmac0Ch29RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_I3c1RxToDmac0Ch30RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_I3c1TxToDmac0Ch31RequestEna, true);
#endif /* dma0 */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dma1))
	/* Enable the DMA requests with only 1 mux option.  The other request
	 * choices should be configured for the application
	 */
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm11RxToDmac1Ch32RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm11TxToDmac1Ch33RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm12RxToDmac1Ch34RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm12TxToDmac1Ch35RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm16RxToDmac1Ch28RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Flexcomm16TxToDmac1Ch29RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_I3c1RxToDmac1Ch30RequestEna, true);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_I3c1TxToDmac1Ch31RequestEna, true);
#endif /* dma1 */
	INPUTMUX_Deinit(INPUTMUX);

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

	/* Read 192M FRO clock Trim settings from fuses.
	 * NOTE: Reading OTP fuses requires a VDDCORE voltage of at least 1.0V
	 */
	OTP_FUSE_READ_API(FRO_192MHZ_SC_TRIM, &sc_trim_192);
	OTP_FUSE_READ_API(FRO_192MHZ_RD_TRIM, &rd_trim_192);

	/* Read 96M FRO clock Trim settings from fuses. */
	OTP_FUSE_READ_API(FRO_96MHZ_SC_TRIM, &sc_trim_96);
	OTP_FUSE_READ_API(FRO_96MHZ_RD_TRIM, &rd_trim_96);

	/* Check if the 96MHz fuses have been programmed.
	 * Production devices have 96M trim values programmed in OTP fuses.
	 * However, older EVKs may have pre-production silicon.
	 */
	if ((rd_trim_96 == 0) && (sc_trim_96 == 0)) {
		/* If not programmed then use software to calculate the trim values */
		CLOCK_FroTuneToFreq(96000000u);
		rd_trim_96 = CLKCTL0->FRO_RDTRIM;
		sc_trim_96 = sc_trim_192;
	}

	return 0;
}


#ifdef CONFIG_LV_Z_VDB_CUSTOM_SECTION
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

#endif /* CONFIG_LV_Z_VDB_CUSTOM_SECTION */

#if CONFIG_REGULATOR
/* PMIC setup is dependent on the regulator API */
SYS_INIT(board_config_pmic, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

#ifdef CONFIG_LV_Z_VDB_CUSTOM_SECTION
/* Framebuffers should be setup after PSRAM is initialized but before
 * Graphics framework init
 */
SYS_INIT(init_psram_framebufs, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

SYS_INIT(mimxrt595_evk_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
