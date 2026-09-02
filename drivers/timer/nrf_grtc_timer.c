/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#if (defined(CONFIG_CLOCK_CONTROL_NRF) || defined(CONFIG_CLOCK_CONTROL_NRF_COMMON)) &&             \
	!(defined(CONFIG_SOC_SERIES_NRF54H) || defined(CONFIG_SOC_SERIES_NRF92))
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <nrfx_grtc.h>
#include <zephyr/sys/math_extras.h>
#include <nrf_sys_event.h>

#define GRTC_NODE DT_NODELABEL(grtc)
#define HFCLK_NODE DT_PHANDLE_BY_NAME(GRTC_NODE, clocks, hfclock)
#define LFCLK_NODE DT_PHANDLE_BY_NAME(GRTC_NODE, clocks, lfclock)

/* Ensure that GRTC properties in devicetree are defined correctly. */
#if !DT_NODE_HAS_PROP(GRTC_NODE, owned_channels)
#error GRTC owned-channels DT property is not defined
#endif
#define OWNED_CHANNELS_MASK       NRFX_CONFIG_MASK_DT(GRTC_NODE, owned_channels)
#define CHILD_OWNED_CHANNELS_MASK NRFX_CONFIG_MASK_DT(GRTC_NODE, child_owned_channels)
#if ((OWNED_CHANNELS_MASK | CHILD_OWNED_CHANNELS_MASK) != OWNED_CHANNELS_MASK)
#error GRTC child-owned-channels DT property must be a subset of owned-channels
#endif

#define CHAN_COUNT     NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS
#define EXT_CHAN_COUNT (CHAN_COUNT - 1)

#ifndef GRTC_SYSCOUNTERL_VALUE_Msk
#define GRTC_SYSCOUNTERL_VALUE_Msk GRTC_SYSCOUNTER_SYSCOUNTERL_VALUE_Msk
#endif

#ifndef GRTC_SYSCOUNTERH_VALUE_Msk
#define GRTC_SYSCOUNTERH_VALUE_Msk GRTC_SYSCOUNTER_SYSCOUNTERH_VALUE_Msk
#endif

#define MAX_CC_LATCH_WAIT_TIME_US 77

#define CYC_PER_TICK                                                                               \
	((uint64_t)sys_clock_hw_cycles_per_sec() / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define COUNTER_SPAN (GRTC_SYSCOUNTERL_VALUE_Msk | ((uint64_t)GRTC_SYSCOUNTERH_VALUE_Msk << 32))
#define MAX_ABS_TICKS (COUNTER_SPAN / CYC_PER_TICK)

/* Deadlines nearer than this register a system event, so that the system does
 * not enter a sleep state whose exit latency would overrun them.
 */
#define SYS_EVENT_AHEAD_US     1000U
#define SYS_EVENT_AHEAD_CYCLES k_us_to_cyc_ceil32(SYS_EVENT_AHEAD_US)

#if DT_NODE_HAS_STATUS_OKAY(LFCLK_NODE)
#define LFCLK_FREQUENCY_HZ DT_PROP(LFCLK_NODE, clock_frequency)
#elif defined(CONFIG_CLOCK_CONTROL_NRF)
#define LFCLK_FREQUENCY_HZ CONFIG_CLOCK_CONTROL_NRF_K32SRC_FREQUENCY
#else
#define LFCLK_FREQUENCY_HZ \
	DT_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_lfclk), k32src_frequency)
#endif

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(GRTC_NODE);
#endif

static void sys_clock_timeout_handler(int32_t id, uint64_t cc_val, void *p_context);

static atomic_t int_mask;
static uint8_t ext_channels_allocated;
static uint64_t grtc_start_value;
static nrfx_grtc_channel_t system_clock_channel_data = {
	.handler = sys_clock_timeout_handler,
	.p_context = NULL,
	.channel = (uint8_t)-1,
};
/* Only a kernel timeout is worth waking memory for. On a tickful kernel every
 * arm is the tick period, always inside the window below, and hinting each one
 * would hold the wake-ahead channel permanently.
 */
#if defined(CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT) && defined(CONFIG_TICKLESS_KERNEL) &&              \
	(CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT > 0)
#define USE_SYS_EVENT 1
#endif
static int sys_evt_handle = -1;

#define IS_CHANNEL_ALLOWED_ASSERT(chan)                                                            \
	__ASSERT_NO_MSG((NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK & (1UL << (chan))) &&           \
			((chan) != system_clock_channel_data.channel))

static inline uint64_t counter(void)
{
	return nrfx_grtc_syscounter_get();
}

static inline int get_comparator(uint32_t chan, uint64_t *cc)
{
	return nrfx_grtc_syscounter_cc_value_read(chan, cc);
}

static bool compare_int_lock(int32_t chan)
{
	atomic_val_t prev = atomic_and(&int_mask, ~BIT(chan));

	nrfx_grtc_syscounter_cc_int_disable(chan);

	return prev & BIT(chan);
}

static void compare_int_unlock(int32_t chan, bool key)
{
	if (key) {
		atomic_or(&int_mask, BIT(chan));
		nrfx_grtc_syscounter_cc_int_enable(chan);
	}
}

static void sys_event_unregister(bool canceled)
{
	if (IS_ENABLED(USE_SYS_EVENT) && (sys_evt_handle >= 0)) {
		nrf_sys_event_unregister(sys_evt_handle, canceled);
		sys_evt_handle = -1;
	}
}

/*
 * A free-running 52-bit SYSCOUNTER plus a compare channel armed through CCADD
 * with the SYSCOUNTER as its reference, so the hardware forms CC = SYSCOUNTER +
 * value itself. That is a delay from now rather than an absolute deadline, a
 * RELOAD backend, and it takes one store with nothing to read back and race
 * against.
 *
 * Bit 31 of CCADD selects the reference, leaving 31 bits of value, so the
 * register is narrower than the counter and bounds a single arm rather than
 * the span the core may leave unannounced. The compare fires once and does
 * not reload, hence TIMER_CORE_RELOAD_ONE_SHOT.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_RELOAD_ONE_SHOT
#define TIMER_CORE_COUNTER_WIDTH 52
#define TIMER_CORE_ALARM_MAX_CYCLES NRF_GRTC_SYSCOUNTER_CCADD_MASK

static inline uint64_t timer_driver_cycle_get(void)
{
	return counter();
}

static void timer_driver_set_reload(uint32_t cycles)
{
	sys_event_unregister(true);

	/* nrf_sys_event_register() takes microseconds, not cycles, and predicts
	 * when the interrupt lands. Reporting it early would have the memory
	 * woken sooner than needed and put back to sleep before the interrupt,
	 * so round the conversion up.
	 */
	if (IS_ENABLED(USE_SYS_EVENT)) {
		sys_evt_handle = (cycles <= SYS_EVENT_AHEAD_CYCLES)
				 ? nrf_sys_event_register(k_cyc_to_us_ceil32(cycles), false) : -1;
	}

	nrfx_grtc_syscounter_cc_rel_set(system_clock_channel_data.channel, cycles,
					NRFX_GRTC_CC_RELATIVE_SYSCOUNTER);
}

#include "system_timer_generic.h"

/* The width above has to be the counter's real one: too narrow and a long
 * unannounced span aliases, too wide and the masked delta underflows past the
 * wrap.
 */
BUILD_ASSERT(TIMER_CORE_COUNTER_MASK == COUNTER_SPAN,
	     "TIMER_CORE_COUNTER_WIDTH does not match the SYSCOUNTER width");

static void sys_clock_timeout_handler(int32_t id, uint64_t cc_val, void *p_context)
{
	ARG_UNUSED(id);
	ARG_UNUSED(cc_val);
	ARG_UNUSED(p_context);

	sys_event_unregister(false);

	timer_core_announce();
}

int32_t z_nrf_grtc_timer_chan_alloc(void)
{
	uint8_t chan;
	int err_code;

	/* Prevent allocating all available channels - one must be left for system purposes. */
	if (ext_channels_allocated >= EXT_CHAN_COUNT) {
		return -ENOMEM;
	}
	err_code = nrfx_grtc_channel_alloc(&chan);
	if (err_code < 0) {
		return -ENOMEM;
	}
	ext_channels_allocated++;
	return (int32_t)chan;
}

int32_t z_nrf_grtc_timer_ext_chan_alloc(void)
{
	uint8_t chan;
	int err_code;

	/* Prevent allocating all available channels - one must be left for system purposes. */
	if (ext_channels_allocated >= EXT_CHAN_COUNT) {
		return -ENOMEM;
	}
	err_code = nrfx_grtc_extended_channel_alloc(&chan);
	if (err_code < 0) {
		return -ENOMEM;
	}
	ext_channels_allocated++;
	return (int32_t)chan;
}

void z_nrf_grtc_timer_chan_free(int32_t chan)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);
	int err_code = nrfx_grtc_channel_free(chan);

	if (err_code == 0) {
		ext_channels_allocated--;
	}
}

bool z_nrf_grtc_timer_compare_evt_check(int32_t chan)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);

	uint32_t event_address = nrfx_grtc_event_compare_address_get(chan);

	return *(volatile uint32_t *)event_address != 0;
}

uint32_t z_nrf_grtc_timer_compare_evt_address_get(int32_t chan)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);

	return nrfx_grtc_event_compare_address_get(chan);
}

uint32_t z_nrf_grtc_timer_capture_task_address_get(int32_t chan)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);

	return nrfx_grtc_capture_task_address_get(chan);
}

uint64_t z_nrf_grtc_timer_read(void)
{
	return counter();
}

bool z_nrf_grtc_timer_compare_int_lock(int32_t chan)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);

	return compare_int_lock(chan);
}

void z_nrf_grtc_timer_compare_int_unlock(int32_t chan, bool key)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);

	compare_int_unlock(chan, key);
}

int z_nrf_grtc_timer_compare_read(int32_t chan, uint64_t *val)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);

	return get_comparator(chan, val);
}

static int compare_set_nolocks(int32_t chan, uint64_t target_time,
			       z_nrf_grtc_timer_compare_handler_t handler, void *user_data)
{
	__ASSERT_NO_MSG(target_time < COUNTER_SPAN);
	nrfx_grtc_channel_t user_channel_data = {
		.handler = handler,
		.p_context = user_data,
		.channel = chan,
	};
	return nrfx_grtc_syscounter_cc_absolute_set(&user_channel_data, target_time, true);
}

static int compare_set(int32_t chan, uint64_t target_time,
		       z_nrf_grtc_timer_compare_handler_t handler, void *user_data)
{
	bool key = compare_int_lock(chan);
	int ret = compare_set_nolocks(chan, target_time, handler, user_data);

	compare_int_unlock(chan, key);

	return ret;
}

static void interval_set_nolocks(int32_t chan, uint32_t initial_val, uint32_t interval_value,
				z_nrf_grtc_timer_compare_handler_t handler, void *user_data)
{
	nrfx_grtc_syscounter_cc_interval_set(chan, initial_val, interval_value);
	if (handler) {
		nrfx_grtc_channel_t user_channel_data = {
			.handler = handler,
			.p_context = user_data,
			.channel = chan,
		};
		nrfx_grtc_channel_callback_set(chan, user_channel_data.handler,
					user_channel_data.p_context);
	}
}

static void interval_set(int32_t chan, uint32_t initial_val, uint32_t interval_value,
				z_nrf_grtc_timer_compare_handler_t handler, void *user_data)
{
	bool key = compare_int_lock(chan);

	interval_set_nolocks(chan, initial_val, interval_value, handler, user_data);

	compare_int_unlock(chan, key);
}

int z_nrf_grtc_timer_interval_set(int32_t chan, uint32_t initial_value, uint32_t interval_value,
				z_nrf_grtc_timer_compare_handler_t handler, void *user_data)
{
	if (NRFX_BIT((uint32_t)chan) && NRFX_GRTC_CONFIG_EXTENDED_CC_CHANNELS_MASK == 0) {
		return -EPERM;
	}

	interval_set(chan, initial_value, interval_value,
			(nrfx_grtc_cc_handler_t)handler, user_data);

	return 0;
}

void z_nrf_grtc_timer_interval_stop(int32_t chan)
{
	nrfx_grtc_syscounter_cc_interval_reset(chan);
}

int z_nrf_grtc_timer_set(int32_t chan, uint64_t target_time,
			 z_nrf_grtc_timer_compare_handler_t handler, void *user_data)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);

	return compare_set(chan, target_time, (nrfx_grtc_cc_handler_t)handler, user_data);
}

void z_nrf_grtc_timer_abort(int32_t chan)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);

	bool key = compare_int_lock(chan);
	(void)nrfx_grtc_syscounter_cc_disable(chan);
	compare_int_unlock(chan, key);
}

uint64_t z_nrf_grtc_timer_get_ticks(k_timeout_t t)
{
	int64_t abs_ticks = Z_TICK_ABS(t.ticks);

	if (Z_IS_TIMEOUT_RELATIVE(t)) {
		int64_t grtc_ticks = t.ticks * CYC_PER_TICK;

		return (grtc_ticks > (int64_t)COUNTER_SPAN) ?
			-EINVAL : (counter() + grtc_ticks);
	}

	/* absolute timeout */
	return (abs_ticks > MAX_ABS_TICKS) ? -EINVAL : (abs_ticks * CYC_PER_TICK);
}

int z_nrf_grtc_timer_capture_prepare(int32_t chan)
{
	nrfx_grtc_channel_t user_channel_data = {
		.handler = NULL,
		.p_context = NULL,
		.channel = chan,
	};

	IS_CHANNEL_ALLOWED_ASSERT(chan);

	/* Set the CC value to mark channel as not triggered and also to enable it
	 * (makes CCEN=1). COUNTER_SPAN is used so as not to fire an event unnecessarily
	 * - it can be assumed that such a large value will never be reached.
	 */
	return nrfx_grtc_syscounter_cc_absolute_set(&user_channel_data, COUNTER_SPAN, false);
}

int z_nrf_grtc_timer_capture_read(int32_t chan, uint64_t *captured_time)
{
	int result;

	IS_CHANNEL_ALLOWED_ASSERT(chan);

	if (nrfx_grtc_sys_counter_cc_enable_check(chan)) {
		/* If the channel is enabled, it means that there was no capture
		 * triggering event.
		 */
		return -EBUSY;
	}
	result = nrfx_grtc_syscounter_cc_value_read(chan, captured_time);
	__ASSERT_NO_MSG(*captured_time < COUNTER_SPAN);

	return result;
}

uint64_t z_nrf_grtc_timer_startup_value_get(void)
{
	return grtc_start_value;
}

#if defined(CONFIG_POWEROFF) && defined(CONFIG_NRF_GRTC_START_SYSCOUNTER)
int z_nrf_grtc_wakeup_prepare(uint64_t wake_time_us)
{
	if (!nrfx_grtc_init_check()) {
		return -ENOTSUP;
	}

	int err_code;
	static struct k_spinlock lock;
	static uint8_t systemoff_channel;
	uint64_t now = counter();
	nrfx_grtc_sleep_config_t sleep_cfg;
	/* Minimum time that ensures valid execution of system-off procedure. */
	uint32_t minimum_latency_us;
	uint32_t chan;
	int ret;

	nrfx_grtc_sleep_configuration_get(&sleep_cfg);
	minimum_latency_us = (sleep_cfg.waketime + sleep_cfg.timeout) *
		USEC_PER_SEC / LFCLK_FREQUENCY_HZ +
		CONFIG_NRF_GRTC_SYSCOUNTER_SLEEP_MINIMUM_LATENCY;
	sleep_cfg.auto_mode = false;
	nrfx_grtc_sleep_configure(&sleep_cfg);

	if (minimum_latency_us > wake_time_us) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	err_code = nrfx_grtc_channel_alloc(&systemoff_channel);
	if (err_code < 0) {
		k_spin_unlock(&lock, key);
		return err_code;
	}
	(void)nrfx_grtc_syscounter_cc_int_disable(systemoff_channel);
	ret = compare_set(systemoff_channel,
			  now + wake_time_us * sys_clock_hw_cycles_per_sec() / USEC_PER_SEC, NULL,
			  NULL);
	if (ret < 0) {
		k_spin_unlock(&lock, key);
		return ret;
	}

	for (uint32_t grtc_chan_mask = NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK;
	     grtc_chan_mask > 0; grtc_chan_mask &= ~BIT(chan)) {
		/* Clear all GRTC channels except the systemoff_channel. */
		chan = u32_count_trailing_zeros(grtc_chan_mask);
		if (chan != systemoff_channel) {
			nrfx_grtc_syscounter_cc_disable(chan);
		}
	}

	/* Make sure that wake_time_us was not triggered yet. */
	if (nrfx_grtc_syscounter_compare_event_check(systemoff_channel)) {
		k_spin_unlock(&lock, key);
		return -EINVAL;
	}

	/* This mechanism ensures that stored CC value is latched. */
	uint32_t wait_time =
		nrfy_grtc_timeout_get(NRF_GRTC) * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC /
		LFCLK_FREQUENCY_HZ + MAX_CC_LATCH_WAIT_TIME_US;
	k_busy_wait(wait_time);
	k_spin_unlock(&lock, key);
	return 0;
}
#endif /* CONFIG_POWEROFF */

#if !defined(CONFIG_GEN_SW_ISR_TABLE)
ISR_DIRECT_DECLARE(nrfx_grtc_direct_irq_handler)
{
	nrfx_grtc_irq_handler();
	ISR_DIRECT_PM();
	return 1;
}
#endif

void sys_clock_disable(void)
{
	int err __maybe_unused;
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_LFCLK);

	err = onoff_release(mgr);
	__ASSERT_NO_MSG(err >= 0);

	nrfx_grtc_uninit();
	nrfx_coredep_delay_us(1000);
#elif defined(CONFIG_CLOCK_CONTROL_NRF_COMMON) &&                                                  \
	!(defined(CONFIG_SOC_SERIES_NRF54H) || defined(CONFIG_SOC_SERIES_NRF92))
	err = nrf_clock_control_release(DEVICE_DT_GET_ONE(nordic_nrf_clock_lfclk), NULL);
	__ASSERT_NO_MSG(err >= 0);

	nrfx_grtc_uninit();
	nrfx_coredep_delay_us(1000);
#else
	nrfx_grtc_uninit();
#endif
}

static int sys_clock_driver_init(void)
{
	int err_code;

#if defined(CONFIG_GEN_SW_ISR_TABLE)
	IRQ_CONNECT(DT_IRQN(GRTC_NODE), DT_IRQ(GRTC_NODE, priority), nrfx_isr,
		    nrfx_grtc_irq_handler, 0);
#else
	IRQ_DIRECT_CONNECT(DT_IRQN(GRTC_NODE), DT_IRQ(GRTC_NODE, priority),
			   nrfx_grtc_direct_irq_handler, 0);
	irq_enable(DT_IRQN(GRTC_NODE));
#endif

#if defined(CONFIG_NRF_GRTC_TIMER_CLOCK_MANAGEMENT) && NRF_GRTC_HAS_CLKSEL
#if defined(CONFIG_NRF_GRTC_TIMER_SOURCE_LFLPRC)
	/* Switch to LFPRC as the low-frequency clock source. */
	nrfx_grtc_clock_source_set(NRF_GRTC_CLKSEL_LFLPRC);
#elif defined(CONFIG_NRF_GRTC_TIMER_SOURCE_LFXO)
	/* Switch to LFXO as the low-frequency clock source. */
	nrfx_grtc_clock_source_set(NRF_GRTC_CLKSEL_LFXO);
#else
	/* Use LFCLK as the low-frequency clock source. */
	nrfx_grtc_clock_source_set(NRF_GRTC_CLKSEL_LFCLK);
#endif
#endif

	err_code = nrfx_grtc_init(0);
	if (err_code < 0) {
		return err_code;
	}

#if defined(CONFIG_NRF_GRTC_START_SYSCOUNTER)
	err_code = nrfx_grtc_syscounter_start(true, &system_clock_channel_data.channel);
	if (err_code < 0) {
		return err_code;
	}
#else
	err_code = nrfx_grtc_channel_alloc(&system_clock_channel_data.channel);
	if (err_code < 0) {
		return err_code;
	}
#endif /* CONFIG_NRF_GRTC_START_SYSCOUNTER */

	grtc_start_value = (counter() / CYC_PER_TICK) * CYC_PER_TICK;
	int_mask = NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK;

	timer_core_init();

	/* Enables the channel interrupt, so it comes after the baseline exists.
	 * A compare left pending by whoever was driving the counter before would
	 * otherwise be announced against a zero baseline, which on a counter
	 * another domain started long ago is an enormous bogus elapse.
	 */
	nrfx_grtc_channel_callback_set(system_clock_channel_data.channel,
				       sys_clock_timeout_handler, NULL);

	return 0;
}

static int grtc_post_init(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF) ||                                                           \
	(defined(CONFIG_CLOCK_CONTROL_NRF_COMMON) &&                                               \
	 !(defined(CONFIG_SOC_SERIES_NRF54H) || defined(CONFIG_SOC_SERIES_NRF92)))
	static const enum nrf_lfclk_start_mode mode =
		IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT)
			? CLOCK_CONTROL_NRF_LF_START_NOWAIT
			: (IS_ENABLED(CONFIG_SYSTEM_CLOCK_WAIT_FOR_AVAILABILITY)
				   ? CLOCK_CONTROL_NRF_LF_START_AVAILABLE
				   : CLOCK_CONTROL_NRF_LF_START_STABLE);

	z_nrf_clock_control_lf_on(mode);
#endif

#if defined(CONFIG_NRF_GRTC_ALWAYS_ON)
	nrfx_grtc_active_request_set(true);
#endif

#if DT_PROP(GRTC_NODE, clkout_32k)
	nrfy_grtc_clkout_set(NRF_GRTC, NRF_GRTC_CLKOUT_32K, true);
#endif

#if DT_NODE_HAS_PROP(GRTC_NODE, clkout_fast_frequency_hz)
#if !DT_NODE_HAS_PROP(HFCLK_NODE, clock_frequency)
#error "hfclock reference required when fast clock output is enabled."
#endif

#if DT_PROP(GRTC_NODE, clkout_fast_frequency_hz) > (DT_PROP(HFCLK_NODE, clock_frequency) / 2)
#error "Invalid frequency value for fast clock output."
#endif
	uint32_t base_frequency = DT_PROP(HFCLK_NODE, clock_frequency);
	uint32_t requested_frequency = DT_PROP(GRTC_NODE, clkout_fast_frequency_hz);
	uint32_t grtc_div = base_frequency / (requested_frequency * 2);

	nrfy_grtc_clkout_divider_set(NRF_GRTC, (uint8_t)grtc_div);
	nrfy_grtc_clkout_set(NRF_GRTC, NRF_GRTC_CLKOUT_FAST, true);
#endif

#if DT_PROP(GRTC_NODE, clkout_32k) || DT_NODE_HAS_PROP(GRTC_NODE, clkout_fast_frequency_hz)
	PINCTRL_DT_DEFINE(GRTC_NODE);
	const struct pinctrl_dev_config *pcfg = PINCTRL_DT_DEV_CONFIG_GET(GRTC_NODE);

	return pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
#else
	return 0;
#endif
}

#if defined(CONFIG_NRF_GRTC_TIMER_APP_DEFINED_INIT)
int nrf_grtc_timer_clock_driver_init(void)
{
	return sys_clock_driver_init();
}
#else
/* Init must follow soc init and precede LOG_CORE_INIT() */
SYS_INIT(sys_clock_driver_init, EARLY, 1);
SYS_INIT(grtc_post_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
#endif
