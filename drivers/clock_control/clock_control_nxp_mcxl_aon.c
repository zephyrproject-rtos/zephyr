/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/clock/nxp_mcxl_aon_clock.h>

#include <fsl_clock.h>
#if defined(CONFIG_ADVC_DRIVER_USED)
#include <fsl_advc.h>
#endif

#define DT_DRV_COMPAT nxp_mcxl_aon_clock

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
LOG_MODULE_REGISTER(clock_control_mcxl_aon);

/*! @brief Source index table - attach id. */
static const clock_attach_id_t mcxl_aon_root_attach[] = {
	[MCXL_AON_CLK_ROOT_SRC_FRODIV1]  = kFROdiv1_to_AON_CPU,
	[MCXL_AON_CLK_ROOT_SRC_FRODIV2]  = kFROdiv2_to_AON_CPU,
	[MCXL_AON_CLK_ROOT_SRC_FRODIV4]  = kFROdiv4_to_AON_CPU,
	[MCXL_AON_CLK_ROOT_SRC_ROOT_AUX] = kROOT_AUX_to_AON_CPU,
	[MCXL_AON_CLK_ROOT_SRC_XTAL32K]  = kXTAL32K_to_AON_CPU,
};

/*! @brief Gate index table - clock_ip_name_t. */
static const clock_ip_name_t mcxl_aon_gate_table[] = {
	[MCXL_AON_GATE_APB]      = kCLOCK_GateAonAPB,
	[MCXL_AON_GATE_UART]     = kCLOCK_GateAonUART,
	[MCXL_AON_GATE_I2C]      = kCLOCK_GateAonI2C,
	[MCXL_AON_GATE_PORT]     = kCLOCK_GateAonPORT,
	[MCXL_AON_GATE_GPIO]     = kCLOCK_GateAonGPIO,
	[MCXL_AON_GATE_QTMR0]    = kCLOCK_GateAonQTMR0,
	[MCXL_AON_GATE_QTMR1]    = kCLOCK_GateAonQTMR1,
	[MCXL_AON_GATE_LPTMR]    = kCLOCK_GateAonLPTMR,
	[MCXL_AON_GATE_LPADC]    = kCLOCK_GateAonLPADC,
	[MCXL_AON_GATE_SYS]      = kCLOCK_GateAonSYS,
	[MCXL_AON_GATE_ROOT_AUX] = kCLOCK_GateAonRootAux,
};

/*! @brief Selector index table - clock_select_name_t. */
static const clock_select_name_t mcxl_aon_sel_table[] = {
	[MCXL_AON_SEL_ROOT]     = kCLOCK_SelAonROOT,
	[MCXL_AON_SEL_ROOT_AUX] = kCLOCK_SelAonROOT_AUX,
	[MCXL_AON_SEL_COM]      = kCLOCK_SelAonCOM,
	[MCXL_AON_SEL_TMR]      = kCLOCK_SelAonTMR,
	[MCXL_AON_SEL_LPADC]    = kCLOCK_SelAonLPADC,
};

/*! @brief Divider index table - clock_div_name_t. */
static const clock_div_name_t mcxl_aon_div_table[] = {
	[MCXL_AON_DIV_CPU] = kCLOCK_DIVAonCPU,
	[MCXL_AON_DIV_CMP] = kCLOCK_DIVAonCMP,
	[MCXL_AON_DIV_SYS] = kCLOCK_DIVAonSYS,
};

BUILD_ASSERT(MCXL_AON_GATE_ROOT_AUX <= MCXL_CLOCK_GATE_IDX_MASK,
	     "AON gate index namespace does not fit the gate_idx cell field");
BUILD_ASSERT(MCXL_AON_SEL_LPADC <= MCXL_CLOCK_SEL_IDX_MASK,
	     "AON selector index namespace does not fit the sel_idx cell field");
BUILD_ASSERT(MCXL_AON_DIV_SYS <= MCXL_CLOCK_DIV_IDX_MASK,
	     "AON divider index namespace does not fit the div_idx cell field");

BUILD_ASSERT(ARRAY_SIZE(mcxl_aon_root_attach) == (MCXL_AON_CLK_ROOT_SRC_XTAL32K + 1),
	     "mcxl_aon_root_attach[] size must match the root source namespace");
BUILD_ASSERT(ARRAY_SIZE(mcxl_aon_gate_table) == (MCXL_AON_GATE_ROOT_AUX + 1),
	     "mcxl_aon_gate_table[] size must match the gate index namespace");
BUILD_ASSERT(ARRAY_SIZE(mcxl_aon_sel_table) == (MCXL_AON_SEL_LPADC + 1),
	     "mcxl_aon_sel_table[] size must match the selector index namespace");
BUILD_ASSERT(ARRAY_SIZE(mcxl_aon_div_table) == (MCXL_AON_DIV_SYS + 1),
	     "mcxl_aon_div_table[] size must match the divider index namespace");

/*!
 * @brief Decoded fields of the MCXL_AON_CLOCK() clock specifier.
 */
struct mcxl_aon_clk_spec {
	uint8_t gate_idx; /*!< Gate index */
	uint8_t sel_idx;  /*!< Selector index */
	uint8_t src;      /*!< Mux source value for the selector */
	uint8_t div_idx;  /*!< Divider index */
	uint8_t div_val;  /*!< Divisor passed to CLOCK_SetClockDiv() */
	uint8_t flags;    /*!< Clock flags */
};

/*!
 * @brief Unpack all fields from a packed MCXL_AON_CLOCK() clock specifier.
 * @param sub_system  Packed 32-bit clock specifier built with MCXL_AON_CLOCK().
 * @param spec        Output struct populated with gate, selector, source,
 *                    divider, and flag fields.
 */
static void mcxl_aon_decode_spec(clock_control_subsys_t sub_system,
				 struct mcxl_aon_clk_spec *spec)
{
	uint32_t clk_cell = (uint32_t)(uintptr_t)sub_system;

	spec->gate_idx = (uint8_t)MCXL_AON_CLOCK_GATE_IDX(clk_cell);
	spec->sel_idx  = (uint8_t)MCXL_AON_CLOCK_SEL_IDX(clk_cell);
	spec->src      = (uint8_t)MCXL_AON_CLOCK_SRC(clk_cell);
	spec->div_idx  = (uint8_t)MCXL_AON_CLOCK_DIV_IDX(clk_cell);
	spec->div_val  = (uint8_t)MCXL_AON_CLOCK_DIV_VAL(clk_cell);
	spec->flags    = (uint8_t)MCXL_AON_CLOCK_FLAGS(clk_cell);
}

/*!
 * @brief Translate a gate index to the HAL clock_ip_name_t enum value.
 * @param idx  Symbolic gate index.
 * @param out  Output HAL enum value.
 * @return true on success, false if index is NONE or out of range.
 */
static bool mcxl_aon_lookup_gate(uint8_t idx, clock_ip_name_t *out)
{
	if (idx == MCXL_AON_GATE_NONE || idx >= ARRAY_SIZE(mcxl_aon_gate_table)) {
		return false;
	}
	*out = mcxl_aon_gate_table[idx];
	return true;
}

#if defined(CONFIG_ADVC_DRIVER_USED)
/*!
 * @brief Translate a selector index to the HAL clock_select_name_t enum value.
 * @param idx  Symbolic selector index.
 * @param out  Output HAL enum value.
 * @return true on success, false if index is NONE or out of range.
 */
static bool mcxl_aon_lookup_sel(uint8_t idx, clock_select_name_t *out)
{
	if (idx == MCXL_AON_SEL_NONE || idx >= ARRAY_SIZE(mcxl_aon_sel_table)) {
		return false;
	}
	*out = mcxl_aon_sel_table[idx];
	return true;
}

/*!
 * @brief Translate a divider index to the HAL clock_div_name_t enum value.
 * @param idx  Symbolic divider index.
 * @param out  Output HAL enum value.
 * @return true on success, false if idx is NONE or out of range.
 */
static bool mcxl_aon_lookup_div(uint8_t idx, clock_div_name_t *out)
{
	if (idx == MCXL_AON_DIV_NONE || idx >= ARRAY_SIZE(mcxl_aon_div_table)) {
		return false;
	}
	*out = mcxl_aon_div_table[idx];
	return true;
}
#endif /* CONFIG_ADVC_DRIVER_USED */

/*!
 * @brief Enable the clock gate for an AON peripheral.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_AON_CLOCK().
 * @return 0 on success, -EINVAL if the gate index is out of range.
 */
static int nxp_mcxl_aon_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	struct mcxl_aon_clk_spec spec;
	clock_ip_name_t gate;

	ARG_UNUSED(dev);
	mcxl_aon_decode_spec(sub_system, &spec);

	if ((spec.flags & MCXL_AON_CLK_FLAG_FIXED) != 0U) {
		return 0;
	}

	if (spec.gate_idx == MCXL_AON_GATE_NONE) {
		return 0;
	}

	if (!mcxl_aon_lookup_gate(spec.gate_idx, &gate)) {
		LOG_ERR("invalid AON gate index %u", spec.gate_idx);
		return -EINVAL;
	}

	CLOCK_EnableClock(gate);

	return 0;
}

/*!
 * @brief Disable the clock gate for an AON peripheral.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_AON_CLOCK().
 * @return 0 on success, -ENOTSUP if the clock cannot be gated,
 *         -EINVAL if the gate index is out of range.
 */
static int nxp_mcxl_aon_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct mcxl_aon_clk_spec spec;
	clock_ip_name_t gate;

	ARG_UNUSED(dev);
	mcxl_aon_decode_spec(sub_system, &spec);

	if ((spec.flags & MCXL_AON_CLK_FLAG_FIXED) != 0U) {
		return -ENOTSUP;
	}

	if (spec.gate_idx == MCXL_AON_GATE_NONE) {
		return -ENOTSUP;
	}

	if (!mcxl_aon_lookup_gate(spec.gate_idx, &gate)) {
		LOG_ERR("invalid AON gate index %u", spec.gate_idx);
		return -EINVAL;
	}

	CLOCK_DisableClock(gate);

	return 0;
}

/*!
 * @brief Return the HAL functional clock frequency for an AON gate index.
 * @param gate_idx  Gate symbolic index.
 * @param freq      Output frequency in Hz.
 * @return true if a HAL query exists, false if the gate has no rate API.
 */
static bool mcxl_aon_get_gate_freq(uint8_t gate_idx, uint32_t *freq)
{
	switch (gate_idx) {
	case MCXL_AON_GATE_UART:
		*freq = CLOCK_GetLpuartClkFreq(2U); /* AON UART is instance 2 */
		return true;
	case MCXL_AON_GATE_I2C:
		*freq = CLOCK_GetLpi2cClkFreq(2U); /* AON I2C is instance 2 */
		return true;
	case MCXL_AON_GATE_QTMR0:
	case MCXL_AON_GATE_QTMR1:
		*freq = CLOCK_GetQtmrClkFreq();
		return true;
	case MCXL_AON_GATE_LPTMR:
		*freq = CLOCK_GetLptmrClkFreq();
		return true;
	case MCXL_AON_GATE_SYS:
		*freq = CLOCK_GetAonCoreSysClkFreq();
		return true;
	default:
		return false;
	}
}

/*!
 * @brief Return the functional clock frequency for an AON peripheral.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_AON_CLOCK().
 * @param rate       Output frequency in Hz.
 * @return 0 on success, -EINVAL if rate is NULL, -ENOTSUP if no HAL query exists.
 */
static int nxp_mcxl_aon_clock_control_get_rate(const struct device *dev,
					       clock_control_subsys_t sub_system,
					       uint32_t *rate)
{
	struct mcxl_aon_clk_spec spec;
	uint32_t freq = 0U;

	ARG_UNUSED(dev);

	if (rate == NULL) {
		return -EINVAL;
	}

	mcxl_aon_decode_spec(sub_system, &spec);

	if (!mcxl_aon_get_gate_freq(spec.gate_idx, &freq)) {
		return -ENOTSUP;
	}

	*rate = freq;

	return 0;
}

#if defined(CONFIG_ADVC_DRIVER_USED)
/*!
 * @brief Check whether a mux source value is valid for a given AON selector.
 * @param sel_idx  Selector symbolic index.
 * @param src      Mux source value to validate.
 * @return true if src is a valid input for sel_idx, false otherwise.
 */
static bool mcxl_aon_src_valid(uint8_t sel_idx, uint8_t src)
{
	switch (sel_idx) {
	case MCXL_AON_SEL_ROOT_AUX:
		return (src <= MCXL_AON_ROOT_AUX_SRC_AUX);
	case MCXL_AON_SEL_COM:
		return (src <= MCXL_AON_COM_SRC_ROOT_AUX);
	case MCXL_AON_SEL_TMR:
		return (src <= MCXL_AON_TMR_SRC_ROOT_AUX);
	case MCXL_AON_SEL_LPADC:
		return (src <= MCXL_AON_LPADC_SRC_FRO16K);
	default:
		return false;
	}
}
#endif /* CONFIG_ADVC_DRIVER_USED */

/*!
 * @brief Program the clock mux selector and/or divider for an AON peripheral.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_AON_CLOCK().
 * @return 0 on success, -EINVAL if a selector, divider, or source index is
 *         invalid, -ENOTSUP if CONFIG_ADVC_DRIVER_USED is not set.
 */
static int nxp_mcxl_aon_clock_control_configure(const struct device *dev,
						clock_control_subsys_t sub_system,
						void *data)
{
	struct mcxl_aon_clk_spec spec;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	mcxl_aon_decode_spec(sub_system, &spec);

	if (spec.sel_idx == MCXL_AON_SEL_NONE && spec.div_idx == MCXL_AON_DIV_NONE) {
		return 0;
	}

#if defined(CONFIG_ADVC_DRIVER_USED)
	clock_select_name_t sel;
	clock_div_name_t clk_div;

	if (spec.sel_idx != MCXL_AON_SEL_NONE) {
		if (spec.sel_idx == MCXL_AON_SEL_ROOT) {
			/* CLOCK_AttachClk() runs the ADVC handshake for the CPU root. */
			if (spec.src == MCXL_AON_CLK_ROOT_SRC_NONE ||
			    spec.src >= ARRAY_SIZE(mcxl_aon_root_attach)) {
				LOG_ERR("invalid AON root src %u", spec.src);
				return -EINVAL;
			}
			CLOCK_AttachClk(mcxl_aon_root_attach[spec.src]);
		} else {
			if (!mcxl_aon_lookup_sel(spec.sel_idx, &sel)) {
				LOG_ERR("invalid AON selector index %u", spec.sel_idx);
				return -EINVAL;
			}
			if (!mcxl_aon_src_valid(spec.sel_idx, spec.src)) {
				LOG_ERR("invalid src %u for AON selector index %u",
					spec.src, spec.sel_idx);
				return -EINVAL;
			}
			CLOCK_SetClockSelect(sel, (uint32_t)spec.src);
		}
	}

	if (spec.div_idx != MCXL_AON_DIV_NONE) {
		if (!mcxl_aon_lookup_div(spec.div_idx, &clk_div)) {
			LOG_ERR("invalid AON divider index %u", spec.div_idx);
			return -EINVAL;
		}
		if ((spec.flags & MCXL_AON_CLK_FLAG_DIV_HALT) != 0U) {
			CLOCK_HaltClockDiv(clk_div);
		} else {
			CLOCK_SetClockDiv(clk_div, (uint32_t)spec.div_val);
		}
	}

	return 0;
#else
	LOG_WRN("AON clock reconfiguration requires CONFIG_ADVC_DRIVER_USED");
	return -ENOTSUP;
#endif /* CONFIG_ADVC_DRIVER_USED */
}

/*!
 * @brief Return the run status of an AON clock.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_AON_CLOCK().
 * @return CLOCK_CONTROL_STATUS_ON for fixed clocks,
 *         CLOCK_CONTROL_STATUS_UNKNOWN otherwise.
 */
static enum clock_control_status
nxp_mcxl_aon_clock_control_get_status(const struct device *dev,
				      clock_control_subsys_t sub_system)
{
	struct mcxl_aon_clk_spec spec;
	clock_ip_name_t gate;

	ARG_UNUSED(dev);
	mcxl_aon_decode_spec(sub_system, &spec);

	if ((spec.flags & MCXL_AON_CLK_FLAG_FIXED) != 0U) {
		/* Fixed clocks are always on. */
		return CLOCK_CONTROL_STATUS_ON;
	}

	if (spec.gate_idx == MCXL_AON_GATE_NONE) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (!mcxl_aon_lookup_gate(spec.gate_idx, &gate)) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	/* No HAL gate-status readback; gate index validated above. */
	ARG_UNUSED(gate);
	return CLOCK_CONTROL_STATUS_UNKNOWN;
}

/*!
 * @brief  Power management callback; no clock state change needed on suspend/resume.
 * @return 0 always.
 */
static int nxp_mcxl_aon_clock_control_pm(const struct device *dev,
					 enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);
	return 0;
}

/*!
 * @brief Zephyr device driver init callback.
 *
 * Enables the AON APB gate, optionally initializes ADVC (CM33 only), enables
 * the ADVC voltage-handshake in fsl_clock (both cores), and registers the PM
 * callback.
 *
 * @return 0 on success, negative errno on failure.
 */
static int nxp_mcxl_aon_clock_control_init(const struct device *dev)
{
	/* Enable AON APB gate so CGU and peripheral registers are accessible. */
	CLOCK_EnableClock(kCLOCK_GateAonAPB);

#if defined(CONFIG_ADVC_DRIVER_USED)
#if __CORTEX_M == 33U
	/*
	 * ADVC_Init() loads the calibration table (CM33 only; absent from the
	 * CM0+ library).  ADVC_Enable() activates voltage tracking.  Only the
	 * CM33 calls these; the CM0+ shares the same hardware and must not
	 * double-initialize it.
	 */
	ADVC_Init();
	if (!ADVC_IsEnabled()) {
		advc_result_t res = ADVC_Enable(kADVC_ModeOptimal, NULL);

		if (res != kADVC_Stat_Ok) {
			LOG_WRN("ADVC_Enable failed (%d); AON clock changes "
				"will proceed without voltage coordination",
				(int)res);
		}
	}
#endif /* __CORTEX_M == 33U */
	/*
	 * Arm the ADVC pre/post voltage-change hooks inside fsl_clock so that
	 * CLOCK_AttachClk() and CLOCK_SetClockDiv() run the handshake on this
	 * core.  Must be called on both CM33 and CM0+.
	 */
	CLOCK_EnableADVCControl();
#endif /* CONFIG_ADVC_DRIVER_USED */

	return pm_device_driver_init(dev, nxp_mcxl_aon_clock_control_pm);
}

static DEVICE_API(clock_control, nxp_mcxl_aon_clock_control_api) = {
	.on = nxp_mcxl_aon_clock_control_on,
	.off = nxp_mcxl_aon_clock_control_off,
	.get_rate = nxp_mcxl_aon_clock_control_get_rate,
	.get_status = nxp_mcxl_aon_clock_control_get_status,
	.configure = nxp_mcxl_aon_clock_control_configure,
};

DEVICE_DT_INST_DEFINE(0, nxp_mcxl_aon_clock_control_init, NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &nxp_mcxl_aon_clock_control_api);
