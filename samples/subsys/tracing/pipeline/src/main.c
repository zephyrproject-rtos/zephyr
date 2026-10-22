/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Sensor-acquisition pipeline tracing sample.
 *
 * This is a small but representative real-time application whose only purpose
 * is to produce an instructive trace. It models a data pipeline that is common
 * in embedded systems and exercises most of the kernel's synchronisation
 * primitives so that, captured with the CTF backend and opened in the trace
 * viewer (scripts/tracing/trace_viewer.py), the schedule tells a clear story.
 *
 * Pipeline stages (highest to lowest scheduling priority):
 *
 *   aggregator (prio 3)  Drains a message queue of sensor readings, updates a
 *                        shared aggregate (mutex protected), and every few
 *                        readings publishes a frame to the consumers (condition
 *                        variable) and writes a summary to a shared "bus".
 *   consumer0/1 (prio 6) Wait on the condition variable for a new frame, then
 *                        do a chunk of CPU-bound "processing" (k_busy_wait).
 *   sensors (prio 8)     Three periodic producers (different periods) that push
 *                        readings into the message queue and sleep.
 *   storage (prio 9)     Lowest priority; periodically grabs the shared bus and
 *                        holds it for a while to simulate a slow flush.
 *
 * Primitives exercised: k_msgq, k_mutex, k_condvar, k_sem, k_work_delayable on
 * the system work queue, and a k_timer. Application phases are annotated with
 * sys_trace_named_event() so they are easy to find in the trace.
 *
 * The "bus" lock is selectable at build time (see Kconfig):
 *
 *   - CONFIG_SAMPLE_BUS_MUTEX (default): the bus is a mutex, so the high
 *     priority aggregator briefly boosts the low priority storage thread via
 *     priority inheritance and never waits long.
 *   - CONFIG_SAMPLE_BUS_SEM: the bus is a plain semaphore (no inheritance). A
 *     medium priority consumer can then run while the low priority storage
 *     thread holds the bus, leaving the high priority aggregator blocked - a
 *     textbook priority inversion that is immediately visible in the trace.
 *
 * See the README for a guided tour of what to look for in the viewer.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/tracing/tracing.h>

#define STACKSIZE 2048

/* Scheduling priorities (lower number == higher priority). */
#define AGG_PRIO      3
#define CONSUMER_PRIO 6
#define SENSOR_PRIO   8
#define STORAGE_PRIO  9

#define NUM_SENSORS   3
#define FRAME_EVERY   4      /* publish a frame every N readings */
#define CONSUMER_WORK_US 3000    /* CPU-bound processing per frame */
#define STORAGE_GAP_MS    2      /* storage idle time between bus holds */
#define STORAGE_HOLD_US   12000  /* how long storage hogs the bus */
#define AGG_BUS_US        500    /* aggregator's write to the bus */

struct sensor_cfg {
	const char *name;
	uint32_t id;
	uint32_t period_ms;
	uint32_t seed;
};

static struct sensor_cfg sensor_cfgs[NUM_SENSORS] = {
	{ .name = "sensor_temp",  .id = 0, .period_ms = 23, .seed = 0x1234 },
	{ .name = "sensor_press", .id = 1, .period_ms = 37, .seed = 0x9e37 },
	{ .name = "sensor_imu",   .id = 2, .period_ms = 53, .seed = 0xbeef },
};

struct sensor_reading {
	uint32_t sensor_id;
	uint32_t seq;
	uint32_t value;
};

/* Producer -> aggregator queue. */
K_MSGQ_DEFINE(sensor_q, sizeof(struct sensor_reading), 16, 4);

/* Shared aggregate state, protected by agg_mutex. */
K_MUTEX_DEFINE(agg_mutex);
static uint64_t agg_sum;
static uint32_t agg_count;
static uint32_t agg_min = UINT32_MAX;
static uint32_t agg_max;

/* Published frame + condition variable used to wake the consumers. */
struct frame {
	uint32_t seq;
	uint32_t count;
	uint32_t avg;
	uint32_t min;
	uint32_t max;
};
K_MUTEX_DEFINE(frame_mutex);
K_CONDVAR_DEFINE(frame_cond);
static struct frame published_frame;

/*
 * The shared "bus" both the aggregator and the storage thread use. Selecting a
 * semaphore here (CONFIG_SAMPLE_BUS_SEM) removes priority inheritance and lets
 * a priority inversion form; the default mutex avoids it.
 */
#ifdef CONFIG_SAMPLE_BUS_SEM
K_SEM_DEFINE(bus_sem, 1, 1);
#define BUS_LOCK()   k_sem_take(&bus_sem, K_FOREVER)
#define BUS_UNLOCK() k_sem_give(&bus_sem)
#else
K_MUTEX_DEFINE(bus_mutex);
#define BUS_LOCK()   k_mutex_lock(&bus_mutex, K_FOREVER)
#define BUS_UNLOCK() k_mutex_unlock(&bus_mutex)
#endif

static uint32_t frames_published;

/* Simple per-sensor LCG so each sensor produces its own varying values. */
static uint32_t lcg_next(uint32_t *state)
{
	*state = (*state * 1103515245u) + 12345u;
	return (*state >> 16) & 0x3ffu;
}

static void sensor_entry(void *p1, void *p2, void *p3)
{
	const struct sensor_cfg *cfg = p1;
	uint32_t state = cfg->seed;
	uint32_t seq = 0;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_thread_name_set(k_current_get(), cfg->name);
	/* Re-assert our priority so it is recorded in the trace. */
	k_thread_priority_set(k_current_get(), SENSOR_PRIO);

	while (1) {
		struct sensor_reading r = {
			.sensor_id = cfg->id,
			.seq = seq++,
			.value = lcg_next(&state),
		};

		/* Producing a reading is the start of the pipeline. */
		sys_trace_named_event("sensor_sample", cfg->id, r.value);

		/* Block here if the aggregator has fallen behind. */
		k_msgq_put(&sensor_q, &r, K_FOREVER);

		k_msleep(cfg->period_ms);
	}
}

static void publish_frame(void)
{
	struct frame f;

	k_mutex_lock(&agg_mutex, K_FOREVER);
	f.count = agg_count;
	f.avg = agg_count ? (uint32_t)(agg_sum / agg_count) : 0;
	f.min = agg_min;
	f.max = agg_max;
	k_mutex_unlock(&agg_mutex);

	/* Hand the frame to the consumers and wake them. */
	k_mutex_lock(&frame_mutex, K_FOREVER);
	f.seq = ++frames_published;
	published_frame = f;
	sys_trace_named_event("frame_publish", f.seq, f.avg);
	k_condvar_broadcast(&frame_cond);
	k_mutex_unlock(&frame_mutex);

	/*
	 * Push a summary onto the shared bus. The consumers were just woken by
	 * the broadcast above, so if the low priority storage thread is holding
	 * the bus right now they will run while we wait for it - the medium
	 * priority work that turns this into a priority inversion (semaphore
	 * variant) or a brief, inheritance-bounded wait (mutex variant).
	 */
	BUS_LOCK();
	sys_trace_named_event("bus_write", f.seq, 0);
	k_busy_wait(AGG_BUS_US);
	BUS_UNLOCK();
}

static void aggregator_entry(void *p1, void *p2, void *p3)
{
	uint32_t since_frame = 0;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_thread_name_set(k_current_get(), "aggregator");
	k_thread_priority_set(k_current_get(), AGG_PRIO);

	while (1) {
		struct sensor_reading r;

		/* Highest priority worker: blocks here until a reading lands. */
		k_msgq_get(&sensor_q, &r, K_FOREVER);

		k_mutex_lock(&agg_mutex, K_FOREVER);
		agg_sum += r.value;
		agg_count++;
		agg_min = MIN(agg_min, r.value);
		agg_max = MAX(agg_max, r.value);
		k_mutex_unlock(&agg_mutex);

		if (++since_frame >= FRAME_EVERY) {
			since_frame = 0;
			publish_frame();
		}
	}
}

static void consumer_entry(void *p1, void *p2, void *p3)
{
	int idx = POINTER_TO_INT(p1);
	uint32_t last_seen = 0;
	char name[16];

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	snprintk(name, sizeof(name), "consumer%d", idx);
	k_thread_name_set(k_current_get(), name);
	k_thread_priority_set(k_current_get(), CONSUMER_PRIO);

	while (1) {
		struct frame f;

		/* Wait for a frame we have not processed yet. */
		k_mutex_lock(&frame_mutex, K_FOREVER);
		while (published_frame.seq == last_seen) {
			k_condvar_wait(&frame_cond, &frame_mutex, K_FOREVER);
		}
		f = published_frame;
		last_seen = f.seq;
		k_mutex_unlock(&frame_mutex);

		/* CPU-bound processing of the frame. */
		sys_trace_named_event("consume_begin", idx, f.seq);
		k_busy_wait(CONSUMER_WORK_US);
		sys_trace_named_event("consume_end", idx, f.avg);

		if ((f.seq % 16u) == 0u) {
			printk("consumer%d: frame %u avg=%u min=%u max=%u "
			       "count=%u\n", idx, f.seq, f.avg, f.min, f.max,
			       f.count);
		}
	}
}

static void storage_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_thread_name_set(k_current_get(), "storage");
	k_thread_priority_set(k_current_get(), STORAGE_PRIO);

	while (1) {
		/*
		 * Lowest priority thread holds the shared bus for a long time
		 * to simulate a slow flush to storage. It keeps the bus busy a
		 * large fraction of the time, so the aggregator regularly finds
		 * it taken. While holding it we can be preempted by anything of
		 * higher priority (every other thread) - with the semaphore bus
		 * that is exactly what strands the high priority aggregator.
		 */
		BUS_LOCK();
		sys_trace_named_event("store_begin", 0, 0);
		k_busy_wait(STORAGE_HOLD_US);
		sys_trace_named_event("store_end", 0, 0);
		BUS_UNLOCK();

		k_msleep(STORAGE_GAP_MS);
	}
}

/* Periodic background flush on the system work queue. */
static void flush_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	static uint32_t flush_seq;

	sys_trace_named_event("flush", flush_seq++, k_msgq_num_used_get(&sensor_q));
	k_work_reschedule(dwork, K_MSEC(500));
}
static K_WORK_DELAYABLE_DEFINE(flush_work, flush_handler);

/* Heartbeat timer: fires from the system clock ISR. */
static void heartbeat_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	sys_trace_named_event("heartbeat", frames_published, agg_count);
}
static K_TIMER_DEFINE(heartbeat_timer, heartbeat_handler, NULL);

K_THREAD_DEFINE(aggregator_thread, STACKSIZE, aggregator_entry, NULL, NULL, NULL,
		AGG_PRIO, 0, 0);
K_THREAD_DEFINE(consumer0_thread, STACKSIZE, consumer_entry, INT_TO_POINTER(0),
		NULL, NULL, CONSUMER_PRIO, 0, 0);
K_THREAD_DEFINE(consumer1_thread, STACKSIZE, consumer_entry, INT_TO_POINTER(1),
		NULL, NULL, CONSUMER_PRIO, 0, 0);
K_THREAD_DEFINE(sensor0_thread, STACKSIZE, sensor_entry, &sensor_cfgs[0],
		NULL, NULL, SENSOR_PRIO, 0, 0);
K_THREAD_DEFINE(sensor1_thread, STACKSIZE, sensor_entry, &sensor_cfgs[1],
		NULL, NULL, SENSOR_PRIO, 0, 0);
K_THREAD_DEFINE(sensor2_thread, STACKSIZE, sensor_entry, &sensor_cfgs[2],
		NULL, NULL, SENSOR_PRIO, 0, 0);
K_THREAD_DEFINE(storage_thread, STACKSIZE, storage_entry, NULL, NULL, NULL,
		STORAGE_PRIO, 0, 0);

int main(void)
{
	printk("Sensor pipeline starting (%s bus)\n",
	       IS_ENABLED(CONFIG_SAMPLE_BUS_SEM) ? "semaphore" : "mutex");

	k_work_schedule(&flush_work, K_MSEC(500));
	k_timer_start(&heartbeat_timer, K_MSEC(1000), K_MSEC(1000));

	return 0;
}
