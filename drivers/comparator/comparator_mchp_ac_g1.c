/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>

#define DT_DRV_COMPAT microchip_ac_g1_comparator

LOG_MODULE_REGISTER(comparator_mchp_ac_g1, CONFIG_COMPARATOR_LOG_LEVEL);

/* clang-format off */
#define AC_REG                   (((const struct comparator_mchp_dev_config *)(dev)->config)->regs)
#define TIMEOUT_VALUE_US           1000
#define DELAY_US                   2
#define COMPARATOR_INT_STATUS_NONE (-1)
/* clang-format on */

enum mchp_comp_pos_input {
	MCHP_COMP_POS_INPUT_PIN0 = 0,
	MCHP_COMP_POS_INPUT_PIN1,
	MCHP_COMP_POS_INPUT_PIN2,
	MCHP_COMP_POS_INPUT_PIN3,
	MCHP_COMP_POS_INPUT_VSCALE,
};

enum mchp_comp_neg_input {
	MCHP_COMP_NEG_INPUT_PIN0 = 0,
	MCHP_COMP_NEG_INPUT_PIN1,
	MCHP_COMP_NEG_INPUT_PIN2,
	MCHP_COMP_NEG_INPUT_PIN3,
	MCHP_COMP_NEG_INPUT_GND,
	MCHP_COMP_NEG_INPUT_VSCALE,
	MCHP_COMP_NEG_INPUT_BANDGAP,
	MCHP_COMP_NEG_INPUT_DAC,
};

enum mchp_comp_output_mode {
	MCHP_COMP_OUTPUT_OFF = 0,
	MCHP_COMP_OUTPUT_ASYNC,
	MCHP_COMP_OUTPUT_SYNC,
};

enum mchp_comp_filter {
	MCHP_COMP_FILTER_OFF = 0,
	MCHP_COMP_FILTER_MAJ3,
	MCHP_COMP_FILTER_MAJ5,
};

enum mchp_comp_hysteresis {
	MCHP_COMP_HYST_50MV = 0,
	MCHP_COMP_HYST_100MV,
	MCHP_COMP_HYST_150MV,
};

struct comparator_mchp_channel_cfg {
	uint8_t channel_id;

	enum mchp_comp_pos_input pos_input;
	enum mchp_comp_neg_input neg_input;
	enum mchp_comp_output_mode output_mode;
	enum mchp_comp_filter filter_length;
	enum mchp_comp_hysteresis hysteresis_level;

	uint8_t vddana_scale_value;
	bool single_shot_mode;
	bool hysteresis_enable;
	bool run_standby;
	bool event_input_enable;
	bool event_output_enable;
	bool swap_inputs;
};

struct comparator_mchp_dev_data {
	uint32_t interrupt_mask;   /* Configured interrupt trigger type */
	uint32_t interrupt_status; /* Last interrupt status or edge detected */
	comparator_callback_t callback;
	void *user_data;
};

struct comparator_mchp_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

struct comparator_mchp_dev_config {
	ac_registers_t *regs;
	const struct pinctrl_dev_config *pcfg;
	struct comparator_mchp_clock comparator_clock;
	void (*config_func)(const struct device *dev);
	struct comparator_mchp_channel_cfg channel_config;
};

#if CONFIG_COMPARATOR_LOG_LEVEL_DBG
/* Print channel configuration */
static void comparator_print_channel_cfg(const struct comparator_mchp_channel_cfg *cfg)
{
	static const char *const pos_input_names[] = {"PIN0", "PIN1", "PIN2", "PIN3", "VSCALE"};
	static const char *const neg_input_names[] = {"PIN0", "PIN1",   "PIN2",    "PIN3",
						      "GND",  "VSCALE", "BANDGAP", "DAC"};
	static const char *const output_mode_names[] = {"OFF", "ASYNC", "SYNC"};
	static const char *const filter_names[] = {"OFF", "MAJ3", "MAJ5"};
	static const char *const hysteresis_names[] = {"HYST50", "HYST100", "HYST150"};

	LOG_DBG("=== Comparator Channel Configuration ===");
	LOG_DBG("Channel ID           : %u", cfg->channel_id);
	LOG_DBG("Positive Input       : %s", pos_input_names[cfg->pos_input]);
	LOG_DBG("Negative Input       : %s", neg_input_names[cfg->neg_input]);
	LOG_DBG("Output Mode          : %s", output_mode_names[cfg->output_mode]);
	LOG_DBG("Filter Length        : %s", filter_names[cfg->filter_length]);
	LOG_DBG("Hysteresis Enabled   : %s", cfg->hysteresis_enable ? "Yes" : "No");
	LOG_DBG("Hysteresis Level     : %s", hysteresis_names[cfg->hysteresis_level]);
	LOG_DBG("Single-shot Mode     : %s", cfg->single_shot_mode ? "Yes" : "No");
	LOG_DBG("Run in Standby       : %s", cfg->run_standby ? "Yes" : "No");
	LOG_DBG("Swap Inputs          : %s", cfg->swap_inputs ? "Yes" : "No");
	LOG_DBG("Event Input Enabled  : %s", cfg->event_input_enable ? "Yes" : "No");
	LOG_DBG("Event Output Enabled : %s", cfg->event_output_enable ? "Yes" : "No");
	LOG_DBG("========================================");
}

/* Print all the comparator register values */
static void comparator_print_reg(const struct device *dev)
{
	LOG_DBG("=============== Comparator Registers ===============");
	LOG_DBG("%-20s: 0x%02x", "AC_CTRLA", AC_REG->AC_CTRLA);
	LOG_DBG("%-20s: 0x%02x", "AC_CTRLB", AC_REG->AC_CTRLB);
	LOG_DBG("%-20s: 0x%04x", "AC_EVCTRL", AC_REG->AC_EVCTRL);
	LOG_DBG("%-20s: 0x%02x", "AC_INTENCLR", AC_REG->AC_INTENCLR);
	LOG_DBG("%-20s: 0x%02x", "AC_INTENSET", AC_REG->AC_INTENSET);
	LOG_DBG("%-20s: 0x%02x", "AC_INTFLAG", AC_REG->AC_INTFLAG);
	LOG_DBG("%-20s: 0x%02x", "AC_STATUSA", AC_REG->AC_STATUSA);
	LOG_DBG("%-20s: 0x%02x", "AC_STATUSB", AC_REG->AC_STATUSB);
	LOG_DBG("%-20s: 0x%02x", "AC_DBGCTRL", AC_REG->AC_DBGCTRL);
	LOG_DBG("%-20s: 0x%02x", "AC_WINCTRL", AC_REG->AC_WINCTRL);
	LOG_DBG("%-20s: 0x%02x", "AC_SCALER[0]", AC_REG->AC_SCALER[0]);
	LOG_DBG("%-20s: 0x%02x", "AC_SCALER[1]", AC_REG->AC_SCALER[1]);
	LOG_DBG("%-20s: 0x%08x", "AC_COMPCTRL[0]", (uint32_t)AC_REG->AC_COMPCTRL[0]);
	LOG_DBG("%-20s: 0x%08x", "AC_COMPCTRL[1]", (uint32_t)AC_REG->AC_COMPCTRL[1]);
	LOG_DBG("%-20s: 0x%08x", "AC_SYNCBUSY", (uint32_t)AC_REG->AC_SYNCBUSY);
	LOG_DBG("%-20s: 0x%04x", "AC_CALIB", AC_REG->AC_CALIB);
	LOG_DBG("===================================================");
}
#endif /* CONFIG_COMPARATOR_LOG_LEVEL_DBG */

/* Wait for synchronization */
static void ac_wait_sync(ac_registers_t *ac_reg, uint32_t mask)
{
	if (WAIT_FOR(((ac_reg->AC_SYNCBUSY & mask) != mask), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for AC_SYNCBUSY bits to clear (mask=0x%X)", mask);
	}
}

/* AC utility functions for enabling, configuring, and syncing the analog comparator */
static void ac_enable(ac_registers_t *ac_reg, bool enable)
{
	if (enable == true) {
		ac_reg->AC_CTRLA |= AC_CTRLA_ENABLE_Msk;
	} else {
		ac_reg->AC_CTRLA &= ~AC_CTRLA_ENABLE_Msk;
	}

	ac_wait_sync(ac_reg, AC_SYNCBUSY_ENABLE_Msk);
}

/* Enable a specific comparator channel and wait for sync */
static void ac_channel_enable(ac_registers_t *ac_reg, uint8_t channel_id)
{
	ac_reg->AC_COMPCTRL[channel_id] |= AC_COMPCTRL_ENABLE_Msk;

	/* Only 2 possible values for channel_id: 0 or 1 */
	if (channel_id == 0) {
		ac_wait_sync(ac_reg, AC_SYNCBUSY_COMPCTRL0_Msk);
	} else {
		ac_wait_sync(ac_reg, AC_SYNCBUSY_COMPCTRL1_Msk);
	}
}

/* Enable interrupt for the given comparator channel */
static inline void ac_enable_interrupt(ac_registers_t *ac_reg, uint8_t channel_id)
{
	if (channel_id == 0) {
		ac_reg->AC_INTENSET = AC_INTENSET_COMP0_Msk;
	} else {
		ac_reg->AC_INTENSET = AC_INTENSET_COMP1_Msk;
	}
}

/* Disable interrupt for the given comparator channel */
static inline void ac_disable_interrupt(ac_registers_t *ac_reg, uint8_t channel_id)
{
	if (channel_id == 0) {
		ac_reg->AC_INTENCLR = AC_INTENCLR_COMP0_Msk;
	} else {
		ac_reg->AC_INTENCLR = AC_INTENCLR_COMP1_Msk;
	}
}

/* Trigger a single-shot comparison for the specified channel */
static inline void ac_start_conversion(ac_registers_t *ac_reg, uint8_t channel_id)
{
	ac_reg->AC_CTRLB |= (AC_CTRLB_START0_Msk << channel_id);
}

/* Wait until the comparator result for the specified channel is ready */
static int ac_wait_for_conversion(ac_registers_t *ac_reg, uint8_t channel_id)
{
	uint32_t ready_mask = AC_STATUSB_READY0_Msk << channel_id;

	if (WAIT_FOR(((ac_reg->AC_STATUSB & ready_mask) == ready_mask), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for AC_STATUSB ready (channel=%u)", channel_id);
		return -ETIMEDOUT;
	}

	return 0;
}

/* Get the current comparator output state for the specified channel
 * Returns true if output is HIGH, false if LOW
 */
static inline bool ac_get_result(ac_registers_t *ac_reg, uint8_t channel_id)
{
	uint32_t state_mask = AC_STATUSA_STATE0_Msk << channel_id;

	return (ac_reg->AC_STATUSA & state_mask) != 0;
}

/* Configure comparator channel: inputs, mode, hysteresis, output, interrupt, scaler, etc. */
static int ac_configure_channel(const struct device *dev)
{
	/* Get device config */
	const struct comparator_mchp_dev_config *const dev_cfg = dev->config;
	const struct comparator_mchp_channel_cfg *channel_config = &dev_cfg->channel_config;
	uint8_t channel_id = channel_config->channel_id;

	/* Reset COMPCTRL before new config */
	AC_REG->AC_COMPCTRL[channel_id] = 0;

	/* Set single-shot or continuous mode */
	AC_REG->AC_COMPCTRL[channel_id] = AC_COMPCTRL_SINGLE(channel_config->single_shot_mode);

	/* Set MUX inputs */
	AC_REG->AC_COMPCTRL[channel_id] |= AC_COMPCTRL_MUXPOS(channel_config->pos_input);
	AC_REG->AC_COMPCTRL[channel_id] |= AC_COMPCTRL_MUXNEG(channel_config->neg_input);

	/* Configure VDDANA scaler if used */
	if ((channel_config->neg_input == MCHP_COMP_NEG_INPUT_VSCALE) ||
	    (channel_config->pos_input == MCHP_COMP_POS_INPUT_VSCALE)) {
		AC_REG->AC_SCALER[channel_id] = AC_SCALER_VALUE(channel_config->vddana_scale_value);
	}

	/* Set Output mode */
	AC_REG->AC_COMPCTRL[channel_id] |= AC_COMPCTRL_OUT(channel_config->output_mode);

	/* Set Filter length */
	AC_REG->AC_COMPCTRL[channel_id] |= AC_COMPCTRL_FLEN(channel_config->filter_length);

	if (channel_config->single_shot_mode == false) {
		/* Enable hysterisis */
		AC_REG->AC_COMPCTRL[channel_id] |=
			AC_COMPCTRL_HYSTEN(channel_config->hysteresis_enable ? 1 : 0);

		/* Set hysterisis */
		if (channel_config->hysteresis_enable == true) {
			AC_REG->AC_COMPCTRL[channel_id] |=
				AC_COMPCTRL_HYST(channel_config->hysteresis_level);
		}
	}

	/* Set Comparator speed */
	AC_REG->AC_COMPCTRL[channel_id] |= AC_COMPCTRL_SPEED(AC_COMPCTRL_SPEED_HIGH_Val);

	/* Run in standby if enabled */
	if (channel_config->run_standby == true) {
		AC_REG->AC_COMPCTRL[channel_id] |= AC_COMPCTRL_RUNSTDBY(1);
	}

	LOG_DBG("Configuration done AC_REG->AC_COMPCTRL[0] : 0x%x",
		AC_REG->AC_COMPCTRL[channel_id]);

	/* Interrupt Enable */
	ac_enable_interrupt(AC_REG, channel_id);

	return 0;
}

static void comparator_mchp_isr(const struct device *dev)
{
	struct comparator_mchp_dev_data *const dev_data = dev->data;

	/* Store comparator status (latched value from AC_STATUSA) */
	dev_data->interrupt_status = AC_REG->AC_STATUSA;

	/* Clear interrupt flags (write 1 to clear) */
	AC_REG->AC_INTFLAG = AC_INTFLAG_Msk;

	if (dev_data->callback != NULL) {
		dev_data->callback(dev, dev_data->user_data);
	}
}

static int comparator_mchp_get_output(const struct device *dev)
{
	const struct comparator_mchp_dev_config *const dev_cfg = dev->config;
	const struct comparator_mchp_channel_cfg *channel_config = &dev_cfg->channel_config;
	uint8_t channel_id = channel_config->channel_id;
	int ret = 0;
	bool result = false;

#if CONFIG_COMPARATOR_LOG_LEVEL_DBG
	/* Optional debug */
	comparator_print_channel_cfg(channel_config);
#endif /* CONFIG_COMPARATOR_LOG_LEVEL_DBG */

	/* Trigger conversion in single-shot mode */
	if (channel_config->single_shot_mode == true) {
		ac_start_conversion(AC_REG, channel_id);
	}

	ret = ac_wait_for_conversion(AC_REG, channel_id);
	if (ret == 0) {

#if CONFIG_COMPARATOR_LOG_LEVEL_DBG
		/* Optional debug */
		comparator_print_reg(dev);
#endif /* CONFIG_COMPARATOR_LOG_LEVEL_DBG */
		result = ac_get_result(AC_REG, channel_id);
		ret = result ? 1 : 0;
		LOG_DBG("AC comparator result: %s", result ? "HIGH" : "LOW");
	}

	return ret;
}

static int comparator_mchp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct comparator_mchp_dev_config *const dev_cfg = dev->config;
	struct comparator_mchp_dev_data *const dev_data = dev->data;
	const struct comparator_mchp_channel_cfg *channel_config = &dev_cfg->channel_config;
	uint8_t channel_id = channel_config->channel_id;
	uint8_t intsel_val;

	LOG_DBG("Setting comparator trigger mode: %d", trigger);

	/* Map trigger enum to hardware INTSEL value */
	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		intsel_val = AC_COMPCTRL_INTSEL_EOC_Val;
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		intsel_val = AC_COMPCTRL_INTSEL_RISING_Val;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		intsel_val = AC_COMPCTRL_INTSEL_FALLING_Val;
		break;
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		intsel_val = AC_COMPCTRL_INTSEL_TOGGLE_Val;
		break;
	default:
		LOG_ERR("Invalid comparator trigger: %d, defaulting to NONE", trigger);
		intsel_val = AC_COMPCTRL_INTSEL_EOC_Val;
		trigger = COMPARATOR_TRIGGER_NONE;
		break;
	}

	/* Update INTSEL field */
	AC_REG->AC_COMPCTRL[channel_id] &= ~AC_COMPCTRL_INTSEL_Msk;
	AC_REG->AC_COMPCTRL[channel_id] |= AC_COMPCTRL_INTSEL(intsel_val);

	if (trigger != COMPARATOR_TRIGGER_NONE) {
		ac_enable_interrupt(AC_REG, channel_id);
	} else {
		ac_disable_interrupt(AC_REG, channel_id);
	}
	LOG_DBG("Trigger mode: %d, INTSEL set to: %d", trigger, intsel_val);

	/* Store trigger mode (not INTSEL value!) */
	dev_data->interrupt_mask = trigger;
	dev_data->interrupt_status = COMPARATOR_INT_STATUS_NONE;

	return 0;
}

static int comparator_mchp_set_trigger_callback(const struct device *dev,
						comparator_callback_t callback, void *user_data)
{
	struct comparator_mchp_dev_data *const dev_data = dev->data;
	const struct comparator_mchp_dev_config *const dev_cfg = dev->config;
	const struct comparator_mchp_channel_cfg *channel_config = &dev_cfg->channel_config;
	uint8_t channel_id = channel_config->channel_id;

	ac_disable_interrupt(AC_REG, channel_id);

	dev_data->callback = callback;
	dev_data->user_data = user_data;

	ac_enable_interrupt(AC_REG, channel_id);

	return 0;
}

static int comparator_mchp_trigger_is_pending(const struct device *dev)
{
	const struct comparator_mchp_dev_config *const dev_cfg = dev->config;
	struct comparator_mchp_dev_data *const dev_data = dev->data;
	const struct comparator_mchp_channel_cfg *channel_config = &dev_cfg->channel_config;
	uint8_t channel_id = channel_config->channel_id;
	int ret = 0;
	uint32_t state_mask = AC_STATUSA_STATE0_Msk << channel_id;

	LOG_DBG("Checking if comparator trigger is pending...");

	/* Only process in polling mode (no callback handler registered) */
	if (dev_data->callback == NULL &&
	    dev_data->interrupt_status != COMPARATOR_INT_STATUS_NONE) {
		switch (dev_data->interrupt_mask) {
		case COMPARATOR_TRIGGER_RISING_EDGE:
			if ((dev_data->interrupt_status & state_mask) != 0) {
				LOG_DBG("Rising edge trigger is pending");
				ret = 1;
			}
			break;
		case COMPARATOR_TRIGGER_FALLING_EDGE:
			if ((dev_data->interrupt_status & state_mask) == 0) {
				LOG_DBG("Falling edge trigger is pending");
				ret = 1;
			}
			break;
		case COMPARATOR_TRIGGER_BOTH_EDGES:
			LOG_DBG("Both edge trigger is pending");
			ret = 1;
			break;
		default:
			break;
		}
		/* Mark as consumed */
		dev_data->interrupt_status = COMPARATOR_INT_STATUS_NONE;
	}

	return ret;
}

static int comparator_mchp_init(const struct device *dev)
{
	const struct comparator_mchp_dev_config *const dev_cfg = dev->config;
	struct comparator_mchp_dev_data *const dev_data = dev->data;
	int ret;
	uint8_t channel_id = dev_cfg->channel_config.channel_id;
	uint32_t sw0_reg;

	dev_data->interrupt_status = COMPARATOR_INT_STATUS_NONE;

	/* Turn on GCLK for AC */
	ret = clock_control_on(dev_cfg->comparator_clock.clock_dev,
			       dev_cfg->comparator_clock.gclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable GCLK for COMP: %d", ret);
		return ret;
	}

	/* Turn on MCLK for AC */
	ret = clock_control_on(dev_cfg->comparator_clock.clock_dev,
			       dev_cfg->comparator_clock.mclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable MCLK for COMP: %d", ret);
		return ret;
	}

	/* Apply pinctrl default state */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d", ret);
		return ret;
	}

	/* Reset the comparator peripheral */
	AC_REG->AC_CTRLA = AC_CTRLA_SWRST_Msk;
	ac_wait_sync(AC_REG, AC_SYNCBUSY_SWRST_Msk);

	/* Configure calibration */
	sw0_reg = ((fuses_sw0_fuses_registers_t *)SW0_ADDR)->FUSES_SW0_WORD_0;
	dev_cfg->regs->AC_CALIB =
		(sw0_reg & FUSES_SW0_WORD_0_AC_BIAS0_Msk) >> FUSES_SW0_WORD_0_AC_BIAS0_Pos;

	/* Configure ISR */
	dev_cfg->config_func(dev);
	ac_configure_channel(dev);
	ac_channel_enable(AC_REG, channel_id);

#if CONFIG_COMPARATOR_LOG_LEVEL_DBG
	comparator_print_reg(dev);
#endif /* CONFIG_COMPARATOR_LOG_LEVEL_DBG */

	ac_enable(AC_REG, true);
	ac_wait_sync(AC_REG, AC_SYNCBUSY_ENABLE_Msk);

	/* If everything is OK but the clocks are already enabled, return 0. */
	return (ret == -EALREADY) ? 0 : ret;
}

static DEVICE_API(comparator, comparator_mchp_api) = {
	.get_output = comparator_mchp_get_output,
	.set_trigger = comparator_mchp_set_trigger,
	.set_trigger_callback = comparator_mchp_set_trigger_callback,
	.trigger_is_pending = comparator_mchp_trigger_is_pending,
};

#define COMPARATOR_MCHP_DATA_DEFN(n)                                                               \
	static struct comparator_mchp_dev_data comparator_mchp_data_##n;

/* Connect a single IRQ for instance n at index idx */
#define COMPARATOR_MCHP_IRQ_CONNECT(idx, n)                                                        \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, idx),                                                    \
		(                                                                                  \
			IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq),                               \
						DT_INST_IRQ_BY_IDX(n, idx, priority),              \
						comparator_mchp_isr,                               \
						DEVICE_DT_INST_GET(n), 0);                         \
			irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));                               \
		)                                                                                  \
	)

/* Generate config function for instance n */
#define COMPARATOR_MCHP_DEFINE_CONFIG_FUNC(n)                                                      \
	static void comparator_mchp_config_##n(const struct device *dev)                           \
	{                                                                                          \
		/* Connect all IRQs declared in devicetree */                                      \
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)),                                               \
				COMPARATOR_MCHP_IRQ_CONNECT,                                       \
				(), n);             \
	}

#define COMPARATOR_MCHP_CONFIG_DEFN(n)                                                             \
	static void comparator_mchp_config_##n(const struct device *dev);                          \
	static const struct comparator_mchp_dev_config comparator_mchp_cfg_##n = {                 \
		.regs = (ac_registers_t *)DT_INST_REG_ADDR(n),                                     \
		.config_func = comparator_mchp_config_##n,                                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.comparator_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                  \
		.comparator_clock.mclk_sys =                                                       \
			(void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),                 \
		.comparator_clock.gclk_sys =                                                       \
			(void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),                 \
		.channel_config = {                                                                \
			.channel_id = DT_PROP_OR(DT_DRV_INST(n), comparator_channel, 0),           \
			.pos_input = DT_ENUM_IDX_OR(DT_DRV_INST(n), positive_mux_input, 0),        \
			.neg_input = DT_ENUM_IDX_OR(DT_DRV_INST(n), negative_mux_input, 0),        \
			.output_mode = DT_ENUM_IDX_OR(DT_DRV_INST(n), output_mode, 0),             \
			.filter_length = DT_ENUM_IDX_OR(DT_DRV_INST(n), filter_length, 0),         \
			.hysteresis_level = DT_ENUM_IDX_OR(DT_DRV_INST(n), hysteresis_level, 0),   \
			.vddana_scale_value = DT_PROP_OR(DT_DRV_INST(n), vddana_scale_value, 0),   \
			.single_shot_mode = DT_PROP_OR(DT_DRV_INST(n), single_shot_mode, 0),       \
			.hysteresis_enable = DT_PROP_OR(DT_DRV_INST(n), hysteresis_enable, 0),     \
			.run_standby = DT_PROP_OR(DT_DRV_INST(n), run_standby, 0),                 \
			.event_input_enable = DT_PROP_OR(DT_DRV_INST(n), event_input_enable, 0),   \
			.event_output_enable = DT_PROP_OR(DT_DRV_INST(n), event_output_enable, 0), \
			.swap_inputs = DT_PROP_OR(DT_DRV_INST(n), swap_inputs, 0),                 \
		}}

#define COMPARATOR_MCHP_DEVICE_INIT(n)                                                             \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	COMPARATOR_MCHP_CONFIG_DEFN(n);                                                            \
	COMPARATOR_MCHP_DATA_DEFN(n);                                                              \
	DEVICE_DT_INST_DEFINE(n, comparator_mchp_init, NULL, &comparator_mchp_data_##n,            \
			      &comparator_mchp_cfg_##n, POST_KERNEL,                               \
			      CONFIG_COMPARATOR_INIT_PRIORITY, &comparator_mchp_api);              \
	COMPARATOR_MCHP_DEFINE_CONFIG_FUNC(n);

DT_INST_FOREACH_STATUS_OKAY(COMPARATOR_MCHP_DEVICE_INIT);
