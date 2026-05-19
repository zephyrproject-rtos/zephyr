/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_timer_channel

#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(counter_bflb_timer, CONFIG_COUNTER_LOG_LEVEL);

#include <common_defines.h>
#include <bouffalolab/common/timer_reg.h>

/* Timer clock source values used in TIMER_TCCR register. */
#define BFLB_TIMER_CLKSRC_32K     1U
#define BFLB_TIMER_CLKSRC_1K      2U
#define BFLB_TIMER_CLKSRC_XTAL    3U
#define BFLB_TIMER_PRELOAD_NONE   0U
#define BFLB_TIMER_ALARM_CHANNELS 2U

/* Per-channel register offsets, indexed by channel (0 or 1). */
static const uint32_t tmr_match_offset[] = {TIMER_TMR0_0_OFFSET, TIMER_TMR1_0_OFFSET};
static const uint32_t tmr_counter_offset[] = {TIMER_TCR0_OFFSET, TIMER_TCR1_OFFSET};
static const uint32_t tmr_status_offset[] = {TIMER_TSR0_OFFSET, TIMER_TSR1_OFFSET};
static const uint32_t tmr_int_en_offset[] = {TIMER_TIER0_OFFSET, TIMER_TIER1_OFFSET};
static const uint32_t tmr_int_clr_offset[] = {TIMER_TICR0_OFFSET, TIMER_TICR1_OFFSET};
static const uint32_t tmr_preload_val_offset[] = {TIMER_TPLVR0_OFFSET, TIMER_TPLVR1_OFFSET};
static const uint32_t tmr_preload_ctrl_offset[] = {TIMER_TPLCR0_OFFSET, TIMER_TPLCR1_OFFSET};
static const uint32_t tmr_cs_shift[] = {TIMER_CS_0_SHIFT, TIMER_CS_1_SHIFT};
static const uint32_t tmr_cs_mask[] = {TIMER_CS_0_MASK, TIMER_CS_1_MASK};
static const uint32_t tmr_div_mask[] = {TIMER_TCDR0_MASK, TIMER_TCDR1_MASK};

/* Bits in TCER register. */
#define TMR_EN_BIT(ch)      BIT((ch) + 1)
#define TMR_CNT_CLR_BIT(ch) BIT((ch) + 5)

/* Bit in TCMR register. */
#define TMR_MODE_BIT(ch) BIT((ch) + 1)

/*
 * Comparator bit in TSR / TIER / TICR registers.
 * Comparator 0 = top value, 1 = alarm 0, 2 = alarm 1.
 * Bits are identical for both channels; channel selects the register offset.
 */
#define TMR_COMP_BIT(comp) BIT(comp)
#define TMR_ALL_COMP_BITS  (TMR_COMP_BIT(0) | TMR_COMP_BIT(1) | TMR_COMP_BIT(2))

struct counter_bflb_timer_config {
	struct counter_config_info info;
	uintptr_t base;
	uint8_t channel;
	void (*irq_config_func)(const struct device *dev);
};

struct counter_bflb_timer_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	counter_alarm_callback_t alarm_cb[BFLB_TIMER_ALARM_CHANNELS];
	void *alarm_user_data[BFLB_TIMER_ALARM_CHANNELS];
	bool alarm_active[BFLB_TIMER_ALARM_CHANNELS];
	/*
	 * Software cycle base for SoCs without hardware counter clear.
	 * Tracks the raw hardware counter value at the start of the current
	 * virtual cycle. Always 0 on SoCs with CONFIG_COUNTER_BFLB_TIMER_HAS_CNT_CLR.
	 */
	uint32_t cycle_start;
};

static inline uint32_t counter_bflb_timer_read_counter(const struct counter_bflb_timer_config *cfg,
						       const struct counter_bflb_timer_data *data)
{
	return sys_read32(cfg->base + tmr_counter_offset[cfg->channel]) - data->cycle_start;
}

static inline uint32_t counter_bflb_timer_get_top(const struct counter_bflb_timer_config *cfg,
						  const struct counter_bflb_timer_data *data)
{
	return sys_read32(cfg->base + tmr_match_offset[cfg->channel]) - data->cycle_start;
}

static uint32_t counter_bflb_timer_ticks_add(uint32_t a, uint32_t b, uint32_t top)
{
	if (top == UINT32_MAX) {
		return a + b;
	}

	return (uint32_t)(((uint64_t)a + b) % ((uint64_t)top + 1U));
}

static void counter_bflb_timer_clear_counter(const struct counter_bflb_timer_config *cfg,
					     struct counter_bflb_timer_data *data)
{
	uint32_t tmp;

	if (IS_ENABLED(CONFIG_COUNTER_BFLB_TIMER_HAS_CNT_CLR)) {
		bool was_enabled;

		tmp = sys_read32(cfg->base + TIMER_TCER_OFFSET);
		was_enabled = (tmp & TMR_EN_BIT(cfg->channel)) != 0U;

		tmp |= TMR_EN_BIT(cfg->channel);
		tmp |= TMR_CNT_CLR_BIT(cfg->channel);
		sys_write32(tmp, cfg->base + TIMER_TCER_OFFSET);

		tmp &= ~TMR_CNT_CLR_BIT(cfg->channel);
		if (!was_enabled) {
			tmp &= ~TMR_EN_BIT(cfg->channel);
		}
		sys_write32(tmp, cfg->base + TIMER_TCER_OFFSET);
	} else {
		/* No HW clear: record current counter as the new cycle base. */
		data->cycle_start = sys_read32(cfg->base + tmr_counter_offset[cfg->channel]);
	}
}

static uint32_t counter_bflb_timer_clock_source_from_freq(uint32_t freq)
{
	if (freq <= 2000U) {
		return BFLB_TIMER_CLKSRC_1K;
	}

	if (freq <= 40000U) {
		return BFLB_TIMER_CLKSRC_32K;
	}

	return BFLB_TIMER_CLKSRC_XTAL;
}

static void counter_bflb_timer_disable_alarm_locked(const struct counter_bflb_timer_config *cfg,
						    struct counter_bflb_timer_data *data,
						    uint8_t alarm_chan)
{
	uint8_t ch = cfg->channel;
	uint32_t int_en;

	int_en = sys_read32(cfg->base + tmr_int_en_offset[ch]);
	int_en &= ~TMR_COMP_BIT(alarm_chan + 1);
	sys_write32(int_en, cfg->base + tmr_int_en_offset[ch]);

	sys_write32(TMR_COMP_BIT(alarm_chan + 1), cfg->base + tmr_int_clr_offset[ch]);

	data->alarm_cb[alarm_chan] = NULL;
	data->alarm_user_data[alarm_chan] = NULL;
	data->alarm_active[alarm_chan] = false;
}

static void
counter_bflb_timer_disable_all_alarms_locked(const struct counter_bflb_timer_config *cfg,
					     struct counter_bflb_timer_data *data)
{
	for (uint8_t i = 0U; i < BFLB_TIMER_ALARM_CHANNELS; i++) {
		counter_bflb_timer_disable_alarm_locked(cfg, data, i);
	}
}

static void counter_bflb_timer_isr(const struct device *dev)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	struct counter_bflb_timer_data *data = dev->data;
	uint8_t ch = cfg->channel;
	counter_top_callback_t top_cb = NULL;
	void *top_user_data = NULL;
	counter_alarm_callback_t cb[BFLB_TIMER_ALARM_CHANNELS] = {0};
	void *user_data[BFLB_TIMER_ALARM_CHANNELS] = {0};
	uint32_t ticks[BFLB_TIMER_ALARM_CHANNELS] = {0};
	bool alarm_fired[BFLB_TIMER_ALARM_CHANNELS] = {false};
	uint32_t status;
	uint32_t int_en;
	uint32_t clear_mask;
	uint32_t advance = 0U;
	bool any_alarm = false;
	unsigned int key;

	status = sys_read32(cfg->base + tmr_status_offset[ch]);
	if ((status & TMR_ALL_COMP_BITS) == 0U) {
		return;
	}

	key = irq_lock();

	status = sys_read32(cfg->base + tmr_status_offset[ch]);
	if ((status & TMR_ALL_COMP_BITS) == 0U) {
		irq_unlock(key);
		return;
	}

	clear_mask = 0U;
	if ((status & TMR_COMP_BIT(0)) != 0U) {
		clear_mask |= TMR_COMP_BIT(0);
		top_cb = data->top_cb;
		top_user_data = data->top_user_data;
		if (IS_ENABLED(CONFIG_COUNTER_BFLB_TIMER_HAS_CNT_CLR)) {
			counter_bflb_timer_clear_counter(cfg, data);
		} else {
			/* Defer cycle_start advance until after alarm processing
			 * so that read_counter returns correct virtual values.
			 */
			advance = counter_bflb_timer_get_top(cfg, data) + 1U;
		}
	}

	/* Process alarms whose comparator status is set (hardware match). */
	int_en = sys_read32(cfg->base + tmr_int_en_offset[ch]);
	for (uint8_t i = 0U; i < BFLB_TIMER_ALARM_CHANNELS; i++) {
		uint32_t alarm_bit = TMR_COMP_BIT(i + 1);

		if ((status & alarm_bit) == 0U) {
			continue;
		}
		clear_mask |= alarm_bit;
		if ((int_en & alarm_bit) == 0U) {
			continue;
		}
		cb[i] = data->alarm_cb[i];
		user_data[i] = data->alarm_user_data[i];
		data->alarm_cb[i] = NULL;
		data->alarm_user_data[i] = NULL;
		data->alarm_active[i] = false;
		int_en &= ~alarm_bit;
		ticks[i] = counter_bflb_timer_read_counter(cfg, data);
		alarm_fired[i] = true;
		any_alarm = true;
	}
	if (any_alarm) {
		sys_write32(int_en, cfg->base + tmr_int_en_offset[ch]);
	}

	if (clear_mask != 0U) {
		sys_write32(clear_mask, cfg->base + tmr_int_clr_offset[ch]);
	}

	/* Advance the software cycle base and shift comparator values
	 * for the next virtual cycle (non-CNT_CLR SoCs only).
	 */
	if (advance > 0U) {
		data->cycle_start += advance;

		/* Shift COMP0 (top) forward. */
		uint32_t comp = sys_read32(cfg->base + tmr_match_offset[ch]);

		sys_write32(comp + advance, cfg->base + tmr_match_offset[ch]);

		/* Shift still-active alarm comparators forward. */
		for (uint8_t i = 0U; i < BFLB_TIMER_ALARM_CHANNELS; i++) {
			if (data->alarm_active[i]) {
				uint32_t off = (i + 1U) * sizeof(uint32_t);

				comp = sys_read32(cfg->base + tmr_match_offset[ch] + off);
				sys_write32(comp + advance, cfg->base + tmr_match_offset[ch] + off);
			}
		}
	}

	irq_unlock(key);

	if (top_cb != NULL) {
		top_cb(dev, top_user_data);
	}

	for (uint8_t i = 0U; i < BFLB_TIMER_ALARM_CHANNELS; i++) {
		if (alarm_fired[i] && (cb[i] != NULL)) {
			cb[i](dev, i, ticks[i], user_data[i]);
		}
	}
}

static int counter_bflb_timer_start(const struct device *dev)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base + TIMER_TCER_OFFSET);
	tmp |= TMR_EN_BIT(cfg->channel);
	sys_write32(tmp, cfg->base + TIMER_TCER_OFFSET);

	return 0;
}

static int counter_bflb_timer_stop(const struct device *dev)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	struct counter_bflb_timer_data *data = dev->data;
	uint32_t tmp;
	unsigned int key;

	key = irq_lock();

	tmp = sys_read32(cfg->base + TIMER_TCER_OFFSET);
	tmp &= ~TMR_EN_BIT(cfg->channel);
	sys_write32(tmp, cfg->base + TIMER_TCER_OFFSET);

	/* The hardware counter may reset when the timer is disabled.
	 * Re-sync cycle_start and adjust COMP0 by the same delta so
	 * that the virtual top value is preserved.
	 */
	if (!IS_ENABLED(CONFIG_COUNTER_BFLB_TIMER_HAS_CNT_CLR)) {
		uint32_t new_cs = sys_read32(cfg->base + tmr_counter_offset[cfg->channel]);
		uint32_t delta = new_cs - data->cycle_start;

		if (delta != 0U) {
			uint32_t comp0 = sys_read32(cfg->base + tmr_match_offset[cfg->channel]);

			sys_write32(comp0 + delta, cfg->base + tmr_match_offset[cfg->channel]);
			data->cycle_start = new_cs;
		}
	}

	counter_bflb_timer_disable_all_alarms_locked(cfg, data);

	irq_unlock(key);

	return 0;
}

static int counter_bflb_timer_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	struct counter_bflb_timer_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	*ticks = counter_bflb_timer_read_counter(cfg, data);
	irq_unlock(key);

	return 0;
}

static int counter_bflb_timer_set_alarm(const struct device *dev, uint8_t chan_id,
					const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	struct counter_bflb_timer_data *data = dev->data;
	uint8_t ch = cfg->channel;
	uint32_t alarm_bit = TMR_COMP_BIT(chan_id + 1);
	uint32_t now;
	uint32_t top;
	uint32_t target;
	uint32_t int_en;
	unsigned int key;

	if (chan_id >= BFLB_TIMER_ALARM_CHANNELS) {
		return -ENOTSUP;
	}

	if ((alarm_cfg == NULL) || (alarm_cfg->callback == NULL)) {
		return -EINVAL;
	}

	key = irq_lock();

	if (data->alarm_active[chan_id]) {
		irq_unlock(key);
		return -EBUSY;
	}

	top = counter_bflb_timer_get_top(cfg, data);
	if (alarm_cfg->ticks > top) {
		irq_unlock(key);
		return -EINVAL;
	}

	now = counter_bflb_timer_read_counter(cfg, data);
	target = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)
			 ? alarm_cfg->ticks
			 : counter_bflb_timer_ticks_add(now, alarm_cfg->ticks, top);

	int_en = sys_read32(cfg->base + tmr_int_en_offset[ch]);
	int_en &= ~alarm_bit;
	sys_write32(int_en, cfg->base + tmr_int_en_offset[ch]);

	sys_write32(alarm_bit, cfg->base + tmr_int_clr_offset[ch]);

	sys_write32(data->cycle_start + target,
		    cfg->base + tmr_match_offset[ch] + ((chan_id + 1U) * sizeof(uint32_t)));

	data->alarm_cb[chan_id] = alarm_cfg->callback;
	data->alarm_user_data[chan_id] = alarm_cfg->user_data;
	data->alarm_active[chan_id] = true;

	int_en |= alarm_bit;
	sys_write32(int_en, cfg->base + tmr_int_en_offset[ch]);

	irq_unlock(key);

	return 0;
}

static int counter_bflb_timer_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	struct counter_bflb_timer_data *data = dev->data;
	unsigned int key;

	if (chan_id >= BFLB_TIMER_ALARM_CHANNELS) {
		return -ENOTSUP;
	}

	key = irq_lock();
	counter_bflb_timer_disable_alarm_locked(cfg, data, chan_id);
	irq_unlock(key);

	return 0;
}

static int counter_bflb_timer_set_top_value(const struct device *dev,
					    const struct counter_top_cfg *top_cfg)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	struct counter_bflb_timer_data *data = dev->data;
	uint8_t ch = cfg->channel;
	uint32_t now;
	uint32_t int_en;
	unsigned int key;
	int ret = 0;

	if (top_cfg == NULL) {
		return -EINVAL;
	}

	key = irq_lock();

	for (uint8_t i = 0U; i < BFLB_TIMER_ALARM_CHANNELS; i++) {
		if (data->alarm_active[i]) {
			irq_unlock(key);
			return -EBUSY;
		}
	}

	int_en = sys_read32(cfg->base + tmr_int_en_offset[ch]);
	int_en &= ~TMR_COMP_BIT(0);
	sys_write32(int_en, cfg->base + tmr_int_en_offset[ch]);

	sys_write32(TMR_COMP_BIT(0), cfg->base + tmr_int_clr_offset[ch]);

	data->top_cb = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	if ((top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0U) {
		counter_bflb_timer_clear_counter(cfg, data);
	} else {
		now = counter_bflb_timer_read_counter(cfg, data);
		if (now > top_cfg->ticks) {
			if ((top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) != 0U) {
				counter_bflb_timer_clear_counter(cfg, data);
			} else {
				ret = -ETIME;
			}
		}
	}

	/* Write COMP0 after any cycle_start update from clear_counter. */
	sys_write32(data->cycle_start + top_cfg->ticks, cfg->base + tmr_match_offset[ch]);

	sys_write32(TMR_COMP_BIT(0), cfg->base + tmr_int_clr_offset[ch]);

	int_en = sys_read32(cfg->base + tmr_int_en_offset[ch]);
	if (top_cfg->callback != NULL || top_cfg->ticks < UINT32_MAX) {
		int_en |= TMR_COMP_BIT(0);
	} else {
		int_en &= ~TMR_COMP_BIT(0);
	}
	sys_write32(int_en, cfg->base + tmr_int_en_offset[ch]);

	irq_unlock(key);

	return ret;
}

static uint32_t counter_bflb_timer_get_top_value(const struct device *dev)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	struct counter_bflb_timer_data *data = dev->data;
	uint32_t top;
	unsigned int key;

	key = irq_lock();
	top = counter_bflb_timer_get_top(cfg, data);
	irq_unlock(key);

	return top;
}

static uint32_t counter_bflb_timer_get_pending_int(const struct device *dev)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	uint32_t status;

	status = sys_read32(cfg->base + tmr_status_offset[cfg->channel]);

	return ((status & TMR_ALL_COMP_BITS) != 0U) ? 1U : 0U;
}

static uint32_t counter_bflb_timer_get_freq(const struct device *dev)
{
	const struct counter_bflb_timer_config *cfg = dev->config;

	return cfg->info.freq;
}

static int counter_bflb_timer_init(const struct device *dev)
{
	const struct counter_bflb_timer_config *cfg = dev->config;
	struct counter_bflb_timer_data *data = dev->data;
	uint8_t ch = cfg->channel;
	uint32_t tmp;
	uint32_t source;
	unsigned int key;

	data->top_cb = NULL;
	data->top_user_data = NULL;
	for (uint8_t i = 0U; i < BFLB_TIMER_ALARM_CHANNELS; i++) {
		data->alarm_cb[i] = NULL;
		data->alarm_user_data[i] = NULL;
		data->alarm_active[i] = false;
	}
	data->cycle_start = 0U;

	source = counter_bflb_timer_clock_source_from_freq(cfg->info.freq);

	tmp = sys_read32(cfg->base + TIMER_TCCR_OFFSET);
	tmp &= ~tmr_cs_mask[ch];
	tmp |= (source << tmr_cs_shift[ch]) & tmr_cs_mask[ch];
	sys_write32(tmp, cfg->base + TIMER_TCCR_OFFSET);

	tmp = sys_read32(cfg->base + TIMER_TCDR_OFFSET);
	tmp &= ~tmr_div_mask[ch];
	sys_write32(tmp, cfg->base + TIMER_TCDR_OFFSET);

	/* Free-run mode: counter runs to UINT32_MAX.  ISR handles wrapping
	 * either via hardware clear (CNT_CLR) or software cycle tracking.
	 */
	sys_write32(0U, cfg->base + tmr_preload_val_offset[ch]);
	sys_write32(BFLB_TIMER_PRELOAD_NONE, cfg->base + tmr_preload_ctrl_offset[ch]);

	tmp = sys_read32(cfg->base + TIMER_TCMR_OFFSET);
	tmp &= ~TMR_MODE_BIT(ch);
	sys_write32(tmp, cfg->base + TIMER_TCMR_OFFSET);

	sys_write32(UINT32_MAX, cfg->base + tmr_match_offset[ch]);
	for (uint8_t i = 0U; i < BFLB_TIMER_ALARM_CHANNELS; i++) {
		sys_write32(UINT32_MAX,
			    cfg->base + tmr_match_offset[ch] + ((i + 1U) * sizeof(uint32_t)));
	}

	key = irq_lock();

	tmp = sys_read32(cfg->base + TIMER_TCER_OFFSET);
	tmp &= ~TMR_EN_BIT(ch);
	sys_write32(tmp, cfg->base + TIMER_TCER_OFFSET);

	counter_bflb_timer_disable_all_alarms_locked(cfg, data);
	sys_write32(TMR_COMP_BIT(0), cfg->base + tmr_int_clr_offset[ch]);

	irq_unlock(key);

	cfg->irq_config_func(dev);

	return 0;
}

static DEVICE_API(counter, counter_bflb_timer_api) = {
	.start = counter_bflb_timer_start,
	.stop = counter_bflb_timer_stop,
	.get_value = counter_bflb_timer_get_value,
	.set_alarm = counter_bflb_timer_set_alarm,
	.cancel_alarm = counter_bflb_timer_cancel_alarm,
	.set_top_value = counter_bflb_timer_set_top_value,
	.get_top_value = counter_bflb_timer_get_top_value,
	.get_pending_int = counter_bflb_timer_get_pending_int,
	.get_freq = counter_bflb_timer_get_freq,
};

#define COUNTER_BFLB_TIMER_FREQ(n)                                                                 \
	DT_PROP(DT_CLOCKS_CTLR(DT_PARENT(DT_DRV_INST(n))), clock_frequency)

#define COUNTER_BFLB_TIMER_DEFINE(n)                                                               \
	BUILD_ASSERT(DT_INST_REG_ADDR(n) <= 1, "Timer channel must be 0 or 1");                   \
                                                                                                   \
	static void counter_bflb_timer_irq_config_##n(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), counter_bflb_timer_isr,     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static struct counter_bflb_timer_data counter_bflb_timer_data_##n;                         \
                                                                                                   \
	static const struct counter_bflb_timer_config counter_bflb_timer_config_##n = {            \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.channels = BFLB_TIMER_ALARM_CHANNELS,                             \
				.freq = COUNTER_BFLB_TIMER_FREQ(n),                                \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
			},                                                                         \
		.base = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(n))),                                   \
		.channel = DT_INST_REG_ADDR(n),                                                    \
		.irq_config_func = counter_bflb_timer_irq_config_##n,                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, counter_bflb_timer_init, NULL, &counter_bflb_timer_data_##n,      \
			      &counter_bflb_timer_config_##n, POST_KERNEL,                         \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_bflb_timer_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_BFLB_TIMER_DEFINE)
