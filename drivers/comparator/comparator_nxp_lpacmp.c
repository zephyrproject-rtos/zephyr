/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/clock_control.h>

LOG_MODULE_REGISTER(nxp_lpacmp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_lpacmp

/*
 * The Zephyr comparator API is single-channel oriented. The LPACMP always
 * uses external trigger channel 0 in the Continuous operating mode
 * (RM 54.6.1), so this driver runs the block in Continuous mode and only
 * manages channel 0.
 */
#define LPACMP_CHANNEL 0U

/* Channel 0 match flag inside the COMP_IF register (write-1-to-clear). */
#define LPACMP_CH0_MATCH_IF BIT(0)

struct nxp_lpacmp_config {
	LPACMP_Type *base;
	uint8_t positive_input;
	uint8_t negative_input;
	uint16_t sample_delay;
	uint32_t resample_wait_us;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
};

struct nxp_lpacmp_data {
	enum comparator_trigger trigger;
	comparator_callback_t callback;
	void *user_data;
	bool output_at_trigger_level;
	bool awaiting_release;
};

static void nxp_lpacmp_set_polarity(LPACMP_Type *base, bool higher)
{
	uint32_t sel = base->EXT_TRIG[LPACMP_CHANNEL].SEL;

	sel = (sel & ~LPACMP_SEL_HIGHER_MASK) | LPACMP_SEL_HIGHER(higher ? 1U : 0U);
	base->EXT_TRIG[LPACMP_CHANNEL].SEL = sel;
}

/*
 * The LPACMP has no live comparator-output level register: the only
 * observable comparison result is the channel match flag, which latches
 * whenever a sample lies on the compared (SEL[HIGHER]) side. Take a fresh
 * sample by clearing the flag and waiting for the next sample points.
 */
static bool nxp_lpacmp_resample(const struct nxp_lpacmp_config *config)
{
	config->base->COMP_IF = LPACMP_CH0_MATCH_IF;
	k_busy_wait(config->resample_wait_us);

	return (config->base->COMP_IF & LPACMP_CH0_MATCH_IF) != 0U;
}

static void nxp_lpacmp_update_interrupt(const struct device *dev)
{
	const struct nxp_lpacmp_config *config = dev->config;
	struct nxp_lpacmp_data *data = dev->data;
	uint32_t status = config->base->EXT_TRIG[LPACMP_CHANNEL].STATUS;

	if ((data->callback != NULL) && (data->trigger != COMPARATOR_TRIGGER_NONE)) {
		status |= LPACMP_STATUS_MATCH_IE_MASK;
	} else {
		status &= ~LPACMP_STATUS_MATCH_IE_MASK;
	}

	config->base->EXT_TRIG[LPACMP_CHANNEL].STATUS = status;
}

static int nxp_lpacmp_get_output(const struct device *dev)
{
	const struct nxp_lpacmp_config *config = dev->config;
	bool higher = (config->base->EXT_TRIG[LPACMP_CHANNEL].SEL & LPACMP_SEL_HIGHER_MASK) != 0U;

	/* A match means the output currently sits on the compared side. */
	return (nxp_lpacmp_resample(config) == higher) ? 1 : 0;
}

static int nxp_lpacmp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct nxp_lpacmp_config *config = dev->config;
	struct nxp_lpacmp_data *data = dev->data;
	LPACMP_Type *base = config->base;
	bool higher;

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		data->trigger = trigger;
		nxp_lpacmp_update_interrupt(dev);
		return 0;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		higher = true;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		higher = false;
		break;
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		/*
		 * A channel latches matches on a single polarity (SEL[HIGHER]);
		 * the LPACMP cannot report both edges on one channel.
		 */
		LOG_ERR("Both-edge trigger is not supported.");
		return -ENOTSUP;
	default:
		LOG_ERR("Invalid trigger type.");
		return -EINVAL;
	}

	/* Quiesce the interrupt while the trigger state is rebuilt. */
	base->EXT_TRIG[LPACMP_CHANNEL].STATUS &= ~LPACMP_STATUS_MATCH_IE_MASK;

	nxp_lpacmp_set_polarity(base, higher);

	data->trigger = trigger;
	data->awaiting_release = false;
	/* Baseline for the software edge detection in trigger_is_pending(). */
	data->output_at_trigger_level = nxp_lpacmp_resample(config);

	nxp_lpacmp_update_interrupt(dev);

	return 0;
}

static int nxp_lpacmp_set_trigger_callback(const struct device *dev, comparator_callback_t callback,
					   void *user_data)
{
	const struct nxp_lpacmp_config *config = dev->config;
	struct nxp_lpacmp_data *data = dev->data;
	LPACMP_Type *base = config->base;

	base->EXT_TRIG[LPACMP_CHANNEL].STATUS &= ~LPACMP_STATUS_MATCH_IE_MASK;

	data->callback = callback;
	data->user_data = user_data;

	/* Restore the armed polarity in case an interrupt left it inverted. */
	if (data->awaiting_release) {
		nxp_lpacmp_set_polarity(base, data->trigger == COMPARATOR_TRIGGER_RISING_EDGE);
		data->awaiting_release = false;
	}

	/* Clear any latched match when (re)arming the callback. */
	base->COMP_IF = LPACMP_CH0_MATCH_IF;

	nxp_lpacmp_update_interrupt(dev);

	return 0;
}

static int nxp_lpacmp_trigger_is_pending(const struct device *dev)
{
	const struct nxp_lpacmp_config *config = dev->config;
	struct nxp_lpacmp_data *data = dev->data;
	bool latched = (config->base->COMP_IF & LPACMP_CH0_MATCH_IF) != 0U;
	bool at_trigger_level;
	bool pending;

	if (data->trigger == COMPARATOR_TRIGGER_NONE) {
		config->base->COMP_IF = LPACMP_CH0_MATCH_IF;
		return 0;
	}

	/*
	 * The match flag latches on levels, not edges: it re-asserts at every
	 * sample taken while the output stays on the compared side, and a
	 * stale flag survives after the output has left it. Emulate edge
	 * semantics in software: report a trigger only when a match was seen
	 * while the previous visit still observed the released level.
	 */
	at_trigger_level = nxp_lpacmp_resample(config);
	pending = (latched || at_trigger_level) && !data->output_at_trigger_level;
	data->output_at_trigger_level = at_trigger_level;

	return pending ? 1 : 0;
}

static void nxp_lpacmp_irq_handler(const struct device *dev)
{
	const struct nxp_lpacmp_config *config = dev->config;
	struct nxp_lpacmp_data *data = dev->data;
	LPACMP_Type *base = config->base;

	/*
	 * The match flag is not an edge event: it would re-latch (and
	 * re-interrupt) at every sample taken while the output stays on the
	 * compared side. Emulate an edge interrupt by inverting the compare
	 * polarity on each match: the armed polarity reports the trigger
	 * edge, the inverted polarity silently waits for the output to
	 * release before re-arming.
	 */
	base->EXT_TRIG[LPACMP_CHANNEL].SEL ^= LPACMP_SEL_HIGHER_MASK;
	base->COMP_IF = LPACMP_CH0_MATCH_IF;

	data->awaiting_release = !data->awaiting_release;
	if (!data->awaiting_release) {
		return;
	}

	if (data->callback == NULL) {
		LOG_WRN("No callback can be executed.");
		return;
	}

	data->callback(dev, data->user_data);
}

static int nxp_lpacmp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct nxp_lpacmp_config *config = dev->config;

	if (action == PM_DEVICE_ACTION_RESUME) {
		config->base->CTRL |= LPACMP_CTRL_BLOCK_EN_MASK;
		return 0;
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		config->base->CTRL &= ~LPACMP_CTRL_BLOCK_EN_MASK;
		return 0;
	}

	return -ENOTSUP;
}

static int nxp_lpacmp_init(const struct device *dev)
{
	const struct nxp_lpacmp_config *config = dev->config;
	LPACMP_Type *base = config->base;
	uint32_t sel;
	uint32_t status;
	int ret;

	/*
	 * The CGU PER_CLK_EN[COMP_CLK_EN] gate resets to disabled: without it
	 * the block never samples and its match flag never latches.
	 */
	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("Clock control device not ready");
			return -ENODEV;
		}

		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret != 0) {
			LOG_ERR("Failed to enable peripheral clock (%d)", ret);
			return ret;
		}
	}

	/* Disable comparator before configuring. */
	base->CTRL &= ~LPACMP_CTRL_BLOCK_EN_MASK;

	/*
	 * Run in Continuous mode (CTRL[MODE] = 0): it is the only mode that
	 * matches the always-armed comparator API semantics.
	 */
	base->CTRL &= ~LPACMP_CTRL_MODE_MASK;

	/*
	 * Configure channel 0 inputs. Default the compare polarity to
	 * "higher" so get_output is meaningful before set_trigger() is
	 * called; the polarity is updated by set_trigger().
	 */
	sel = base->EXT_TRIG[LPACMP_CHANNEL].SEL;
	sel &= ~(LPACMP_SEL_INP_SEL_MASK | LPACMP_SEL_INN_SEL_MASK | LPACMP_SEL_HIGHER_MASK);
	sel |= LPACMP_SEL_INP_SEL(config->positive_input) |
	       LPACMP_SEL_INN_SEL(config->negative_input) | LPACMP_SEL_HIGHER(1U);
	base->EXT_TRIG[LPACMP_CHANNEL].SEL = sel;

	base->EXT_TRIG[LPACMP_CHANNEL].DELAY =
		(base->EXT_TRIG[LPACMP_CHANNEL].DELAY & ~LPACMP_DELAY_DEL_MASK) |
		LPACMP_DELAY_DEL(config->sample_delay);

	status = base->EXT_TRIG[LPACMP_CHANNEL].STATUS;
	status &= ~(LPACMP_STATUS_MATCH_IE_MASK | LPACMP_STATUS_TRGOP_EN_MASK |
		    LPACMP_STATUS_WAKEUPEN_MASK | LPACMP_STATUS_TRGOPWDH_MASK);
	status |= LPACMP_STATUS_CHNL_EN_MASK;
	base->EXT_TRIG[LPACMP_CHANNEL].STATUS = status;

	/* Clear any latched match flag before enabling interrupts. */
	base->COMP_IF = LPACMP_CH0_MATCH_IF;

	config->irq_config_func(dev);

	/* The comparator block is enabled by the resume action. */
	return pm_device_driver_init(dev, nxp_lpacmp_pm_callback);
}

static DEVICE_API(comparator, nxp_lpacmp_api) = {
	.get_output = nxp_lpacmp_get_output,
	.set_trigger = nxp_lpacmp_set_trigger,
	.set_trigger_callback = nxp_lpacmp_set_trigger_callback,
	.trigger_is_pending = nxp_lpacmp_trigger_is_pending,
};

#define LPACMP_RESAMPLE_WAIT_US(inst)                                                              \
	(uint32_t)(2ULL * DT_INST_PROP(inst, sample_delay) * USEC_PER_SEC /                        \
			   DT_INST_PROP_BY_PHANDLE_IDX(inst, clocks, 0, clock_frequency) +         \
		   2ULL)

/* The optional second clocks entry is the peripheral clock gate. */
#define LPACMP_HAS_GATE(inst) DT_INST_CLOCKS_HAS_IDX(inst, 1)

#define LPACMP_GATE_DEV(inst)                                                                      \
	COND_CODE_1(LPACMP_HAS_GATE(inst),                                                         \
		    (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 1))), (NULL))

#define LPACMP_GATE_SUBSYS(inst)                                                                   \
	COND_CODE_1(LPACMP_HAS_GATE(inst),                                                         \
		    ((clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(inst, 1, name)), (NULL))

#define NXP_LPACMP_DEVICE_INIT(inst)                                                               \
                                                                                                   \
	static struct nxp_lpacmp_data _CONCAT(data, inst) = {                                      \
		.trigger = COMPARATOR_TRIGGER_NONE,                                                \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, nxp_lpacmp_pm_callback);                                    \
                                                                                                   \
	static void _CONCAT(nxp_lpacmp_irq_config, inst)(const struct device *dev)                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    nxp_lpacmp_irq_handler, DEVICE_DT_INST_GET(inst), 0);                  \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static const struct nxp_lpacmp_config _CONCAT(config, inst) = {                            \
		.base = (LPACMP_Type *)DT_INST_REG_ADDR(inst),                                     \
		.positive_input = DT_INST_ENUM_IDX(inst, positive_input),                          \
		.negative_input = DT_INST_PROP(inst, negative_input),                              \
		.sample_delay = DT_INST_PROP(inst, sample_delay),                                  \
		.resample_wait_us = LPACMP_RESAMPLE_WAIT_US(inst),                                 \
		.clock_dev = LPACMP_GATE_DEV(inst),                                                \
		.clock_subsys = LPACMP_GATE_SUBSYS(inst),                                          \
		.irq_config_func = _CONCAT(nxp_lpacmp_irq_config, inst),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, nxp_lpacmp_init, PM_DEVICE_DT_INST_GET(inst),                  \
			      &_CONCAT(data, inst), &_CONCAT(config, inst), POST_KERNEL,           \
			      CONFIG_COMPARATOR_INIT_PRIORITY, &nxp_lpacmp_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_LPACMP_DEVICE_INIT)
