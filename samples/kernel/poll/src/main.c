/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SAMPLE_COUNT 4
#define HOUSEKEEPING_INTERVAL K_MSEC(120)

struct sensor_sample {
	uint32_t sequence;
	int32_t value;
};

enum event_id {
	SAMPLE_EVENT,
	CONTROL_EVENT,
	SHUTDOWN_EVENT,
	EVENT_COUNT,
};

K_MSGQ_DEFINE_TYPE(sample_queue, struct sensor_sample, SAMPLE_COUNT);
K_SEM_DEFINE(control_request, 0, 1);

static struct k_poll_signal shutdown_signal = K_POLL_SIGNAL_INITIALIZER(shutdown_signal);

static void sample_timer_expiry(struct k_timer *timer)
{
	static uint32_t next_sample;
	struct sensor_sample sample;

	sample.sequence = next_sample;
	sample.value = (int32_t)next_sample + 20;
	next_sample++;

	/* The queue has room for every sample this timer produces. */
	(void)k_msgq_put(&sample_queue, &sample, K_NO_WAIT);

	if (next_sample >= SAMPLE_COUNT) {
		k_timer_stop(timer);
	}
}

static void control_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_sem_give(&control_request);
}

static void shutdown_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	(void)k_poll_signal_raise(&shutdown_signal, 0);
}

K_TIMER_DEFINE(sample_timer, sample_timer_expiry, NULL);
K_TIMER_DEFINE(control_timer, control_timer_expiry, NULL);
K_TIMER_DEFINE(shutdown_timer, shutdown_timer_expiry, NULL);

static void stop_event_sources(void)
{
	k_timer_stop(&sample_timer);
	k_timer_stop(&control_timer);
	k_timer_stop(&shutdown_timer);
}

int main(void)
{
	struct k_poll_event events[EVENT_COUNT] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
					 &sample_queue),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
					 &control_request),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
					 &shutdown_signal),
	};
	uint32_t processed_samples = 0;
	uint32_t control_requests = 0;
	uint32_t housekeeping_runs = 0;
	k_timepoint_t next_housekeeping;
	bool running = true;

	printk("Kernel event dispatcher started\n");

	k_timer_start(&sample_timer, K_MSEC(50), K_MSEC(80));
	k_timer_start(&control_timer, K_MSEC(325), K_NO_WAIT);
	k_timer_start(&shutdown_timer, K_MSEC(400), K_NO_WAIT);

	next_housekeeping = sys_timepoint_calc(HOUSEKEEPING_INTERVAL);
	while (running) {
		int ret = k_poll(events, ARRAY_SIZE(events),
				 sys_timepoint_timeout(next_housekeeping));

		if (ret != 0 && ret != -EAGAIN) {
			printk("poll failed: %d\n", ret);
			stop_event_sources();
			return ret;
		}

		if (sys_timepoint_expired(next_housekeeping)) {
			housekeeping_runs++;
			printk("housekeeping %u\n", housekeeping_runs);
			next_housekeeping = sys_timepoint_calc(HOUSEKEEPING_INTERVAL);
		}

		if (events[SAMPLE_EVENT].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
			struct sensor_sample sample;

			while (k_msgq_get(&sample_queue, &sample, K_NO_WAIT) == 0) {
				processed_samples++;
				printk("sample %u: value=%d\n", sample.sequence, sample.value);
			}
			events[SAMPLE_EVENT].state = K_POLL_STATE_NOT_READY;
		}

		if (events[CONTROL_EVENT].state == K_POLL_STATE_SEM_AVAILABLE) {
			if (k_sem_take(&control_request, K_NO_WAIT) == 0) {
				control_requests++;
				printk("control request handled\n");
			}
			events[CONTROL_EVENT].state = K_POLL_STATE_NOT_READY;
		}

		if (events[SHUTDOWN_EVENT].state == K_POLL_STATE_SIGNALED) {
			unsigned int signaled;
			int result;

			k_poll_signal_check(&shutdown_signal, &signaled, &result);
			if (signaled != 0U) {
				printk("shutdown requested: result=%d\n", result);
				running = false;
			}
			k_poll_signal_reset(&shutdown_signal);
			events[SHUTDOWN_EVENT].state = K_POLL_STATE_NOT_READY;
		}
	}

	stop_event_sources();
	printk("dispatcher complete: samples=%u control=%u housekeeping=%u\n",
	       processed_samples, control_requests, housekeeping_runs);

	return 0;
}
