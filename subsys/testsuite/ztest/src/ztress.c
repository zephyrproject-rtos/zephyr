/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztress.h>
#include <ztest_test.h>
#include <sys/printk.h>
#include <random/rand32.h>
#include <string.h>


/* Timer used for adjusting contexts backoff time to get optimal CPU load. */
static void ctrl_timeout(struct k_timer *timer);
K_TIMER_DEFINE(ctrl_timer, ctrl_timeout, NULL);

/* Timer used for reporting test progress. */
static void progress_timeout(struct k_timer *timer);
K_TIMER_DEFINE(progress_timer, progress_timeout, NULL);

/* Timer used for higher priority context. */
static void ztress_timeout(struct k_timer *timer);
K_TIMER_DEFINE(ztress_timer, ztress_timeout, NULL);

/* Timer handling test timeout which ends test prematurely. */
static k_timeout_t timeout;
static void test_timeout(struct k_timer *timer);
K_TIMER_DEFINE(test_timer, test_timeout, NULL);

static atomic_t active_cnt;
static struct k_thread threads[CONFIG_ZTRESS_MAX_THREADS];
static k_tid_t tids[CONFIG_ZTRESS_MAX_THREADS];

static uint32_t context_cnt;
struct ztress_context_data *tmr_data;

static atomic_t active_mask;
static uint32_t preempt_cnt[CONFIG_ZTRESS_MAX_THREADS];
static uint32_t exec_cnt[CONFIG_ZTRESS_MAX_THREADS];
static k_timeout_t backoff[CONFIG_ZTRESS_MAX_THREADS];
static k_timeout_t init_backoff[CONFIG_ZTRESS_MAX_THREADS];
K_THREAD_STACK_ARRAY_DEFINE(stacks, CONFIG_ZTRESS_MAX_THREADS, CONFIG_ZTRESS_STACK_SIZE);
static k_tid_t idle_tid;

#define THREAD_NAME(i, _) STRINGIFY(ztress_##i),

static const char * const thread_names[] = {
	UTIL_LISTIFY(CONFIG_ZTRESS_MAX_THREADS, THREAD_NAME, _)
};

struct ztress_runtime {
	uint32_t cpu_load;
	uint32_t cpu_load_measurements;
};

static struct ztress_runtime rt;

static void test_timeout(struct k_timer *timer)
{
	ztress_abort();
}

/* Ratio is 1/16, e.g using ratio 14 reduces all timeouts by multipling it by 14/16.
 * 16 fraction is used to avoid dividing which may take more time on certain platforms.
 */
static void adjust_load(uint8_t ratio)
{
	for (uint32_t i = 0; i < context_cnt; i++) {
		uint32_t new_ticks = ratio * (uint32_t)backoff[i].ticks / 16;

		backoff[i].ticks = MAX(4, new_ticks);
	}
}

static void progress_timeout(struct k_timer *timer)
{
	struct ztress_context_data *thread_data = k_timer_user_data_get(timer);
	uint32_t progress = 100;
	uint32_t cnt = context_cnt;

	if (tmr_data != NULL) {
		cnt--;
		if (tmr_data->exec_cnt != 0 && exec_cnt[cnt] != 0) {
			progress = (100 * exec_cnt[cnt]) / tmr_data->exec_cnt;
		}
	}

	for (uint32_t i = 0; i < cnt; i++) {
		if (thread_data[i].exec_cnt == 0 && thread_data[i].preempt_cnt == 0) {
			continue;
		}

		uint32_t exec_progress = (thread_data[i].exec_cnt) ?
				(100 * exec_cnt[i]) / thread_data[i].exec_cnt : 100;
		uint32_t preempt_progress = (thread_data[i].preempt_cnt) ?
				(100 * preempt_cnt[i]) / thread_data[i].preempt_cnt : 100;
		uint32_t thread_progress = MIN(exec_progress, preempt_progress);

		progress = MIN(progress, thread_progress);
	}


	uint64_t rem = 1000 * (k_timer_expires_ticks(&test_timer) - sys_clock_tick_get()) /
			CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	printk("\r%u%% remaining:%u ms", progress, (uint32_t)rem);
}

static void control_load(void)
{
	static uint64_t prev_cycles;
	static uint64_t total_cycles;

	k_thread_runtime_stats_t rt_stats_thread;
	k_thread_runtime_stats_t rt_stats_all;
	int err = 0;

	err = k_thread_runtime_stats_get(idle_tid, &rt_stats_thread);
	if (err < 0) {
		return;
	}

	err = k_thread_runtime_stats_all_get(&rt_stats_all);
	if (err < 0) {
		return;
	}

	int load = 1000 - (1000 * (rt_stats_thread.execution_cycles - prev_cycles) /
			(rt_stats_all.execution_cycles - total_cycles));

	prev_cycles = rt_stats_thread.execution_cycles;
	total_cycles = rt_stats_all.execution_cycles;

	int avg_load = (rt.cpu_load * rt.cpu_load_measurements + load) /
			(rt.cpu_load_measurements + 1);

	rt.cpu_load = avg_load;
	rt.cpu_load_measurements++;

	if (load > 800 && load < 850) {
		/* Expected load */
	} else if (load > 850) {
		/* Slightly reduce load. */
		adjust_load(18);
	} else if (load < 300) {
		adjust_load(8);
	} else if (load < 500) {
		adjust_load(12);
	} else {
		adjust_load(14);
	}
}

static void ctrl_timeout(struct k_timer *timer)
{
	control_load();
}

void preempt_update(void)
{
	uint32_t mask = active_mask;

	while (mask) {
		int idx = 31 - __builtin_clz(mask);

		/* Clear mask to ensure that other context does not count same thread. */
		if ((atomic_and(&active_mask, ~BIT(idx)) & BIT(idx)) != 0) {
			preempt_cnt[idx]++;
		}

		mask &= ~BIT(idx);
	}
}

static bool cont_check(struct ztress_context_data *context_data, uint32_t priority)
{
	if (context_data->preempt_cnt != 0 && preempt_cnt[priority] >= context_data->preempt_cnt) {
		atomic_dec(&active_cnt);
		return false;
	}

	if (context_data->exec_cnt != 0 && exec_cnt[priority] >= context_data->exec_cnt) {
		atomic_dec(&active_cnt);
		return false;
	}

	return active_cnt > 0;
}

static k_timeout_t randomize_t(k_timeout_t t)
{
	if (t.ticks <= 4) {
		return t;
	}

	uint32_t mask = BIT_MASK(31 - __builtin_clz((uint32_t)t.ticks));

	t.ticks += (sys_rand32_get() & mask);

	return t;
}

static void microdelay(void)
{
	static volatile int microdelay_cnt;
	uint32_t repeat = sys_rand32_get() & 0xff;

	for (int i = 0; i < repeat; i++) {
		microdelay_cnt++;
	}
}

static void ztress_timeout(struct k_timer *timer)
{
	struct ztress_context_data *context_data = k_timer_user_data_get(timer);
	uint32_t priority = 0;
	bool cont_test, cont;

	preempt_update();
	cont_test = cont_check(context_data, priority);
	cont = context_data->handler(context_data->user_data,
				     exec_cnt[priority],
				     !cont_test,
				     priority);
	exec_cnt[priority]++;

	if (cont == true && cont_test == true) {
		k_timer_start(timer, randomize_t(backoff[priority]), K_NO_WAIT);
	}
}

static void sleep(k_timeout_t t)
{
	if (K_TIMEOUT_EQ(t, K_NO_WAIT) == false) {
		t = randomize_t(t);
		k_sleep(t);
	}
}

static void ztress_thread(void *data, void *prio, void *unused)
{
	struct ztress_context_data *context_data = data;
	uint32_t priority = (uint32_t)(uintptr_t)prio;
	bool cont_test, cont;

	do {
		uint32_t cnt = exec_cnt[priority];

		preempt_update();
		exec_cnt[priority] = cnt + 1;
		cont_test = cont_check(context_data, priority);
		microdelay();
		atomic_or(&active_mask, BIT(priority));
		cont = context_data->handler(context_data->user_data, cnt, !cont_test, priority);
		atomic_and(&active_mask, ~BIT(priority));

		sleep(backoff[priority]);
	} while (cont == true && cont_test == true);
}

static void thread_cb(const struct k_thread *cthread, void *user_data)
{
	const char *tname = k_thread_name_get((struct k_thread *)cthread);

	if (strcmp(tname, "idle 00") == 0) {
		idle_tid = (struct k_thread *)cthread;
	}
}

static void ztress_init(struct ztress_context_data *thread_data)
{
	memset(exec_cnt, 0, sizeof(exec_cnt));
	memset(preempt_cnt, 0, sizeof(preempt_cnt));
	memset(&rt, 0, sizeof(rt));
	k_thread_foreach(thread_cb, NULL);
	k_msleep(10);

	if (idle_tid == NULL) {
		printk("Failed to identify idle thread. CPU load will not be tracked\n");
	}

	k_timer_start(&ctrl_timer, K_MSEC(100), K_MSEC(100));
	k_timer_user_data_set(&progress_timer, thread_data);
	k_timer_start(&progress_timer,
		      K_MSEC(CONFIG_ZTRESS_REPORT_PROGRESS_MS),
		      K_MSEC(CONFIG_ZTRESS_REPORT_PROGRESS_MS));
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT) == false) {
		k_timer_start(&test_timer, timeout, K_NO_WAIT);
	}
}

static void ztress_end(int old_prio)
{
	k_timer_stop(&ctrl_timer);
	k_timer_stop(&progress_timer);
	k_timer_stop(&test_timer);
	k_thread_priority_set(k_current_get(), old_prio);
}

static void active_cnt_init(struct ztress_context_data *data)
{
	if (data->preempt_cnt != 0 || data->exec_cnt != 0) {
		active_cnt++;
	}
}

int ztress_execute(struct ztress_context_data *timer_data,
		   struct ztress_context_data *thread_data,
		   size_t cnt)
{
	/* Start control timer. */
	int old_prio = k_thread_priority_get(k_current_get());
	int priority, ztress_prio = 0;

	if (cnt > CONFIG_ZTRESS_MAX_THREADS) {
		return -EINVAL;
	}

	if (cnt + 2 > CONFIG_NUM_PREEMPT_PRIORITIES) {
		return -EINVAL;
	}

	/* Skip test if system clock is set too high compared to CPU frequency.
	 * It can happen when system clock is set globally for the test which is
	 * run on various platforms. 1000 factor is an arbitrary value.
	 */
	if ((sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC) < 1000) {
		ztest_test_skip();
	}

	ztress_init(thread_data);

	context_cnt = cnt + (timer_data ? 1 : 0);
	priority = K_LOWEST_APPLICATION_THREAD_PRIO - cnt - 1;

	k_thread_priority_set(k_current_get(), priority);
	priority++;

	tmr_data = timer_data;

	if (timer_data != NULL) {
		active_cnt_init(timer_data);
		backoff[ztress_prio] = timer_data->t;
		init_backoff[ztress_prio] = timer_data->t;
		k_timer_user_data_set(&ztress_timer, timer_data);
		ztress_prio++;
	}

	for (int i = 0; i < cnt; i++) {
		active_cnt_init(&thread_data[i]);
		backoff[ztress_prio] = thread_data[i].t;
		init_backoff[ztress_prio] = thread_data[i].t;
		tids[i] = k_thread_create(&threads[i], stacks[i], CONFIG_ZTRESS_STACK_SIZE,
					  ztress_thread,
					  &thread_data[i], (void *)(uintptr_t)ztress_prio, NULL,
					  priority, 0, K_NO_WAIT);
		(void)k_thread_name_set(tids[i], thread_names[i]);
		priority++;
		ztress_prio++;
	}

	if (timer_data != NULL) {
		k_timer_start(&ztress_timer, backoff[0], K_NO_WAIT);
	}

	/* Wait until all threads complete. */
	for (int i = 0; i < cnt; i++) {
		k_thread_join(tids[i], K_FOREVER);
	}

	/* Abort to stop timer. */
	if (timer_data != NULL) {
		ztress_abort();
		(void)k_timer_status_sync(&ztress_timer);
	}

	/* print raport */
	ztress_report();

	ztress_end(old_prio);

	return 0;
}

void ztress_abort(void)
{
	atomic_set(&active_cnt, 0);
}

void ztress_set_timeout(k_timeout_t t)
{
	timeout = t;
}

void ztress_report(void)
{
	printk("\nZtress execution report:\n");
	for (uint32_t i = 0; i < context_cnt; i++) {
		printk("\t context %u:\n\t\t - executed:%u, preempted:%u\n",
			i, exec_cnt[i], preempt_cnt[i]);
		printk("\t\t - ticks initial:%u, optimized:%u\n",
			(uint32_t)init_backoff[i].ticks, (uint32_t)backoff[i].ticks);
	}

	printk("\tAvarage CPU load:%u%%, measurements:%u\n",
			rt.cpu_load / 10, rt.cpu_load_measurements);
}

int ztress_exec_count(uint32_t id)
{
	if (id >= context_cnt) {
		return -EINVAL;
	}

	return exec_cnt[id];
}

int ztress_preempt_count(uint32_t id)
{
	if (id >= context_cnt) {
		return -EINVAL;
	}

	return preempt_cnt[id];
}

uint32_t ztress_optimized_ticks(uint32_t id)
{
	if (id >= context_cnt) {
		return -EINVAL;
	}

	return backoff[id].ticks;
}
