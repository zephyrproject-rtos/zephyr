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
#include <zephyr/precision_timing/precision_pps_output.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define CALLBACK_TIMEOUT      K_MSEC(2000)
#define WAIT_STATE_TIMEOUT_US 2000000

/* Fake protocol-neutral clock and output-channel provider, guarded by fake_lock. */
struct fake_clock {
	struct precision_clock clock;
	struct precision_clock_output_caps caps;
	struct precision_clock_output_status status;
	precision_time_t offset_ns;
	precision_time_t stop_advance_ns;
	int read_error;
	int read_error_after_stop;
	int caps_error;
	int start_error;
	int stop_error;
	int status_error;
	uint32_t caps_calls;
	uint32_t start_calls;
	uint32_t stop_calls;
	uint32_t status_calls;
	uint32_t read_delay_ms;
	uint32_t read_delay_after_stop_ms;
	bool sample_before_delay;
	bool align_delayed_sample;
	bool provider_configured;
	bool retain_config_on_start_error;
	bool clear_status_error_on_stop;
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
	struct precision_pps_output_state state;
	k_tid_t thread;
	uint32_t events;
	uint32_t calls;
	atomic_t observed_events;
	bool attempt_self_stop;
	int self_stop_result;
	struct precision_pps_output *stop_target;
	int other_stop_result;
	struct k_sem callback_entered;
	struct k_sem callback_release;
	bool block_callback;
	atomic_t completed_calls;
};

static struct cb_record rec_a;
static struct cb_record rec_b;

struct stop_thread_ctx {
	struct precision_pps_output *instance;
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

#define MAX_TRACKED_PPS_INSTANCES 6

static struct precision_pps_output *tracked_pps_instances[MAX_TRACKED_PPS_INSTANCES];
static size_t tracked_pps_instance_count;

static void track_pps_instance(struct precision_pps_output *instance)
{
	zassert_true(tracked_pps_instance_count < ARRAY_SIZE(tracked_pps_instances));
	tracked_pps_instances[tracked_pps_instance_count++] = instance;
}

/*
 * Give every test distinct storage while retaining static lifetime in case a
 * failed assertion leaves delayed work pending for after_each() to cancel.
 */
#define TEST_PPS_INSTANCE(name)                                                                    \
	static struct precision_pps_output name;                                                   \
	track_pps_instance(&name)

static precision_time_t fake_monotonic_ns(void)
{
	return (precision_time_t)k_ticks_to_ns_floor64(k_uptime_ticks());
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
		f->offset_ns = 900LL * NSEC_PER_MSEC -
			       fake_monotonic_ns() % NSEC_PER_SEC;
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

static int fake_get_output_caps(const struct precision_clock *precision_clk, uint32_t channel,
				struct precision_clock_output_caps *caps)
{
	struct fake_clock *f = precision_clk->data;
	int error;

	ARG_UNUSED(channel);

	k_mutex_lock(&fake_lock, K_FOREVER);
	f->caps_calls++;
	error = f->caps_error;
	*caps = f->caps;
	k_mutex_unlock(&fake_lock);

	return error;
}

static void fake_mark_active(struct fake_clock *f,
			     const struct precision_clock_output_waveform_config *config)
{
	f->provider_configured = true;
	f->status.configured = true;
	f->status.kind = PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM;
	f->status.config.waveform = *config;
	f->status.hardware_active_valid =
		(f->caps.flags & PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE) != 0U;
	f->status.hardware_active = f->status.hardware_active_valid;
}

static int fake_output_start_waveform(const struct precision_clock *precision_clk, uint32_t channel,
				      const struct precision_clock_output_waveform_config *config)
{
	struct fake_clock *f = precision_clk->data;
	int error;

	ARG_UNUSED(channel);

	k_mutex_lock(&fake_lock, K_FOREVER);
	f->start_calls++;
	error = f->start_error;
	if (f->provider_configured) {
		error = -EBUSY;
	} else if (error == 0 || f->retain_config_on_start_error) {
		/* A failed start may still own the channel until a successful stop. */
		fake_mark_active(f, config);
	}
	k_mutex_unlock(&fake_lock);

	return error;
}

static int fake_output_stop(const struct precision_clock *precision_clk, uint32_t channel)
{
	struct fake_clock *f = precision_clk->data;
	int error;

	ARG_UNUSED(channel);

	k_mutex_lock(&fake_lock, K_FOREVER);
	f->stop_calls++;
	f->offset_ns += f->stop_advance_ns;
	error = f->stop_error;
	if (error == 0) {
		f->provider_configured = false;
		f->read_error = f->read_error_after_stop;
		f->status.configured = false;
		f->read_delay_ms = f->read_delay_after_stop_ms;
		f->read_delay_after_stop_ms = 0;
		if (f->clear_status_error_on_stop) {
			f->status_error = 0;
		}
	}
	k_mutex_unlock(&fake_lock);

	return error;
}

static int fake_get_output_status(const struct precision_clock *precision_clk, uint32_t channel,
				  struct precision_clock_output_status *status)
{
	struct fake_clock *f = precision_clk->data;
	int error;

	ARG_UNUSED(channel);

	k_mutex_lock(&fake_lock, K_FOREVER);
	f->status_calls++;
	error = f->status_error;
	*status = f->status;
	k_mutex_unlock(&fake_lock);

	return error;
}

static const struct precision_clock_api fake_api = {
	.read = fake_read,
	.get_output_caps = fake_get_output_caps,
	.output_start_waveform = fake_output_start_waveform,
	.output_stop = fake_output_stop,
	.get_output_status = fake_get_output_status,
};

static void fake_init(struct fake_clock *f)
{
	k_mutex_lock(&fake_lock, K_FOREVER);
	memset(f, 0, sizeof(*f));
	f->clock.api = &fake_api;
	f->clock.data = f;
	f->caps = (struct precision_clock_output_caps){
		.flags = PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM |
			 PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH,
		.channel_count = 1,
		.resolution_ns = 1,
		.min_lead_time_ns = NSEC_PER_MSEC,
		.min_period_ns = NSEC_PER_MSEC,
		.max_period_ns = 2LL * NSEC_PER_SEC,
		.min_pulse_width_ns = 1,
		.max_pulse_width_ns = 999999999,
	};
	k_mutex_unlock(&fake_lock);
}

static struct precision_pps_output_config default_config(void)
{
	return (struct precision_pps_output_config){
		.channel = 0,
		.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT,
		.pulse_width_ns = 0,
		.start_guard_ns = 10LL * NSEC_PER_MSEC,
		.step_limit_ns = 100LL * NSEC_PER_MSEC,
		.poll_interval_ms = 20,
	};
}

static struct precision_pps_output_config exact_config(void)
{
	struct precision_pps_output_config config = default_config();

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

static void test_callback(struct precision_pps_output *instance, uint32_t events,
			  const struct precision_pps_output_state *state, void *user_data)
{
	struct cb_record *rec = user_data;

	rec->thread = k_current_get();
	rec->events = events;
	atomic_or(&rec->observed_events, events);
	rec->state = *state;
	rec->calls++;
	if (rec->attempt_self_stop) {
		rec->attempt_self_stop = false;
		rec->self_stop_result = precision_pps_output_stop(instance);
	}
	if (rec->block_callback) {
		rec->block_callback = false;
		k_sem_give(&rec->callback_entered);
		(void)k_sem_take(&rec->callback_release, K_FOREVER);
	}
	if (rec->stop_target != NULL) {
		struct precision_pps_output *target = rec->stop_target;

		rec->stop_target = NULL;
		rec->other_stop_result = precision_pps_output_stop(target);
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
	rec->thread = NULL;
	rec->events = PRECISION_PPS_OUTPUT_EVENT_NONE;
	atomic_clear(&rec->observed_events);
	rec->calls = 0;
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

static void start_instance(struct precision_pps_output *instance, struct fake_clock *f,
			   const struct precision_pps_output_config *config, struct cb_record *rec)
{
	zassert_ok(precision_pps_output_init(instance, &f->clock, config, test_callback, rec));
	zassert_ok(precision_pps_output_start(instance));
}

static bool poll_advanced(struct precision_pps_output *instance, precision_time_t after_phc_ns,
			  struct precision_pps_output_state *out)
{
	return precision_pps_output_state_get(instance, out) == 0 &&
	       out->phc_time_ns > after_phc_ns;
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
	ctx->result = precision_pps_output_stop(ctx->instance);
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
	rec_a.block_callback = false;
	rec_b.block_callback = false;
	rec_a.stop_target = NULL;
	rec_b.stop_target = NULL;
	k_sem_give(&rec_a.callback_release);
	k_sem_give(&rec_b.callback_release);
	release_parked_reads();
	if (stop_thread_active) {
		join_stop_thread();
	}
	for (size_t i = 0; i < tracked_pps_instance_count; i++) {
		if (tracked_pps_instances[i]->started) {
			(void)precision_pps_output_stop(tracked_pps_instances[i]);
		}
	}
}

ZTEST(precision_timing_pps_output, test_kconfig_defaults_allow_instance_overrides)
{
	struct precision_pps_output_config config = PRECISION_PPS_OUTPUT_CONFIG_DEFAULTS;

	TEST_PPS_INSTANCE(pps);
	fake_init(&fake);
	zassert_ok(precision_pps_output_init(&pps, &fake.clock, &config, test_callback, &rec_a));
	zassert_equal(pps.config.channel, CONFIG_PRECISION_PPS_OUTPUT_CHANNEL);
	zassert_equal(pps.config.start_guard_ns, CONFIG_PRECISION_PPS_OUTPUT_START_GUARD_NS);
	zassert_equal(pps.config.poll_interval_ms, CONFIG_PRECISION_PPS_OUTPUT_POLL_INTERVAL_MS);

	/* A caller override wins over the defaults; unused exact width is cleared. */
	config.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT;
	config.pulse_width_ns = 123;
	config.poll_interval_ms = 17;
	zassert_ok(precision_pps_output_init(&pps, &fake.clock, &config, test_callback, &rec_a));
	zassert_equal(pps.config.pulse_width_ns, 0);
	zassert_equal(pps.config.poll_interval_ms, 17);
	zassert_equal(config.pulse_width_ns, 123, "caller configuration must not be mutated");
}

ZTEST(precision_timing_pps_output, test_init_rejects_invalid_arguments)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_config bad;
	struct precision_pps_output_state state;

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);

	zassert_equal(precision_pps_output_init(NULL, &fake.clock, &config, test_callback, &rec_a),
		      -EINVAL);
	zassert_equal(precision_pps_output_init(&pps, NULL, &config, test_callback, &rec_a),
		      -EINVAL);
	zassert_equal(precision_pps_output_init(&pps, &fake.clock, NULL, test_callback, &rec_a),
		      -EINVAL);
	zassert_equal(precision_pps_output_init(&pps, &fake.clock, &config, NULL, &rec_a), -EINVAL);

	bad = config;
	bad.width_policy = (enum precision_clock_output_width_policy)5;
	zassert_equal(precision_pps_output_init(&pps, &fake.clock, &bad, test_callback, &rec_a),
		      -EINVAL);

	bad = exact_config();
	bad.pulse_width_ns = NSEC_PER_SEC;
	zassert_equal(precision_pps_output_init(&pps, &fake.clock, &bad, test_callback, &rec_a),
		      -EINVAL);

	bad = exact_config();
	bad.pulse_width_ns = 0;
	zassert_equal(precision_pps_output_init(&pps, &fake.clock, &bad, test_callback, &rec_a),
		      -EINVAL);

	bad = config;
	bad.start_guard_ns = -1;
	zassert_equal(precision_pps_output_init(&pps, &fake.clock, &bad, test_callback, &rec_a),
		      -EINVAL);

	bad = config;
	bad.step_limit_ns = -1;
	zassert_equal(precision_pps_output_init(&pps, &fake.clock, &bad, test_callback, &rec_a),
		      -EINVAL);

	bad = config;
	bad.poll_interval_ms = 0;
	zassert_equal(precision_pps_output_init(&pps, &fake.clock, &bad, test_callback, &rec_a),
		      -EINVAL);

	zassert_equal(precision_pps_output_state_get(NULL, &state), -EINVAL);
	zassert_equal(precision_pps_output_start(NULL), -EINVAL);
	zassert_equal(precision_pps_output_stop(NULL), -EINVAL);

	/* A valid but not-started instance rejects observation and stop. */
	zassert_ok(precision_pps_output_init(&pps, &fake.clock, &config, test_callback, &rec_a));
	zassert_equal(precision_pps_output_state_get(&pps, NULL), -EINVAL);
	zassert_equal(precision_pps_output_state_get(&pps, &state), -EINVAL);
	zassert_equal(precision_pps_output_stop(&pps), -EALREADY);
	zassert_equal(fake.caps_calls, 0, "no clock query before start");
}

ZTEST(precision_timing_pps_output, test_start_validates_caps_against_policy)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_config exact = exact_config();
	struct precision_pps_output_state state;

	TEST_PPS_INSTANCE(no_waveform);
	TEST_PPS_INSTANCE(bad_resolution);
	TEST_PPS_INSTANCE(fixed_width);
	TEST_PPS_INSTANCE(bad_width_range);
	TEST_PPS_INSTANCE(bad_period_range);
	TEST_PPS_INSTANCE(caps_error);

	fake_init(&fake);
	fake.caps.flags = 0;
	zassert_ok(precision_pps_output_init(&no_waveform, &fake.clock, &config, test_callback,
					     &rec_a));
	zassert_equal(precision_pps_output_start(&no_waveform), -ENOTSUP);
	zassert_equal(precision_pps_output_state_get(&no_waveform, &state), -EINVAL);

	fake_init(&fake);
	fake.caps.resolution_ns = 3;
	zassert_ok(precision_pps_output_init(&bad_resolution, &fake.clock, &config, test_callback,
					     &rec_a));
	zassert_equal(precision_pps_output_start(&bad_resolution), -ERANGE);

	/* Exact width demands programmable width support. */
	fake_init(&fake);
	fake.caps.flags = PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM;
	zassert_ok(precision_pps_output_init(&fixed_width, &fake.clock, &exact, test_callback,
					     &rec_a));
	zassert_equal(precision_pps_output_start(&fixed_width), -ENOTSUP);

	/* Exact width outside the reported pulse-width range. */
	fake_init(&fake);
	fake.caps.max_pulse_width_ns = exact.pulse_width_ns - 1;
	zassert_ok(precision_pps_output_init(&bad_width_range, &fake.clock, &exact, test_callback,
					     &rec_a));
	zassert_equal(precision_pps_output_start(&bad_width_range), -ERANGE);

	fake_init(&fake);
	fake.caps.min_period_ns = 2LL * NSEC_PER_SEC;
	zassert_ok(precision_pps_output_init(&bad_period_range, &fake.clock, &config, test_callback,
					     &rec_a));
	zassert_equal(precision_pps_output_start(&bad_period_range), -ERANGE);

	fake_init(&fake);
	fake.caps_error = -EIO;
	zassert_ok(precision_pps_output_init(&caps_error, &fake.clock, &config, test_callback,
					     &rec_a));
	zassert_equal(precision_pps_output_start(&caps_error), -EIO);
	zassert_equal(precision_pps_output_state_get(&caps_error, &state), -EINVAL);
}

ZTEST(precision_timing_pps_output, test_start_rejects_double_start)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_equal(precision_pps_output_start(&pps), -EALREADY);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_initial_arm_stops_then_starts_fresh_output)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_true(rec_a.state.active);
	zassert_false(rec_a.state.rearm_pending);
	zassert_equal(rec_a.state.generation, 1);
	zassert_equal(rec_a.state.rearm_count, 0);
	zassert_equal(rec_a.state.last_error, 0);
	zassert_equal(rec_a.state.config.period_ns, (precision_time_t)NSEC_PER_SEC);
	zassert_equal(rec_a.state.config.width_policy,
		      PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	zassert_equal(rec_a.state.config.first_rising_time % (precision_time_t)NSEC_PER_SEC, 0);
	zassert_equal(fake.start_calls, 1);
	zassert_equal(fake.stop_calls, 1);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_stop_crosses_target_boundary)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	/* Model PHC time advancing while the synchronous provider stop blocks. */
	fake.stop_advance_ns = 2 * (precision_time_t)NSEC_PER_SEC;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_true(rec_a.state.active);
	zassert_true(rec_a.state.config.first_rising_time >
		     fake_monotonic_ns() + fake.offset_ns);
	zassert_equal(fake.start_calls, 1);
	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_read_failure_after_stop_retries)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	fake.read_error_after_stop = -EIO;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_READ_ERROR) != 0);
	zassert_false(rec_a.state.phc_read_valid);
	zassert_true(rec_a.state.rearm_pending);
	zassert_equal(rec_a.state.last_error, -EIO);
	zassert_equal(fake.start_calls, 0);
	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.read_error_after_stop = 0;
	fake.read_error = 0;
	k_mutex_unlock(&fake_lock);
	wait_callback(&rec_a);
	zassert_true(rec_a.state.active);
	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_exact_width_is_programmed_and_reported)
{
	struct precision_pps_output_config config = exact_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);
	zassert_equal(rec_a.state.config.pulse_width_ns, config.pulse_width_ns);

	k_mutex_lock(&fake_lock, K_FOREVER);
	zassert_equal(fake.status.config.waveform.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);
	zassert_equal(fake.status.config.waveform.pulse_width_ns, config.pulse_width_ns);
	k_mutex_unlock(&fake_lock);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_adopts_already_active_matching_output)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	fake.provider_configured = true;
	fake.status.configured = true;
	fake.status.kind = PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM;
	fake.status.config.waveform = (struct precision_clock_output_waveform_config){
		.first_rising_time = 0,
		.period_ns = NSEC_PER_SEC,
		.width_policy = PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT,
		.pulse_width_ns = 0,
	};

	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_equal(rec_a.events, PRECISION_PPS_OUTPUT_EVENT_ARMED);
	zassert_true(rec_a.state.active);
	zassert_equal(rec_a.state.generation, 1);
	zassert_equal(rec_a.state.rearm_count, 0);
	zassert_equal(fake.start_calls, 0);
	zassert_equal(fake.stop_calls, 0);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_configured_only_status_stays_active)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state before;
	struct precision_pps_output_state after;
	uint32_t calls_before;

	TEST_PPS_INSTANCE(pps);

	/* Default caps do not advertise hardware-active: validity stays false. */
	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	zassert_ok(precision_pps_output_state_get(&pps, &before));

	k_mutex_lock(&fake_lock, K_FOREVER);
	zassert_false(fake.status.hardware_active_valid,
		      "generator state must be reported as unobservable");
	zassert_false(fake.status.hardware_active, "invalid false state must be ignored");
	k_mutex_unlock(&fake_lock);

	calls_before = rec_a.calls;
	k_msleep(4 * config.poll_interval_ms + 10);

	zassert_equal(rec_a.calls, calls_before,
		      "a configured, matching, unobservable output must not rearm");
	zassert_ok(precision_pps_output_state_get(&pps, &after));
	zassert_true(after.active);
	zassert_equal(after.generation, before.generation);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_hardware_active_stays_healthy_before_and_after_first_edge)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state before;
	struct precision_pps_output_state after;
	precision_time_t first_edge;
	precision_time_t now;
	uint32_t status_calls;
	uint32_t stop_calls;

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	fake.caps.flags |= PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE;
	config.start_guard_ns = 2LL * NSEC_PER_SEC;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	zassert_ok(precision_pps_output_state_get(&pps, &before));

	k_mutex_lock(&fake_lock, K_FOREVER);
	zassert_true(fake.status.configured);
	zassert_true(fake.status.hardware_active_valid);
	zassert_true(fake.status.hardware_active);
	first_edge = fake.status.config.waveform.first_rising_time;
	status_calls = fake.status_calls;
	stop_calls = fake.stop_calls;
	k_mutex_unlock(&fake_lock);

	/* The generator is armed, but no edge has occurred during these polls. */
	k_msleep(4 * config.poll_interval_ms + 10);
	zassert_ok(precision_clock_read(&fake.clock, &now));
	zassert_true(now < first_edge);
	zassert_ok(precision_pps_output_state_get(&pps, &after));
	zassert_true(after.active);
	zassert_equal(after.generation, before.generation);
	zassert_equal(after.rearm_count, 0);

	k_mutex_lock(&fake_lock, K_FOREVER);
	zassert_true(fake.status_calls > status_calls);
	zassert_equal(fake.start_calls, 1);
	zassert_equal(fake.stop_calls, stop_calls);
	k_mutex_unlock(&fake_lock);

	/* Running the same waveform past its first edge must also stay healthy. */
	k_msleep((first_edge - now) / NSEC_PER_MSEC + 4 * config.poll_interval_ms + 10);
	zassert_ok(precision_pps_output_state_get(&pps, &after));
	zassert_true(after.phc_time_ns > first_edge);
	zassert_true(after.active);
	zassert_equal(after.generation, before.generation);
	zassert_equal(after.rearm_count, 0);

	k_mutex_lock(&fake_lock, K_FOREVER);
	zassert_equal(fake.start_calls, 1);
	zassert_equal(fake.stop_calls, stop_calls);
	k_mutex_unlock(&fake_lock);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_hardware_active_false_recovers)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	fake.caps.flags |= PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	k_mutex_lock(&fake_lock, K_FOREVER);
	zassert_true(fake.status.hardware_active_valid);
	zassert_true(fake.status.hardware_active);
	k_mutex_unlock(&fake_lock);

	set_hardware_active(&fake, false);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_OUTPUT_INACTIVE) != 0);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_RECOVERED) != 0);
	zassert_false((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_true(rec_a.state.active);
	zassert_equal(rec_a.state.generation, 2);
	zassert_equal(rec_a.state.rearm_count, 1);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_normal_drift_does_not_rearm)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state before;
	struct precision_pps_output_state after;
	uint32_t calls_before;

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	zassert_ok(precision_pps_output_state_get(&pps, &before));

	calls_before = rec_a.calls;
	bump_offset(&fake, 1000);

	zassert_true(WAIT_FOR(poll_advanced(&pps, before.phc_time_ns, &after),
			      WAIT_STATE_TIMEOUT_US, k_msleep(1)));
	zassert_equal(after.continuity_error_ns, 1000);
	zassert_equal(after.generation, before.generation);
	zassert_equal(rec_a.calls, calls_before, "a bounded drift must not produce an event");

	zassert_ok(precision_pps_output_stop(&pps));
}

static void check_slow_read(bool sample_before_delay, bool after_stop)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state before;
	struct precision_pps_output_state after;
	static struct k_sem read_entered;
	static struct k_sem read_release;

	TEST_PPS_INSTANCE(pps);
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
	zassert_true(WAIT_FOR(precision_pps_output_state_get(&pps, &before) == 0 &&
				     before.active && !before.rearm_pending,
			     WAIT_STATE_TIMEOUT_US, k_msleep(1)));

	if (!after_stop) {
		/* Park a normal poll so its delayed read is observed before checking state. */
		k_mutex_lock(&fake_lock, K_FOREVER);
		fake.sample_before_delay = sample_before_delay;
		fake.read_delay_ms = 150;
		fake.read_entered = &read_entered;
		fake.read_release = &read_release;
		k_mutex_unlock(&fake_lock);
		zassert_ok(k_sem_take(&read_entered, CALLBACK_TIMEOUT));
		k_sem_give(&read_release);
		zassert_ok(precision_pps_output_state_get(&pps, &after));
		zassert_equal(after.generation, before.generation,
			      "a slow read must not interrupt a continuous waveform");
		before = after;
	}

	/* The following fast poll must also account for the previous read's uncertainty. */
	zassert_true(WAIT_FOR(poll_advanced(&pps, before.phc_time_ns, &after),
			      WAIT_STATE_TIMEOUT_US, k_msleep(1)));
	zassert_true(after.active);
	zassert_false(after.rearm_pending);
	zassert_equal(after.generation, 1);
	zassert_equal(after.rearm_count, 0);
	zassert_ok(precision_pps_output_stop(&pps));
	zassert_false((atomic_get(&rec_a.observed_events) &
		       PRECISION_PPS_OUTPUT_EVENT_HARD_STEP) != 0,
		      "read latency must not emit a hard-step event");
	if (after_stop && sample_before_delay) {
		zassert_true((atomic_get(&rec_a.observed_events) &
			      PRECISION_PPS_OUTPUT_EVENT_START_ERROR) != 0,
			     "a late initial sample must exercise start-error recovery");
	}
}

ZTEST(precision_timing_pps_output, test_read_delay_before_sampling_preserves_output)
{
	check_slow_read(false, false);
}

ZTEST(precision_timing_pps_output, test_read_delay_after_sampling_preserves_output)
{
	check_slow_read(true, false);
}

ZTEST(precision_timing_pps_output, test_post_stop_read_delay_preserves_output)
{
	check_slow_read(true, true);
}

static void check_step_during_slow_read(precision_time_t step_ns)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state after;

	TEST_PPS_INSTANCE(pps);
	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.read_delay_ms = 150;
	fake.offset_ns += step_ns;
	k_mutex_unlock(&fake_lock);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_HARD_STEP) != 0);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_RECOVERED) != 0);
	zassert_equal(rec_a.state.generation, 2);
	zassert_equal(rec_a.state.rearm_count, 1);
	if (step_ns > 0) {
		zassert_true(rec_a.state.continuity_error_ns > config.step_limit_ns);
		zassert_true(rec_a.state.continuity_error_ns <= step_ns);
	} else {
		zassert_true(rec_a.state.continuity_error_ns < -config.step_limit_ns);
		zassert_true(rec_a.state.continuity_error_ns >= step_ns);
	}
	zassert_true(WAIT_FOR(poll_advanced(&pps, rec_a.state.phc_time_ns, &after),
			      WAIT_STATE_TIMEOUT_US, k_msleep(1)));
	zassert_equal(after.generation, 2, "a real step must cause exactly one rearm");
	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_forward_step_outside_read_uncertainty_rearms)
{
	check_step_during_slow_read(400LL * NSEC_PER_MSEC);
}

ZTEST(precision_timing_pps_output, test_backward_step_outside_read_uncertainty_rearms)
{
	check_step_during_slow_read(-400LL * NSEC_PER_MSEC);
}

ZTEST(precision_timing_pps_output, test_forward_hard_step_rearms)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_HARD_STEP) != 0);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_RECOVERED) != 0);
	zassert_false((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.continuity_error_ns, 400LL * NSEC_PER_MSEC);
	zassert_equal(rec_a.state.generation, 2);
	zassert_equal(rec_a.state.rearm_count, 1);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_backward_hard_step_rearms)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	bump_offset(&fake, -150LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_HARD_STEP) != 0);
	zassert_false((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_equal(rec_a.state.continuity_error_ns, -150LL * NSEC_PER_MSEC);
	zassert_equal(rec_a.state.generation, 2);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_inactive_output_recovers)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	set_configured(&fake, false);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_OUTPUT_INACTIVE) != 0);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_RECOVERED) != 0);
	zassert_false((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_true(rec_a.state.active);
	zassert_equal(rec_a.state.generation, 2);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_mismatched_output_recovers)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	mismatch_active_output(&fake);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_OUTPUT_INACTIVE) != 0);
	zassert_false((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_true(rec_a.state.active);
	zassert_equal(rec_a.state.generation, 2);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_read_and_status_errors_preserve_output)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state before;
	uint32_t stop_calls_before;
	uint32_t start_calls_before;

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	zassert_ok(precision_pps_output_state_get(&pps, &before));

	stop_calls_before = fake.stop_calls;
	start_calls_before = fake.start_calls;
	set_read_error(&fake, -EIO);
	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_READ_ERROR) != 0);
	zassert_true(rec_a.state.active);
	zassert_equal(fake.stop_calls, stop_calls_before);

	set_read_error(&fake, 0);
	set_status_error(&fake, -EAGAIN);
	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_READ_RECOVERED) != 0);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_STATUS_ERROR) != 0);
	zassert_true(rec_a.state.active);
	zassert_equal(fake.stop_calls, stop_calls_before);
	zassert_equal(fake.start_calls, start_calls_before);

	set_status_error(&fake, 0);
	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_STATUS_RECOVERED) != 0);
	zassert_equal(rec_a.state.last_error, 0);
	zassert_equal(rec_a.state.generation, before.generation);
	zassert_equal(rec_a.state.rearm_count, before.rearm_count);
	zassert_equal(fake.stop_calls, stop_calls_before);
	zassert_equal(fake.start_calls, start_calls_before);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_combined_errors_keep_most_recent_code)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	set_read_error(&fake, -EIO);
	set_status_error(&fake, -EAGAIN);
	wait_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_READ_ERROR) != 0U);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_STATUS_ERROR) != 0U);
	zassert_equal(rec_a.state.last_error, -EAGAIN);

	set_read_error(&fake, 0);
	set_status_error(&fake, 0);
	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_persistent_status_errors_stop_and_rearm)
{
	struct precision_pps_output_config config = default_config();
	uint32_t stop_calls_before;
	uint32_t start_calls_before;

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	stop_calls_before = fake.stop_calls;
	start_calls_before = fake.start_calls;
	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.status_error = -EIO;
	fake.clear_status_error_on_stop = true;
	k_mutex_unlock(&fake_lock);

	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_STATUS_ERROR) != 0);
	zassert_equal(fake.stop_calls, stop_calls_before);
	zassert_equal(fake.start_calls, start_calls_before);

	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_RECOVERED) != 0);
	zassert_false((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_true(rec_a.state.active);
	zassert_false(rec_a.state.rearm_pending);
	zassert_equal(rec_a.state.generation, 2);
	zassert_equal(rec_a.state.rearm_count, 1);
	zassert_equal(fake.stop_calls, stop_calls_before + 1);
	zassert_equal(fake.start_calls, start_calls_before + 1);

	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_STATUS_RECOVERED) != 0);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_start_error_retries_next_poll)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	fake.start_error = -ETIME;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_START_ERROR) != 0);
	zassert_equal(fake.start_calls, 1);
	zassert_true(rec_a.state.rearm_pending);
	zassert_false(rec_a.state.active);

	set_start_error(&fake, 0);
	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_equal(fake.start_calls, 2);
	zassert_true(rec_a.state.active);

	zassert_ok(precision_pps_output_stop(&pps));
}

static void check_failed_start_cleanup(bool cleanup_fails)
{
	struct precision_pps_output_config config = default_config();
	struct precision_clock_output_waveform_config retained;
	uint32_t stop_calls;

	TEST_PPS_INSTANCE(pps);
	fake_init(&fake);
	fake.start_error = -EIO;
	fake.retain_config_on_start_error = true;
	rec_a.block_callback = true;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_parked_callback(&rec_a);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_START_ERROR) != 0);
	zassert_true(rec_a.state.rearm_pending);
	zassert_false(rec_a.state.active);
	k_mutex_lock(&fake_lock, K_FOREVER);
	retained = fake.status.config.waveform;
	stop_calls = fake.stop_calls;
	k_mutex_unlock(&fake_lock);
	zassert_true(fake.provider_configured);
	zassert_true(fake.status.configured);
	zassert_equal(fake.start_calls, 1);

	set_start_error(&fake, 0);
	if (cleanup_fails) {
		set_stop_error(&fake, -EBUSY);
		rec_a.block_callback = true;
		k_sem_give(&rec_a.callback_release);
		wait_callback(&rec_a);
		wait_parked_callback(&rec_a);

		/* A failed cleanup still owns the failed start's exact waveform. */
		zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_STOP_ERROR) != 0);
		zassert_equal(rec_a.state.last_error, -EBUSY);
		zassert_true(rec_a.state.rearm_pending);
		zassert_false(rec_a.state.active);
		zassert_true(fake.provider_configured);
		zassert_true(fake.status.configured);
		zassert_equal(fake.status.config.waveform.first_rising_time,
			      retained.first_rising_time);
		zassert_equal(fake.start_calls, 1, "owned channel restarted before cleanup");
		zassert_equal(fake.stop_calls, stop_calls + 1);
		zassert_equal(precision_pps_output_start(&pps), -EALREADY);
		stop_calls++;
		set_stop_error(&fake, 0);
	}

	rec_a.block_callback = true;
	k_sem_give(&rec_a.callback_release);
	wait_callback(&rec_a);
	wait_parked_callback(&rec_a);

	/* The provider rejects reuse while owned, so arming proves cleanup won. */
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_false((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_START_ERROR) != 0);
	zassert_true(rec_a.state.active);
	zassert_false(rec_a.state.rearm_pending);
	zassert_equal(rec_a.state.last_error, 0);
	zassert_equal(fake.stop_calls, stop_calls + 1);
	zassert_equal(fake.start_calls, 2);
	zassert_true(fake.provider_configured);

	k_sem_give(&rec_a.callback_release);
	wait_callback(&rec_a);
	zassert_ok(precision_pps_output_stop(&pps));
	zassert_false(fake.provider_configured);
}

ZTEST(precision_timing_pps_output, test_failed_start_retains_output_until_cleanup)
{
	check_failed_start_cleanup(false);
}

ZTEST(precision_timing_pps_output, test_failed_start_cleanup_failure_prevents_restart)
{
	check_failed_start_cleanup(true);
}

ZTEST(precision_timing_pps_output, test_stop_error_retries_before_start)
{
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	fake.stop_error = -EIO;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_STOP_ERROR) != 0);
	zassert_equal(fake.stop_calls, 1);
	zassert_equal(fake.start_calls, 0);

	set_stop_error(&fake, 0);
	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_equal(fake.stop_calls, 2);
	zassert_equal(fake.start_calls, 1);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_callback_runs_on_dedicated_queue)
{
	extern struct k_work_q k_sys_work_q;
	struct precision_pps_output_config config = default_config();

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_not_null(rec_a.thread);
	zassert_not_equal(rec_a.thread, k_current_get(), "callback ran on the caller thread");
	zassert_not_equal(rec_a.thread, k_sys_work_q.thread_id,
			  "callback ran on the system workqueue");

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_callback_snapshot_matches_state_get)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state queried;

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	zassert_ok(precision_pps_output_state_get(&pps, &queried));
	zassert_equal(queried.generation, rec_a.state.generation);
	zassert_equal(queried.rearm_count, rec_a.state.rearm_count);
	zassert_equal(queried.active, rec_a.state.active);
	zassert_equal(queried.phc_time_ns, rec_a.state.phc_time_ns);
	zassert_equal(queried.last_error, rec_a.state.last_error);
	zassert_equal(queried.config.first_rising_time, rec_a.state.config.first_rising_time);
	zassert_equal(queried.config.period_ns, rec_a.state.config.period_ns);
	zassert_equal(queried.config.width_policy, rec_a.state.config.width_policy);

	zassert_ok(precision_pps_output_stop(&pps));
	zassert_equal(precision_pps_output_state_get(&pps, &queried), -EINVAL);
}

ZTEST(precision_timing_pps_output, test_stop_stops_output_and_blocks_further_calls)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state state;
	uint32_t calls_before;

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	calls_before = rec_a.calls;
	zassert_ok(precision_pps_output_stop(&pps));
	zassert_true(fake.stop_calls >= 1);
	zassert_equal(precision_pps_output_state_get(&pps, &state), -EINVAL);

	/* Cancellation is synchronous: no poll can be pending or running afterward. */
	zassert_equal(k_sem_take(&rec_a.sem, K_NO_WAIT), -EBUSY);
	zassert_equal(rec_a.calls, calls_before);

	/* A stopped instance restarts without reinitialization. */
	zassert_ok(precision_pps_output_start(&pps));
	wait_callback(&rec_a);
	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_stop_failure_retains_started)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state state;

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	set_stop_error(&fake, -EIO);
	zassert_equal(precision_pps_output_stop(&pps), -EIO);

	/* Started state is retained so the caller cannot release the clock. */
	zassert_ok(precision_pps_output_state_get(&pps, &state));

	/* Polling resumed: a forced hard step still produces a callback. */
	bump_offset(&fake, 500LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_HARD_STEP) != 0);

	set_stop_error(&fake, 0);
	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_self_stop_from_callback_returns_deadlock)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state state;

	TEST_PPS_INSTANCE(pps);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	rec_a.attempt_self_stop = true;
	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);

	zassert_equal(rec_a.self_stop_result, -EDEADLK);

	/* The rejected recursive call must not have torn down the instance. */
	zassert_ok(precision_pps_output_state_get(&pps, &state));
	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_counters_saturate)
{
	struct precision_pps_output_config config = default_config();
	static struct k_sem read_entered;
	static struct k_sem read_release;

	TEST_PPS_INSTANCE(pps);

	k_sem_init(&read_entered, 0, 1);
	k_sem_init(&read_release, 0, 1);

	fake_init(&fake);
	start_instance(&pps, &fake, &config, &rec_a);
	wait_callback(&rec_a);

	/* Park the next poll inside the clock read to preset the counters. */
	k_mutex_lock(&fake_lock, K_FOREVER);
	fake.read_entered = &read_entered;
	fake.read_release = &read_release;
	k_mutex_unlock(&fake_lock);
	zassert_ok(k_sem_take(&read_entered, K_SECONDS(2)));

	/*
	 * White-box preset: the struct is exposed by value, so the counters can
	 * be primed near saturation. The give/take pair below orders these
	 * writes before the parked poll resumes and reads them.
	 */
	pps.state.generation = UINT32_MAX;
	pps.state.rearm_count = UINT32_MAX - 1;
	bump_offset(&fake, 500LL * NSEC_PER_MSEC);
	k_sem_give(&read_release);

	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_RECOVERED) != 0);
	zassert_equal(rec_a.state.generation, UINT32_MAX, "generation must saturate");
	zassert_equal(rec_a.state.rearm_count, UINT32_MAX, "rearm count must saturate");

	/* A further rearm must not wrap past the maximum. */
	bump_offset(&fake, 500LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);
	zassert_equal(rec_a.state.generation, UINT32_MAX);
	zassert_equal(rec_a.state.rearm_count, UINT32_MAX);

	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_two_instances_run_and_stop_independently)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_config exact = exact_config();
	struct precision_pps_output_state state;

	TEST_PPS_INSTANCE(pps);
	TEST_PPS_INSTANCE(pps_other);

	fake_init(&fake);
	fake_init(&fake_other);

	start_instance(&pps, &fake, &config, &rec_a);
	start_instance(&pps_other, &fake_other, &exact, &rec_b);
	wait_callback(&rec_a);
	wait_callback(&rec_b);

	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_true((rec_b.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);
	zassert_ok(precision_pps_output_state_get(&pps, &state));
	zassert_equal(state.config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	zassert_ok(precision_pps_output_state_get(&pps_other, &state));
	zassert_equal(state.config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_EXACT);

	/* Stopping the first instance leaves the second running. */
	zassert_ok(precision_pps_output_stop(&pps));
	zassert_equal(precision_pps_output_state_get(&pps, &state), -EINVAL);

	bump_offset(&fake_other, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_b);
	zassert_true((rec_b.events & PRECISION_PPS_OUTPUT_EVENT_HARD_STEP) != 0);
	zassert_ok(precision_pps_output_state_get(&pps_other, &state));
	zassert_true(state.active);

	zassert_ok(precision_pps_output_stop(&pps_other));
}

ZTEST(precision_timing_pps_output, test_callback_stops_other_instance_on_shared_queue)
{
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_config exact = exact_config();
	struct precision_pps_output_state state;
	uint32_t other_calls;

	TEST_PPS_INSTANCE(pps);
	TEST_PPS_INSTANCE(pps_other);
	fake_init(&fake);
	fake_init(&fake_other);
	start_instance(&pps_other, &fake_other, &exact, &rec_b);
	wait_callback(&rec_b);
	other_calls = rec_b.calls;

	rec_a.block_callback = true;
	rec_a.stop_target = &pps_other;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_parked_callback(&rec_a);

	/*
	 * The shared queue is parked in A's callback. Make B need recovery and
	 * wait until its poll is queued, so stopping B must cancel queued work
	 * without waiting for the same queue thread to execute it.
	 */
	bump_offset(&fake_other, 400LL * NSEC_PER_MSEC);
	zassert_true(WAIT_FOR((k_work_delayable_busy_get(&pps_other.work) & K_WORK_QUEUED) != 0,
			     WAIT_STATE_TIMEOUT_US, k_msleep(1)));
	k_sem_give(&rec_a.callback_release);
	wait_callback(&rec_a);
	zassert_ok(rec_a.other_stop_result);
	zassert_equal(precision_pps_output_state_get(&pps_other, &state), -EINVAL);
	zassert_false(fake_other.provider_configured);
	zassert_equal(rec_b.calls, other_calls);
	zassert_equal(k_sem_take(&rec_b.sem, K_NO_WAIT), -EBUSY);

	zassert_ok(precision_pps_output_state_get(&pps, &state));
	zassert_true(state.active);
	zassert_equal(state.config.width_policy, PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT);
	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_HARD_STEP) != 0);
	zassert_equal(rec_b.calls, other_calls, "stopped B received another callback");
	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_stop_waits_for_running_callback)
{
	static struct stop_thread_ctx ctx;
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state state;

	TEST_PPS_INSTANCE(pps);
	ctx = (struct stop_thread_ctx){.instance = &pps, .rec = &rec_a};
	fake_init(&fake);
	rec_a.block_callback = true;
	start_instance(&pps, &fake, &config, &rec_a);
	wait_parked_callback(&rec_a);
	zassert_true((rec_a.events & PRECISION_PPS_OUTPUT_EVENT_ARMED) != 0);

	start_stop_thread(&ctx);
	zassert_true(WAIT_FOR(atomic_get(&ctx.started) != 0, 1000000, k_msleep(1)));
	k_msleep(20);
	zassert_false(atomic_get(&ctx.done), "stop returned while callback owned user data");
	zassert_equal(atomic_get(&rec_a.completed_calls), 0);

	k_sem_give(&rec_a.callback_release);
	join_stop_thread();
	zassert_ok(ctx.result);
	zassert_equal(ctx.completed_calls, 1,
		      "stop returned before the callback's final user-data access");
	zassert_equal(precision_pps_output_state_get(&pps, &state), -EINVAL);
	zassert_false(fake.provider_configured);

	/* Successful stop permits immediate reuse of both instance and user data. */
	cb_record_reset(&rec_a);
	start_instance(&pps, &fake, &config, &rec_b);
	wait_callback(&rec_b);
	bump_offset(&fake, 400LL * NSEC_PER_MSEC);
	wait_callback(&rec_b);
	zassert_true((rec_b.events & PRECISION_PPS_OUTPUT_EVENT_HARD_STEP) != 0);
	zassert_equal(rec_a.calls, 0, "old callback user data was used after stop");
	zassert_equal(atomic_get(&rec_a.completed_calls), 0);
	zassert_ok(precision_pps_output_stop(&pps));
}

ZTEST(precision_timing_pps_output, test_stop_waits_for_in_flight_poll)
{
	static struct precision_pps_output pps;
	static struct stop_thread_ctx ctx;
	static struct k_sem read_entered;
	static struct k_sem read_release;
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state state;

	track_pps_instance(&pps);
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

	(void)k_sem_take(&rec_a.sem, K_NO_WAIT);
	zassert_equal(precision_pps_output_state_get(&pps, &state), -EINVAL);
}

ZTEST(precision_timing_pps_output, test_stop_other_instance_while_poll_is_blocked)
{
	static struct precision_pps_output pps;
	static struct precision_pps_output pps_other;
	static struct stop_thread_ctx ctx;
	static struct k_sem read_entered;
	static struct k_sem read_release;
	struct precision_pps_output_config config = default_config();
	struct precision_pps_output_state state;

	track_pps_instance(&pps);
	track_pps_instance(&pps_other);
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
	zassert_ok(precision_pps_output_stop(&pps_other));
	zassert_equal(precision_pps_output_state_get(&pps_other, &state), -EINVAL);

	/* Release the parked poll and let its stop complete. */
	k_sem_give(&read_release);
	join_stop_thread();
	zassert_ok(ctx.result);
	zassert_equal(precision_pps_output_state_get(&pps, &state), -EINVAL);
}

ZTEST_SUITE(precision_timing_pps_output, NULL, NULL, before_each, after_each, NULL);
