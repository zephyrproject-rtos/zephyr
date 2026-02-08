/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <stm32_ll_opamp.h>

/* There is a spell-mistake in original driver - L268, stm32g4xx_ll_opamp.h
 * use custom definitions for the macros to avoid modifying original driver
 */
#define OPAMP_INTERNAL_OUTPUT_DISABLED (0x00000000UL)
#define OPAMP_INTERNAL_OUTPUT_ENABLED  OPAMP_CSR_OPAMPINTEN

#define OPAMP_NODE   DT_PHANDLE(DT_PATH(zephyr_user), opamp)
#define OPAMP_LL_DEV ((OPAMP_TypeDef *)DT_REG_ADDR(OPAMP_NODE))

#define OPAMP_SUPPORT_PROGRAMMABLE_GAIN DT_NODE_HAS_PROP(OPAMP_NODE, programmable_gain)

#define OPAMP_INM_FILTERING_NONE  (0)
#define OPAMP_INM_FILTERING_VINM0 (1)
#define OPAMP_INM_FILTERING_VINM1 (2)

#define OPAMP_FILTERING_INPUT                                                                      \
	CONCAT(OPAMP_INM_FILTERING_, DT_STRING_TOKEN(OPAMP_NODE, st_inm_filtering))
#if OPAMP_FILTERING_INPUT
#define OPAMP_FILTERING_INPUT_PRESENT 1
#else
#define OPAMP_FILTERING_INPUT_PRESENT 0
#endif

#if OPAMP_SUPPORT_PROGRAMMABLE_GAIN
static const enum opamp_gain programmable_gain[] = {
	DT_FOREACH_PROP_ELEM_SEP(OPAMP_NODE, programmable_gain,
				 DT_ENUM_IDX_BY_IDX, (,)) };
#endif

#define STM32_OPAMP_TRIM_VAL_MAX       0x1f /* maximum allowed trimming value */
#define STM32_OPAMP_TRIM_VAL_UNDEFINED 0xff /* undefined trimming value */

#define IS_OPAMP_TRIM_IN_RANGE(trim_val) ((trim_val) <= STM32_OPAMP_TRIM_VAL_MAX)

const struct device *get_opamp_device(void)
{
	return DEVICE_DT_GET(OPAMP_NODE);
}

static const struct device *init_opamp(void)
{
	const struct device *const opamp_dev = DEVICE_DT_GET(OPAMP_NODE);
	const uint32_t dts_functional_mode = DT_ENUM_IDX(OPAMP_NODE, functional_mode);

	bool ret = device_is_ready(opamp_dev);

	if (dts_functional_mode != OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL) {
		/* The device shall be ready for all modes except differential,
		 * which will be handled below during mode testing.
		 */
		zassert_true(ret, "OPAMP device %s shall be ready", opamp_dev->name);

		/* Verify OPAMP is enabled */
		zassert_true(LL_OPAMP_IsEnabled(OPAMP_LL_DEV) == 1,
			     "OPAMP device %s shall be enabled (e.g. OPAEN bit is set)",
			     opamp_dev->name);
	}

	/* Verify clock is enabled: RCC_APB2ENR SYSCFGEN bit 0 shall be set */
	zassert_equal(IS_BIT_SET(RCC->APB2ENR, 0), 1,
		      "RCC_APB2ENR SYSCFGEN bit 0 shall be set (RCC->APB2ENR = 0x%x)",
		      RCC->APB2ENR);

	return opamp_dev;
}

ZTEST(opamp_stm32, test_init_opamp)
{
	const struct device *opamp_dev = init_opamp();
	const uint32_t dts_functional_mode = DT_ENUM_IDX(OPAMP_NODE, functional_mode);

	/* inp shall be ALWAYS defined in DTS */
	const uint32_t inp = LL_OPAMP_GetInputNonInverting(OPAMP_LL_DEV);

	zassert_true(inp == LL_OPAMP_INPUT_NONINVERT_IO0 || inp == LL_OPAMP_INPUT_NONINVERT_IO1 ||
			     inp == LL_OPAMP_INPUT_NONINVERT_IO2 ||
			     inp == LL_OPAMP_INPUT_NONINVERT_IO3 ||
			     inp == LL_OPAMP_INPUT_NONINVERT_DAC,
		     "%s: OPAMP shall have at least one inp defined", opamp_dev->name);

	/* Secondary inp may be defined in DTS */
#if DT_NODE_HAS_PROP(OPAMP_NODE, st_inputs_mux_mode)
	/* INP secondary could be any value defined in DTS */
	const uint32_t inp_sec = LL_OPAMP_GetInputNonInvertingSecondary(OPAMP_LL_DEV);

	zassert_true(inp_sec == LL_OPAMP_INPUT_NONINVERT_IO0_SEC ||
			     inp_sec == LL_OPAMP_INPUT_NONINVERT_IO1_SEC ||
			     inp_sec == LL_OPAMP_INPUT_NONINVERT_IO2_SEC ||
			     inp_sec == LL_OPAMP_INPUT_NONINVERT_IO3_SEC ||
			     inp_sec == LL_OPAMP_INPUT_NONINVERT_DAC_SEC,
		     "%s: OPAMP shall have at least one inp secondary defined", opamp_dev->name);
#endif /* st_inputs_mux_mode */

	/* Verify registers have expected values for functional mode set.
	 * Functional_mode is a required property, therefore no need to check
	 * it is present on preprocessor level.
	 */
	const uint32_t functional_mode = LL_OPAMP_GetFunctionalMode(OPAMP_LL_DEV);
	const uint32_t inm = LL_OPAMP_GetInputInverting(OPAMP_LL_DEV);
	const uint32_t inm_sec = LL_OPAMP_GetInputInvertingSecondary(OPAMP_LL_DEV);

	switch (dts_functional_mode) {
	case OPAMP_FUNCTIONAL_MODE_FOLLOWER:
		zassert_equal(functional_mode, LL_OPAMP_MODE_FOLLOWER,
			      "%s: OPAMP shall be in follower mode", opamp_dev->name);

		/* INM shall stay not-connected in follower mode */
		zassert_equal(inm, LL_OPAMP_INPUT_INVERT_CONNECT_NO,
			      "%s: OPAMP shall have INM disconnected "
			      "(LL_OPAMP_INPUT_INVERT_CONNECT_NO = 0x%x), but it is 0x%x",
			      opamp_dev->name, (uint32_t)LL_OPAMP_INPUT_INVERT_CONNECT_NO, inm);
		break;
	case OPAMP_FUNCTIONAL_MODE_STANDALONE:
		zassert_equal(functional_mode, LL_OPAMP_MODE_STANDALONE,
			      "%s: OPAMP shall be in standalone mode", opamp_dev->name);

		/* INM shall be tied to VINM0 in standalone mode */
		zassert_equal(inm, LL_OPAMP_INPUT_INVERT_IO0,
			      "%s: inm shall be VINM0 (0x%x), but it is 0x%x", opamp_dev->name,
			      (uint32_t)LL_OPAMP_INPUT_INVERT_IO0, inm);

#if DT_NODE_HAS_PROP(OPAMP_NODE, st_inputs_mux_mode)
		/* INM secondary shall be either VINM0 or VINM1 */
		zassert_true(inm_sec == LL_OPAMP_INPUT_INVERT_IO0_SEC ||
				     inm_sec == LL_OPAMP_INPUT_INVERT_IO1_SEC,
			     "%s: inm secondary shall be VINM0 (0x%x) or VINM1 (0x%x), "
			     "but it is 0x%x",
			     opamp_dev->name, (uint32_t)LL_OPAMP_INPUT_INVERT_IO0_SEC,
			     (uint32_t)LL_OPAMP_INPUT_INVERT_IO1_SEC, inm_sec);
#endif /* st_inputs_mux_mode */

		break;
	case OPAMP_FUNCTIONAL_MODE_INVERTING:
#if OPAMP_FILTERING_INPUT_PRESENT
		zassert_equal(functional_mode, LL_OPAMP_MODE_PGA_IO0_IO1_BIAS,
			      "%s: OPAMP shall be in LL_OPAMP_MODE_PGA_IO0_IO1_BIAS",
			      opamp_dev->name);
#else  /* OPAMP_FILTERING_INPUT_PRESENT */
		zassert_equal(functional_mode, LL_OPAMP_MODE_PGA_IO0_BIAS,
			      "%s: OPAMP shall be in LL_OPAMP_MODE_PGA_IO0_BIAS", opamp_dev->name);
#endif /* OPAMP_FILTERING_INPUT_PRESENT */

		/* VM_SEL = b10 in all PGA modes (OPAMPx_CSR register bits 6 to 5) */
		zassert_true(IS_BIT_SET(OPAMP_LL_DEV->CSR, 6),
			     "%s: OPAMP CSR VM_SEL (bit 6) shall be SET in "
			     "inverting mode",
			     opamp_dev->name);
		zassert_false(IS_BIT_SET(OPAMP_LL_DEV->CSR, 5),
			      "%s: OPAMP CSR VM_SEL (bit 5) shall be RESET in "
			      "inverting mode",
			      opamp_dev->name);

#if DT_NODE_HAS_PROP(OPAMP_NODE, st_inputs_mux_mode)
		/* The inverting mode is a sub-mode of PGA mode.
		 * In this case VMS_SEL is defined by OPAMPx_TCMR register bit 0.
		 * The value of VMS_SEL in OPAMPx_TCMR is therefore don't care here.
		 *
		 * From RM0440 Rev 9 pp. 809/2140:
		 * When PGA (VM_SEL = “10”) or Follower mode (VM_SEL = “11”) is used:
		 *	0: Resistor feedback output selected (PGA mode)
		 *	1: VOUT selected as input minus (follower mode)
		 */
#endif /* st_inputs_mux_mode */
		break;
	case OPAMP_FUNCTIONAL_MODE_NON_INVERTING:
#if OPAMP_FILTERING_INPUT_PRESENT
		zassert_equal(functional_mode, LL_OPAMP_MODE_PGA_IO0,
			      "%s: OPAMP shall be in LL_OPAMP_MODE_PGA_IO0", opamp_dev->name);
#else /* OPAMP_FILTERING_INPUT_PRESENT */
		zassert_equal(functional_mode, LL_OPAMP_MODE_PGA,
			      "%s: OPAMP shall be in LL_OPAMP_MODE_PGA", opamp_dev->name);

#endif /* OPAMP_FILTERING_INPUT_PRESENT */

		/* VM_SEL = b10 in all PGA modes (OPAMPx_CSR register bits 6 to 5) */
		zassert_true(IS_BIT_SET(OPAMP_LL_DEV->CSR, 6),
			     "%s: OPAMP CSR VM_SEL (bit 6) shall be SET in "
			     "inverting mode",
			     opamp_dev->name);
		zassert_false(IS_BIT_SET(OPAMP_LL_DEV->CSR, 5),
			      "%s: OPAMP CSR VM_SEL (bit 5) shall be RESET in "
			      "inverting mode",
			      opamp_dev->name);
		break;
	case OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL:
		/* differential mode is not supported directly - device shall not be ready */
		/* The device shall not be ready in differential mode */
		zassert_false(device_is_ready(opamp_dev),
			      "OPAMP device %s shall be NOT ready in differential mode",
			      opamp_dev->name);
		break;
	default:
		zassert_unreachable("Unsupported functional mode %d", dts_functional_mode);
		break;
	};

	/* Gain shall have register reset value (0x0) after init */
	const uint32_t gain = LL_OPAMP_GetPGAGain(OPAMP_LL_DEV);

	zassert_equal(gain, 0x0,
		      "%s: OPAMP gain shall be reset to 0x0 after init, "
		      "but it is 0x%x",
		      opamp_dev->name, gain);

	/* Power mode: Allowed to be NORMAL or HIGHSPEED */
	const uint32_t dts_power_mode = DT_ENUM_IDX(OPAMP_NODE, st_power_mode);
	const uint32_t power_mode = LL_OPAMP_GetPowerMode(OPAMP_LL_DEV);

	if (dts_power_mode == 0) {
		/* Power mode: NORMAL (enum value 0) */
		zassert_true(power_mode == LL_OPAMP_POWERMODE_NORMALSPEED,
			     "%s: OPAMP power mode shall match DTS setting 0x%x != 0x%x",
			     opamp_dev->name, (uint32_t)power_mode,
			     (uint32_t)LL_OPAMP_POWERMODE_NORMALSPEED);
	} else {
		/* Power mode: HIGHSPEED (enum value 1) */
		zassert_true(power_mode == LL_OPAMP_POWERMODE_HIGHSPEED,
			     "%s: OPAMP power mode shall match DTS setting 0x%x != 0x%x",
			     opamp_dev->name, (uint32_t)power_mode,
			     (uint32_t)LL_OPAMP_POWERMODE_HIGHSPEED);
	}

#if DT_PROP(OPAMP_NODE, st_lock_enable)
	/* Verify that OPAMP is locked when lock-enable is set in DTS */
	zassert_true(LL_OPAMP_IsLocked(OPAMP_LL_DEV) == 1,
		     "%s: OPAMP shall be locked when lock-enable is set in DTS", opamp_dev->name);
	if (LL_OPAMP_GetInputsMuxMode(OPAMP_LL_DEV) != LL_OPAMP_INPUT_MUX_DISABLE) {
		/* Same DTS lock property locks timer mux if enabled */
		zassert_true(LL_OPAMP_IsTimerMuxLocked(OPAMP_LL_DEV) == 1,
			     "%s: OPAMP timer MUX shall be locked when lock-enable is set in DTS",
			     opamp_dev->name);
	}
#endif /* lock-enable */

	zassert_equal(LL_OPAMP_GetMode(OPAMP_LL_DEV), LL_OPAMP_MODE_FUNCTIONAL,
		      "%s: OPAMP shall be in functional mode after init", opamp_dev->name);

	/* The function LL_OPAMP_GetCalibrationSelection() is buggy
	 * at least for stm32g4 and returns the wrong value after initialization,
	 * therefore read it manually. CALSEL bits are 13:12
	 */
	const uint32_t calsel_val = OPAMP_LL_DEV->CSR & (BIT(13) | BIT(12));

	zassert_equal(calsel_val, 0,
		      "%s: OPAMP's CALSEL (calibration selection) shall be in "
		      "reset state (0) after init, but it is 0x%x",
		      opamp_dev->name, calsel_val);

#if DT_NODE_HAS_PROP(OPAMP_NODE, io_channels)
	/* io-channels is present - the OPAMP shall be connected to ADC */
	zassert_equal(LL_OPAMP_GetInternalOutput(OPAMP_LL_DEV), OPAMP_INTERNAL_OUTPUT_ENABLED,
		      "%s: OPAMP expected to be connected to ADC", opamp_dev->name);
#endif /* io_channels */

	const uint32_t trim_mode = LL_OPAMP_GetTrimmingMode(OPAMP_LL_DEV);
#if DT_PROP(OPAMP_NODE, st_enable_self_calibration)
	/* Calibration is running at driver initalization step (POST_KERNEL).
	 * Verify that OPAMP is configured to be user calibrated
	 */
	zassert_equal(trim_mode, LL_OPAMP_TRIMMING_USER,
		      "%s: OPAMP shall have user trimming mode after self-calibration",
		      opamp_dev->name);

#else /* st_enable_self_calibration */
	/* Calibration is not used */
#if DT_NODE_HAS_PROP(OPAMP_NODE, st_pmos_trimming_value) ||                                        \
	DT_NODE_HAS_PROP(OPAMP_NODE, st_nmos_trimming_value)
#if DT_NODE_HAS_PROP(OPAMP_NODE, st_pmos_trimming_value)
	/* PMOS trim value is provided - verify user trimming mode is enabled */
	const uint8_t dts_pmos_trimming = DT_PROP(OPAMP_NODE, st_pmos_trimming_value);
	const uint8_t pmos_trimming =
		LL_OPAMP_GetTrimmingValue(OPAMP_LL_DEV, LL_OPAMP_TRIMMING_PMOS);

	if (IS_OPAMP_TRIM_IN_RANGE(dts_pmos_trimming)) {
		/* Verify if user-trimming is enabled */
		zassert_equal(trim_mode, LL_OPAMP_TRIMMING_USER,
			      "%s: OPAMP shall have user trimming mode when trimming "
			      "values are set in DTS",
			      opamp_dev->name);
		/* Verify PMOS trimming value match DTS defined one */
		zassert_equal(pmos_trimming, dts_pmos_trimming,
			      "%s: OPAMP PMOS trimming value shall match "
			      "DTS setting: 0x%x != 0x%x",
			      opamp_dev->name, pmos_trimming, dts_pmos_trimming);
	}
#endif /* st_pmos_trimming_value */
#if DT_NODE_HAS_PROP(OPAMP_NODE, st_nmos_trimming_value)
	/* NMOS trim value is provided - verify user trimming mode is enabled */
	const uint8_t dts_nmos_trimming = DT_PROP(OPAMP_NODE, st_nmos_trimming_value);
	const uint8_t nmos_trimming =
		LL_OPAMP_GetTrimmingValue(OPAMP_LL_DEV, LL_OPAMP_TRIMMING_NMOS);

	if (IS_OPAMP_TRIM_IN_RANGE(dts_nmos_trimming)) {
		/* Verify if user-trimming is enabled */
		zassert_equal(trim_mode, LL_OPAMP_TRIMMING_USER,
			      "%s: OPAMP shall have user trimming mode when trimming "
			      "values are set in DTS",
			      opamp_dev->name);
		/* Verify NMOS trimming value match DTS defined one */
		zassert_equal(nmos_trimming, dts_nmos_trimming,
			      "%s: OPAMP NMOS trimming value shall match "
			      "DTS setting: 0x%x != 0x%x",
			      opamp_dev->name, nmos_trimming, dts_nmos_trimming);
	}
#endif /* st_nmos_trimming_value */
#else  /* st_pmos_trimming_value || st_nmos_trimming_value */
	/* No trimming values provided and calibration is disabled.
	 * Factory trimming mode shall be used
	 */
	zassert_equal(trim_mode, LL_OPAMP_TRIMMING_FACTORY,
		      "%s: OPAMP shall have factory trimming mode", opamp_dev->name);
#endif /* st_pmos_trimming_value || st_nmos_trimming_value */
#endif /* st_enable_self_calibration */
}

#if OPAMP_SUPPORT_PROGRAMMABLE_GAIN
/* Test OPAMP opamp_set_gain API, Only OPAMPs
 * that support PGA need to test this API.
 */
ZTEST(opamp_stm32, test_gain_set)
{
	int ret;

	const uint32_t dts_functional_mode = DT_ENUM_IDX(OPAMP_NODE, functional_mode);
	const struct device *opamp_dev = init_opamp();

	if (dts_functional_mode == OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL) {
		/* differential mode is not supported directly - the test is done in
		 * init_opamp() function
		 */
		return;
	}
	zassert_not_equal(opamp_dev, NULL, "Failed to get OPAMP device");

	for (uint8_t index = 0; index < ARRAY_SIZE(programmable_gain); ++index) {
		ret = opamp_set_gain(opamp_dev, programmable_gain[index]);
#if DT_PROP(OPAMP_NODE, st_lock_enable)
		/* Lock enabled: Gain can NOT be set */
		zassert_not_ok(ret, "lock enabled: opamp_set_gain() failed with code %d", ret);
#else  /* lock-enable */
		/* Lock disabled: Gain can be set */
		zassert_ok(ret, "opamp_set_gain() failed with code %d", ret);
		printk("%s: gain set to %d, 0x%x\n", opamp_dev->name, programmable_gain[index],
		       LL_OPAMP_GetPGAGain(OPAMP_LL_DEV));
#endif /* lock-enable */
	}

	/* Twister could execute gain setting test (this test) before init test without
	 * power-cycling the device. To have reproducible tests, the gain shall be reverted
	 * to OPAMPx_CSR registers default value (e.g. resetting bits 14 to 16 to zero).
	 * Bits 17 to 18 shall be untouched since they may contain information about
	 * PGA sub-mode (e.g. inverting/non-inverting, filtering and bias connections).
	 * Refer to RM0440 Rev 9 pp. 791/2140 for more details on OPAMPx_CSR register reset value.
	 */
	OPAMP_LL_DEV->CSR &= ~(BIT(14) | BIT(15) | BIT(16));
}
#endif

static void *opamp_setup(void)
{
	k_object_access_grant(get_opamp_device(), k_current_get());

	return NULL;
}

ZTEST_SUITE(opamp_stm32, NULL, opamp_setup, NULL, NULL, NULL);
