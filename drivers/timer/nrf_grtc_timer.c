/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#if defined(CONFIG_CLOCK_CONTROL_NRF)
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#endif
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <nrfx_grtc.h>
#include <zephyr/sys/math_extras.h>

#define GRTC_NODE DT_NODELABEL(grtc)

/* Ensure that GRTC properties in devicetree are defined correctly. */
#if !DT_NODE_HAS_PROP(GRTC_NODE, owned_channels)
#error GRTC owned-channels DT property is not defined
#endif
#define OWNED_CHANNELS_MASK       NRFX_CONFIG_GRTC_MASK_DT(owned_channels)
#define CHILD_OWNED_CHANNELS_MASK NRFX_CONFIG_GRTC_MASK_DT(child_owned_channels)
#if ((OWNED_CHANNELS_MASK | CHILD_OWNED_CHANNELS_MASK) != OWNED_CHANNELS_MASK)
#error GRTC child-owned-channels DT property must be a subset of owned-channels
#endif

#define CHAN_COUNT     NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS
#define EXT_CHAN_COUNT (CHAN_COUNT - 1)
/* The reset value of waketime is 1, which doesn't seem to work.
 * It's being looked into, but for the time being use 4.
 * Timeout must always be higher than waketime, so setting that to 5.
 */
#define WAKETIME       (4)
#define TIMEOUT        (WAKETIME + 1)

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
#define MAX_TICKS                                                                                  \
	(((COUNTER_SPAN / CYC_PER_TICK) > INT_MAX) ? INT_MAX : (COUNTER_SPAN / CYC_PER_TICK))

#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

/* The maximum SYSCOUNTERVALID settling time equals 1x32k cycles + 20x1MHz cycles. */
#define GRTC_SYSCOUNTERVALID_SETTLE_MAX_TIME_US 51

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(GRTC_NODE);
#endif

static void sys_clock_timeout_handler(int32_t id, uint64_t cc_val, void *p_context);

static struct k_spinlock lock;
static uint64_t last_count;
static atomic_t int_mask;
static uint8_t ext_channels_allocated;
static nrfx_grtc_channel_t system_clock_channel_data = {
	.handler = sys_clock_timeout_handler,
	.p_context = NULL,
	.channel = (uint8_t)-1,
};

#define IS_CHANNEL_ALLOWED_ASSERT(chan)                                                            \
	__ASSERT_NO_MSG((NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK & (1UL << (chan))) &&           \
			((chan) != system_clock_channel_data.channel))

static inline void grtc_active_set(void)
{
#if defined(NRF_GRTC_HAS_SYSCOUNTER_ARRAY) && (NRF_GRTC_HAS_SYSCOUNTER_ARRAY == 1)
	nrfy_grtc_sys_counter_active_set(NRF_GRTC, true);
	while (!nrfy_grtc_sys_conter_ready_check(NRF_GRTC)) {
	}
#else
	nrfy_grtc_sys_counter_active_state_request_set(NRF_GRTC, true);
	k_busy_wait(GRTC_SYSCOUNTERVALID_SETTLE_MAX_TIME_US);
#endif
}

static inline void grtc_wakeup(void)
{
	if (IS_ENABLED(CONFIG_NRF_GRTC_SLEEP_ALLOWED)) {
		grtc_active_set();
	}
}

static inline void grtc_sleep(void)
{
	if (IS_ENABLED(CONFIG_NRF_GRTC_SLEEP_ALLOWED)) {
#if defined(NRF_GRTC_HAS_SYSCOUNTER_ARRAY) && (NRF_GRTC_HAS_SYSCOUNTER_ARRAY == 1)
		nrfy_grtc_sys_counter_active_set(NRF_GRTC, false);
#else
		nrfy_grtc_sys_counter_active_state_request_set(NRF_GRTC, false);
#endif
	}
}

static inline uint64_t counter_sub(uint64_t a, uint64_t b)
{
	return (a - b);
}

static inline uint64_t counter(void)
{
	uint64_t now;

	grtc_wakeup();
	nrfx_grtc_syscounter_get(&now);
	grtc_sleep();
	return now;
}

static inline uint64_t get_comparator(uint32_t chan)
{
	uint64_t cc;
	nrfx_err_t result;

	result = nrfx_grtc_syscounter_cc_value_read(chan, &cc);
	if (result != NRFX_SUCCESS) {
		if (result != NRFX_ERROR_INVALID_PARAM) {
			return -EAGAIN;
		}
		return -EPERM;
	}
	return cc;
}

static void system_timeout_set(uint64_t value)
{
	if (value <= NRF_GRTC_SYSCOUNTER_CCADD_MASK) {
		grtc_wakeup();
		nrfx_grtc_syscounter_cc_relative_set(&system_clock_channel_data, value, true,
						     NRFX_GRTC_CC_RELATIVE_SYSCOUNTER);
		grtc_sleep();
	} else {
		nrfx_grtc_syscounter_cc_absolute_set(&system_clock_channel_data, value + counter(),
						     true);
	}
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

static void sys_clock_timeout_handler(int32_t id, uint64_t cc_val, void *p_context)
{
	ARG_UNUSED(id);
	ARG_UNUSED(p_context);
	uint64_t dticks;
	uint64_t now = counter();

	if (unlikely(now < cc_val)) {
		return;
	}
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* protection is not needed because we are in the GRTC interrupt
		 * so it won't get preempted by the interrupt.
		 */
		system_timeout_set(CYC_PER_TICK);
	}

	dticks = counter_sub(now, last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? (int32_t)dticks : (dticks > 0));
}

int32_t z_nrf_grtc_timer_chan_alloc(void)
{
	uint8_t chan;
	nrfx_err_t err_code;

	/* Prevent allocating all available channels - one must be left for system purposes. */
	if (ext_channels_allocated >= EXT_CHAN_COUNT) {
		return -ENOMEM;
	}
	err_code = nrfx_grtc_channel_alloc(&chan);
	if (err_code != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	ext_channels_allocated++;
	return (int32_t)chan;
}

void z_nrf_grtc_timer_chan_free(int32_t chan)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);
	nrfx_err_t err_code = nrfx_grtc_channel_free(chan);

	if (err_code == NRFX_SUCCESS) {
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

uint64_t z_nrf_grtc_timer_compare_read(int32_t chan)
{
	IS_CHANNEL_ALLOWED_ASSERT(chan);

	return get_comparator(chan);
}

static int compare_set_nolocks(int32_t chan, uint64_t target_time,
			       z_nrf_grtc_timer_compare_handler_t handler, void *user_data)
{
	nrfx_err_t result;

	__ASSERT_NO_MSG(target_time < COUNTER_SPAN);
	nrfx_grtc_channel_t user_channel_data = {
		.handler = handler,
		.p_context = user_data,
		.channel = chan,
	};
	result = nrfx_grtc_syscounter_cc_absolute_set(&user_channel_data, target_time, true);
	if (result != NRFX_SUCCESS) {
		return -EPERM;
	}
	return 0;
}

static int compare_set(int32_t chan, uint64_t target_time,
		       z_nrf_grtc_timer_compare_handler_t handler, void *user_data)
{
	bool key = compare_int_lock(chan);
	int ret = compare_set_nolocks(chan, target_time, handler, user_data);

	compare_int_unlock(chan, key);

	return ret;
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
	uint64_t curr_time;
	int64_t curr_tick;
	int64_t result;
	int64_t abs_ticks;

	curr_time = counter();
	curr_tick = sys_clock_tick_get();

	abs_ticks = Z_TICK_ABS(t.ticks);
	if (abs_ticks < 0) {
		/* relative timeout */
		return (t.ticks > (int64_t)COUNTER_SPAN) ? -EINVAL : (curr_time + t.ticks);
	}

	/* absolute timeout */
	result = abs_ticks - curr_tick;

	if (result > (int64_t)COUNTER_SPAN) {
		return -EINVAL;
	}

	return curr_time + result;
}

int z_nrf_grtc_timer_capture_prepare(int32_t chan)
{
	nrfx_grtc_channel_t user_channel_data = {
		.handler = NULL,
		.p_context = NULL,
		.channel = chan,
	};
	nrfx_err_t result;

	IS_CHANNEL_ALLOWED_ASSERT(chan);

	/* Set the CC value to mark channel as not triggered and also to enable it
	 * (makes CCEN=1). COUNTER_SPAN is used so as not to fire an event unnecessarily
	 * - it can be assumed that such a large value will never be reached.
	 */
	result = nrfx_grtc_syscounter_cc_absolute_set(&user_channel_data, COUNTER_SPAN, false);

	if (result != NRFX_SUCCESS) {
		return -EPERM;
	}

	return 0;
}

int z_nrf_grtc_timer_capture_read(int32_t chan, uint64_t *captured_time)
{
	/* TODO: The implementation should probably go to nrfx_grtc and this
	 *       should be just a wrapper for some nrfx_grtc_syscounter_capture_read.
	 */

	uint64_t capt_time;

	IS_CHANNEL_ALLOWED_ASSERT(chan);

	/* TODO: Use `nrfy_grtc_sys_counter_enable_check` when available (NRFX-2480) */
	if (NRF_GRTC->CC[chan].CCEN == GRTC_CC_CCEN_ACTIVE_Enable) {
		/* If the channel is enabled (.CCEN), it means that there was no capture
		 * triggering event.
		 */
		return -EBUSY;
	}

	capt_time = nrfy_grtc_sys_counter_cc_get(NRF_GRTC, chan);

	__ASSERT_NO_MSG(capt_time < COUNTER_SPAN);

	*captured_time = capt_time;

	return 0;
}

#if defined(CONFIG_NRF_GRTC_SLEEP_ALLOWED)
int z_nrf_grtc_wakeup_prepare(uint64_t wake_time_us)
{
	nrfx_err_t err_code;
	static uint8_t systemoff_channel;
	uint64_t now = counter();
	/* Minimum time that ensures valid execution of system-off procedure. */
	uint32_t minimum_latency_us = nrfy_grtc_waketime_get(NRF_GRTC) +
				      nrfy_grtc_timeout_get(NRF_GRTC) +
				      CONFIG_NRF_GRTC_SLEEP_MINIMUM_LATENCY;
	uint32_t chan;
	int ret;

	if (minimum_latency_us > wake_time_us) {
		return -EINVAL;
	}
	k_spinlock_key_t key = k_spin_lock(&lock);

	err_code = nrfx_grtc_channel_alloc(&systemoff_channel);
	if (err_code != NRFX_SUCCESS) {
		k_spin_unlock(&lock, key);
		return -ENOMEM;
	}
	(void)nrfx_grtc_syscounter_cc_int_disable(systemoff_channel);
	ret = compare_set(systemoff_channel, now + wake_time_us, NULL, NULL);
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
	if (nrfy_grtc_sys_counter_compare_event_check(NRF_GRTC, systemoff_channel)) {
		k_spin_unlock(&lock, key);
		return -EINVAL;
	}

	/* This mechanism ensures that stored CC value is latched. */
	uint32_t wait_time =
		nrfy_grtc_timeout_get(NRF_GRTC) * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 32768 +
		MAX_CC_LATCH_WAIT_TIME_US;
	k_busy_wait(wait_time);
#if NRF_GRTC_HAS_CLKSEL
	nrfy_grtc_clksel_set(NRF_GRTC, NRF_GRTC_CLKSEL_LFXO);
#endif
	k_spin_unlock(&lock, key);
	return 0;
}
#endif /* CONFIG_NRF_GRTC_SLEEP_ALLOWED */

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = (uint32_t)counter();

	k_spin_unlock(&lock, key);
	return ret;
}

uint64_t sys_clock_cycle_get_64(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t ret = counter();

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	return (uint32_t)(counter_sub(counter(), last_count) / CYC_PER_TICK);
}

static int sys_clock_driver_init(void)
{
	nrfx_err_t err_code;

#if defined(CONFIG_NRF_GRTC_TIMER_CLOCK_MANAGEMENT) &&                                             \
	(defined(NRF_GRTC_HAS_CLKSEL) && (NRF_GRTC_HAS_CLKSEL == 1))
	/* Use System LFCLK as the low-frequency clock source. */
	nrfy_grtc_clksel_set(NRF_GRTC, NRF_GRTC_CLKSEL_LFCLK);
#endif

#if defined(CONFIG_NRF_GRTC_START_SYSCOUNTER)
	/* SYSCOUNTER needs to be turned off before initialization. */
	nrfy_grtc_sys_counter_set(NRF_GRTC, false);
	nrfy_grtc_timeout_set(NRF_GRTC, TIMEOUT);
	nrfy_grtc_waketime_set(NRF_GRTC, WAKETIME);
#endif /* CONFIG_NRF_GRTC_START_SYSCOUNTER */

	IRQ_CONNECT(DT_IRQN(GRTC_NODE), DT_IRQ(GRTC_NODE, priority), nrfx_grtc_irq_handler, 0, 0);

	err_code = nrfx_grtc_init(0);
	if (err_code != NRFX_SUCCESS) {
		return -EPERM;
	}

#if defined(CONFIG_NRF_GRTC_START_SYSCOUNTER)
	err_code = nrfx_grtc_syscounter_start(true, &system_clock_channel_data.channel);
	if (err_code != NRFX_SUCCESS) {
		return err_code == NRFX_ERROR_NO_MEM ? -ENOMEM : -EPERM;
	}
	if (IS_ENABLED(CONFIG_NRF_GRTC_SLEEP_ALLOWED)) {
		nrfy_grtc_sys_counter_auto_mode_set(NRF_GRTC, false);
	}
#else
	err_code = nrfx_grtc_channel_alloc(&system_clock_channel_data.channel);
	if (err_code != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif /* CONFIG_NRF_GRTC_START_SYSCOUNTER */

	if (!IS_ENABLED(CONFIG_NRF_GRTC_SLEEP_ALLOWED)) {
		grtc_active_set();
	}

	int_mask = NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK;
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		system_timeout_set(CYC_PER_TICK);
	}

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	static const enum nrf_lfclk_start_mode mode =
		IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT)
			? CLOCK_CONTROL_NRF_LF_START_NOWAIT
			: (IS_ENABLED(CONFIG_SYSTEM_CLOCK_WAIT_FOR_AVAILABILITY)
				   ? CLOCK_CONTROL_NRF_LF_START_AVAILABLE
				   : CLOCK_CONTROL_NRF_LF_START_STABLE);

	z_nrf_clock_control_lf_on(mode);
#endif

	return 0;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);
	uint64_t cyc, off, now;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : MIN(MAX_TICKS, MAX(ticks - 1, 0));

	now = counter();

	/* Round up to the next tick boundary */
	off = (now - last_count) + (CYC_PER_TICK - 1);
	off = (off / CYC_PER_TICK) * CYC_PER_TICK;

	/* Get the offset with respect to now */
	off -= (now - last_count);

	/* Add the offset to get to the next tick boundary */
	cyc = (uint64_t)ticks * CYC_PER_TICK + off;

	/* Due to elapsed time the calculation above might produce a
	 * duration that laps the counter.  Don't let it.
	 */
	if (cyc > MAX_CYCLES) {
		cyc = MAX_CYCLES;
	}

	system_timeout_set(cyc == 0 ? 1 : cyc);
}

#if defined(CONFIG_NRF_GRTC_TIMER_APP_DEFINED_INIT)
int nrf_grtc_timer_clock_driver_init(void)
{
	return sys_clock_driver_init();
}
#else
SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
#endif
