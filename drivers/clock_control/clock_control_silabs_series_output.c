/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_series_clock_output

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include <sl_clock_manager.h>
#include <sl_status.h>

#if defined(_CMU_EXPORTCLKCTRL_CLKOUTPRESC_MASK)
#define SILABS_CLOCK_OUTPUT_HAS_DIV 1
#else
#define SILABS_CLOCK_OUTPUT_HAS_DIV 0
#endif

struct silabs_clock_output {
	uint32_t value;
	uint32_t mask;
};

struct silabs_clock_output_config {
	CMU_TypeDef *cmu;
	const struct pinctrl_dev_config *pcfg;
	struct silabs_clock_output output[3];
	uint8_t export_presc;
	uint8_t presc;
};

static enum clock_control_status silabs_clock_output_get_status(const struct device *dev,
								clock_control_subsys_t sys)
{
	const struct silabs_clock_output_config *config = dev->config;
	uintptr_t idx = (uintptr_t)sys;

	if (idx >= ARRAY_SIZE(config->output)) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if ((config->cmu->EXPORTCLKCTRL & config->output[idx].mask) != 0) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static int silabs_clock_output_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct silabs_clock_output_config *config = dev->config;
	uintptr_t idx = (uintptr_t)sys;

	if (idx >= ARRAY_SIZE(config->output)) {
		return -EINVAL;
	}

	if (!config->output[idx].value) {
		return -ENODEV;
	}

	if (silabs_clock_output_get_status(dev, sys) == CLOCK_CONTROL_STATUS_ON) {
		return -EALREADY;
	}

	config->cmu->EXPORTCLKCTRL_SET = config->output[idx].value;

	return 0;
}

static int silabs_clock_output_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct silabs_clock_output_config *config = dev->config;
	uintptr_t idx = (uintptr_t)sys;

	if (idx >= ARRAY_SIZE(config->output)) {
		return -EINVAL;
	}

	config->cmu->EXPORTCLKCTRL_CLR = config->output[idx].mask;

	return 0;
}

static int silabs_clock_output_get_rate(const struct device *dev, clock_control_subsys_t sys,
					uint32_t *rate)
{
	const struct silabs_clock_output_config *config = dev->config;
	uintptr_t idx = (uintptr_t)sys;
	sl_status_t status;

	if (idx >= ARRAY_SIZE(config->output)) {
		return -EINVAL;
	}

	if (!config->output[idx].value) {
		return -ENODEV;
	}

	switch (FIELD_GET(config->output[idx].mask, config->output[idx].value)) {
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_HCLK:
		status = sl_clock_manager_get_clock_branch_frequency(SL_CLOCK_BRANCH_HCLK, rate);
		break;
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFEXPCLK:
		status = sl_clock_manager_get_clock_branch_frequency(SL_CLOCK_BRANCH_EXPORTCLK,
								     rate);
		break;
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_ULFRCO:
		status = sl_clock_manager_get_oscillator_frequency(SL_OSCILLATOR_ULFRCO, rate);
		break;
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_LFRCO:
		status = sl_clock_manager_get_oscillator_frequency(SL_OSCILLATOR_LFRCO, rate);
		break;
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_LFXO:
		status = sl_clock_manager_get_oscillator_frequency(SL_OSCILLATOR_LFXO, rate);
		break;
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFRCODPLL:
		status = sl_clock_manager_get_oscillator_frequency(SL_OSCILLATOR_HFRCODPLL, rate);
		break;
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFXO:
		status = sl_clock_manager_get_oscillator_frequency(SL_OSCILLATOR_HFXO, rate);
		break;
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_FSRCO:
		status = sl_clock_manager_get_oscillator_frequency(SL_OSCILLATOR_FSRCO, rate);
		break;
#if defined(_CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFRCOEM23)
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFRCOEM23:
		status = sl_clock_manager_get_oscillator_frequency(SL_OSCILLATOR_HFRCOEM23, rate);
		break;
#endif
#if defined(_CMU_EXPORTCLKCTRL_CLKOUTSEL0_SOCPLL)
	case _CMU_EXPORTCLKCTRL_CLKOUTSEL0_SOCPLL:
		status = sl_clock_manager_get_oscillator_frequency(SL_OSCILLATOR_SOCPLL0, rate);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	if (status != SL_STATUS_OK) {
		return -ENOTSUP;
	}

	*rate /= config->presc + 1;

	return 0;
}

static int silabs_clock_output_init(const struct device *dev)
{
	const struct silabs_clock_output_config *config = dev->config;
	int ret;

	config->cmu->EXPORTCLKCTRL =
		FIELD_PREP(_CMU_EXPORTCLKCTRL_PRESC_MASK, config->export_presc);

#if SILABS_CLOCK_OUTPUT_HAS_DIV
	config->cmu->EXPORTCLKCTRL_SET =
		FIELD_PREP(_CMU_EXPORTCLKCTRL_CLKOUTPRESC_MASK, config->presc);
#endif

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	return 0;
}

static DEVICE_API(clock_control, silabs_clock_output_api) = {
	.on = silabs_clock_output_on,
	.off = silabs_clock_output_off,
	.get_rate = silabs_clock_output_get_rate,
	.get_status = silabs_clock_output_get_status,
};

#define SILABS_CLOCK_OUTPUT_VALUE(n)                                                               \
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(n), (                                                  \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(hclk)),                   \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_HCLK), (                                \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(exportclk)),              \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFEXPCLK), (                            \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(ulfrco)),                 \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_ULFRCO), (                              \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(lfrco)),                  \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_LFRCO), (                               \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(lfxo)),                   \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_LFXO), (                                \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(hfrcodpll)),              \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFRCODPLL), (                           \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(hfxo)),                   \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFXO), (                                \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(fsrco)),                  \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_FSRCO), (                               \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(hfrcoem23)),              \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFRCOEM23), (                           \
		COND_CODE_1(DT_SAME_NODE(DT_CLOCKS_CTLR(n), DT_NODELABEL(socpll)),                 \
			    (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_SOCPLL), (SILABS_CLOCK_OUTPUT_INVALID)  \
		)))))))))))))))))))                                                                \
	), (_CMU_EXPORTCLKCTRL_CLKOUTSEL0_DISABLED))

#define SILABS_CLOCK_OUTPUT(n)                                                                     \
	[DT_REG_ADDR(n)] = {                                                                       \
		.value = SILABS_CLOCK_OUTPUT_VALUE(n)                                              \
			 << (_CMU_EXPORTCLKCTRL_CLKOUTSEL1_SHIFT * DT_REG_ADDR(n)),                \
		.mask = _CMU_EXPORTCLKCTRL_CLKOUTSEL0_MASK                                         \
			<< (_CMU_EXPORTCLKCTRL_CLKOUTSEL1_SHIFT * DT_REG_ADDR(n)),                 \
	},

#define CLOCK_OUTPUT_INIT(idx)                                                                     \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static const struct silabs_clock_output_config silabs_clock_output_config##idx = {         \
		.cmu = (CMU_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(idx)),                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.export_presc = DT_PROP_OR(DT_NODELABEL(exportclk), clock_div, 1) - 1,             \
		.presc = DT_INST_PROP_OR(idx, clock_div, 1) - 1,                                   \
		.output = {DT_INST_FOREACH_CHILD(idx, SILABS_CLOCK_OUTPUT)},                       \
	};                                                                                         \
	BUILD_ASSERT(DT_PROP_OR(DT_NODELABEL(exportclk), clock_div, 1) >= 1,                       \
		     "exportclk clock-div too small");                                             \
	BUILD_ASSERT(DT_PROP_OR(DT_NODELABEL(exportclk), clock_div, 1) <= 32,                      \
		     "exportclk clock-div too large");                                             \
	IF_DISABLED(SILABS_CLOCK_OUTPUT_HAS_DIV,                                                   \
		    (BUILD_ASSERT(DT_INST_PROP_OR(idx, clock_div, 1) == 1,                         \
				  "clock-div not supported for clkout on this device");))          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, silabs_clock_output_init, NULL, NULL,                           \
			      &silabs_clock_output_config##idx, PRE_KERNEL_1,                      \
			      CONFIG_CLOCK_CONTROL_SILABS_SERIES_OUTPUT_INIT_PRIORITY,             \
			      &silabs_clock_output_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_OUTPUT_INIT)
