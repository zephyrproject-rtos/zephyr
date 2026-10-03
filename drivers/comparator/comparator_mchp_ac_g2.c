/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Analog comparator driver for the Microchip AC G2 block (ATDF
 * cmp_ac_u2501_v2), found on PIC32CM SG/GC.
 *
 * The block holds two comparators behind one register file, one clock pair
 * and one interrupt line. Each comparator is a devicetree child of the block
 * and gets its own struct device; the block-wide reset, clock and interrupt
 * setup happens once, driven by whichever child initializes first.
 *
 * Continuous mode only. Window mode and single-shot mode are not
 * implemented: neither has an expression in the Zephyr comparator API.
 */

#define DT_DRV_COMPAT microchip_ac_g2_comparator

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(comparator_mchp_ac_g2, CONFIG_COMPARATOR_LOG_LEVEL);

/* The block node, shared by every instance. There is exactly one AC. */
#define AC_NODE DT_INST_PARENT(0)

#define AC_MCHP_G2_SYNC_TIMEOUT_US 10000

/* COMPCTRL.INTSEL */
#define AC_MCHP_G2_INTSEL_TOGGLE  0
#define AC_MCHP_G2_INTSEL_RISING  1
#define AC_MCHP_G2_INTSEL_FALLING 2

struct comparator_mchp_ac_g2_config {
	ac_registers_t *regs;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
	uint8_t channel;
	uint32_t compctrl;
};

struct comparator_mchp_ac_g2_data {
	comparator_callback_t callback;
	void *user_data;
	bool pending;
};

/* Set by whichever comparator initializes first; read by the other. */
static bool ac_mchp_g2_block_started;

/*
 * DACCTRL holds both comparators' reference levels in one word and is
 * enable-protected: writing it with CTRLA.ENABLE set raises a bus fault
 * rather than being discarded. The protection is the block's, not the
 * comparator's. So the word is assembled from every instance's devicetree
 * value at build time and written once, while the block is still off.
 */
#define AC_MCHP_G2_DACCTRL_FIELD(n)                                                               \
	| (DT_INST_PROP(n, dac_value)                                                              \
	   << (DT_INST_REG_ADDR(n) == 0 ? AC_DACCTRL_VALUE0_Pos : AC_DACCTRL_VALUE1_Pos))

static const uint32_t ac_mchp_g2_dacctrl = 0 DT_INST_FOREACH_STATUS_OKAY(AC_MCHP_G2_DACCTRL_FIELD);

static bool ac_mchp_g2_wait_sync(ac_registers_t *regs, uint32_t mask, const char *what)
{
	if (WAIT_FOR((regs->AC_SYNCBUSY & mask) == 0, AC_MCHP_G2_SYNC_TIMEOUT_US,
		     k_busy_wait(1))) {
		return true;
	}

	LOG_ERR("timeout waiting for %s: SYNCBUSY reads 0x%08x, mask 0x%08x", what,
		regs->AC_SYNCBUSY, mask);
	return false;
}

/*
 * COMPCTRL is write-protected while its own ENABLE bit is set, so every
 * reconfiguration stops the comparator, rewrites the word and starts it
 * again. The block around it stays enabled, which is what lets the two
 * comparators be reconfigured independently.
 */
static int comparator_mchp_ac_g2_apply(const struct device *dev, uint32_t compctrl)
{
	const struct comparator_mchp_ac_g2_config *cfg = dev->config;
	ac_registers_t *regs = cfg->regs;
	uint32_t busy = BIT(AC_SYNCBUSY_COMPCTRL_Pos + cfg->channel);

	regs->CHANNEL[0].AC_COMPCTRL[cfg->channel] = compctrl & ~AC_COMPCTRL_ENABLE_Msk;
	if (!ac_mchp_g2_wait_sync(regs, busy, "COMPCTRL disable")) {
		return -ETIMEDOUT;
	}

	regs->CHANNEL[0].AC_COMPCTRL[cfg->channel] = compctrl | AC_COMPCTRL_ENABLE_Msk;
	if (!ac_mchp_g2_wait_sync(regs, busy, "COMPCTRL enable")) {
		return -ETIMEDOUT;
	}

	/*
	 * STATUSB.READYn goes high once the comparator has settled after
	 * being started. Reading the output before that yields whatever the
	 * analog side happened to be doing while it powered up.
	 */
	if (!WAIT_FOR((regs->AC_STATUSB & BIT(AC_STATUSB_READY_Pos + cfg->channel)) != 0,
		      AC_MCHP_G2_SYNC_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("comparator %u never became ready: STATUSB reads 0x%08x", cfg->channel,
			regs->AC_STATUSB);
		return -ETIMEDOUT;
	}

	return 0;
}

static int comparator_mchp_ac_g2_get_output(const struct device *dev)
{
	const struct comparator_mchp_ac_g2_config *cfg = dev->config;

	return (cfg->regs->AC_STATUSA & BIT(AC_STATUSA_STATE_Pos + cfg->channel)) != 0;
}

static int comparator_mchp_ac_g2_set_trigger(const struct device *dev,
					      enum comparator_trigger trigger)
{
	const struct comparator_mchp_ac_g2_config *cfg = dev->config;
	struct comparator_mchp_ac_g2_data *data = dev->data;
	ac_registers_t *regs = cfg->regs;
	uint32_t intsel;
	int ret;

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		intsel = AC_MCHP_G2_INTSEL_TOGGLE;
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		intsel = AC_MCHP_G2_INTSEL_RISING;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		intsel = AC_MCHP_G2_INTSEL_FALLING;
		break;
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		intsel = AC_MCHP_G2_INTSEL_TOGGLE;
		break;
	default:
		return -EINVAL;
	}

	regs->AC_INTENCLR = BIT(cfg->channel);

	ret = comparator_mchp_ac_g2_apply(dev, (cfg->compctrl & ~AC_COMPCTRL_INTSEL_Msk) |
						(intsel << AC_COMPCTRL_INTSEL_Pos));
	if (ret < 0) {
		return ret;
	}

	data->pending = false;
	regs->AC_INTFLAG = BIT(cfg->channel);

	if (trigger != COMPARATOR_TRIGGER_NONE) {
		regs->AC_INTENSET = BIT(cfg->channel);
	}

	return 0;
}

static int comparator_mchp_ac_g2_set_trigger_callback(const struct device *dev,
						       comparator_callback_t callback,
						       void *user_data)
{
	struct comparator_mchp_ac_g2_data *data = dev->data;
	unsigned int key = irq_lock();

	data->callback = callback;
	data->user_data = user_data;

	/*
	 * A trigger that arrived while nobody was listening is delivered to
	 * the callback the moment one is installed, which is what the API
	 * asks for.
	 */
	if (callback != NULL && data->pending) {
		data->pending = false;
		irq_unlock(key);
		callback(dev, user_data);
		return 0;
	}

	irq_unlock(key);
	return 0;
}

static int comparator_mchp_ac_g2_trigger_is_pending(const struct device *dev)
{
	struct comparator_mchp_ac_g2_data *data = dev->data;
	unsigned int key = irq_lock();
	bool pending = data->pending;

	data->pending = false;
	irq_unlock(key);

	return pending;
}

static DEVICE_API(comparator, comparator_mchp_ac_g2_api) = {
	.get_output = comparator_mchp_ac_g2_get_output,
	.set_trigger = comparator_mchp_ac_g2_set_trigger,
	.set_trigger_callback = comparator_mchp_ac_g2_set_trigger_callback,
	.trigger_is_pending = comparator_mchp_ac_g2_trigger_is_pending,
};

/*
 * One vector serves both comparators and the window monitor, so the handler
 * takes the devices it might have to notify as its argument and decides from
 * INTFLAG which of them the interrupt was for.
 */
static void comparator_mchp_ac_g2_isr(const void *arg)
{
	const struct device *const *devices = arg;
	ac_registers_t *regs = NULL;
	uint32_t handled;

	for (int i = 0; i < 2; i++) {
		if (devices[i] != NULL) {
			const struct comparator_mchp_ac_g2_config *cfg = devices[i]->config;

			regs = cfg->regs;
			break;
		}
	}

	if (regs == NULL) {
		return;
	}

	/*
	 * INTFLAG latches on every output toggle regardless of INTENSET, so a
	 * masked comparator (trigger NONE) keeps setting its bit while its
	 * neighbour's interrupt is what actually woke this vector. Masking
	 * INTFLAG with INTENSET before dispatching keeps that comparator from
	 * ever seeing a callback or a latched pending of its own accord, and
	 * write-clearing only the masked-in bits leaves the rest alone for
	 * set_trigger() to clear when that comparator is re-armed.
	 */
	handled = regs->AC_INTFLAG & regs->AC_INTENSET;
	if (handled == 0) {
		return;
	}

	regs->AC_INTFLAG = handled;

	for (int i = 0; i < 2; i++) {
		const struct device *dev = devices[i];
		const struct comparator_mchp_ac_g2_config *cfg;
		struct comparator_mchp_ac_g2_data *data;

		if (dev == NULL) {
			continue;
		}

		cfg = dev->config;
		if ((handled & BIT(cfg->channel)) == 0) {
			continue;
		}

		data = dev->data;
		if (data->callback != NULL) {
			data->callback(dev, data->user_data);
		} else {
			data->pending = true;
		}
	}
}

static const struct device *comparator_mchp_ac_g2_devices[2];

static int comparator_mchp_ac_g2_start_block(const struct comparator_mchp_ac_g2_config *cfg)
{
	ac_registers_t *regs = cfg->regs;
	int ret;

	if (ac_mchp_g2_block_started) {
		return 0;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock controller is not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->mclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("failed to gate the APB clock on: %d", ret);
		return ret;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->gclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("failed to gate the generic clock on: %d", ret);
		return ret;
	}

	regs->AC_CTRLA = AC_CTRLA_SWRST_Msk;
	if (!ac_mchp_g2_wait_sync(regs, AC_SYNCBUSY_SWRST_Msk, "software reset")) {
		return -ETIMEDOUT;
	}

	regs->CHANNEL[0].AC_DACCTRL = ac_mchp_g2_dacctrl;

	regs->AC_CTRLA = AC_CTRLA_ENABLE_Msk;
	if (!ac_mchp_g2_wait_sync(regs, AC_SYNCBUSY_ENABLE_Msk, "block enable")) {
		return -ETIMEDOUT;
	}

	IRQ_CONNECT(DT_IRQN(AC_NODE), DT_IRQ(AC_NODE, priority), comparator_mchp_ac_g2_isr,
		    comparator_mchp_ac_g2_devices, 0);
	irq_enable(DT_IRQN(AC_NODE));

	ac_mchp_g2_block_started = true;
	return 0;
}

static int comparator_mchp_ac_g2_init(const struct device *dev)
{
	const struct comparator_mchp_ac_g2_config *cfg = dev->config;
	int ret;

	ret = comparator_mchp_ac_g2_start_block(cfg);
	if (ret < 0) {
		return ret;
	}

	/*
	 * pinctrl-0 is not required by the binding: a comparator whose muxes
	 * name no pin, such as one comparing two on-die sources, needs no pin
	 * configuration. A missing default state is not an error; every
	 * other failure from pinctrl is.
	 */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		LOG_ERR("failed to apply the pin configuration: %d", ret);
		return ret;
	}

	comparator_mchp_ac_g2_devices[cfg->channel] = dev;

	/* Interrupts stay masked until set_trigger asks for them. */
	cfg->regs->AC_INTENCLR = BIT(cfg->channel);

	return comparator_mchp_ac_g2_apply(dev, cfg->compctrl);
}

#define AC_MCHP_G2_MUXPOS(n) DT_INST_ENUM_IDX(n, positive_mux_input)

#define AC_MCHP_G2_MUXNEG(n) DT_INST_ENUM_IDX(n, negative_mux_input)

#define AC_MCHP_G2_COMPCTRL(n)                                                                    \
	((AC_MCHP_G2_MUXPOS(n) << AC_COMPCTRL_MUXPOS_Pos) |                                        \
	 (AC_MCHP_G2_MUXNEG(n) << AC_COMPCTRL_MUXNEG_Pos) |                                        \
	 (DT_INST_ENUM_IDX(n, hysteresis_level) << AC_COMPCTRL_HYST_Pos) |                         \
	 (DT_INST_ENUM_IDX(n, filter_length) << AC_COMPCTRL_FLEN_Pos) |                            \
	 (DT_INST_PROP(n, swap_inputs) ? AC_COMPCTRL_SWAP_Msk : 0) |                               \
	 (DT_INST_PROP(n, low_speed) ? AC_COMPCTRL_SPEED_Msk : 0) |                                \
	 (DT_INST_PROP(n, run_in_standby) ? AC_COMPCTRL_RUNSTDBY_Msk : 0))

#define COMPARATOR_MCHP_AC_G2_DEFINE(n)                                                            \
	BUILD_ASSERT(DT_INST_REG_ADDR(n) < 2, "comparator index must be 0 or 1");                  \
	BUILD_ASSERT(DT_INST_PROP(n, dac_value) < 128, "dac-value must fit in seven bits");        \
	BUILD_ASSERT(DT_SAME_NODE(DT_INST_PARENT(n), AC_NODE),                                     \
		     "every AC G2 comparator must be a child of the same block");                  \
	BUILD_ASSERT(DT_NODE_HAS_STATUS(AC_NODE, okay), "the AC G2 block must be enabled");        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct comparator_mchp_ac_g2_data comparator_mchp_ac_g2_data_##n;                   \
	static const struct comparator_mchp_ac_g2_config comparator_mchp_ac_g2_config_##n = {      \
		.regs = (ac_registers_t *)DT_REG_ADDR(AC_NODE),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                   \
		.mclk_sys =                                                                        \
			(clock_control_subsys_t)DT_CLOCKS_CELL_BY_NAME(AC_NODE, mclk, subsystem),  \
		.gclk_sys =                                                                        \
			(clock_control_subsys_t)DT_CLOCKS_CELL_BY_NAME(AC_NODE, gclk, subsystem),  \
		.channel = DT_INST_REG_ADDR(n),                                                    \
		.compctrl = AC_MCHP_G2_COMPCTRL(n),                                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, comparator_mchp_ac_g2_init, NULL,                                 \
			      &comparator_mchp_ac_g2_data_##n, &comparator_mchp_ac_g2_config_##n,  \
			      POST_KERNEL, CONFIG_COMPARATOR_INIT_PRIORITY,                        \
			      &comparator_mchp_ac_g2_api);

DT_INST_FOREACH_STATUS_OKAY(COMPARATOR_MCHP_AC_G2_DEFINE)
