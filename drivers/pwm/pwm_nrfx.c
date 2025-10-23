/*
 * Copyright (c) 2018, Cue Health Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_pwm

#include <nrfx_pwm.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <hal/nrf_gpio.h>
#include <stdbool.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/cache.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_nrfx, CONFIG_PWM_LOG_LEVEL);

#define PWM_NRFX_IS_FAST(inst) NRF_DT_IS_FAST(DT_DRV_INST(inst))

#if NRF_DT_INST_ANY_IS_FAST
#define PWM_NRFX_FAST_PRESENT 1
/* If fast instances are used then system managed device PM cannot be used because
 * it may call PM actions from locked context and fast PWM PM actions can only be
 * called in a thread context.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_PM_DEVICE_SYSTEM_MANAGED));
#endif

#if defined(PWM_NRFX_FAST_PRESENT) && CONFIG_CLOCK_CONTROL_NRF_HSFLL_GLOBAL
#define PWM_NRFX_USE_CLOCK_CONTROL 1
#endif

#define PWM_NRFX_CH_POLARITY_MASK BIT(15)
#define PWM_NRFX_CH_COMPARE_MASK  BIT_MASK(15)
#define PWM_NRFX_CH_VALUE(compare_value, inverted) \
	(compare_value | (inverted ? 0 : PWM_NRFX_CH_POLARITY_MASK))

struct pwm_nrfx_config {
	nrfx_pwm_config_t initial_config;
	nrf_pwm_sequence_t seq;
	const struct pinctrl_dev_config *pcfg;
	uint32_t clock_freq;
#ifdef CONFIG_DCACHE
	uint32_t mem_attr;
#endif
#ifdef PWM_NRFX_USE_CLOCK_CONTROL
	const struct device *clk_dev;
	struct nrf_clock_spec clk_spec;
#endif
};

struct pwm_nrfx_data {
	nrfx_pwm_t pwm;
	uint32_t period_cycles;
	/* Bit mask indicating channels that need the PWM generation. */
	uint8_t  pwm_needed;
	uint8_t  prescaler;
	bool     stop_requested;
#ifdef PWM_NRFX_USE_CLOCK_CONTROL
	bool     clock_requested;
#endif
};

#if NRF_ERRATA_STATIC_CHECK(52, 109)
/* Forward-declare pwm_nrfx_<inst>_data structs to be able to access nrfx_pwm_t needed for the
 * workaround.
 */
#define _PWM_DATA_STRUCT_NAME_GET(inst) pwm_nrfx_##inst##_data
#define _PWM_DATA_STRUCT_DECLARE(inst) static struct pwm_nrfx_data _PWM_DATA_STRUCT_NAME_GET(inst);
DT_INST_FOREACH_STATUS_OKAY(_PWM_DATA_STRUCT_DECLARE);

/* Create an array of pointers to all active PWM instances to loop over them in an EGU interrupt
 * handler.
 */
#define _PWM_DATA_STRUCT_PWM_PTR_COMMA_GET(inst) &_PWM_DATA_STRUCT_NAME_GET(inst).pwm,
static nrfx_pwm_t *pwm_instances[] = {
	DT_INST_FOREACH_STATUS_OKAY(_PWM_DATA_STRUCT_PWM_PTR_COMMA_GET)
};

/* Define an interrupt handler for the EGU instance used by the workaround which calls
 * nrfx_pwm_nrf52_anomaly_109_handler for all active PWM instances.
 */
void anomaly_109_egu_handler(void)
{
	for (int i = 0; i < ARRAY_SIZE(pwm_instances); i++) {
		nrfx_pwm_nrf52_anomaly_109_handler(pwm_instances[i]);
	}
}

#define ANOMALY_109_EGU_IRQ_CONNECT(idx) _EGU_IRQ_CONNECT(idx)
#define _EGU_IRQ_CONNECT(idx)				       \
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(egu##idx)),	       \
		    DT_IRQ(DT_NODELABEL(egu##idx), priority),  \
		    anomaly_109_egu_handler, 0, 0)
#else
#define ANOMALY_109_EGU_IRQ_CONNECT(idx)
#endif

/* Ensure the pwm_needed bit mask can accommodate all available channels. */
#if (NRF_PWM_CHANNEL_COUNT > 8)
#error "Current implementation supports maximum 8 channels."
#endif

#ifdef PWM_NRFX_FAST_PRESENT
static bool pwm_is_fast(const struct pwm_nrfx_config *config)
{
	return config->clock_freq > MHZ(16);
}
#else
static bool pwm_is_fast(const struct pwm_nrfx_config *config)
{
	return false;
}
#endif

static uint16_t *seq_values_ptr_get(const struct device *dev)
{
	const struct pwm_nrfx_config *config = dev->config;

	return (uint16_t *)config->seq.values.p_raw;
}

static void pwm_handler(nrfx_pwm_event_t event, void *p_context)
{
	ARG_UNUSED(event);
	ARG_UNUSED(p_context);
}

static bool pwm_period_check_and_set(const struct device *dev,
				     uint32_t channel, uint32_t period_cycles)
{
	const struct pwm_nrfx_config *config = dev->config;
	struct pwm_nrfx_data *data = dev->data;
	uint8_t prescaler;
	uint32_t countertop;

	/* If the currently configured period matches the requested one,
	 * nothing more needs to be done.
	 */
	if (period_cycles == data->period_cycles) {
		return true;
	}

	/* If any other channel is driven by the PWM peripheral, the period
	 * that is currently set cannot be changed, as this would influence
	 * the output for that channel.
	 */
	if ((data->pwm_needed & ~BIT(channel)) != 0) {
		LOG_ERR("Incompatible period.");
		return false;
	}

	/* Try to find a prescaler that will allow setting the requested period
	 * after prescaling as the countertop value for the PWM peripheral.
	 */
	prescaler = 0;
	countertop = period_cycles;
	do {
		if (countertop <= PWM_COUNTERTOP_COUNTERTOP_Msk) {
			data->period_cycles = period_cycles;
			data->prescaler     = prescaler;

			nrf_pwm_configure(data->pwm.p_reg,
					  data->prescaler,
					  config->initial_config.count_mode,
					  (uint16_t)countertop);
			return true;
		}

		countertop >>= 1;
		++prescaler;
	} while (prescaler <= PWM_PRESCALER_PRESCALER_Msk);

	LOG_ERR("Prescaler for period_cycles %u not found.", period_cycles);
	return false;
}

static bool channel_psel_get(uint32_t channel, uint32_t *psel, struct pwm_nrfx_data *data)
{
	*psel = nrf_pwm_pin_get(data->pwm.p_reg, (uint8_t)channel);

	return (((*psel & PWM_PSEL_OUT_CONNECT_Msk) >> PWM_PSEL_OUT_CONNECT_Pos)
		== PWM_PSEL_OUT_CONNECT_Connected);
}

static int stop_pwm(const struct device *dev)
{
	struct pwm_nrfx_data *data = dev->data;

	/* Don't wait here for the peripheral to actually stop. Instead,
	* ensure it is stopped before starting the next playback.
	*/
	nrfx_pwm_stop(&data->pwm, false);

#if PWM_NRFX_USE_CLOCK_CONTROL
	const struct pwm_nrfx_config *config = dev->config;

	if (data->clock_requested) {
		int ret = nrf_clock_control_release(config->clk_dev, &config->clk_spec);

		if (ret < 0) {
			LOG_ERR("Global HSFLL release failed: %d", ret);
			return ret;
		}

		data->clock_requested = false;
	}
#endif

	return 0;
}

static int pwm_nrfx_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	/* We assume here that period_cycles will always be 16MHz
	 * peripheral clock. Since pwm_nrfx_get_cycles_per_sec() function might
	 * be removed, see ISSUE #6958.
	 * TODO: Remove this comment when issue has been resolved.
	 */
	const struct pwm_nrfx_config *config = dev->config;
	struct pwm_nrfx_data *data = dev->data;
	uint16_t compare_value;
	bool inverted = (flags & PWM_POLARITY_INVERTED);
	bool needs_pwm = false;

	if (channel >= NRF_PWM_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel: %u.", channel);
		return -EINVAL;
	}

	/* If this PWM is in center-aligned mode, pulse and period lengths
	 * are effectively doubled by the up-down count, so halve them here
	 * to compensate.
	 */
	if (config->initial_config.count_mode == NRF_PWM_MODE_UP_AND_DOWN) {
		period_cycles /= 2;
		pulse_cycles /= 2;
	}

	if (pulse_cycles == 0) {
		/* Constantly inactive (duty 0%). */
		compare_value = 0;
	} else if (pulse_cycles >= period_cycles) {
		/* Constantly active (duty 100%). */
		/* This value is always greater than or equal to COUNTERTOP. */
		compare_value = PWM_NRFX_CH_COMPARE_MASK;
		needs_pwm = pwm_is_fast(config) ||
			(IS_ENABLED(NRF_PWM_HAS_IDLEOUT) &&
			 IS_ENABLED(CONFIG_PWM_NRFX_NO_GLITCH_DUTY_100));
	} else {
		/* PWM generation needed. Check if the requested period matches
		 * the one that is currently set, or the PWM peripheral can be
		 * reconfigured accordingly.
		 */
		if (!pwm_period_check_and_set(dev, channel, period_cycles)) {
			return -EINVAL;
		}

		compare_value = (uint16_t)(pulse_cycles >> data->prescaler);
		needs_pwm = true;
	}

	seq_values_ptr_get(dev)[channel] = PWM_NRFX_CH_VALUE(compare_value, inverted);

#ifdef CONFIG_DCACHE
	if (config->mem_attr & DT_MEM_CACHEABLE) {
		sys_cache_data_flush_range(seq_values_ptr_get(dev), config->seq.length);
	}
#endif

	LOG_DBG("channel %u, pulse %u, period %u, prescaler: %u.",
		channel, pulse_cycles, period_cycles, data->prescaler);

	/* If this channel does not need to be driven by the PWM peripheral
	 * because its state is to be constant (duty 0% or 100%), set properly
	 * the GPIO configuration for its output pin. This will provide
	 * the correct output state for this channel when the PWM peripheral
	 * is stopped.
	 */
	if (!needs_pwm) {
		uint32_t psel;

		if (channel_psel_get(channel, &psel, data)) {
			uint32_t out_level = (pulse_cycles == 0) ? 0 : 1;

			if (inverted) {
				out_level ^= 1;
			}

			nrf_gpio_pin_write(psel, out_level);
		}

		data->pwm_needed &= ~BIT(channel);
	} else {
		data->pwm_needed |= BIT(channel);
	}

	/* If the PWM generation is not needed for any channel (all are set
	 * to constant inactive or active state), stop the PWM peripheral.
	 * Otherwise, request a playback of the defined sequence so that
	 * the PWM peripheral loads `seq_values` into its internal compare
	 * registers and drives its outputs accordingly.
	 */
	if (data->pwm_needed == 0) {
		if (pwm_is_fast(config)) {
#if PWM_NRFX_USE_CLOCK_CONTROL
			if (data->clock_requested) {
				int ret = nrf_clock_control_release(config->clk_dev,
							    &config->clk_spec);

				if (ret < 0) {
					LOG_ERR("Global HSFLL release failed: %d", ret);
					return ret;
				}

				data->clock_requested = false;
			}
#endif
			return 0;
		}
		int ret = stop_pwm(dev);

		if (ret < 0) {
			LOG_ERR("PWM stop failed: %d", ret);
			return ret;
		}

		data->stop_requested = true;
	} else {
		if (data->stop_requested) {
			data->stop_requested = false;

			/* After a stop is requested, the PWM peripheral stops
			 * pulse generation at the end of the current period,
			 * and till that moment, it ignores any start requests,
			 * so ensure here that it is stopped.
			 */
			while (!nrfx_pwm_stopped_check(&data->pwm)) {
			}
		}

		/* It is sufficient to play the sequence once without looping.
		 * The PWM generation will continue with the loaded values
		 * until another playback is requested (new values will be
		 * loaded then) or the PWM peripheral is stopped.
		 */
#if PWM_NRFX_USE_CLOCK_CONTROL
		if (config->clk_dev && !data->clock_requested) {
			int ret = nrf_clock_control_request_sync(config->clk_dev,
								 &config->clk_spec,
								 K_FOREVER);

			if (ret < 0) {
				LOG_ERR("Global HSFLL request failed: %d", ret);
				return ret;
			}

			data->clock_requested = true;
		}
#endif
		nrfx_pwm_simple_playback(&data->pwm, &config->seq, 1,
					 NRFX_PWM_FLAG_NO_EVT_FINISHED);
	}

	return 0;
}

static int pwm_nrfx_get_cycles_per_sec(const struct device *dev, uint32_t channel,
				       uint64_t *cycles)
{
	const struct pwm_nrfx_config *config = dev->config;

	*cycles = config->clock_freq;

	return 0;
}

static DEVICE_API(pwm, pwm_nrfx_drv_api_funcs) = {
	.set_cycles = pwm_nrfx_set_cycles,
	.get_cycles_per_sec = pwm_nrfx_get_cycles_per_sec,
};

static int pwm_resume(const struct device *dev)
{
	const struct pwm_nrfx_config *config = dev->config;
	struct pwm_nrfx_data *data = dev->data;

	uint8_t initially_inverted = 0;

	(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	for (size_t i = 0; i < NRF_PWM_CHANNEL_COUNT; i++) {
		uint32_t psel;

		if (channel_psel_get(i, &psel, data)) {
			/* Mark channels as inverted according to what initial
			 * state of their outputs has been set by pinctrl (high
			 * idle state means that the channel is inverted).
			 */
			initially_inverted |= nrf_gpio_pin_out_read(psel) ?
					      BIT(i) : 0;
		}
	}

	for (size_t i = 0; i < NRF_PWM_CHANNEL_COUNT; i++) {
		bool inverted = initially_inverted & BIT(i);

		seq_values_ptr_get(dev)[i] = PWM_NRFX_CH_VALUE(0, inverted);
	}

	return 0;
}

static int pwm_suspend(const struct device *dev)
{
	const struct pwm_nrfx_config *config = dev->config;
	struct pwm_nrfx_data *data = dev->data;

	int ret = stop_pwm(dev);

	if (ret < 0) {
		LOG_ERR("PWM stop failed: %d", ret);
		return ret;
	}

	while (!nrfx_pwm_stopped_check(&data->pwm)) {
	}

	memset(dev->data, 0, sizeof(struct pwm_nrfx_data));
	(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);

	return 0;
}

static int pwm_nrfx_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	if (action == PM_DEVICE_ACTION_RESUME) {
		return pwm_resume(dev);
	} else if (IS_ENABLED(CONFIG_PM_DEVICE) && (action == PM_DEVICE_ACTION_SUSPEND)) {
		return pwm_suspend(dev);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int pwm_nrfx_init(const struct device *dev)
{
	const struct pwm_nrfx_config *config = dev->config;
	struct pwm_nrfx_data *data = dev->data;

	int err;

	ANOMALY_109_EGU_IRQ_CONNECT(NRFX_PWM_NRF52_ANOMALY_109_EGU_INSTANCE);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	}

	err = nrfx_pwm_init(&data->pwm, &config->initial_config, pwm_handler, dev->data);
	if (err < 0) {
		LOG_ERR("Failed to initialize device: %s", dev->name);
		return err;
	}

	return pm_device_driver_init(dev, pwm_nrfx_pm_action);
}

#define PWM_MEM_REGION(inst)     DT_PHANDLE(DT_DRV_INST(inst), memory_regions)

#define PWM_MEMORY_SECTION(inst)					      \
	COND_CODE_1(DT_NODE_HAS_PROP(inst, memory_regions),		      \
		(__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(	      \
			PWM_MEM_REGION(inst)))))),			      \
		())

#define PWM_GET_MEM_ATTR(inst)						      \
	COND_CODE_1(DT_NODE_HAS_PROP(inst, memory_regions),		      \
		(DT_PROP_OR(PWM_MEM_REGION(inst), zephyr_memory_attr, 0)), (0))

/* Fast instances depend on the global HSFLL clock controller (as they need
 * to request the highest frequency from it to operate correctly), so they
 * must be initialized after that controller driver, hence the default PWM
 * initialization priority may be too early for them.
 */
#if defined(CONFIG_CLOCK_CONTROL_NRF_HSFLL_GLOBAL_INIT_PRIORITY) && \
	CONFIG_PWM_INIT_PRIORITY < CONFIG_CLOCK_CONTROL_NRF_HSFLL_GLOBAL_INIT_PRIORITY
#define PWM_INIT_PRIORITY(inst)								\
	COND_CODE_1(PWM_NRFX_IS_FAST(inst),						\
		    (UTIL_INC(CONFIG_CLOCK_CONTROL_NRF_HSFLL_GLOBAL_INIT_PRIORITY)),	\
		    (CONFIG_PWM_INIT_PRIORITY))
#else
#define PWM_INIT_PRIORITY(inst) CONFIG_PWM_INIT_PRIORITY
#endif

#define PWM_NRFX_DEFINE(inst)							     \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(DT_DRV_INST(inst));			     \
	NRF_DT_CHECK_NODE_HAS_REQUIRED_MEMORY_REGIONS(DT_DRV_INST(inst));	     \
	static struct pwm_nrfx_data pwm_nrfx_##inst##_data = {			     \
		.pwm = NRFX_PWM_INSTANCE(DT_INST_REG_ADDR(inst)),		     \
	};									     \
	static uint16_t pwm_##inst##_seq_values[NRF_PWM_CHANNEL_COUNT]		     \
			PWM_MEMORY_SECTION(inst);				     \
	PINCTRL_DT_INST_DEFINE(inst);						     \
	static const struct pwm_nrfx_config pwm_nrfx_##inst##_config = {	     \
		.initial_config = {						     \
			.skip_gpio_cfg = true,					     \
			.skip_psel_cfg = true,					     \
			.base_clock = NRF_PWM_CLK_1MHz,				     \
			.count_mode = (DT_INST_PROP(inst, center_aligned)	     \
				       ? NRF_PWM_MODE_UP_AND_DOWN		     \
				       : NRF_PWM_MODE_UP),			     \
			.top_value = 1000,					     \
			.load_mode = NRF_PWM_LOAD_INDIVIDUAL,			     \
			.step_mode = NRF_PWM_STEP_TRIGGERED,			     \
		},								     \
		.seq.values.p_raw = pwm_##inst##_seq_values,			     \
		.seq.length = NRF_PWM_CHANNEL_COUNT,				     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			     \
		.clock_freq = COND_CODE_1(DT_INST_CLOCKS_HAS_IDX(inst, 0),	     \
			(DT_PROP(DT_INST_CLOCKS_CTLR(inst), clock_frequency)),	     \
			(16ul * 1000ul * 1000ul)),				     \
		IF_ENABLED(CONFIG_DCACHE,					     \
			(.mem_attr = PWM_GET_MEM_ATTR(inst),))			     \
		IF_ENABLED(PWM_NRFX_USE_CLOCK_CONTROL,				     \
			(.clk_dev = PWM_NRFX_IS_FAST(inst)			     \
				    ? DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst))	     \
				    : NULL,					     \
			 .clk_spec = {						     \
				.frequency =					     \
					NRF_PERIPH_GET_FREQUENCY(DT_DRV_INST(inst)), \
			 },))							     \
	};									     \
	static int pwm_nrfx_init##inst(const struct device *dev)		     \
	{									     \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),	     \
			    nrfx_pwm_irq_handler, &pwm_nrfx_##inst##_data.pwm, 0);   \
		return pwm_nrfx_init(dev);					     \
	};									     \
	PM_DEVICE_DT_INST_DEFINE(inst, pwm_nrfx_pm_action);			     \
	DEVICE_DT_INST_DEINIT_DEFINE(inst,					     \
				     pwm_nrfx_init##inst, NULL,			     \
				     PM_DEVICE_DT_INST_GET(inst),		     \
				     &pwm_nrfx_##inst##_data,			     \
				     &pwm_nrfx_##inst##_config,			     \
				     POST_KERNEL, PWM_INIT_PRIORITY(inst),	     \
				     &pwm_nrfx_drv_api_funcs)

DT_INST_FOREACH_STATUS_OKAY(PWM_NRFX_DEFINE)
