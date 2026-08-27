/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_sim_counter

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/counter/counter_capture_emul.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <hw_counter.h>
#include <limits.h>

#define DRIVER_CONFIG_INFO_FLAGS (COUNTER_CONFIG_INFO_COUNT_UP)
#define DRIVER_CONFIG_INFO_CHANNELS CONFIG_COUNTER_NATIVE_SIM_NBR_CHANNELS
#define COUNTER_NATIVE_SIM_IRQ_FLAGS (0)
#define COUNTER_NATIVE_SIM_IRQ_PRIORITY (2)

#define COUNTER_PERIOD (USEC_PER_SEC / CONFIG_COUNTER_NATIVE_SIM_FREQUENCY)
#define TOP_VALUE (UINT_MAX)

static struct counter_alarm_cfg pending_alarm[DRIVER_CONFIG_INFO_CHANNELS];
static bool is_alarm_pending[DRIVER_CONFIG_INFO_CHANNELS];
static struct counter_top_cfg top;
static bool is_top_set;
static const struct device *dev_p;

#ifdef CONFIG_COUNTER_CAPTURE
/* Capture edges supported by the emulated counter */
enum ctr_capture_emul_cap {
	CTR_CAPTURE_EMUL_CAP_EDGE_RISING = BIT(0),
	CTR_CAPTURE_EMUL_CAP_EDGE_FALLING = BIT(1),
};

#define CTR_CAPTURE_EMUL_CAPS(inst)                                                      \
	((DT_INST_PROP(inst, rising_edge) ? CTR_CAPTURE_EMUL_CAP_EDGE_RISING : 0) |       \
	 (DT_INST_PROP(inst, falling_edge) ? CTR_CAPTURE_EMUL_CAP_EDGE_FALLING : 0))

/* Wraps counter_config_info so dev->config still resolves to it (info must stay first). */
struct ctr_native_sim_config {
	struct counter_config_info info;
	uint32_t capture_caps;
};

struct ctr_capture_channel {
	counter_capture_cb_t cb;
	void *user_data;
	counter_capture_flags_t flags;
	bool enabled;
	int level; /* last driven input level, for edge detection and input_get */
};

static struct ctr_capture_channel capture_chan[DRIVER_CONFIG_INFO_CHANNELS];

static inline bool ctr_capture_has_caps(const struct device *dev, uint32_t caps)
{
	const struct ctr_native_sim_config *cfg = dev->config;

	return (caps & cfg->capture_caps) == caps;
}
#endif /* CONFIG_COUNTER_CAPTURE */

static void schedule_next_isr(void)
{
	int64_t current_value = hw_counter_get_value();
	uint32_t next_time = top.ticks; /* top.ticks is TOP_VALUE if is_top_set == false */

	if (current_value == top.ticks) {
		current_value = -1;
	}

	for (int i = 0; i < DRIVER_CONFIG_INFO_CHANNELS; i++) {
		if (is_alarm_pending[i]) {
			if (pending_alarm[i].ticks > current_value) {
				/* If the alarm is not after a wrap */
				next_time = MIN(pending_alarm[i].ticks, next_time);
			}
		}
	}

	/* We will at least get an interrupt at top.ticks even if is_top_set == false,
	 * which is fine. We may use that to set the next alarm if needed
	 */
	hw_counter_set_target(next_time);
}

static void counter_isr(const void *arg)
{
	ARG_UNUSED(arg);
	uint32_t current_value = hw_counter_get_value();

	for (int i = 0; i < DRIVER_CONFIG_INFO_CHANNELS; i++) {
		if (is_alarm_pending[i] && (current_value == pending_alarm[i].ticks)) {
			is_alarm_pending[i] = false;
			if (pending_alarm[i].callback) {
				pending_alarm[i].callback(dev_p, i, current_value,
							  pending_alarm[i].user_data);
			}
		}
	}

	if (is_top_set && (current_value == top.ticks) && top.callback) {
		top.callback(dev_p, top.user_data);
	}

	schedule_next_isr();
}

static int ctr_init(const struct device *dev)
{
	dev_p = dev;
	memset(is_alarm_pending, 0, sizeof(is_alarm_pending));
#ifdef CONFIG_COUNTER_CAPTURE
	memset(capture_chan, 0, sizeof(capture_chan));
#endif
	is_top_set = false;
	top.ticks = TOP_VALUE;

	IRQ_CONNECT(COUNTER_EVENT_IRQ, COUNTER_NATIVE_SIM_IRQ_PRIORITY,
		    counter_isr, NULL, COUNTER_NATIVE_SIM_IRQ_FLAGS);
	irq_enable(COUNTER_EVENT_IRQ);
	hw_counter_set_period(COUNTER_PERIOD);
	hw_counter_set_wrap_value((uint64_t)top.ticks + 1);
	hw_counter_reset();

	return 0;
}

static int ctr_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	schedule_next_isr();
	hw_counter_start();
	return 0;
}

static int ctr_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	hw_counter_stop();
	return 0;
}

static int ctr_get_value(const struct device *dev, uint32_t *ticks)
{
	ARG_UNUSED(dev);

	*ticks = hw_counter_get_value();
	return 0;
}

static int ctr_reset(const struct device *dev)
{
	ARG_UNUSED(dev);

	hw_counter_reset();
	return 0;
}

static uint32_t ctr_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static bool is_any_alarm_pending(void)
{
	for (int i = 0; i < DRIVER_CONFIG_INFO_CHANNELS; i++) {
		if (is_alarm_pending[i]) {
			return true;
		}
	}
	return false;
}

static int ctr_set_top_value(const struct device *dev,
			     const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);

	if (is_any_alarm_pending()) {
		posix_print_warning("Can't set top value while alarm is active\n");
		return -EBUSY;
	}

	uint32_t current_value = hw_counter_get_value();

	if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		if (current_value >= cfg->ticks) {
			if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				hw_counter_reset();
			}
			return -ETIME;
		}
	} else {
		hw_counter_reset();
	}

	top = *cfg;
	hw_counter_set_wrap_value((uint64_t)top.ticks + 1);

	if ((cfg->ticks == TOP_VALUE) && !cfg->callback) {
		is_top_set = false;
	} else {
		is_top_set = true;
	}

	schedule_next_isr();

	return 0;
}

static uint32_t ctr_get_top_value(const struct device *dev)
{
	return top.ticks;
}

static int ctr_set_alarm(const struct device *dev, uint8_t chan_id,
			 const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);

	if (is_alarm_pending[chan_id]) {
		return -EBUSY;
	}

	uint32_t ticks = alarm_cfg->ticks;

	if (ticks > top.ticks) {
		posix_print_warning("Alarm ticks %u exceed top ticks %u\n", ticks,
				top.ticks);
		return -EINVAL;
	}

	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		uint32_t current_value = hw_counter_get_value();

		ticks += current_value;
		if (ticks > top.ticks) { /* Handle wrap arounds */
			ticks -= (top.ticks + 1); /* The count period is top.ticks + 1 */
		}
	}

	pending_alarm[chan_id] = *alarm_cfg;
	pending_alarm[chan_id].ticks = ticks;
	is_alarm_pending[chan_id] = true;

	schedule_next_isr();

	return 0;
}

static int ctr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);

	if (!hw_counter_is_started()) {
		posix_print_warning("Counter not started\n");
		return -ENOTSUP;
	}

	is_alarm_pending[chan_id] = false;

	schedule_next_isr();

	return 0;
}

#ifdef CONFIG_COUNTER_CAPTURE
static int ctr_capture_configure(const struct device *dev, uint8_t chan_id,
				 counter_capture_flags_t flags, counter_capture_cb_t cb,
				 void *user_data)
{
	uint32_t edges = 0;

	if (flags & COUNTER_CAPTURE_RISING_EDGE) {
		edges |= CTR_CAPTURE_EMUL_CAP_EDGE_RISING;
	}
	if (flags & COUNTER_CAPTURE_FALLING_EDGE) {
		edges |= CTR_CAPTURE_EMUL_CAP_EDGE_FALLING;
	}

	if (edges == 0) {
		return -EINVAL;
	}

	if (!ctr_capture_has_caps(dev, edges)) {
		return -ENOTSUP;
	}

	capture_chan[chan_id].flags = flags;
	capture_chan[chan_id].cb = cb;
	capture_chan[chan_id].user_data = user_data;
	capture_chan[chan_id].enabled = false;

	return 0;
}

static int ctr_enable_capture(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);

	if (capture_chan[chan_id].cb == NULL) {
		return -EINVAL;
	}

	capture_chan[chan_id].enabled = true;

	return 0;
}

static int ctr_disable_capture(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);

	capture_chan[chan_id].enabled = false;

	return 0;
}

int counter_capture_emul_input_set(const struct device *dev, uint8_t chan_id, int level)
{
	struct ctr_capture_channel *ch;
	counter_capture_flags_t edge_flag;
	bool rising, falling;
	int prev;

	if (chan_id >= DRIVER_CONFIG_INFO_CHANNELS) {
		return -EINVAL;
	}

	ch = &capture_chan[chan_id];
	prev = ch->level;
	ch->level = level ? 1 : 0;

	rising = (prev == 0) && (ch->level == 1);
	falling = (prev == 1) && (ch->level == 0);

	if (!ch->enabled || ch->cb == NULL) {
		return 0;
	}

	if (rising && (ch->flags & COUNTER_CAPTURE_RISING_EDGE)) {
		edge_flag = COUNTER_CAPTURE_RISING_EDGE;
	} else if (falling && (ch->flags & COUNTER_CAPTURE_FALLING_EDGE)) {
		edge_flag = COUNTER_CAPTURE_FALLING_EDGE;
	} else {
		return 0;
	}

	uint32_t ticks = hw_counter_get_value();

	if (ch->flags & COUNTER_CAPTURE_SINGLE_SHOT) {
		ch->enabled = false;
	}

	ch->cb(dev, chan_id, edge_flag, ticks, ch->user_data);

	return 0;
}

int counter_capture_emul_input_get(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);

	if (chan_id >= DRIVER_CONFIG_INFO_CHANNELS) {
		return -EINVAL;
	}

	return capture_chan[chan_id].level;
}
#endif /* CONFIG_COUNTER_CAPTURE */

static DEVICE_API(counter, ctr_api) = {
	.start = ctr_start,
	.stop = ctr_stop,
	.get_value = ctr_get_value,
	.reset = ctr_reset,
	.set_alarm = ctr_set_alarm,
	.cancel_alarm = ctr_cancel_alarm,
	.set_top_value = ctr_set_top_value,
	.get_pending_int = ctr_get_pending_int,
	.get_top_value = ctr_get_top_value,
#ifdef CONFIG_COUNTER_CAPTURE
	.capture_configure = ctr_capture_configure,
	.enable_capture = ctr_enable_capture,
	.disable_capture = ctr_disable_capture,
#endif
};

#ifdef CONFIG_COUNTER_CAPTURE
static const struct ctr_native_sim_config ctr_config = {
	.info = {
		.max_top_value = UINT_MAX,
		.freq = CONFIG_COUNTER_NATIVE_SIM_FREQUENCY,
		.channels = DRIVER_CONFIG_INFO_CHANNELS,
		.flags = DRIVER_CONFIG_INFO_FLAGS,
	},
	.capture_caps = CTR_CAPTURE_EMUL_CAPS(0),
};
#else
static const struct counter_config_info ctr_config = {
	.max_top_value = UINT_MAX,
	.freq = CONFIG_COUNTER_NATIVE_SIM_FREQUENCY,
	.channels = DRIVER_CONFIG_INFO_CHANNELS,
	.flags = DRIVER_CONFIG_INFO_FLAGS
};
#endif /* CONFIG_COUNTER_CAPTURE */

DEVICE_DT_INST_DEFINE(0, ctr_init,
		    NULL, NULL, &ctr_config, PRE_KERNEL_1,
		    CONFIG_COUNTER_INIT_PRIORITY, &ctr_api);
