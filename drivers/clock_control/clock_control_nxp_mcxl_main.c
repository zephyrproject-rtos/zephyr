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
#include <zephyr/dt-bindings/clock/nxp_mcxl_main_clock.h>

#include <fsl_clock.h>

#if defined(CONFIG_CLOCK_CONTROL_NXP_MCXL_MAIN_INIT)
#include <fsl_pmu.h>
#endif /* CONFIG_CLOCK_CONTROL_NXP_MCXL_MAIN_INIT */

#define DT_DRV_COMPAT nxp_mcxl_main_clock

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
LOG_MODULE_REGISTER(clock_control_mcxl);

#if defined(__CORTEX_M) && (__CORTEX_M == 33U)

extern uint32_t SystemCoreClock;

/*! @brief Source index table - attach id. */
static const clock_attach_id_t mcxl_main_clk_attach[] = {
	[MCXL_MAIN_CLK_SYS_SRC_SIRC] = kSIRC_to_MAIN_CLK,
	[MCXL_MAIN_CLK_SYS_SRC_FIRC] = kFIRC_to_MAIN_CLK,
	[MCXL_MAIN_CLK_SYS_SRC_ROSC] = kROSC_to_MAIN_CLK,
	[MCXL_MAIN_CLK_SYS_SRC_PMUIRC] = kPMUIRC_to_MAIN_CLK,
	[MCXL_MAIN_CLK_SYS_SRC_LPIRC] = kLPIRC_to_MAIN_CLK,
};

/*! @brief Gate index table - clock_ip_name_t. */
static const clock_ip_name_t mcxl_main_gate_table[] = {
	[MCXL_MAIN_GATE_LPUART0] = kCLOCK_GateLPUART0,
	[MCXL_MAIN_GATE_LPUART1] = kCLOCK_GateLPUART1,
	[MCXL_MAIN_GATE_LPI2C0] = kCLOCK_GateLPI2C0,
	[MCXL_MAIN_GATE_LPI2C1] = kCLOCK_GateLPI2C1,
	[MCXL_MAIN_GATE_LPSPI0] = kCLOCK_GateLPSPI0,
	[MCXL_MAIN_GATE_LPSPI1] = kCLOCK_GateLPSPI1,
	[MCXL_MAIN_GATE_GPIO1] = kCLOCK_GateGPIO1,
	[MCXL_MAIN_GATE_GPIO2] = kCLOCK_GateGPIO2,
	[MCXL_MAIN_GATE_GPIO3] = kCLOCK_GateGPIO3,
	[MCXL_MAIN_GATE_PORT1] = kCLOCK_GatePORT1,
	[MCXL_MAIN_GATE_PORT2] = kCLOCK_GatePORT2,
	[MCXL_MAIN_GATE_PORT3] = kCLOCK_GatePORT3,
	[MCXL_MAIN_GATE_PERIPH_GROUP0] = kCLOCK_GatePERIPH_GROUP0,
	[MCXL_MAIN_GATE_PERIPH_GROUP1] = kCLOCK_GatePERIPH_GROUP1,
	[MCXL_MAIN_GATE_CRC] = kCLOCK_GateCrc,
	[MCXL_MAIN_GATE_DMA0] = kCLOCK_GateDMA0,
	[MCXL_MAIN_GATE_DMA1] = kCLOCK_GateDMA1,
	[MCXL_MAIN_GATE_ADC0] = kCLOCK_GateADC0,
};

/*! @brief Selector index table - clock_select_name_t. */
static const clock_select_name_t mcxl_main_sel_table[] = {
	[MCXL_MAIN_SEL_SCGSCS] = kCLOCK_SelSCGSCS,
	[MCXL_MAIN_SEL_FIRC] = kCLOCK_SelFIRC,
	[MCXL_MAIN_SEL_PERIPH_GROUP0] = kCLOCK_SelPERIPH_GROUP0,
	[MCXL_MAIN_SEL_PERIPH_GROUP1] = kCLOCK_SelPERIPH_GROUP1,
	[MCXL_MAIN_SEL_ADC0] = kCLOCK_SelADC0,
	[MCXL_MAIN_SEL_CLKOUT] = kCLOCK_SelCLKOUT,
};

/*! @brief Divider index table - clock_div_name_t. */
static const clock_div_name_t mcxl_main_div_table[] = {
	[MCXL_MAIN_DIV_FRO_HF_DIV] = kCLOCK_DivFRO_HF_DIV,
	[MCXL_MAIN_DIV_AHBCLK] = kCLOCK_DivAHBCLK,
	[MCXL_MAIN_DIV_AHBAIPSCLK] = kCLOCK_DivAHBAIPSCLK,
	[MCXL_MAIN_DIV_PERIPH_GROUP0] = kCLOCK_DivPeriphGroup0,
	[MCXL_MAIN_DIV_PERIPH_GROUP1] = kCLOCK_DivPeriphGroup1,
	[MCXL_MAIN_DIV_ADC0] = kCLOCK_DivADC0,
};

BUILD_ASSERT(MCXL_MAIN_GATE_ADC0 <= MCXL_CLOCK_GATE_IDX_MASK,
	     "main gate index namespace does not fit the gate_idx cell field");
BUILD_ASSERT(MCXL_MAIN_SEL_CLKOUT <= MCXL_CLOCK_SEL_IDX_MASK,
	     "main selector index namespace does not fit the sel_idx cell field");
BUILD_ASSERT(MCXL_MAIN_DIV_ADC0 <= MCXL_CLOCK_DIV_IDX_MASK,
	     "main divider index namespace does not fit the div_idx cell field");

BUILD_ASSERT(ARRAY_SIZE(mcxl_main_clk_attach) == (MCXL_MAIN_CLK_SYS_SRC_LPIRC + 1),
	     "mcxl_main_clk_attach[] size must match the source index namespace");
BUILD_ASSERT(ARRAY_SIZE(mcxl_main_gate_table) == (MCXL_MAIN_GATE_ADC0 + 1),
	     "mcxl_main_gate_table[] size must match the gate index namespace");
BUILD_ASSERT(ARRAY_SIZE(mcxl_main_sel_table) == (MCXL_MAIN_SEL_CLKOUT + 1),
	     "mcxl_main_sel_table[] size must match the selector index namespace");
BUILD_ASSERT(ARRAY_SIZE(mcxl_main_div_table) == (MCXL_MAIN_DIV_ADC0 + 1),
	     "mcxl_main_div_table[] size must match the divider index namespace");

/*!
 * @brief Decoded fields of the MCXL_MAIN_CLOCK() clock specifier.
 */
struct mcxl_main_clk_spec {
	uint8_t gate_idx; /*!< Gate index */
	uint8_t sel_idx;  /*!< Selector index */
	uint8_t src;      /*!< Mux source value for the selector */
	uint8_t div_idx;  /*!< Divider index */
	uint8_t div_val;  /*!< Divisor passed to CLOCK_SetClockDiv() */
	uint8_t flags;    /*!< Clock flags */
};

/*!
 * @brief Unpack all fields from a packed MCXL_MAIN_CLOCK() clock specifier.
 * @param sub_system  Packed 32-bit clock specifier built with MCXL_MAIN_CLOCK().
 * @param spec        Output struct populated with gate, selector, source,
 *                    divider, and flag fields.
 */
static void mcxl_main_decode_spec(clock_control_subsys_t sub_system,
				  struct mcxl_main_clk_spec *spec)
{
	uint32_t clk_cell = (uint32_t)(uintptr_t)sub_system;

	spec->gate_idx = (uint8_t)MCXL_MAIN_CLOCK_GATE_IDX(clk_cell);
	spec->sel_idx = (uint8_t)MCXL_MAIN_CLOCK_SEL_IDX(clk_cell);
	spec->src = (uint8_t)MCXL_MAIN_CLOCK_SRC(clk_cell);
	spec->div_idx = (uint8_t)MCXL_MAIN_CLOCK_DIV_IDX(clk_cell);
	spec->div_val = (uint8_t)MCXL_MAIN_CLOCK_DIV_VAL(clk_cell);
	spec->flags = (uint8_t)MCXL_MAIN_CLOCK_FLAGS(clk_cell);
}

/*!
 * @brief Translate a gate index to the HAL clock_ip_name_t enum value.
 * @param idx  Symbolic gate index.
 * @param out  Output HAL enum value.
 * @return true on success, false if index is NONE or out of range.
 */
static bool mcxl_main_lookup_gate(uint8_t idx, clock_ip_name_t *out)
{
	if (idx == MCXL_MAIN_GATE_NONE || idx >= ARRAY_SIZE(mcxl_main_gate_table)) {
		return false;
	}
	*out = mcxl_main_gate_table[idx];
	return true;
}

/*!
 * @brief Translate a selector index to the HAL clock_select_name_t enum value.
 * @param idx  Symbolic selector index.
 * @param out  Output HAL enum value.
 * @return true on success, false if index is NONE or out of range.
 */
static bool mcxl_main_lookup_sel(uint8_t idx, clock_select_name_t *out)
{
	if (idx == MCXL_MAIN_SEL_NONE || idx >= ARRAY_SIZE(mcxl_main_sel_table)) {
		return false;
	}
	*out = mcxl_main_sel_table[idx];
	return true;
}

/*!
 * @brief Translate a divider index to the HAL clock_div_name_t enum value.
 * @param idx  Symbolic divider index.
 * @param out  Output HAL enum value.
 * @return true on success, false if idx is NONE or out of range.
 */
static bool mcxl_main_lookup_div(uint8_t idx, clock_div_name_t *out)
{
	if (idx == MCXL_MAIN_DIV_NONE || idx >= ARRAY_SIZE(mcxl_main_div_table)) {
		return false;
	}
	*out = mcxl_main_div_table[idx];
	return true;
}

/*!
 * @brief Driver configuration read from Devicetree at build time.
 */
struct mcxl_main_clock_control_config {
	uint8_t firc_mode;            /*!< FIRC enable mode */
	uint8_t sys_clk_src;          /*!< MAIN_CLK source index */
	uint32_t frohf_frequency;     /*!< Requested FROHF frequency in Hz */
	uint8_t frohf_div;            /*!< FRO_HF_DIV prescaler */
	uint8_t ahb_div;              /*!< AHB bus clock prescaler */
	uint8_t slow_clk_div;         /*!< Slow peripheral bus prescaler. */
	uint32_t core_clock_frequency;/*!< Expected core clock frequency in Hz after init */
	bool rosc_enable;             /*!< Initialize the 32 kHz ROSC during bring-up */
};

/*!
 * @brief Enable the clock gate for a peripheral.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_MAIN_CLOCK().
 * @return 0 on success, -EINVAL if the gate index is out of range.
 */
static int nxp_mcxl_main_clock_control_on(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct mcxl_main_clk_spec spec;
	clock_ip_name_t gate;

	ARG_UNUSED(dev);
	mcxl_main_decode_spec(sub_system, &spec);

	if ((spec.flags & MCXL_MAIN_CLK_FLAG_FIXED) != 0U) {
		return 0;
	}

	if (spec.gate_idx == MCXL_MAIN_GATE_NONE) {
		return 0;
	}

	if (!mcxl_main_lookup_gate(spec.gate_idx, &gate)) {
		LOG_ERR("invalid main gate index %u", spec.gate_idx);
		return -EINVAL;
	}

	CLOCK_EnableClock(gate);

	return 0;
}

/*!
 * @brief Disable the clock gate for a peripheral.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_MAIN_CLOCK().
 * @return 0 on success, -ENOTSUP if the clock cannot be gated,
 *         -EINVAL if the gate index is out of range.
 */
static int nxp_mcxl_main_clock_control_off(const struct device *dev,
					   clock_control_subsys_t sub_system)
{
	struct mcxl_main_clk_spec spec;
	clock_ip_name_t gate;

	ARG_UNUSED(dev);
	mcxl_main_decode_spec(sub_system, &spec);

	if ((spec.flags & MCXL_MAIN_CLK_FLAG_FIXED) != 0U) {
		return -ENOTSUP;
	}

	if (spec.gate_idx == MCXL_MAIN_GATE_NONE) {
		return -ENOTSUP;
	}

	if (!mcxl_main_lookup_gate(spec.gate_idx, &gate)) {
		LOG_ERR("invalid main gate index %u", spec.gate_idx);
		return -EINVAL;
	}

	CLOCK_DisableClock(gate);

	return 0;
}

/*!
 * @brief Return the HAL functional clock frequency for a gate index.
 * @param gate_idx  Gate symbolic index.
 * @param freq      Output frequency in Hz.
 * @return true if a HAL query exists, false if the gate has no rate API.
 */
static bool mcxl_main_get_gate_freq(uint8_t gate_idx, uint32_t *freq)
{
	switch (gate_idx) {
	case MCXL_MAIN_GATE_LPUART0:
		*freq = CLOCK_GetLpuartClkFreq(0U);
		return true;
	case MCXL_MAIN_GATE_LPUART1:
		*freq = CLOCK_GetLpuartClkFreq(1U);
		return true;
	case MCXL_MAIN_GATE_LPI2C0:
		*freq = CLOCK_GetLpi2cClkFreq(0U);
		return true;
	case MCXL_MAIN_GATE_LPI2C1:
		*freq = CLOCK_GetLpi2cClkFreq(1U);
		return true;
	case MCXL_MAIN_GATE_LPSPI0:
		*freq = CLOCK_GetLpspiClkFreq(0U);
		return true;
	case MCXL_MAIN_GATE_LPSPI1:
		*freq = CLOCK_GetLpspiClkFreq(1U);
		return true;
	case MCXL_MAIN_GATE_ADC0:
		*freq = CLOCK_GetAdcClkFreq();
		return true;
	default:
		return false;
	}
}

/*!
 * @brief Return the functional clock frequency for a peripheral.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_MAIN_CLOCK().
 * @param rate       Output frequency in Hz.
 * @return 0 on success, -EINVAL if rate is NULL, -ENOTSUP if no HAL query exists.
 */
static int nxp_mcxl_main_clock_control_get_rate(const struct device *dev,
						clock_control_subsys_t sub_system,
						uint32_t *rate)
{
	struct mcxl_main_clk_spec spec;
	uint32_t freq = 0U;

	ARG_UNUSED(dev);

	if (rate == NULL) {
		return -EINVAL;
	}

	mcxl_main_decode_spec(sub_system, &spec);

	if (!mcxl_main_get_gate_freq(spec.gate_idx, &freq)) {
		return -ENOTSUP;
	}

	*rate = freq;

	return 0;
}

/*!
 * @brief Check whether a mux source value is valid for a given selector.
 * @param sel_idx  Selector symbolic index.
 * @param src      Mux source value to validate.
 * @return true if src is a valid input for sel_idx, false otherwise.
 */
static bool mcxl_main_src_valid(uint8_t sel_idx, uint8_t src)
{
	switch (sel_idx) {
	case MCXL_MAIN_SEL_SCGSCS:
		return (src == MCXL_MAIN_SCGSCS_SRC_SIRC)   ||
		       (src == MCXL_MAIN_SCGSCS_SRC_FIRC)   ||
		       (src == MCXL_MAIN_SCGSCS_SRC_ROSC)   ||
		       (src == MCXL_MAIN_SCGSCS_SRC_PMUIRC) ||
		       (src == MCXL_MAIN_SCGSCS_SRC_LPIRC);
	case MCXL_MAIN_SEL_FIRC:
		return (src == MCXL_MAIN_FIRC_SRC_DIRECT) ||
		       (src == MCXL_MAIN_FIRC_SRC_DIV);
	case MCXL_MAIN_SEL_PERIPH_GROUP0:
	case MCXL_MAIN_SEL_PERIPH_GROUP1:
		return (src <= MCXL_MAIN_PERIPH_GRP_SRC_FROHF_DIV);
	case MCXL_MAIN_SEL_ADC0:
		return (src == MCXL_MAIN_ADC0_SRC_FRO12M)    ||
		       (src == MCXL_MAIN_ADC0_SRC_XTAL32K)   ||
		       (src == MCXL_MAIN_ADC0_SRC_FROHF_DIV);
	case MCXL_MAIN_SEL_CLKOUT:
		return (src == MCXL_MAIN_CLKOUT_SRC_FRO12M)    ||
		       (src == MCXL_MAIN_CLKOUT_SRC_SLOW_CLK)  ||
		       (src == MCXL_MAIN_CLKOUT_SRC_CLK16K)    ||
		       (src == MCXL_MAIN_CLKOUT_SRC_FRO10M)    ||
		       (src == MCXL_MAIN_CLKOUT_SRC_FROHF_DIV);
	default:
		return false;
	}
}

/*!
 * @brief Program the clock mux selector and/or divider for a peripheral clock.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_MAIN_CLOCK().
 * @return 0 on success, -EINVAL if a selector or divider index is invalid or
 *         the mux source value is not valid for the given selector.
 */
static int nxp_mcxl_main_clock_control_configure(const struct device *dev,
						 clock_control_subsys_t sub_system,
						 void *data)
{
	struct mcxl_main_clk_spec spec;
	clock_select_name_t sel;
	clock_div_name_t clk_div;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	mcxl_main_decode_spec(sub_system, &spec);

	if (spec.sel_idx != MCXL_MAIN_SEL_NONE) {
		if (!mcxl_main_lookup_sel(spec.sel_idx, &sel)) {
			LOG_ERR("invalid main selector index %u", spec.sel_idx);
			return -EINVAL;
		}
		if (!mcxl_main_src_valid(spec.sel_idx, spec.src)) {
			LOG_ERR("invalid src %u for main selector index %u",
				spec.src, spec.sel_idx);
			return -EINVAL;
		}
		CLOCK_SetClockSelect(sel, (uint32_t)spec.src);
	}

	if (spec.div_idx != MCXL_MAIN_DIV_NONE) {
		if (!mcxl_main_lookup_div(spec.div_idx, &clk_div)) {
			LOG_ERR("invalid main divider index %u", spec.div_idx);
			return -EINVAL;
		}
		if ((spec.flags & MCXL_MAIN_CLK_FLAG_DIV_HALT) != 0U) {
			CLOCK_HaltClockDiv(clk_div);
		} else {
			CLOCK_SetClockDiv(clk_div, (uint32_t)spec.div_val);
		}
	}

	return 0;
}

/*!
 * @brief Return the run status of a clock.
 * @param sub_system Packed 32-bit clock specifier built with MCXL_MAIN_CLOCK().
 * @return CLOCK_CONTROL_STATUS_ON for fixed clocks,
 *         CLOCK_CONTROL_STATUS_UNKNOWN otherwise.
 */
static enum clock_control_status
nxp_mcxl_main_clock_control_get_status(const struct device *dev,
				       clock_control_subsys_t sub_system)
{
	struct mcxl_main_clk_spec spec;
	clock_ip_name_t gate;

	ARG_UNUSED(dev);
	mcxl_main_decode_spec(sub_system, &spec);

	if ((spec.flags & MCXL_MAIN_CLK_FLAG_FIXED) != 0U) {
		/* Fixed clocks are always on. */
		return CLOCK_CONTROL_STATUS_ON;
	}

	if (spec.gate_idx == MCXL_MAIN_GATE_NONE) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (!mcxl_main_lookup_gate(spec.gate_idx, &gate)) {
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
static int nxp_mcxl_main_clock_control_pm(const struct device *dev,
					  enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);
	return 0;
}

/*!
 * @brief Validate the sys_clk_src field in the driver configuration.
 * @param config  Driver configuration from Devicetree.
 * @return 0 on success, -EINVAL if the source index is out of range or
 *         FIRC is selected while disabled.
 */
static int nxp_mcxl_main_validate_sys_clk_src(const struct mcxl_main_clock_control_config *config)
{
	if (config->sys_clk_src >= ARRAY_SIZE(mcxl_main_clk_attach)) {
		LOG_ERR("MAIN_CLK source index %u out of range", config->sys_clk_src);
		return -EINVAL;
	}

	if (config->sys_clk_src == MCXL_MAIN_CLK_SYS_SRC_FIRC &&
	    config->firc_mode == MCXL_MAIN_CLK_FIRC_DISABLE) {
		LOG_ERR("FIRC selected as MAIN_CLK source but FIRC is disabled");
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_CLOCK_CONTROL_NXP_MCXL_MAIN_INIT)
/*!
 * @brief Perform the full system-clock bring-up sequence.
 * @param config  Driver configuration from Devicetree.
 * @return 0 on success, -EIO if the resulting core clock does not match
 *         the expected frequency.
 */
static int nxp_mcxl_main_bring_up(const struct mcxl_main_clock_control_config *config)
{
	vdd_core_main_config_t vdd_core_main_config;

	CLOCK_EnableClock(kCLOCK_GateAonAPB);

	CLOCK_GetVDDCoreMainConfig(kCLOCK_StandardDrive, &vdd_core_main_config);
	PMU_UpdateVDDCore1P1InActiveMode(AON__PMU, vdd_core_main_config.vddCoreMainAconfig);
	PMU_UpdateLvdLvTrim(AON__PMU, vdd_core_main_config.lvdLvTrim);
	PMU_UpdateHvdLvTrim(AON__PMU, vdd_core_main_config.hvdLvTrim);

	CLOCK_AttachClk(kSIRC_to_MAIN_CLK);

	CLOCK_SetFlashWaitStateBasedOnFreq(MCXL_MAIN_SIRC_FREQ_HZ);

	if (config->firc_mode != MCXL_MAIN_CLK_FIRC_DISABLE) {
		/* FROHF is derived from FIRC; program the requested frequency. */
		if (CLOCK_SetupFROHFClocking(config->frohf_frequency, 0U) != kStatus_Success) {
			LOG_ERR("FROHF clocking setup failed for %u Hz",
				config->frohf_frequency);
			return -EIO;
		}
	}

	CLOCK_SetClockDiv(kCLOCK_DivFRO_HF_DIV, (uint32_t)config->frohf_div);

	CLOCK_SetClockDiv(kCLOCK_DivAHBCLK, (uint32_t)config->ahb_div);
	CLOCK_SetClockDiv(kCLOCK_DivAHBAIPSCLK, (uint32_t)config->slow_clk_div);

	CLOCK_SetFlashWaitStateBasedOnFreq(config->core_clock_frequency);

	CLOCK_AttachClk(mcxl_main_clk_attach[config->sys_clk_src]);

	if (config->rosc_enable) {
		if (!CLOCK_IsRoscInitialized()) {
			rosc_init_config_t rosc_init_config;

			CLOCK_GetDefaultInitRoscConfig(&rosc_init_config);
			(void)CLOCK_InitRosc(&rosc_init_config);
		}
	}

	SystemCoreClock = CLOCK_GetCoreSysClkFreq();

	if (SystemCoreClock != config->core_clock_frequency) {
		LOG_ERR("Core clock mismatch: expected %u Hz, got %u Hz",
			config->core_clock_frequency, SystemCoreClock);
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_CLOCK_CONTROL_NXP_MCXL_MAIN_INIT */

/*!
 * @brief Zephyr device driver init callback.
 *
 * Validates configuration, optionally runs bring_up(), and publishes the
 * current core clock frequency to SystemCoreClock.
 *
 * @param dev Clock controller device.
 * @return 0 on success, negative errno on failure.
 */
static int nxp_mcxl_main_clock_control_init(const struct device *dev)
{
	const struct mcxl_main_clock_control_config *config = dev->config;
	int ret;

	ret = nxp_mcxl_main_validate_sys_clk_src(config);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_CLOCK_CONTROL_NXP_MCXL_MAIN_INIT)
	ret = nxp_mcxl_main_bring_up(config);
	if (ret != 0) {
		return ret;
	}
#else
	SystemCoreClock = CLOCK_GetCoreSysClkFreq();
#endif /* CONFIG_CLOCK_CONTROL_NXP_MCXL_MAIN_INIT */

	return pm_device_driver_init(dev, nxp_mcxl_main_clock_control_pm);
}

static DEVICE_API(clock_control, nxp_mcxl_main_clock_control_api) = {
	.on = nxp_mcxl_main_clock_control_on,
	.off = nxp_mcxl_main_clock_control_off,
	.get_rate = nxp_mcxl_main_clock_control_get_rate,
	.get_status = nxp_mcxl_main_clock_control_get_status,
	.configure = nxp_mcxl_main_clock_control_configure,
};

static const struct mcxl_main_clock_control_config mcxl_main_config = {
	.firc_mode = DT_INST_PROP(0, firc_mode),
	.sys_clk_src = DT_INST_PROP(0, sys_clk_src),
	.frohf_frequency = DT_INST_PROP(0, frohf_frequency),
	.frohf_div = DT_INST_PROP(0, frohf_div),
	.ahb_div = DT_INST_PROP(0, ahb_div),
	.slow_clk_div = DT_INST_PROP(0, slow_clk_div),
	.core_clock_frequency = DT_INST_PROP(0, core_clock_frequency),
	.rosc_enable = DT_INST_PROP(0, rosc_enable),
};

DEVICE_DT_INST_DEFINE(0, nxp_mcxl_main_clock_control_init, NULL, NULL, &mcxl_main_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &nxp_mcxl_main_clock_control_api);

#endif /* __CORTEX_M == 33U */
