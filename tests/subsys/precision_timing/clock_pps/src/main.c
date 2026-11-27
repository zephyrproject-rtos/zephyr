/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/precision_timing/precision_clock_pps.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define CALLBACK_TIMEOUT      K_MSEC(2000)
#define WAIT_STATE_TIMEOUT_US 2000000

/* Fake protocol-neutral clock and output-channel provider, guarded by fake_lock. */
struct fake_clock {
	struct precision_clock clock;
	struct precision_clock_output_channel_caps caps[2];
	uint32_t channel_count;
	uint32_t start_channel;
	struct precision_clock_output_status status;
	precision_time_t offset_ns;
	precision_time_t stop_advance_ns;
	int read_error;
	int read_error_after_stop;
	int caps_error;
	int start_error;
	int stop_error;
	int status_error;
	atomic_t caps_calls;
	atomic_t start_calls;
	atomic_t stop_calls;
	atomic_t status_calls;
	uint32_t read_delay_ms;
	uint32_t read_delay_after_stop_ms;
	bool sample_before_delay;
	bool align_delayed_sample;
	bool provider_configured;
	struct k_sem *read_entered;
	struct k_sem *read_release;
};

static K_MUTEX_DEFINE(fake_lock);
static struct fake_clock fake;
static struct fake_clock fake_other;
static struct k_sem *active_read_release;

/* Per-instance callback bookkeeping so concurrent instances stay independent. */
struct cb_record {
	struct k_sem sem;
	struct precision_clock_pps_state state;
	struct precision_clock_pps_state state_after_release;
	k_tid_t thread;
	atomic_t calls;
	atomic_t observed_events;
	bool attempt_self_stop;
	int self_stop_result;
	struct precision_clock_pps *stop_target;
	int other_stop_result;
	struct k_sem callback_entered;
	struct k_sem callback_release;
	bool block_callback;
	atomic_t completed_calls;
};

static struct cb_record rec_a;
static struct cb_record rec_b;

struct stop_thread_ctx {
	struct precision_clock_pps *instance;
	struct cb_record *rec;
	atomic_t started;
	atomic_t done;
	uint32_t completed_calls;
	int result;
};

#define STOP_STACK_SIZE 2048

K_THREAD_STACK_DEFINE(stop_thread_stack, STOP_STACK_SIZE);
static struct k_thread stop_thread_data;
static bool stop_thread_active;

#define MAX_TRACKED_PPS_INSTANCES 2

/* Track only initialized, static instances so teardown can cancel failed tests safely. */
static struct precision_clock_pps *tracked_pps_instances[MAX_TRACKED_PPS_INSTANCES];
static size_t tracked_pps_instance_count;

static void track_pps_instance(struct precision_clock_pps *instance)
{
	zassert_true(tracked_pps_instance_count < ARRAY_SIZE(tracked_pps_instances));
	tracked_pps_instances[tracked_pps_instance_count++] = instance;
}

static precision_time_t fake_monotonic_ns(void)
{
	int64_t ticks = k_uptime_ticks();

	return (precision_time_t)k_ticks_to_ns_floor64(ticks);
}

static int fake_read(const struct precision_clock *precision_clk, precision_time_t *tm)
{
	struct fake_clock *f = precision_clk->data;
	struct k_sem *entered;
	struct k_sem *release;
	precision_time_t offset;
	uint32_t delay_ms;
	bool sample_before_delay;
	bool align_delayed_sample;
	int error;

	k_mutex_lock(&fake_lock, K_FOREVER);
	entered = f->read_entered;
	release = f->read_release;
	f->read_entered = NULL;
	f->read_release = NULL;
	if (release != NULL) {
		active_read_release = release;
	}
	k_mutex_unlock(&fake_lock);

	if (entered != NULL) {
		k_sem_give(entered);
	}
	if (release != NULL) {
		(void)k_sem_take(release, K_FOREVER);
	}

	k_mutex_lock(&fake_lock, K_FOREVER);
	if (active_read_release == release) {
		active_read_release = NULL;
	}
	error = f->read_error;
	offset = f->offset_ns;
	delay_ms = f->read_delay_ms;
	sample_before_delay = f->sample_before_delay;
	align_delayed_sample = f->align_delayed_sample && delay_ms != 0U;
	if (align_delayed_sample) {
		f->align_delayed_sample = false;
	}
	f->read_delay_ms = 0;
	k_mutex_unlock(&fake_lock);

	if (error < 0) {
		return error;
	}

	if (align_delayed_sample) {
		/* Force the delayed initial read to cross the selected second boundary. */
		k_mutex_lock(&fake_lock, K_FOREVER);
		f->offset_ns = 900LL * NSEC_PER_MSEC - fake_monotonic_ns() % NSEC_PER_SEC;
		offset = f->offset_ns;
		k_mutex_unlock(&fake_lock);
	}
	if (sample_before_delay) {
		*tm = fake_monotonic_ns() + offset;
	}
	if (delay_ms != 0U) {
		k_msleep(delay_ms);
	}
	if (!sample_before_delay) {
		*tm = fake_monotonic_ns() + offset;
	}
	return 0;
}

static int fake_output_get_caps(const struct precision_clock *precision_clk,
				struct precision_clock_output_caps *caps)
{
	struct fake_clock *f = precision_clk->data;
	int error;

	k_mutex_lock(&fake_lock, K_FOREVER);
	atomic_inc(&f->caps_calls);
	error = f->caps_error;
	*caps = (struct precision_clock_output_caps){
		.channel_count = f->channel_count,
		.channels = f->channel_count == 0U ? NULL : f->caps,
	};
	k_mutex_unlock(&fake_lock);

	return error;
}

static void fake_mark_active(struct fake_clock *f, uint32_t channel,
			     const struct precision_clock_output_config *config)
{
	f->provider_configured = true;
	f->status.configured = true;
	f->status.config = *config;
	f->status.hardware_active_valid =
		(f->caps[channel].flags & PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE) != 0U;
	f->status.hardware_active = f->status.hardware_active_valid;
}

static int fake_output_start(const struct precision_clock *precision_clk, uint32_t channel,
			     const struct precision_clock_output_config *config)
{
	struct fake_clock *f = precision_clk->data;
	int error;

	if (channel >= f->channel_count) {
		return -ENOTSUP;
	}

	k_mutex_lock(&fake_lock, K_FOREVER);
	f->start_channel = channel;
	atomic_inc(&f->start_calls);
	error = f->start_error;
	if (f->provider_configured) {
		error = -EBUSY;
	} else if (error == 0 || error == -EINPROGRESS) {
		fake_mark_active(f, channel, config);
	}
	k_mutex_unlock(&fake_lock);

	return error;
}

static int fake_output_stop(const struct precision_clock *precision_clk, uint32_t channel)
{
	struct fake_clock *f = precision_clk->data;
	int error;

	if (channel >= f->channel_count) {
		return -ENOTSUP;
	}

	k_mutex_lock(&fake_lock, K_FOREVER);
	atomic_inc(&f->stop_calls);
	f->offset_ns += f->stop_advance_ns;
	error = f->stop_error;
	if (error == 0) {
		f->provider_configured = false;
		f->read_error = f->read_error_after_stop;
		f->status.configured = false;
		f->read_delay_ms = f->read_delay_after_stop_ms;
		f->read_delay_after_stop_ms = 0;
	}
	k_mutex_unlock(&fake_lock);

	return error;
}

static int fake_output_get_status(const struct precision_clock *precision_clk, uint32_t channel,
				  struct precision_clock_output_status *status)
{
	struct fake_clock *f = precision_clk->data;
	int error;

	if (channel >= f->channel_count) {
		return -ENOTSUP;
	}

	k_mutex_lock(&fake_lock, K_FOREVER);
	atomic_inc(&f->status_calls);
	error = f->status_error;
	*status = f->status;
	k_mutex_unlock(&fake_lock);

	return error;
}

static const struct precision_clock_api fake_api = {
	.read = fake_read,
	.output_get_caps = fake_output_get_caps,
	.output_start = fake_output_start,
	.output_stop = fake_output_stop,
	.output_get_status = fake_output_get_status,
};

static void fake_init(struct fake_clock *f)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	memset(f, 0, sizeof(*f));
	f->clock.api = &fake_api;
	f->clock.data = f;
	f->channel_count = 1;
	f->caps[0] = (struct precision_clock_output_channel_caps){
		.flags = PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM |
			 PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH,
		.resolution_ns = 1,
		.min_lead_time_ns = NSEC_PER_MSEC,
		.min_period_ns = NSEC_PER_MSEC,
		.max_period_ns = 2LL * NSEC_PER_SEC,
		.min_pulse_width_ns = 1,
		.max_pulse_width_ns = 999999999,
	};
	k_mutex_unlock(&fake_lock);
}

static struct precision_clock_pps_config default_config(void)
{
	return (struct precision_clock_pps_config){
		.channel = 0,
		.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT,
		.pulse_width_ns = 0,
		.start_guard_ns = 10LL * NSEC_PER_MSEC,
		.step_limit_ns = 100LL * NSEC_PER_MSEC,
		.poll_interval_ms = 20,
	};
}

static struct precision_clock_pps_config exact_config(void)
{
	struct precision_clock_pps_config config = default_config();

	config.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_EXACT;
	config.pulse_width_ns = 200LL * NSEC_PER_MSEC;
	return config;
}

static void bump_offset(struct fake_clock *f, precision_time_t delta_ns)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	f->offset_ns += delta_ns;
	k_mutex_unlock(&fake_lock);
}

static void set_configured(struct fake_clock *f, bool configured)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	f->status.configured = configured;
	k_mutex_unlock(&fake_lock);
}

static void set_hardware_active(struct fake_clock *f, bool active)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	f->status.hardware_active = active;
	k_mutex_unlock(&fake_lock);
}

static void mismatch_active_output(struct fake_clock *f)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	f->status.config.waveform.first_rising_time += 1;
	k_mutex_unlock(&fake_lock);
}

static void set_read_error(struct fake_clock *f, int error)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	f->read_error = error;
	k_mutex_unlock(&fake_lock);
}

static void set_status_error(struct fake_clock *f, int error)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	f->status_error = error;
	k_mutex_unlock(&fake_lock);
}

static void set_start_error(struct fake_clock *f, int error)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	f->start_error = error;
	k_mutex_unlock(&fake_lock);
}

static void set_stop_error(struct fake_clock *f, int error)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	f->stop_error = error;
	k_mutex_unlock(&fake_lock);
}

static void test_callback(struct precision_clock_pps *instance)
{
	struct cb_record *rec = instance->config.user_data;

	rec->thread = k_current_get();
	zassert_ok(precision_clock_pps_state_get(instance, &rec->state));
	atomic_or(&rec->observed_events, rec->state.events);
	atomic_inc(&rec->calls);
	if (rec->attempt_self_stop) {
		rec->attempt_self_stop = false;
		rec->self_stop_result = precision_clock_pps_stop(instance);
	}
	if (rec->block_callback) {
		rec->block_callback = false;
		k_sem_give(&rec->callback_entered);
		(void)k_sem_take(&rec->callback_release, K_FOREVER);
		zassert_ok(precision_clock_pps_state_get(instance, &rec->state_after_release));
	}
	if (rec->stop_target != NULL) {
		struct precision_clock_pps *target = rec->stop_target;

		rec->stop_target = NULL;
		rec->other_stop_result = precision_clock_pps_stop(target);
	}
	k_sem_give(&rec->sem);
	atomic_inc(&rec->completed_calls);
}

static void cb_record_reset(struct cb_record *rec)
{
	k_sem_init(&rec->sem, 0, 1);
	k_sem_init(&rec->callback_entered, 0, 1);
	k_sem_init(&rec->callback_release, 0, 1);
	memset(&rec->state, 0, sizeof(rec->state));
	memset(&rec->state_after_release, 0, sizeof(rec->state_after_release));
	rec->thread = NULL;
	atomic_clear(&rec->observed_events);
	atomic_clear(&rec->calls);
	rec->attempt_self_stop = false;
	rec->self_stop_result = 0;
	rec->stop_target = NULL;
	rec->other_stop_result = 0;
	rec->block_callback = false;
	atomic_clear(&rec->completed_calls);
}

static void wait_callback(struct cb_record *rec)
{
	zassert_equal(k_sem_take(&rec->sem, CALLBACK_TIMEOUT), 0,
		      "timed out waiting for an instance callback");
}

static void wait_parked_callback(struct cb_record *rec)
{
	zassert_ok(k_sem_take(&rec->callback_entered, CALLBACK_TIMEOUT),
		   "timed out waiting for the callback barrier");
}

static void init_instance(struct precision_clock_pps *instance, struct fake_clock *f,
			  struct precision_clock_pps_config config, struct cb_record *rec)
{
	config.callback = rec != NULL ? test_callback : NULL;
	config.user_data = rec;
	zassert_ok(precision_clock_pps_init(instance, &f->clock, &config));
	track_pps_instance(instance);
}

static void start_instance(struct precision_clock_pps *instance, struct fake_clock *f,
			   const struct precision_clock_pps_config *config, struct cb_record *rec)
{
	init_instance(instance, f, *config, rec);
	zassert_ok(precision_clock_pps_start(instance));
}

static void wait_provider_calls(const atomic_t *calls, atomic_val_t minimum)
{
	zassert_true(WAIT_FOR(atomic_get(calls) >= minimum, WAIT_STATE_TIMEOUT_US, k_msleep(1)),
		     "timed out waiting for the provider operation");
}

static void verify_phase(struct precision_clock_pps *instance, enum precision_clock_pps_phase phase)
{
	struct precision_clock_pps_state state;

	zassert_ok(precision_clock_pps_state_get(instance, &state));
	zassert_equal(state.phase, phase);
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	tracked_pps_instance_count = 0;
	cb_record_reset(&rec_a);
	cb_record_reset(&rec_b);
}

static void release_parked_reads(void)
{
	struct k_sem *active;
	struct k_sem *pending;
	struct k_sem *pending_other;

	k_mutex_lock(&fake_lock, K_FOREVER);
	active = active_read_release;
	pending = fake.read_release;
	pending_other = fake_other.read_release;
	active_read_release = NULL;
	fake.read_entered = NULL;
	fake.read_release = NULL;
	fake_other.read_entered = NULL;
	fake_other.read_release = NULL;
	k_mutex_unlock(&fake_lock);

	if (active != NULL) {
		k_sem_give(active);
	}
	if (pending != NULL && pending != active) {
		k_sem_give(pending);
	}
	if (pending_other != NULL && pending_other != active && pending_other != pending) {
		k_sem_give(pending_other);
	}
}

static void stop_thread_fn(void *ctx_ptr, void *unused1, void *unused2)
{
	struct stop_thread_ctx *ctx = ctx_ptr;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	atomic_set(&ctx->started, 1);
	ctx->result = precision_clock_pps_stop(ctx->instance);
	if (ctx->rec != NULL) {
		ctx->completed_calls = atomic_get(&ctx->rec->completed_calls);
	}
	atomic_set(&ctx->done, 1);
}

static void start_stop_thread(struct stop_thread_ctx *ctx)
{
	stop_thread_active = true;
	k_thread_create(&stop_thread_data, stop_thread_stack,
			K_THREAD_STACK_SIZEOF(stop_thread_stack), stop_thread_fn, ctx, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
}

static void join_stop_thread(void)
{
	zassert_ok(k_thread_join(&stop_thread_data, CALLBACK_TIMEOUT));
	stop_thread_active = false;
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Release barriers even when an assertion failed while work was parked. */
	set_stop_error(&fake, 0);
	set_stop_error(&fake_other, 0);
	k_sem_give(&rec_a.callback_release);
	k_sem_give(&rec_b.callback_release);
	release_parked_reads();
	if (stop_thread_active) {
		join_stop_thread();
	}
	for (size_t i = 0; i < tracked_pps_instance_count; i++) {
		(void)precision_clock_pps_stop(tracked_pps_instances[i]);
	}
}

ZTEST(precision_timing_clock_pps, test_kconfig_defaults_allow_instance_overrides)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = PRECISION_CLOCK_PPS_CONFIG_DEFAULTS;

	zassert_equal(config.channel, CONFIG_PRECISION_CLOCK_PPS_CHANNEL);
	zassert_equal(config.start_guard_ns, CONFIG_PRECISION_CLOCK_PPS_START_GUARD_NS);
	zassert_equal(config.step_limit_ns, CONFIG_PRECISION_CLOCK_PPS_STEP_LIMIT_NS);
	zassert_equal(config.poll_interval_ms, CONFIG_PRECISION_CLOCK_PPS_POLL_INTERVAL_MS);
	if (IS_ENABLED(CONFIG_PRECISION_CLOCK_PPS_WIDTH_EXACT)) {
		zassert_equal(config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);
		zassert_equal(config.pulse_width_ns, CONFIG_PRECISION_CLOCK_PPS_PULSE_WIDTH_NS);
	} else {
		zassert_equal(config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
		zassert_equal(config.pulse_width_ns, 0);
	}

	fake_init(&fake);
	fake.caps[1] = fake.caps[0];
	fake.channel_count = ARRAY_SIZE(fake.caps);

	/* The instance keeps a copy and ignores an unused exact pulse width. */
	config.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT;
	config.pulse_width_ns = 123;
	config.poll_interval_ms = 17;
	config.callback = test_callback;
	config.user_data = &rec_a;
	zassert_ok(precision_clock_pps_init(&pps, &fake.clock, &config));
	track_pps_instance(&pps);
	zassert_equal(config.pulse_width_ns, 123);
	config.channel = fake.channel_count;
	config.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_EXACT;
	config.callback = NULL;
	config.user_data = NULL;
	zassert_ok(precision_clock_pps_start(&pps));
	wait_callback(&rec_a);
	zassert_equal(fake.start_channel, CONFIG_PRECISION_CLOCK_PPS_CHANNEL);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(rec_a.state.config.width_policy,
		      PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	zassert_equal(rec_a.state.config.pulse_width_ns, 0);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_init_rejects_invalid_arguments)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_config bad;
	struct precision_clock_pps_state state;

	fake_init(&fake);
	zassert_equal(precision_clock_pps_init(NULL, &fake.clock, &config), -EINVAL);
	zassert_equal(precision_clock_pps_init(&pps, NULL, &config), -EINVAL);
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, NULL), -EINVAL);

	bad = config;
	bad.width_policy = (enum precision_clock_output_width_policy)5;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &bad), -EINVAL);

	bad = exact_config();
	bad.pulse_width_ns = NSEC_PER_SEC;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &bad), -EINVAL);

	bad.pulse_width_ns = 0;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &bad), -EINVAL);

	bad = config;
	bad.start_guard_ns = -1;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &bad), -EINVAL);

	bad = config;
	bad.step_limit_ns = -1;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &bad), -EINVAL);

	bad = config;
	bad.poll_interval_ms = 0;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &bad), -EINVAL);
	zassert_equal(atomic_get(&fake.caps_calls), 0, "invalid requests must not query the clock");

	zassert_equal(precision_clock_pps_state_get(NULL, &state), -EINVAL);
	zassert_equal(precision_clock_pps_start(NULL), -EINVAL);
	zassert_equal(precision_clock_pps_stop(NULL), -EINVAL);

	init_instance(&pps, &fake, config, NULL);
	zassert_equal(precision_clock_pps_state_get(&pps, NULL), -EINVAL);
	zassert_ok(precision_clock_pps_state_get(&pps, &state));
	zassert_equal(state.phase, PRECISION_CLOCK_PPS_STOPPED);
	zassert_equal(state.events, PRECISION_CLOCK_PPS_EVENT_NONE);
	zassert_equal(precision_clock_pps_stop(&pps), -EALREADY);
	zassert_equal(atomic_get(&fake.caps_calls), 1);
	zassert_equal(atomic_get(&fake.status_calls), 0);
	zassert_equal(atomic_get(&fake.stop_calls), 0);
}

ZTEST(precision_timing_clock_pps, test_poll_interval_fits_finite_timeout)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	uint64_t max_ms = k_ticks_to_ms_floor64(UINT32_MAX - 1U);

	fake_init(&fake);
	config.poll_interval_ms = UINT32_MAX;
	if (!IS_ENABLED(CONFIG_TIMEOUT_64BIT) && max_ms < UINT32_MAX) {
		config.poll_interval_ms = (uint32_t)max_ms + 1U;
		zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &config), -ERANGE);
		config.poll_interval_ms = (uint32_t)max_ms;
	}
	init_instance(&pps, &fake, config, NULL);
}

ZTEST(precision_timing_clock_pps, test_init_validates_caps_against_policy)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_config exact = exact_config();

	fake_init(&fake);
	fake.caps[0].flags = 0;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &config), -ENOTSUP);

	fake_init(&fake);
	fake.caps[0].resolution_ns = 3;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &config), -ERANGE);

	/* Exact width demands programmable width support. */
	fake_init(&fake);
	fake.caps[0].flags = PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &exact), -ENOTSUP);

	fake_init(&fake);
	fake.caps[0].max_pulse_width_ns = exact.pulse_width_ns - 1;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &exact), -ERANGE);

	fake_init(&fake);
	fake.caps[0].min_period_ns = 2LL * NSEC_PER_SEC;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &config), -ERANGE);

	fake_init(&fake);
	fake.caps[0].resolution_ns = -1;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &config), -EINVAL);

	fake_init(&fake);
	fake.caps[0].min_lead_time_ns = INT64_MAX;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &config), -ERANGE);

	fake_init(&fake);
	fake.caps_error = -EIO;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &config), -EIO);
}

ZTEST(precision_timing_clock_pps, test_init_rejects_unavailable_channel)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	config.channel = 1;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &config), -ENOTSUP);
	zassert_equal(atomic_get(&fake.caps_calls), 1);
	zassert_equal(atomic_get(&fake.start_calls), 0);
	zassert_equal(atomic_get(&fake.stop_calls), 0);
	zassert_equal(atomic_get(&fake.status_calls), 0);
}

ZTEST(precision_timing_clock_pps, test_init_rejects_zero_output_channels)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	fake.channel_count = 0;
	zassert_equal(precision_clock_pps_init(&pps, &fake.clock, &config), -ENOTSUP);
	zassert_equal(atomic_get(&fake.caps_calls), 1);
	zassert_equal(atomic_get(&fake.start_calls), 0);
	zassert_equal(atomic_get(&fake.stop_calls), 0);
	zassert_equal(atomic_get(&fake.status_calls), 0);
}

ZTEST(precision_timing_clock_pps, test_start_rejects_double_start)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_equal(precision_clock_pps_start(&pps), -EALREADY);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_initial_arm_stops_then_starts_fresh_output)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(rec_a.state.config.period_ns, (precision_time_t)NSEC_PER_SEC);
	zassert_equal(rec_a.state.config.width_policy,
		      PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	zassert_equal(rec_a.state.config.first_rising_time % (precision_time_t)NSEC_PER_SEC, 0);
	zassert_equal(atomic_get(&fake.start_calls), 1);
	zassert_equal(atomic_get(&fake.stop_calls), 1);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_stop_crosses_target_boundary)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	/* Model PHC time advancing while the synchronous provider stop blocks. */
	fake.stop_advance_ns = 2 * (precision_time_t)NSEC_PER_SEC;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_true(rec_a.state.config.first_rising_time > fake_monotonic_ns() + fake.offset_ns);
	zassert_equal(atomic_get(&fake.start_calls), 1);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_read_failure_after_stop_retries)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	fake.read_error_after_stop = -EIO;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_READ_ERROR) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ARMING);
	zassert_equal(atomic_get(&fake.start_calls), 0);
	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.read_error_after_stop = 0;
	fake.read_error = 0;
	k_mutex_unlock(&fake_lock);
	wait_callback(&rec_a);
	zassert_equal(rec_a.state.events,
		      PRECISION_CLOCK_PPS_EVENT_READ_RECOVERED | PRECISION_CLOCK_PPS_EVENT_ARMED);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_exact_width_is_programmed_and_reported)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = exact_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);
	zassert_equal(rec_a.state.config.pulse_width_ns, config.pulse_width_ns);

	k_mutex_lock(&fake_lock, K_FOREVER);
	zassert_equal(fake.status.config.waveform.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);
	zassert_equal(fake.status.config.waveform.pulse_width_ns, config.pulse_width_ns);
	k_mutex_unlock(&fake_lock);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_adopts_already_active_matching_output)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	fake.provider_configured = true;
	fake.status.configured = true;
	fake.status.config.kind = PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM;
	fake.status.config.waveform = (struct precision_clock_output_waveform_config){
		.first_rising_time = 0,
		.period_ns = NSEC_PER_SEC,
		.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT,
		.pulse_width_ns = 0,
	};

	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_ARMED);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 0);
	zassert_equal(atomic_get(&fake.stop_calls), 0);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_configured_only_status_stays_active)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	atomic_val_t calls_before;

	/* Default caps do not advertise hardware-active: validity stays false. */
	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_false(fake.status.hardware_active_valid);
	zassert_false(fake.status.hardware_active, "invalid false state must be ignored");
	calls_before = atomic_get(&rec_a.calls);
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 4);
	verify_phase(&pps, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&rec_a.calls), calls_before);
	zassert_equal(atomic_get(&fake.start_calls), 1);
	zassert_equal(atomic_get(&fake.stop_calls), 1);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_hardware_active_stays_healthy_before_and_after_first_edge)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	precision_time_t first_edge;
	precision_time_t now;
	atomic_val_t stop_calls;

	fake_init(&fake);
	fake.caps[0].flags |= PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE;
	config.start_guard_ns = 2LL * NSEC_PER_SEC;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_true(fake.status.configured);
	zassert_true(fake.status.hardware_active_valid);
	zassert_true(fake.status.hardware_active);
	first_edge = fake.status.config.waveform.first_rising_time;
	stop_calls = atomic_get(&fake.stop_calls);

	/* The generator is armed, but no edge has occurred during these polls. */
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 4);
	zassert_ok(precision_clock_read(&fake.clock, &now));
	zassert_true(now < first_edge);
	verify_phase(&pps, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 1);
	zassert_equal(atomic_get(&fake.stop_calls), stop_calls);

	/* Running the same waveform past its first edge must also stay healthy. */
	k_msleep(DIV_ROUND_UP(first_edge - now, NSEC_PER_MSEC));
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 4);
	zassert_ok(precision_clock_read(&fake.clock, &now));
	zassert_true(now > first_edge);
	verify_phase(&pps, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 1);
	zassert_equal(atomic_get(&fake.stop_calls), stop_calls);
	zassert_equal(atomic_get(&rec_a.calls), 1);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_hardware_active_false_recovers)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	fake.caps[0].flags |= PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	k_mutex_lock(&fake_lock, K_FOREVER);
	zassert_true(fake.status.hardware_active_valid);
	zassert_true(fake.status.hardware_active);
	k_mutex_unlock(&fake_lock);

	set_hardware_active(&fake, false);
	wait_callback(&rec_a);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_OUTPUT_INACTIVE) != 0);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_RECOVERED) != 0);
	zassert_false((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 2);
	zassert_equal(atomic_get(&fake.stop_calls), 2);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_normal_drift_does_not_rearm)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	bump_offset(&fake, 1000);
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 2);
	verify_phase(&pps, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 1);
	zassert_equal(atomic_get(&fake.stop_calls), 1);
	zassert_equal(atomic_get(&rec_a.calls), 1, "a bounded drift must not produce an event");
	zassert_ok(precision_clock_pps_stop(&pps));
}

static void check_slow_read(bool sample_before_delay, bool after_stop)
{
	static struct precision_clock_pps pps;
	static struct k_sem read_entered;
	static struct k_sem read_release;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	k_sem_init(&read_entered, 0, 1);
	k_sem_init(&read_release, 0, 1);

	if (after_stop) {
		fake.sample_before_delay = sample_before_delay;
		fake.read_delay_after_stop_ms = 150;
		fake.align_delayed_sample = sample_before_delay;
	}
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	if (after_stop && sample_before_delay) {
		zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_START_ERROR);
		zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ARMING);
		wait_callback(&rec_a);
	}
	zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_ARMED);

	if (!after_stop) {
		/* Park a normal poll to ensure the injected delay has been consumed. */
		k_mutex_lock(&fake_lock, K_FOREVER);
		fake.sample_before_delay = sample_before_delay;
		fake.read_delay_ms = 150;
		fake.read_entered = &read_entered;
		fake.read_release = &read_release;
		k_mutex_unlock(&fake_lock);
		zassert_ok(k_sem_take(&read_entered, CALLBACK_TIMEOUT));
		k_sem_give(&read_release);
	}

	/* Exercise the delayed read and the following fast poll's uncertainty. */
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 2);
	verify_phase(&pps, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 1);
	zassert_ok(precision_clock_pps_stop(&pps));
	zassert_false((atomic_get(&rec_a.observed_events) & PRECISION_CLOCK_PPS_EVENT_HARD_STEP) !=
			      0,
		      "read latency must not emit a hard-step event");
}

ZTEST(precision_timing_clock_pps, test_read_delay_before_sampling_preserves_output)
{
	check_slow_read(false, false);
}

ZTEST(precision_timing_clock_pps, test_read_delay_after_sampling_preserves_output)
{
	check_slow_read(true, false);
}

ZTEST(precision_timing_clock_pps, test_post_stop_read_delay_preserves_output)
{
	check_slow_read(true, true);
}

static void check_step_during_slow_read(precision_time_t step_ns)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.read_delay_ms = 150;
	fake.offset_ns += step_ns;
	k_mutex_unlock(&fake_lock);
	wait_callback(&rec_a);

	zassert_equal(rec_a.state.events,
		      PRECISION_CLOCK_PPS_EVENT_HARD_STEP | PRECISION_CLOCK_PPS_EVENT_RECOVERED);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 2);
	verify_phase(&pps, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 2, "a real step must cause exactly one rearm");
	zassert_equal(atomic_get(&fake.stop_calls), 2);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_forward_step_outside_read_uncertainty_rearms)
{
	check_step_during_slow_read(400LL * NSEC_PER_MSEC);
}

ZTEST(precision_timing_clock_pps, test_backward_step_outside_read_uncertainty_rearms)
{
	check_step_during_slow_read(-400LL * NSEC_PER_MSEC);
}

ZTEST(precision_timing_clock_pps, test_forward_hard_step_rearms)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_HARD_STEP) != 0);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_RECOVERED) != 0);
	zassert_false((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 2);
	zassert_equal(atomic_get(&fake.stop_calls), 2);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_backward_hard_step_rearms)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	bump_offset(&fake, -150LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_HARD_STEP) != 0);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_RECOVERED) != 0);
	zassert_false((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 2);
	zassert_equal(atomic_get(&fake.stop_calls), 2);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_inactive_output_recovers)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	set_configured(&fake, false);
	wait_callback(&rec_a);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_OUTPUT_INACTIVE) != 0);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_RECOVERED) != 0);
	zassert_false((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 2);
	zassert_equal(atomic_get(&fake.stop_calls), 2);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_mismatched_output_recovers)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	mismatch_active_output(&fake);
	wait_callback(&rec_a);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_OUTPUT_INACTIVE) != 0);
	zassert_false((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_RECOVERED) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 2);
	zassert_equal(atomic_get(&fake.stop_calls), 2);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_read_and_status_errors_preserve_output)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	atomic_val_t stop_calls_before;
	atomic_val_t start_calls_before;

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	stop_calls_before = atomic_get(&fake.stop_calls);
	start_calls_before = atomic_get(&fake.start_calls);
	set_read_error(&fake, -EIO);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_READ_ERROR) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.stop_calls), stop_calls_before);

	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.read_error = 0;
	fake.status_error = -EAGAIN;
	k_mutex_unlock(&fake_lock);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_READ_RECOVERED) != 0);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_STATUS_ERROR) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.stop_calls), stop_calls_before);
	zassert_equal(atomic_get(&fake.start_calls), start_calls_before);

	set_status_error(&fake, 0);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_STATUS_RECOVERED) != 0);
	zassert_equal(atomic_get(&fake.stop_calls), stop_calls_before);
	zassert_equal(atomic_get(&fake.start_calls), start_calls_before);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_callback_reports_errors_from_same_poll)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	rec_a.block_callback = true;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_parked_callback(&rec_a);

	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.read_error = -EIO;
	fake.status_error = -EAGAIN;
	k_mutex_unlock(&fake_lock);
	k_sem_give(&rec_a.callback_release);
	wait_callback(&rec_a);
	wait_callback(&rec_a);
	zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_READ_ERROR |
						  PRECISION_CLOCK_PPS_EVENT_STATUS_ERROR);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_persistent_status_errors_stop_and_rearm)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	atomic_val_t stop_calls_before;
	atomic_val_t start_calls_before;

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	stop_calls_before = atomic_get(&fake.stop_calls);
	start_calls_before = atomic_get(&fake.start_calls);
	set_status_error(&fake, -EIO);

	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_STATUS_ERROR) != 0);
	zassert_equal(atomic_get(&fake.stop_calls), stop_calls_before);
	zassert_equal(atomic_get(&fake.start_calls), start_calls_before);

	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_RECOVERED) != 0);
	zassert_false((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.stop_calls), stop_calls_before + 1);
	zassert_equal(atomic_get(&fake.start_calls), start_calls_before + 1);

	/* Further failed status queries alone must leave that waveform running. */
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 3);
	verify_phase(&pps, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.stop_calls), stop_calls_before + 1);
	zassert_equal(atomic_get(&fake.start_calls), start_calls_before + 1);

	/* A separate clock step must still rearm while status remains unavailable. */
	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_HARD_STEP) != 0);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_RECOVERED) != 0);
	zassert_equal(atomic_get(&fake.start_calls), start_calls_before + 2);

	set_status_error(&fake, 0);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_STATUS_RECOVERED) != 0);

	/* A new outage gets a new recovery attempt; a failed start is retried. */
	set_start_error(&fake, -EIO);
	set_status_error(&fake, -EAGAIN);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_STATUS_ERROR) != 0);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_START_ERROR) != 0);
	set_start_error(&fake, 0);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_RECOVERED) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), start_calls_before + 4);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_start_error_retries_next_poll)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	fake.start_error = -ETIME;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_START_ERROR) != 0);
	zassert_equal(atomic_get(&fake.start_calls), 1);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ARMING);

	set_start_error(&fake, 0);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_equal(atomic_get(&fake.start_calls), 2);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);

	zassert_ok(precision_clock_pps_stop(&pps));
}

static void check_failed_start_cleanup(bool cleanup_fails)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_output_waveform_config retained;
	atomic_val_t stop_calls;

	fake_init(&fake);
	fake.start_error = -EINPROGRESS;
	rec_a.block_callback = true;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_parked_callback(&rec_a);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_START_ERROR) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ARMING);
	k_mutex_lock(&fake_lock, K_FOREVER);
	retained = fake.status.config.waveform;
	stop_calls = atomic_get(&fake.stop_calls);
	k_mutex_unlock(&fake_lock);
	zassert_true(fake.provider_configured);
	zassert_true(fake.status.configured);
	zassert_equal(atomic_get(&fake.start_calls), 1);

	set_start_error(&fake, 0);
	if (cleanup_fails) {
		set_stop_error(&fake, -EBUSY);
		rec_a.block_callback = true;
		k_sem_give(&rec_a.callback_release);
		wait_callback(&rec_a);
		wait_parked_callback(&rec_a);

		/* A failed cleanup still owns the failed start's exact waveform. */
		zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_STOP_ERROR) != 0);
		zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ARMING);

		zassert_true(fake.provider_configured);
		zassert_true(fake.status.configured);
		zassert_equal(fake.status.config.waveform.first_rising_time,
			      retained.first_rising_time);
		zassert_equal(atomic_get(&fake.start_calls), 1,
			      "owned channel restarted before cleanup");
		zassert_equal(atomic_get(&fake.stop_calls), stop_calls + 1);
		zassert_equal(precision_clock_pps_start(&pps), -EALREADY);
		stop_calls++;
		set_stop_error(&fake, 0);
	}

	rec_a.block_callback = true;
	k_sem_give(&rec_a.callback_release);
	wait_callback(&rec_a);
	wait_parked_callback(&rec_a);

	/* The provider rejects reuse while owned, so arming proves cleanup won. */
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_false((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_START_ERROR) != 0);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.stop_calls), stop_calls + 1);
	zassert_equal(atomic_get(&fake.start_calls), 2);
	zassert_true(fake.provider_configured);

	k_sem_give(&rec_a.callback_release);
	wait_callback(&rec_a);
	zassert_ok(precision_clock_pps_stop(&pps));
	zassert_false(fake.provider_configured);
}

ZTEST(precision_timing_clock_pps, test_failed_start_retains_output_until_cleanup)
{
	check_failed_start_cleanup(false);
}

ZTEST(precision_timing_clock_pps, test_failed_start_cleanup_failure_prevents_restart)
{
	check_failed_start_cleanup(true);
}

ZTEST(precision_timing_clock_pps, test_stop_error_retries_before_start)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	fake.stop_error = -EIO;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_STOP_ERROR) != 0);
	zassert_equal(atomic_get(&fake.stop_calls), 1);
	zassert_equal(atomic_get(&fake.start_calls), 0);

	set_stop_error(&fake, 0);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_equal(atomic_get(&fake.stop_calls), 2);
	zassert_equal(atomic_get(&fake.start_calls), 1);

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_callback_runs_on_dedicated_queue)
{
	static struct precision_clock_pps pps;
	extern struct k_work_q k_sys_work_q;
	struct precision_clock_pps_config config = default_config();

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_not_null(rec_a.thread);
	zassert_not_equal(rec_a.thread, k_current_get(), "callback ran on the caller thread");
	zassert_not_equal(rec_a.thread, k_sys_work_q.thread_id,
			  "callback ran on the system workqueue");

	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_callback_state_get_keeps_most_recent_notification)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_state state;
	uint32_t recovered =
		PRECISION_CLOCK_PPS_EVENT_HARD_STEP | PRECISION_CLOCK_PPS_EVENT_RECOVERED;

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_ARMED);

	/* Reading or handling an uneventful poll never clears the notification. */
	zassert_ok(precision_clock_pps_state_get(&pps, &state));
	zassert_equal(state.events, rec_a.state.events);
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 2);
	zassert_ok(precision_clock_pps_state_get(&pps, &state));
	zassert_equal(state.events, PRECISION_CLOCK_PPS_EVENT_ARMED);
	zassert_equal(state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(state.config.first_rising_time, rec_a.state.config.first_rising_time);
	zassert_equal(atomic_get(&rec_a.calls), 1);

	/* The next mask replaces the previous one; it isn't ORed with ARMED. */
	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);
	zassert_equal(rec_a.state.events, recovered);
	zassert_ok(precision_clock_pps_state_get(&pps, &state));
	zassert_equal(state.events, recovered);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_polling_accumulates_and_consumes_events)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_state state;

	fake_init(&fake);
	start_instance(&pps, &fake, &config, NULL);
	wait_provider_calls(&fake.start_calls, 1);
	set_read_error(&fake, -EIO);
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 2);
	set_read_error(&fake, 0);
	wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 2);
	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_provider_calls(&fake.start_calls, 2);

	zassert_ok(precision_clock_pps_state_get(&pps, &state));
	zassert_equal(state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(state.events, PRECISION_CLOCK_PPS_EVENT_ARMED |
					    PRECISION_CLOCK_PPS_EVENT_READ_ERROR |
					    PRECISION_CLOCK_PPS_EVENT_READ_RECOVERED |
					    PRECISION_CLOCK_PPS_EVENT_HARD_STEP |
					    PRECISION_CLOCK_PPS_EVENT_RECOVERED);
	zassert_ok(precision_clock_pps_state_get(&pps, &state));
	zassert_equal(state.events, PRECISION_CLOCK_PPS_EVENT_NONE);
	zassert_ok(precision_clock_pps_stop(&pps));
}

static void check_restart(struct precision_clock_pps *pps, struct precision_clock_pps *blocker,
			  struct cb_record *rec)
{
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_state state;

	fake_init(&fake);
	fake_init(&fake_other);
	init_instance(pps, &fake, config, rec);
	zassert_equal(atomic_get(&fake.caps_calls), 1);

	zassert_ok(precision_clock_pps_start(pps));
	if (rec != NULL) {
		wait_callback(rec);
	} else {
		wait_provider_calls(&fake.start_calls, 1);
	}
	zassert_equal(atomic_get(&fake.caps_calls), 2, "generic start validates once");
	set_read_error(&fake, -EIO);
	if (rec != NULL) {
		wait_callback(rec);
		zassert_equal(rec->state.events, PRECISION_CLOCK_PPS_EVENT_READ_ERROR);
	} else {
		wait_provider_calls(&fake.status_calls, atomic_get(&fake.status_calls) + 2);
	}
	zassert_ok(precision_clock_pps_stop(pps));

	/* Park the queue so we observe restart before the first worker poll. */
	rec_b.block_callback = true;
	start_instance(blocker, &fake_other, &config, &rec_b);
	wait_parked_callback(&rec_b);
	zassert_ok(precision_clock_pps_start(pps));
	zassert_equal(atomic_get(&fake.caps_calls), 2, "PPS restart must not query caps");
	zassert_ok(precision_clock_pps_state_get(pps, &state));
	zassert_equal(state.phase, PRECISION_CLOCK_PPS_STARTING);
	zassert_equal(state.events, PRECISION_CLOCK_PPS_EVENT_NONE, "old events survived restart");
	k_sem_give(&rec_b.callback_release);
	wait_callback(&rec_b);

	wait_provider_calls(&fake.start_calls, 2);
	zassert_equal(atomic_get(&fake.caps_calls), 3, "only generic start may query again");
	zassert_ok(precision_clock_pps_state_get(pps, &state));
	zassert_equal(state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(state.events, PRECISION_CLOCK_PPS_EVENT_ARMED);
	zassert_ok(precision_clock_pps_stop(pps));
	zassert_ok(precision_clock_pps_stop(blocker));
}
ZTEST(precision_timing_clock_pps, test_restart_clears_polled_events_and_reuses_caps)
{
	static struct precision_clock_pps pps;
	static struct precision_clock_pps blocker;

	check_restart(&pps, &blocker, NULL);
}

ZTEST(precision_timing_clock_pps, test_restart_clears_callback_events_and_reuses_caps)
{
	static struct precision_clock_pps pps;
	static struct precision_clock_pps blocker;

	check_restart(&pps, &blocker, &rec_a);
}

ZTEST(precision_timing_clock_pps, test_stop_stops_output_and_blocks_further_calls)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	atomic_val_t status_calls;

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_ok(precision_clock_pps_stop(&pps));
	verify_phase(&pps, PRECISION_CLOCK_PPS_STOPPED);
	zassert_equal(atomic_get(&fake.stop_calls), 2);
	zassert_false(fake.provider_configured);
	zassert_equal(precision_clock_pps_stop(&pps), -EALREADY);

	/* Advancing time after stop must not produce a poll or a callback. */
	status_calls = atomic_get(&fake.status_calls);
	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	zassert_equal(k_sem_take(&rec_a.sem, K_MSEC(2 * config.poll_interval_ms)), -EAGAIN);
	zassert_equal(atomic_get(&fake.status_calls), status_calls);
	zassert_equal(atomic_get(&rec_a.calls), 1);

	/* A stopped instance restarts without reinitialization. */
	zassert_ok(precision_clock_pps_start(&pps));
	wait_callback(&rec_a);
	zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_ARMED);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_stop_failure_restores_active_and_rearming)
{
	static struct precision_clock_pps pps;
	static struct stop_thread_ctx ctx;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_state state;

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	set_stop_error(&fake, -EIO);
	zassert_equal(precision_clock_pps_stop(&pps), -EIO);
	verify_phase(&pps, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_true(fake.provider_configured);

	/* The failed stop did not suspend polling or forget the previous arm. */
	rec_a.block_callback = true;
	bump_offset(&fake, 500LL * NSEC_PER_MSEC);
	wait_parked_callback(&rec_a);
	zassert_equal(rec_a.state.events,
		      PRECISION_CLOCK_PPS_EVENT_HARD_STEP | PRECISION_CLOCK_PPS_EVENT_STOP_ERROR);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_REARMING);

	/* A stop failure must also restore REARMING, without adopting the waveform. */
	ctx = (struct stop_thread_ctx){.instance = &pps};
	start_stop_thread(&ctx);
	zassert_true(WAIT_FOR(precision_clock_pps_state_get(&pps, &state) == 0 &&
				      state.phase == PRECISION_CLOCK_PPS_STOPPING,
			      WAIT_STATE_TIMEOUT_US, k_msleep(1)));
	rec_a.block_callback = true;
	k_sem_give(&rec_a.callback_release);
	wait_callback(&rec_a);
	join_stop_thread();
	zassert_equal(ctx.result, -EIO);
	wait_parked_callback(&rec_a);
	zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_STOP_ERROR);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_REARMING);
	zassert_equal(atomic_get(&fake.start_calls), 1);
	zassert_true(fake.provider_configured);

	set_stop_error(&fake, 0);
	k_sem_give(&rec_a.callback_release);
	wait_callback(&rec_a);
	wait_callback(&rec_a);
	zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_RECOVERED);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(atomic_get(&fake.start_calls), 2);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_self_stop_from_callback_returns_deadlock)
{
	static struct precision_clock_pps pps;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_state state;

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	rec_a.attempt_self_stop = true;
	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);

	zassert_equal(rec_a.self_stop_result, -EDEADLK);

	/* The rejected recursive call must not have torn down the instance. */
	zassert_ok(precision_clock_pps_state_get(&pps, &state));
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_two_instances_run_and_stop_independently)
{
	static struct precision_clock_pps pps;
	static struct precision_clock_pps pps_other;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_config exact = exact_config();
	struct precision_clock_pps_state state;

	fake_init(&fake);
	fake_init(&fake_other);

	start_instance(&pps, &fake, &config, &rec_a);
	start_instance(&pps_other, &fake_other, &exact, &rec_b);
	wait_callback(&rec_a);
	wait_callback(&rec_b);

	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_true((rec_b.state.events & PRECISION_CLOCK_PPS_EVENT_ARMED) != 0);
	zassert_ok(precision_clock_pps_state_get(&pps, &state));
	zassert_equal(state.config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	zassert_ok(precision_clock_pps_state_get(&pps_other, &state));
	zassert_equal(state.config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);

	/* Stopping the first instance leaves the second running. */
	zassert_ok(precision_clock_pps_stop(&pps));
	verify_phase(&pps, PRECISION_CLOCK_PPS_STOPPED);

	bump_offset(&fake_other, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_b);
	zassert_true((rec_b.state.events & PRECISION_CLOCK_PPS_EVENT_HARD_STEP) != 0);
	zassert_ok(precision_clock_pps_state_get(&pps_other, &state));
	zassert_equal(state.phase, PRECISION_CLOCK_PPS_ACTIVE);

	zassert_ok(precision_clock_pps_stop(&pps_other));
}

ZTEST(precision_timing_clock_pps, test_callback_stops_other_instance_on_shared_queue)
{
	static struct precision_clock_pps pps;
	static struct precision_clock_pps pps_other;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_config exact = exact_config();
	struct precision_clock_pps_state state;
	atomic_val_t other_calls;
	atomic_val_t other_status_calls;

	fake_init(&fake);
	fake_init(&fake_other);
	start_instance(&pps_other, &fake_other, &exact, &rec_b);
	wait_callback(&rec_b);
	other_calls = atomic_get(&rec_b.calls);

	rec_a.block_callback = true;
	rec_a.stop_target = &pps_other;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_parked_callback(&rec_a);
	other_status_calls = atomic_get(&fake_other.status_calls);

	/*
	 * The shared queue is parked in A's callback. Make B need recovery and
	 * wait until its poll is queued, so stopping B must cancel queued work
	 * without waiting for the same queue thread to execute it.
	 */
	bump_offset(&fake_other, 400LL * NSEC_PER_MSEC);
	k_msleep(2 * exact.poll_interval_ms);
	zassert_equal(atomic_get(&fake_other.status_calls), other_status_calls);
	k_sem_give(&rec_a.callback_release);
	wait_callback(&rec_a);
	zassert_ok(rec_a.other_stop_result);
	verify_phase(&pps_other, PRECISION_CLOCK_PPS_STOPPED);
	zassert_false(fake_other.provider_configured);
	zassert_equal(atomic_get(&rec_b.calls), other_calls);
	zassert_equal(k_sem_take(&rec_b.sem, K_NO_WAIT), -EBUSY);

	zassert_ok(precision_clock_pps_state_get(&pps, &state));
	zassert_equal(state.phase, PRECISION_CLOCK_PPS_ACTIVE);
	zassert_equal(state.config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);
	zassert_true((rec_a.state.events & PRECISION_CLOCK_PPS_EVENT_HARD_STEP) != 0);
	zassert_equal(atomic_get(&rec_b.calls), other_calls, "stopped B received another callback");
	zassert_equal(atomic_get(&fake_other.status_calls), other_status_calls);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_stop_waits_for_running_callback)
{
	static struct precision_clock_pps pps;
	static struct stop_thread_ctx ctx;
	struct precision_clock_pps_config config = default_config();
	struct precision_clock_pps_state state;

	ctx = (struct stop_thread_ctx){.instance = &pps, .rec = &rec_a};
	fake_init(&fake);
	rec_a.block_callback = true;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_parked_callback(&rec_a);
	zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_ARMED);
	zassert_equal(rec_a.state.phase, PRECISION_CLOCK_PPS_ACTIVE);

	start_stop_thread(&ctx);
	zassert_true(WAIT_FOR(precision_clock_pps_state_get(&pps, &state) == 0 &&
				      state.phase == PRECISION_CLOCK_PPS_STOPPING,
			      WAIT_STATE_TIMEOUT_US, k_msleep(1)));
	zassert_false(atomic_get(&ctx.done), "stop returned while callback owned user data");
	zassert_equal(atomic_get(&rec_a.completed_calls), 0);
	zassert_equal(state.events, rec_a.state.events);
	zassert_equal(precision_clock_pps_stop(&pps), -EBUSY);

	k_sem_give(&rec_a.callback_release);
	join_stop_thread();
	wait_callback(&rec_a);
	zassert_ok(ctx.result);
	zassert_equal(ctx.completed_calls, 1,
		      "stop returned before the callback's final user-data access");
	zassert_equal(rec_a.state_after_release.phase, PRECISION_CLOCK_PPS_STOPPING);
	zassert_equal(rec_a.state_after_release.events, rec_a.state.events);
	verify_phase(&pps, PRECISION_CLOCK_PPS_STOPPED);
	zassert_false(fake.provider_configured);

	/* Successful stop permits reuse of the same instance and its callback context. */
	cb_record_reset(&rec_a);
	zassert_ok(precision_clock_pps_start(&pps));
	wait_callback(&rec_a);
	zassert_equal(rec_a.state.events, PRECISION_CLOCK_PPS_EVENT_ARMED);
	zassert_ok(precision_clock_pps_stop(&pps));
}

ZTEST(precision_timing_clock_pps, test_stop_waits_for_in_flight_poll)
{
	static struct precision_clock_pps pps;
	static struct stop_thread_ctx ctx;
	static struct k_sem read_entered;
	static struct k_sem read_release;
	struct precision_clock_pps_config config = default_config();

	ctx = (struct stop_thread_ctx){.instance = &pps};

	k_sem_init(&read_entered, 0, 1);
	k_sem_init(&read_release, 0, 1);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.read_entered = &read_entered;
	fake.read_release = &read_release;
	k_mutex_unlock(&fake_lock);
	zassert_ok(k_sem_take(&read_entered, K_SECONDS(2)));

	start_stop_thread(&ctx);

	zassert_true(WAIT_FOR(atomic_get(&ctx.started) != 0, 1000000, k_msleep(1)));
	k_msleep(20);
	zassert_false(atomic_get(&ctx.done),
		      "stop returned while the in-flight poll was still blocked");

	k_sem_give(&read_release);
	join_stop_thread();
	zassert_ok(ctx.result);

	zassert_equal(k_sem_take(&rec_a.sem, K_NO_WAIT), -EBUSY);
	verify_phase(&pps, PRECISION_CLOCK_PPS_STOPPED);
}

ZTEST(precision_timing_clock_pps, test_stop_other_instance_while_poll_is_blocked)
{
	static struct precision_clock_pps pps;
	static struct precision_clock_pps pps_other;
	static struct stop_thread_ctx ctx;
	static struct k_sem read_entered;
	static struct k_sem read_release;
	struct precision_clock_pps_config config = default_config();

	ctx = (struct stop_thread_ctx){.instance = &pps};

	k_sem_init(&read_entered, 0, 1);
	k_sem_init(&read_release, 0, 1);

	fake_init(&fake);
	fake_init(&fake_other);
	start_instance(&pps, &fake, &config, &rec_a);
	start_instance(&pps_other, &fake_other, &config, &rec_b);
	wait_callback(&rec_a);
	wait_callback(&rec_b);

	/* Park the first instance's poll in its clock read. */
	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.read_entered = &read_entered;
	fake.read_release = &read_release;
	k_mutex_unlock(&fake_lock);
	zassert_ok(k_sem_take(&read_entered, K_SECONDS(2)));

	/* Stopping the parked instance must wait for its own poll to finish. */
	start_stop_thread(&ctx);
	zassert_true(WAIT_FOR(atomic_get(&ctx.started) != 0, 1000000, k_msleep(1)));
	k_msleep(20);
	zassert_false(atomic_get(&ctx.done), "stop of the parked instance returned early");

	/* The other instance is cancelled independently without waiting. */
	zassert_ok(precision_clock_pps_stop(&pps_other));
	verify_phase(&pps_other, PRECISION_CLOCK_PPS_STOPPED);

	/* Release the parked poll and let its stop complete. */
	k_sem_give(&read_release);
	join_stop_thread();
	zassert_ok(ctx.result);
	verify_phase(&pps, PRECISION_CLOCK_PPS_STOPPED);
}

ZTEST_SUITE(precision_timing_clock_pps, NULL, NULL, before_each, after_each, NULL);
