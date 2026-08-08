/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/debug/debugpoint_internal.h>
#include <zephyr/debug/watchpoint.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>
#if defined(CONFIG_RISCV)
#include <zephyr/arch/riscv/csr.h>
#endif
#include <zephyr/sys/atomic.h>
#include <stdint.h>
#include <limits.h>

static volatile uint32_t g_watched __aligned(4);
static volatile uint32_t g_unwatched;
static volatile uint32_t g_sink;
static volatile uint8_t g_bytes[16] __aligned(8);
static volatile uint8_t g_multi[2] __aligned(2);
static volatile uint32_t g_timing_value __aligned(4);
static volatile uint32_t g_timing_seen;
#if defined(CONFIG_USERSPACE)
#define USER_STACK_SIZE 2048
K_APPMEM_PARTITION_DEFINE(watchpoint_user_partition);
K_APP_DMEM(watchpoint_user_partition)
static volatile uint32_t g_user_watched __aligned(4);
static K_THREAD_STACK_DEFINE(g_user_stack, USER_STACK_SIZE);
static struct k_thread g_user_thread;
static struct k_mem_domain g_user_domain;
#endif

static atomic_t g_cb_count;
static void *g_cb_pc;
static void *g_cb_addr;
static bool g_cb_addr_valid;
static uint32_t g_cb_flags;
static enum k_watchpoint_timing g_cb_timing;
static bool g_cb_rearm_required;
static unsigned int g_cb_cpu;
#if defined(CONFIG_WATCHPOINT_CALLSTACK)
static uintptr_t g_cb_callstack[CONFIG_WATCHPOINT_CALLSTACK_DEPTH];
static size_t g_cb_callstack_depth;
#endif

static void reset_callback_state(void)
{
	atomic_set(&g_cb_count, 0);
	g_cb_pc = NULL;
	g_cb_addr = NULL;
	g_cb_addr_valid = false;
	g_cb_flags = 0U;
	g_cb_timing = K_WATCHPOINT_TIMING_UNKNOWN;
	g_cb_rearm_required = false;
	g_cb_cpu = UINT_MAX;
#if defined(CONFIG_WATCHPOINT_CALLSTACK)
	g_cb_callstack_depth = 0U;
#endif
}

static void callback(const struct k_watchpoint *wp,
		     const struct k_watchpoint_event *event, void *arg)
{
	ARG_UNUSED(wp);
	ARG_UNUSED(arg);

	atomic_inc(&g_cb_count);
	g_cb_pc = event->pc;
	g_cb_addr = event->access_addr;
	g_cb_addr_valid = event->access_addr_valid;
	g_cb_flags = event->flags;
	g_cb_timing = event->timing;
	g_cb_rearm_required = event->rearm_required;
	g_cb_cpu = arch_curr_cpu()->id;
#if defined(CONFIG_WATCHPOINT_CALLSTACK)
	g_cb_callstack_depth = MIN(event->callstack_depth,
				   ARRAY_SIZE(g_cb_callstack));
	if (event->callstack == NULL) {
		g_cb_callstack_depth = 0U;
	}
	for (size_t i = 0; i < g_cb_callstack_depth; i++) {
		g_cb_callstack[i] = event->callstack[i];
	}
#endif
}

#if defined(CONFIG_WATCHPOINT_CALLSTACK)
static void print_saved_callstack(void)
{
	printk("watchpoint callstack depth=%zu\n", g_cb_callstack_depth);
	for (size_t i = 0; i < g_cb_callstack_depth; i++) {
		printk("  #%zu %p\n", i, (void *)g_cb_callstack[i]);
	}
}
#endif

static void timing_callback(const struct k_watchpoint *wp,
			    const struct k_watchpoint_event *event, void *arg)
{
	g_timing_seen = g_timing_value;
	callback(wp, event, arg);
}

static void rearm_if_needed(struct k_watchpoint *wp)
{
	if (!k_watchpoint_is_active(wp)) {
		zassert_ok(k_watchpoint_add(wp), "one-shot re-arm failed");
	}
}

static void wp_add_or_skip(struct k_watchpoint *wp)
{
	int ret = k_watchpoint_add(wp);

	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(ret, "k_watchpoint_add failed: %d", ret);
}

ZTEST_SUITE(watchpoint, NULL, NULL, NULL, NULL, NULL);

ZTEST(watchpoint, test_write_fires)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	g_watched = 0U;
	wp_add_or_skip(&wp);

	g_watched = 0xDEADBEEFU;
	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_equal(g_cb_flags, K_WATCHPOINT_WRITE);
	zassert_not_null(g_cb_pc);
	zassert_equal(g_cb_rearm_required,
		      !k_watchpoint_is_active(&wp));
#if defined(CONFIG_RISCV)
	zassert_equal(g_cb_rearm_required,
		      g_cb_timing != K_WATCHPOINT_TIMING_AFTER);
#endif
	if (g_cb_addr_valid) {
		zassert_equal(g_cb_addr, (void *)&g_watched);
	}
#if defined(CONFIG_WATCHPOINT_CALLSTACK)
	zassert_true(g_cb_callstack_depth > 1U);
	zassert_equal(g_cb_callstack[0], (uintptr_t)g_cb_pc);
	print_saved_callstack();
#endif

	if (g_cb_rearm_required) {
		zassert_false(k_watchpoint_is_active(&wp));
		g_watched = 0xA5A5A5A5U;
		zassert_equal(atomic_get(&g_cb_count), 1);
	}
	rearm_if_needed(&wp);
	g_watched = 0xCAFEBABEU;
	zassert_equal(atomic_get(&g_cb_count), 2);
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_access_and_following_instruction_complete)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	g_watched = 0U;
	g_unwatched = 0U;
	wp_add_or_skip(&wp);

	g_watched = 0x13579BDFU;
	g_unwatched = 0x2468ACE0U;

	zassert_equal(g_watched, 0x13579BDFU);
	zassert_equal(g_unwatched, 0x2468ACE0U);
	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_reported_timing_matches_memory_state)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_timing_value,
				   sizeof(g_timing_value),
				   K_WATCHPOINT_WRITE, timing_callback, NULL);
	const uint32_t old_value = 0x12345678U;
	const uint32_t new_value = 0x89ABCDEFU;

	reset_callback_state();
	g_timing_value = old_value;
	g_timing_seen = 0U;
	wp_add_or_skip(&wp);

	g_timing_value = new_value;
	zassert_equal(atomic_get(&g_cb_count), 1);
	if (g_cb_timing == K_WATCHPOINT_TIMING_BEFORE) {
		zassert_equal(g_timing_seen, old_value);
	} else if (g_cb_timing == K_WATCHPOINT_TIMING_AFTER) {
		zassert_equal(g_timing_seen, new_value);
	} else {
		zassert_equal(g_cb_timing, K_WATCHPOINT_TIMING_UNKNOWN);
	}
	zassert_equal(g_timing_value, new_value);
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_read_fires)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_bytes[1], 1U,
				   K_WATCHPOINT_READ, callback, NULL);

	reset_callback_state();
	g_bytes[1] = 0x5AU;
	wp_add_or_skip(&wp);

	g_sink = g_bytes[1];
	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_equal(g_cb_flags, K_WATCHPOINT_READ);
	zassert_not_null(g_cb_pc);
	if (g_cb_addr_valid) {
		zassert_equal(g_cb_addr, (void *)&g_bytes[1]);
	}

	rearm_if_needed(&wp);
	g_sink = g_bytes[1];
	zassert_equal(atomic_get(&g_cb_count), 2);
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_read_write_fires)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_bytes[2], 1U,
				   K_WATCHPOINT_RW, callback, NULL);

	reset_callback_state();
	g_bytes[2] = 0U;
	wp_add_or_skip(&wp);

	g_bytes[2] = 0xA5U;
	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_true((g_cb_flags & K_WATCHPOINT_WRITE) != 0U);

	rearm_if_needed(&wp);
	g_sink = g_bytes[2];
	zassert_equal(atomic_get(&g_cb_count), 2);
	zassert_true((g_cb_flags & K_WATCHPOINT_READ) != 0U);
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_byte_range)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_bytes[1], 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	g_bytes[0] = 0U;
	g_bytes[1] = 0U;
	wp_add_or_skip(&wp);

	g_bytes[0] = 0x11U;
	zassert_equal(atomic_get(&g_cb_count), 0);
	g_bytes[1] = 0x22U;
	zassert_equal(atomic_get(&g_cb_count), 1);

	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_exact_range_or_not_supported)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_bytes[0], 4U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	int ret = k_watchpoint_add(&wp);

	if (ret == -ENOTSUP) {
		return;
	}
	zassert_ok(ret);
	g_bytes[4] = 0x11U;
	zassert_equal(atomic_get(&g_cb_count), 0);
	g_bytes[3] = 0x5AU;
	zassert_equal(atomic_get(&g_cb_count), 1);

	rearm_if_needed(&wp);
	g_bytes[0] = 0xA5U;
	zassert_equal(atomic_get(&g_cb_count), 2);
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_cross_granule_range_or_not_supported)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_bytes[7], 2U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	int ret = k_watchpoint_add(&wp);

	if (ret == -ENOTSUP) {
		return;
	}
	zassert_ok(ret);
	g_bytes[7] = 0x3CU;
	zassert_equal(atomic_get(&g_cb_count), 1);
	rearm_if_needed(&wp);
	g_bytes[8] = 0xC3U;
	zassert_equal(atomic_get(&g_cb_count), 2);
	zassert_ok(k_watchpoint_remove(&wp));
}
ZTEST(watchpoint, test_remove_stops)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	wp_add_or_skip(&wp);
	g_watched = 1U;
	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_ok(k_watchpoint_remove(&wp));

	g_watched = 2U;
	g_watched = 3U;
	zassert_equal(atomic_get(&g_cb_count), 1);
}

ZTEST(watchpoint, test_unwatched_no_fire)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	wp_add_or_skip(&wp);
	g_unwatched = 0x12345678U;
	zassert_equal(atomic_get(&g_cb_count), 0);
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_double_add_busy)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	wp_add_or_skip(&wp);
	zassert_equal(k_watchpoint_add(&wp), -EBUSY);
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_remove_idempotent)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	zassert_ok(k_watchpoint_remove(&wp));
	reset_callback_state();
	wp_add_or_skip(&wp);
	g_watched = 99U;
	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_ok(k_watchpoint_remove(&wp));
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_descriptor_retarget_after_remove)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_bytes[0], 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	wp.addr = (void *)&g_bytes[0];
	wp_add_or_skip(&wp);
	zassert_ok(k_watchpoint_remove(&wp));

	wp.addr = (void *)&g_bytes[1];
	zassert_ok(k_watchpoint_add(&wp));
	g_bytes[0] = 0x12U;
	zassert_equal(atomic_get(&g_cb_count), 0);
	g_bytes[1] = 0x34U;
	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_ok(k_watchpoint_remove(&wp));
}

static void count_callback(const struct k_watchpoint *wp,
			   const struct k_watchpoint_event *event, void *arg)
{
	ARG_UNUSED(wp);
	ARG_UNUSED(event);

	atomic_inc((atomic_t *)arg);
}

static void debugpoint_count_callback(const struct z_debugpoint_config *config,
				      const struct z_debugpoint_event *event,
				      void *user_data)
{
	ARG_UNUSED(config);
	ARG_UNUSED(event);

	atomic_inc((atomic_t *)user_data);
}

ZTEST(watchpoint, test_debugpoint_stale_handle_rejected)
{
	static atomic_t count;
	struct z_debugpoint_config config = {
		.type = Z_DEBUGPOINT_WATCH_WRITE,
		.addr = (void *)&g_watched,
		.size = 1U,
		.callback = debugpoint_count_callback,
		.user_data = &count,
	};
	z_debugpoint_handle_t first = Z_DEBUGPOINT_HANDLE_INVALID;
	z_debugpoint_handle_t second = Z_DEBUGPOINT_HANDLE_INVALID;

	atomic_set(&count, 0);
	g_watched = 0U;
	int ret = z_debugpoint_add(&config, &first);

	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(ret);
	zassert_not_equal(first, Z_DEBUGPOINT_HANDLE_INVALID);
	zassert_ok(z_debugpoint_remove(first));

	zassert_ok(z_debugpoint_add(&config, &second));
	zassert_not_equal(second, Z_DEBUGPOINT_HANDLE_INVALID);
	zassert_not_equal(second, first);

	/* A stale remove and delayed hit must not affect the reused slot. */
	zassert_ok(z_debugpoint_remove(first));
	struct z_debugpoint_event stale_event = {
		.type = Z_DEBUGPOINT_WATCH_WRITE,
		.timing = Z_DEBUGPOINT_TIMING_AFTER,
	};

	z_debugpoint_hit(first, &stale_event);
	zassert_equal(atomic_get(&count), 0);

	g_watched = 1U;
	zassert_equal(atomic_get(&count), 1);
	zassert_ok(z_debugpoint_remove(second));
}

ZTEST(watchpoint, test_multiple_watchpoints)
{
	static atomic_t first_count;
	static atomic_t second_count;
	static K_WATCHPOINT_DEFINE(first, (void *)&g_multi[0], 1U,
				   K_WATCHPOINT_WRITE, count_callback,
				   &first_count);
	static K_WATCHPOINT_DEFINE(second, (void *)&g_multi[1], 1U,
				   K_WATCHPOINT_WRITE, count_callback,
				   &second_count);

	atomic_set(&first_count, 0);
	atomic_set(&second_count, 0);
	wp_add_or_skip(&first);

	int ret = k_watchpoint_add(&second);

	if (ret == -ENOSPC || ret == -ENOTSUP) {
		zassert_ok(k_watchpoint_remove(&first));
		return;
	}
	zassert_ok(ret);
	g_multi[0] = 1U;
	g_multi[1] = 2U;
	rearm_if_needed(&first);
	rearm_if_needed(&second);
	g_multi[0] = 3U;

	zassert_equal(atomic_get(&first_count), 2);
	zassert_equal(atomic_get(&second_count), 1);
	zassert_ok(k_watchpoint_remove(&first));
	zassert_ok(k_watchpoint_remove(&second));
}

static volatile uint8_t g_nested_first;
static volatile uint8_t g_nested_second;
static atomic_t g_nested_first_count;
static atomic_t g_nested_second_count;
static atomic_t g_nested_callback_active;
static atomic_t g_nested_reentrant;

static void nested_access_callback(const struct k_watchpoint *wp,
				   const struct k_watchpoint_event *event,
				   void *arg)
{
	ARG_UNUSED(wp);
	ARG_UNUSED(event);
	ARG_UNUSED(arg);

	atomic_inc(&g_nested_first_count);
	atomic_set(&g_nested_callback_active, 1);
	g_nested_second = 0xA5U;
	atomic_set(&g_nested_callback_active, 0);
}

static void nested_second_callback(const struct k_watchpoint *wp,
				   const struct k_watchpoint_event *event,
				   void *arg)
{
	ARG_UNUSED(wp);
	ARG_UNUSED(event);
	ARG_UNUSED(arg);

	if (atomic_get(&g_nested_callback_active) != 0) {
		atomic_set(&g_nested_reentrant, 1);
	}
	atomic_inc(&g_nested_second_count);
}

ZTEST(watchpoint, test_callback_access_does_not_recurse)
{
	static K_WATCHPOINT_DEFINE(first, (void *)&g_nested_first, 1U,
				   K_WATCHPOINT_WRITE,
				   nested_access_callback, NULL);
	static K_WATCHPOINT_DEFINE(second, (void *)&g_nested_second, 1U,
				   K_WATCHPOINT_WRITE,
				   nested_second_callback, NULL);

	atomic_set(&g_nested_first_count, 0);
	atomic_set(&g_nested_second_count, 0);
	atomic_set(&g_nested_callback_active, 0);
	atomic_set(&g_nested_reentrant, 0);
	wp_add_or_skip(&first);

	int ret = k_watchpoint_add(&second);

	if (ret == -ENOSPC || ret == -ENOTSUP) {
		zassert_ok(k_watchpoint_remove(&first));
		return;
	}
	zassert_ok(ret);

	zassert_equal(atomic_get(&g_nested_second_count), 0);
	g_nested_first = 0x5AU;
	zassert_equal(atomic_get(&g_nested_first_count), 1);
	zassert_equal(atomic_get(&g_nested_reentrant), 0,
		      "second callback was recursive");

	int second_count = atomic_get(&g_nested_second_count);

	zassert_true(second_count == 0 || second_count == 1);
	rearm_if_needed(&second);
	g_nested_second = 0x3CU;
	zassert_equal(atomic_get(&g_nested_second_count), second_count + 1);
	zassert_ok(k_watchpoint_remove(&first));
	zassert_ok(k_watchpoint_remove(&second));
}

ZTEST(watchpoint, test_invalid_args)
{
	static K_WATCHPOINT_DEFINE(zero_size, (void *)&g_watched, 0U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	static K_WATCHPOINT_DEFINE(no_flag, (void *)&g_watched, 1U,
				   0U, callback, NULL);
	static K_WATCHPOINT_DEFINE(bad_flags, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE | BIT(8), callback, NULL);
	static K_WATCHPOINT_DEFINE(no_callback, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, NULL, NULL);
	static K_WATCHPOINT_DEFINE(overflow, (void *)UINTPTR_MAX, 2U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	zassert_equal(k_watchpoint_add(NULL), -EINVAL);
	zassert_equal(k_watchpoint_add(&zero_size), -EINVAL);
	zassert_equal(k_watchpoint_add(&no_flag), -EINVAL);
	zassert_equal(k_watchpoint_add(&bad_flags), -EINVAL);
	zassert_equal(k_watchpoint_add(&no_callback), -EINVAL);
	zassert_equal(k_watchpoint_add(&overflow), -EINVAL);
	zassert_equal(k_watchpoint_remove(NULL), -EINVAL);
	zassert_false(k_watchpoint_is_active(NULL));
}

ZTEST(watchpoint, test_zero_address_is_valid)
{
	static K_WATCHPOINT_DEFINE(wp, NULL, 1U, K_WATCHPOINT_WRITE,
				   callback, NULL);
	int ret = k_watchpoint_add(&wp);

	if (ret == -ENOTSUP) {
		return;
	}
	zassert_ok(ret);
	zassert_true(k_watchpoint_is_active(&wp));
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_zero_initialized_descriptor)
{
	static struct k_watchpoint wp = {
		.addr = (void *)&g_watched,
		.size = sizeof(g_watched),
		.flags = K_WATCHPOINT_WRITE,
		.cb = callback,
	};

	int ret = k_watchpoint_add(&wp);

	if (ret == -ENOTSUP) {
		return;
	}
	zassert_ok(ret);
	zassert_true(k_watchpoint_is_active(&wp));
	zassert_ok(k_watchpoint_remove(&wp));
}

static atomic_t g_callback_add_ret;
static atomic_t g_callback_remove_ret;
static atomic_t g_callback_reject_count;
static K_WATCHPOINT_DEFINE(g_callback_nested_wp, (void *)&g_unwatched, 1U,
			   K_WATCHPOINT_WRITE, callback, NULL);

static void api_rejecting_callback(const struct k_watchpoint *wp,
				   const struct k_watchpoint_event *event,
				   void *arg)
{
	ARG_UNUSED(event);
	ARG_UNUSED(arg);

	atomic_inc(&g_callback_reject_count);
	atomic_set(&g_callback_add_ret,
		   k_watchpoint_add(&g_callback_nested_wp));
	atomic_set(&g_callback_remove_ret,
		   k_watchpoint_remove((struct k_watchpoint *)wp));
}

ZTEST(watchpoint, test_callback_context_rejected)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE,
				   api_rejecting_callback, NULL);

	atomic_set(&g_callback_nested_wp._state, 0);
	atomic_set(&g_callback_add_ret, 0);
	atomic_set(&g_callback_remove_ret, 0);
	atomic_set(&g_callback_reject_count, 0);
	wp_add_or_skip(&wp);
	g_watched = 0x10203040U;

	int add_ret = atomic_get(&g_callback_add_ret);
	int remove_ret = atomic_get(&g_callback_remove_ret);

	if (k_watchpoint_is_active(&g_callback_nested_wp)) {
		zassert_ok(k_watchpoint_remove(&g_callback_nested_wp));
	}
	zassert_equal(atomic_get(&g_callback_reject_count), 1,
		      "callback did not run");
	zassert_equal(add_ret, -EWOULDBLOCK,
		      "callback add returned %d", add_ret);
	zassert_equal(remove_ret, -EWOULDBLOCK,
		      "callback remove returned %d", remove_ret);
	zassert_ok(k_watchpoint_remove(&wp));
}

#if defined(CONFIG_DEBUGPOINT)
#define RESOURCE_TEST_COUNT (CONFIG_DEBUGPOINT_MAX_SLOTS + 1)
#else
#define RESOURCE_TEST_COUNT 1
#endif

ZTEST(watchpoint, test_resource_exhaustion_and_reuse)
{
	static volatile uint8_t targets[RESOURCE_TEST_COUNT]
		__aligned(16);
	struct k_watchpoint watchpoints[RESOURCE_TEST_COUNT];
	static atomic_t unexpected_hits;
	int armed = 0;
	int failure = 0;

	atomic_set(&unexpected_hits, 0);
	for (int i = 0; i < ARRAY_SIZE(watchpoints); i++) {
		watchpoints[i] = (struct k_watchpoint)
			K_WATCHPOINT_INITIALIZER((void *)&targets[i], 1U,
						 K_WATCHPOINT_WRITE,
						 count_callback,
						 &unexpected_hits);
		int ret = k_watchpoint_add(&watchpoints[i]);

		if (ret != 0) {
			failure = ret;
			break;
		}
		armed++;
	}

	if (armed == 0 && failure == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_true(armed > 0);
	zassert_true(failure == -ENOSPC || failure == -ENOTSUP);

	zassert_ok(k_watchpoint_remove(&watchpoints[0]));
	zassert_ok(k_watchpoint_add(&watchpoints[armed]));
	zassert_ok(k_watchpoint_remove(&watchpoints[armed]));

	for (int i = 1; i < armed; i++) {
		zassert_ok(k_watchpoint_remove(&watchpoints[i]));
	}
	zassert_equal(atomic_get(&unexpected_hits), 0);
}

#if defined(CONFIG_RISCV) && defined(CONFIG_DEBUGPOINT)
extern int z_riscv_debugpoint_handle(struct arch_esf *esf);

static const uint16_t g_ebreak32[] __aligned(2) = {0x0073U, 0x0010U};
static const uint16_t g_ebreak16 __aligned(2) = 0x9002U;

ZTEST(watchpoint, test_riscv_unrepresentable_ranges)
{
	static K_WATCHPOINT_DEFINE(non_power_of_two, (void *)&g_bytes[0], 3U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	static K_WATCHPOINT_DEFINE(unaligned, (void *)&g_bytes[1], 2U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	zassert_equal(k_watchpoint_add(&non_power_of_two), -ENOTSUP);
	zassert_equal(k_watchpoint_add(&unaligned), -ENOTSUP);
}

ZTEST(watchpoint, test_riscv_overlap_rejected)
{
	static K_WATCHPOINT_DEFINE(first, (void *)&g_bytes[0], 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	static K_WATCHPOINT_DEFINE(second, (void *)&g_bytes[0], 1U,
				   K_WATCHPOINT_READ, callback, NULL);

	wp_add_or_skip(&first);
	zassert_equal(k_watchpoint_add(&second), -ENOTSUP);
	zassert_ok(k_watchpoint_remove(&first));
}

ZTEST(watchpoint, test_riscv_ebreak_not_consumed)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	static K_WATCHPOINT_DEFINE(zero_wp, NULL, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	struct arch_esf esf = {0};

	wp_add_or_skip(&wp);
	esf.mepc = (uintptr_t)g_ebreak32;
	zassert_equal(z_riscv_debugpoint_handle(&esf), -ENOENT);
	esf.mepc = (uintptr_t)&g_ebreak16;
	zassert_equal(z_riscv_debugpoint_handle(&esf), -ENOENT);
	zassert_ok(k_watchpoint_remove(&wp));

	wp_add_or_skip(&zero_wp);
	esf.mepc = (uintptr_t)g_ebreak32;
	zassert_equal(z_riscv_debugpoint_handle(&esf), -ENOENT);
	esf.mepc = UINTPTR_MAX - 1U;
	zassert_equal(z_riscv_debugpoint_handle(&esf), -ENOENT);
	zassert_ok(k_watchpoint_remove(&zero_wp));
}
#endif

static K_SEM_DEFINE(g_isr_done, 0, 1);
static int g_isr_add_ret;
static int g_isr_remove_ret;
static K_WATCHPOINT_DEFINE(g_isr_wp, (void *)&g_watched, 1U,
			   K_WATCHPOINT_WRITE, callback, NULL);
static struct k_watchpoint g_fake_active_wp =
	K_WATCHPOINT_INITIALIZER((void *)&g_watched, 1U,
				 K_WATCHPOINT_WRITE, callback, NULL);

static void isr_context_timer(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	g_isr_add_ret = k_watchpoint_add(&g_isr_wp);
	g_isr_remove_ret = k_watchpoint_remove(&g_fake_active_wp);
	k_sem_give(&g_isr_done);
}

K_TIMER_DEFINE(g_isr_timer, isr_context_timer, NULL);

static K_SEM_DEFINE(g_isr_write_done, 0, 1);

static void isr_write_timer(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	g_watched = 0x76543210U;
	k_sem_give(&g_isr_write_done);
}

K_TIMER_DEFINE(g_isr_write_timer, isr_write_timer, NULL);

ZTEST(watchpoint, test_hit_from_isr)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	k_sem_reset(&g_isr_write_done);
	wp_add_or_skip(&wp);
	k_timer_start(&g_isr_write_timer, K_NO_WAIT, K_NO_WAIT);

	zassert_ok(k_sem_take(&g_isr_write_done, K_MSEC(500)));
	zexpect_equal(atomic_get(&g_cb_count), 1);
	zexpect_equal(g_watched, 0x76543210U);
	zexpect_not_null(g_cb_pc);
	zexpect_equal(g_cb_rearm_required,
		      !k_watchpoint_is_active(&wp));
#if defined(CONFIG_RISCV)
	zexpect_equal(g_cb_rearm_required,
		      g_cb_timing != K_WATCHPOINT_TIMING_AFTER);
#endif
#if defined(CONFIG_WATCHPOINT_CALLSTACK)
	zexpect_true(g_cb_callstack_depth > 0U);
	if (g_cb_callstack_depth > 0U) {
		zexpect_equal(g_cb_callstack[0], (uintptr_t)g_cb_pc);
	}
#endif
	zassert_ok(k_watchpoint_remove(&wp));
}

ZTEST(watchpoint, test_isr_context_rejected)
{
	atomic_set(&g_isr_wp._state, 0);
	g_isr_add_ret = 0;
	g_isr_remove_ret = 0;

	k_timer_start(&g_isr_timer, K_NO_WAIT, K_NO_WAIT);
	zassert_ok(k_sem_take(&g_isr_done, K_MSEC(500)));
	zassert_equal(g_isr_add_ret, -EWOULDBLOCK);
	zassert_equal(g_isr_remove_ret, -EWOULDBLOCK);
}

ZTEST(watchpoint, test_irq_locked_context_rejected)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	static struct k_watchpoint fake_active =
		K_WATCHPOINT_INITIALIZER((void *)&g_watched, 1U,
				 K_WATCHPOINT_WRITE, callback, NULL);

	atomic_set(&wp._state, 0);
	unsigned int key = irq_lock();
	int add_ret = k_watchpoint_add(&wp);
	int remove_ret = k_watchpoint_remove(&fake_active);

	irq_unlock(key);
	zassert_equal(add_ret, -EWOULDBLOCK);
	zassert_equal(remove_ret, -EWOULDBLOCK);
}

#if defined(CONFIG_USERSPACE)
static void user_writer_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	g_user_watched = 0xA55AA55AU;
}

ZTEST(watchpoint, test_user_mode_write_fires)
{
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_user_watched,
				   sizeof(g_user_watched),
				   K_WATCHPOINT_WRITE, callback, NULL);
	struct k_mem_partition *partitions[] = {
		&watchpoint_user_partition,
	};

	k_mem_domain_init(&g_user_domain, ARRAY_SIZE(partitions), partitions);
	reset_callback_state();
	g_user_watched = 0U;
	wp_add_or_skip(&wp);

	k_tid_t tid = k_thread_create(&g_user_thread, g_user_stack,
				      USER_STACK_SIZE, user_writer_fn,
				      NULL, NULL, NULL, K_PRIO_PREEMPT(7),
				      K_USER, K_FOREVER);

	k_mem_domain_add_thread(&g_user_domain, tid);
	k_thread_start(tid);
	zassert_ok(k_thread_join(tid, K_MSEC(500)));
	zexpect_equal(g_user_watched, 0xA55AA55AU);
	zexpect_equal(atomic_get(&g_cb_count), 1);
#if defined(CONFIG_WATCHPOINT_CALLSTACK)
	zexpect_true(g_cb_callstack_depth > 1U);
#endif
	zassert_ok(k_watchpoint_remove(&wp));
}
#endif

#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_CPU_MASK)
#define SMP_STACK_SIZE 3072

static K_THREAD_STACK_DEFINE(g_writer_stack, SMP_STACK_SIZE);
static K_THREAD_STACK_DEFINE(g_remover_stack, SMP_STACK_SIZE);
static K_THREAD_STACK_DEFINE(g_adder_stack, SMP_STACK_SIZE);
static struct k_thread g_writer_thread;
static struct k_thread g_remover_thread;
static struct k_thread g_adder_thread;

static void writer_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	*(volatile uint32_t *)p1 = 0xABCD1234U;
}

static int run_on_cpu(k_thread_entry_t entry, unsigned int cpu,
		      void *arg)
{
	k_tid_t tid = k_thread_create(&g_writer_thread, g_writer_stack,
				      SMP_STACK_SIZE, entry, arg, NULL, NULL,
				      K_PRIO_PREEMPT(7), 0, K_FOREVER);

	k_thread_cpu_pin(tid, cpu);
	k_thread_start(tid);
	return k_thread_join(tid, K_MSEC(500));
}

ZTEST(watchpoint, test_smp_fires_on_remote_cpu)
{
	if (arch_num_cpus() < 2) {
		ztest_test_skip();
	}

	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);

	reset_callback_state();
	wp_add_or_skip(&wp);

	unsigned int remote_cpu =
		(arch_curr_cpu()->id + 1U) % arch_num_cpus();
	k_tid_t tid = k_thread_create(&g_writer_thread, g_writer_stack,
				      SMP_STACK_SIZE, writer_fn,
				      (void *)&g_watched, NULL, NULL,
				      K_PRIO_PREEMPT(7), 0, K_FOREVER);

	k_thread_cpu_pin(tid, remote_cpu);
	k_thread_start(tid);
	zassert_ok(k_thread_join(&g_writer_thread, K_MSEC(500)));

	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_equal(g_cb_cpu, remote_cpu);

	rearm_if_needed(&wp);
	zassert_ok(run_on_cpu(writer_fn, remote_cpu, (void *)&g_watched));
	zassert_equal(atomic_get(&g_cb_count), 2);
	zassert_equal(g_cb_cpu, remote_cpu);
	zassert_ok(k_watchpoint_remove(&wp));
}

struct smp_watchpoint_request {
	struct k_watchpoint *wp;
	bool add;
	int ret;
};

static void watchpoint_api_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct smp_watchpoint_request *request = p1;

	request->ret = request->add ? k_watchpoint_add(request->wp) :
				     k_watchpoint_remove(request->wp);
}

ZTEST(watchpoint, test_smp_add_remove_from_remote_cpu)
{
	if (arch_num_cpus() < 2) {
		ztest_test_skip();
	}

	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	unsigned int coordinator_cpu = arch_curr_cpu()->id;
	unsigned int remote_cpu =
		(coordinator_cpu + 1U) % arch_num_cpus();
	struct smp_watchpoint_request request = {
		.wp = &wp,
		.add = true,
		.ret = -EINPROGRESS,
	};

	reset_callback_state();
	zassert_ok(run_on_cpu(watchpoint_api_fn, remote_cpu, &request));
	if (request.ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(request.ret);

	unsigned int key = irq_lock();
	unsigned int writer_cpu = arch_curr_cpu()->id;

	g_watched = 0x55AA55AAU;
	irq_unlock(key);
	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_equal(g_cb_cpu, writer_cpu);

	request.add = false;
	request.ret = -EINPROGRESS;
	zassert_ok(run_on_cpu(watchpoint_api_fn, remote_cpu, &request));
	zassert_ok(request.ret);

	g_watched = 0xAA55AA55U;
	zassert_equal(atomic_get(&g_cb_count), 1);
}

struct concurrent_add_request {
	struct k_watchpoint *wp;
	struct k_sem *ready;
	struct k_sem *start;
	int ret;
};

static void concurrent_add_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct concurrent_add_request *request = p1;

	k_sem_give(request->ready);
	if (k_sem_take(request->start, K_MSEC(500)) != 0) {
		request->ret = -ETIMEDOUT;
		return;
	}

	request->ret = k_watchpoint_add(request->wp);
}

ZTEST(watchpoint, test_smp_concurrent_add_serialized)
{
	if (arch_num_cpus() < 2) {
		ztest_test_skip();
	}

	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	struct k_sem ready;
	struct k_sem start;
	struct concurrent_add_request first = {
		.wp = &wp,
		.ready = &ready,
		.start = &start,
		.ret = -EINPROGRESS,
	};
	struct concurrent_add_request second = {
		.wp = &wp,
		.ready = &ready,
		.start = &start,
		.ret = -EINPROGRESS,
	};
	unsigned int coordinator_cpu = arch_curr_cpu()->id;
	unsigned int remote_cpu =
		(coordinator_cpu + 1U) % arch_num_cpus();

	wp_add_or_skip(&wp);
	zassert_ok(k_watchpoint_remove(&wp));
	k_sem_init(&ready, 0, 2);
	k_sem_init(&start, 0, 2);

	k_tid_t first_tid = k_thread_create(
		&g_remover_thread, g_remover_stack, SMP_STACK_SIZE,
		concurrent_add_fn, &first, NULL, NULL,
		k_thread_priority_get(k_current_get()), 0, K_FOREVER);
	k_tid_t second_tid = k_thread_create(
		&g_adder_thread, g_adder_stack, SMP_STACK_SIZE,
		concurrent_add_fn, &second, NULL, NULL,
		k_thread_priority_get(k_current_get()), 0, K_FOREVER);

	k_thread_cpu_pin(first_tid, coordinator_cpu);
	k_thread_cpu_pin(second_tid, remote_cpu);
	k_thread_start(first_tid);
	k_thread_start(second_tid);
	zassert_ok(k_sem_take(&ready, K_MSEC(500)));
	zassert_ok(k_sem_take(&ready, K_MSEC(500)));
	k_sem_give(&start);
	k_sem_give(&start);
	zassert_ok(k_thread_join(first_tid, K_MSEC(2000)));
	zassert_ok(k_thread_join(second_tid, K_MSEC(2000)));

	zassert_true((first.ret == 0 && second.ret == -EBUSY) ||
		     (first.ret == -EBUSY && second.ret == 0),
		     "concurrent add returned %d and %d",
		     first.ret, second.ret);
	zassert_true(k_watchpoint_is_active(&wp));
	zassert_ok(k_watchpoint_remove(&wp));
}

#if defined(CONFIG_RISCV)
#define TEST_CSR_TSELECT 0x7a0
#define TEST_CSR_TDATA1  0x7a1
#define TEST_CSR_TDATA2  0x7a2

#define TEST_TDATA1_LOAD          BIT(0)
#define TEST_TDATA1_STORE         BIT(1)
#define TEST_TDATA1_EXECUTE       BIT(2)
#define TEST_TDATA1_M             BIT(6)
#define TEST_TDATA1_TIMING        BIT(18)
#define TEST_TDATA1_DMODE         BIT(__riscv_xlen - 5)
#define TEST_TDATA1_TYPE_SHIFT    (__riscv_xlen - 4)
#define TEST_TDATA1_TYPE_MASK     (0xfUL << TEST_TDATA1_TYPE_SHIFT)
#define TEST_TDATA1_MCONTROL      (2UL << TEST_TDATA1_TYPE_SHIFT)
#define TEST_TDATA1_MCONTROL6     (6UL << TEST_TDATA1_TYPE_SHIFT)

struct trigger_backup {
	uintptr_t tdata1;
	uintptr_t tdata2;
	int ret;
};

static struct trigger_backup g_trigger_backup;
static volatile uint32_t g_foreign_target __aligned(16);

static void reserve_foreign_trigger(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uintptr_t saved_tselect = csr_read_imm(TEST_CSR_TSELECT);

	csr_write_imm(TEST_CSR_TSELECT, 0);
	if (csr_read_imm(TEST_CSR_TSELECT) != 0U) {
		g_trigger_backup.ret = -ENOTSUP;
		csr_write_imm(TEST_CSR_TSELECT, saved_tselect);
		return;
	}

	uintptr_t tdata1 = csr_read_imm(TEST_CSR_TDATA1);
	uintptr_t type = tdata1 & TEST_TDATA1_TYPE_MASK;

	g_trigger_backup.tdata1 = tdata1;
	g_trigger_backup.tdata2 = csr_read_imm(TEST_CSR_TDATA2);
	if ((tdata1 & (TEST_TDATA1_DMODE | TEST_TDATA1_LOAD |
		       TEST_TDATA1_STORE | TEST_TDATA1_EXECUTE)) != 0U ||
	    (type != TEST_TDATA1_MCONTROL &&
	     type != TEST_TDATA1_MCONTROL6)) {
		g_trigger_backup.ret = -EBUSY;
		csr_write_imm(TEST_CSR_TSELECT, saved_tselect);
		return;
	}

	uintptr_t requested = type | TEST_TDATA1_M | TEST_TDATA1_STORE;

	if (type == TEST_TDATA1_MCONTROL) {
		requested |= TEST_TDATA1_TIMING;
	}

	csr_write_imm(TEST_CSR_TDATA1, 0);
	csr_write_imm(TEST_CSR_TDATA2, (uintptr_t)&g_foreign_target);
	csr_write_imm(TEST_CSR_TDATA1, requested);

	uintptr_t actual = csr_read_imm(TEST_CSR_TDATA1);

	if ((actual & (TEST_TDATA1_TYPE_MASK | TEST_TDATA1_STORE)) !=
		    (requested &
		     (TEST_TDATA1_TYPE_MASK | TEST_TDATA1_STORE)) ||
	    csr_read_imm(TEST_CSR_TDATA2) !=
		    (uintptr_t)&g_foreign_target) {
		csr_write_imm(TEST_CSR_TDATA1, 0);
		csr_write_imm(TEST_CSR_TDATA2, g_trigger_backup.tdata2);
		csr_write_imm(TEST_CSR_TDATA1, g_trigger_backup.tdata1);
		g_trigger_backup.ret = -ENOTSUP;
	} else {
		g_trigger_backup.ret = 0;
	}

	csr_write_imm(TEST_CSR_TSELECT, saved_tselect);
}

static void restore_foreign_trigger(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uintptr_t saved_tselect = csr_read_imm(TEST_CSR_TSELECT);

	csr_write_imm(TEST_CSR_TSELECT, 0);
	uintptr_t current = csr_read_imm(TEST_CSR_TDATA1);
	bool preserved =
		(current & (TEST_TDATA1_TYPE_MASK | TEST_TDATA1_STORE)) ==
		((g_trigger_backup.tdata1 & TEST_TDATA1_TYPE_MASK) |
		 TEST_TDATA1_STORE);
	preserved = preserved &&
		    csr_read_imm(TEST_CSR_TDATA2) ==
			    (uintptr_t)&g_foreign_target;

	csr_write_imm(TEST_CSR_TDATA1, 0);
	csr_write_imm(TEST_CSR_TDATA2, g_trigger_backup.tdata2);
	csr_write_imm(TEST_CSR_TDATA1, g_trigger_backup.tdata1);
	csr_write_imm(TEST_CSR_TSELECT, saved_tselect);
	g_trigger_backup.ret = preserved ? 0 : -EIO;
}

ZTEST(watchpoint, test_riscv_smp_remaps_around_foreign_trigger)
{
	if (arch_num_cpus() < 2) {
		ztest_test_skip();
	}

	static K_WATCHPOINT_DEFINE(flush, (void *)&g_unwatched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	static K_WATCHPOINT_DEFINE(wp, (void *)&g_watched, 1U,
				   K_WATCHPOINT_WRITE, callback, NULL);
	unsigned int remote_cpu =
		(arch_curr_cpu()->id + 1U) % arch_num_cpus();

	wp_add_or_skip(&flush);
	zassert_ok(k_watchpoint_remove(&flush));

	g_trigger_backup.ret = -EINPROGRESS;
	zassert_ok(run_on_cpu(reserve_foreign_trigger, remote_cpu, NULL));
	if (g_trigger_backup.ret != 0) {
		ztest_test_skip();
	}

	reset_callback_state();
	int add_ret = k_watchpoint_add(&wp);
	int writer_ret = 0;
	int remove_ret = 0;

	if (add_ret == 0) {
		writer_ret = run_on_cpu(writer_fn, remote_cpu,
					       (void *)&g_watched);
		remove_ret = k_watchpoint_remove(&wp);
	}

	int restore_ret =
		run_on_cpu(restore_foreign_trigger, remote_cpu, NULL);
	int preserve_ret = g_trigger_backup.ret;

	zassert_ok(restore_ret);
	zassert_ok(preserve_ret, "foreign trigger was modified");
	zassert_ok(add_ret);
	zassert_ok(writer_ret);
	zassert_ok(remove_ret);
	zassert_equal(atomic_get(&g_cb_count), 1);
	zassert_equal(g_cb_cpu, remote_cpu);
}
#endif
static atomic_t g_blocking_entered;
static atomic_t g_blocking_release;
static atomic_t g_remove_early;
static atomic_t g_remove_started;
static int g_remove_ret;
static int g_readd_ret;
static volatile uint32_t g_blocked __aligned(4);
static volatile uint32_t g_readd_target __aligned(4);
static K_WATCHPOINT_DEFINE(g_blocking_wp, (void *)&g_blocked, 1U,
			   K_WATCHPOINT_WRITE, NULL, NULL);

static void blocking_callback(const struct k_watchpoint *wp,
			      const struct k_watchpoint_event *event, void *arg)
{
	ARG_UNUSED(wp);
	ARG_UNUSED(event);
	ARG_UNUSED(arg);

	atomic_set(&g_blocking_entered, 1);
	while (atomic_get(&g_blocking_release) == 0) {
		compiler_barrier();
	}
}


static void remover_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_set(&g_remove_started, 1);
	g_remove_ret = k_watchpoint_remove((struct k_watchpoint *)p1);
	if (atomic_get(&g_blocking_release) == 0) {
		atomic_set(&g_remove_early, 1);
	}
}

static void readd_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct k_watchpoint *wp = p1;
	int64_t deadline = k_uptime_get() + 2000;

	while (atomic_get(&wp->_state) != 0 && k_uptime_get() < deadline) {
		k_yield();
	}
	if (atomic_get(&wp->_state) != 0) {
		g_readd_ret = -ETIMEDOUT;
		return;
	}

	wp->addr = (void *)&g_readd_target;
	g_readd_ret = k_watchpoint_add(wp);
}

ZTEST(watchpoint, test_smp_remove_waits_for_callback)
{
	if (arch_num_cpus() < 2) {
		ztest_test_skip();
	}

	g_blocking_wp.addr = (void *)&g_blocked;
	g_blocking_wp.cb = blocking_callback;
	atomic_set(&g_blocking_wp._state, 0);
	atomic_set(&g_blocking_entered, 0);
	atomic_set(&g_blocking_release, 0);
	atomic_set(&g_remove_early, 0);
	atomic_set(&g_remove_started, 0);
	g_remove_ret = -EINPROGRESS;
	g_readd_ret = -EINPROGRESS;
	wp_add_or_skip(&g_blocking_wp);

	unsigned int coordinator_cpu = arch_curr_cpu()->id;
	unsigned int writer_cpu =
		(coordinator_cpu + 1U) % arch_num_cpus();
	k_tid_t writer = k_thread_create(
		&g_writer_thread, g_writer_stack, SMP_STACK_SIZE, writer_fn,
		(void *)&g_blocked, NULL, NULL,
		k_thread_priority_get(k_current_get()), 0, K_FOREVER);

	k_thread_cpu_pin(writer, writer_cpu);
	k_thread_start(writer);

	int64_t deadline = k_uptime_get() + 500;

	while (atomic_get(&g_blocking_entered) == 0 &&
	       k_uptime_get() < deadline) {
		k_yield();
	}
	zassert_equal(atomic_get(&g_blocking_entered), 1);

	k_tid_t remover = k_thread_create(
		&g_remover_thread, g_remover_stack, SMP_STACK_SIZE, remover_fn,
		&g_blocking_wp, NULL, NULL,
		k_thread_priority_get(k_current_get()), 0, K_FOREVER);

	k_thread_cpu_pin(remover, coordinator_cpu);
	k_thread_start(remover);
	while (atomic_get(&g_remove_started) == 0) {
		k_yield();
	}
	zassert_equal(g_remove_ret, -EINPROGRESS);

	k_tid_t adder = k_thread_create(
		&g_adder_thread, g_adder_stack, SMP_STACK_SIZE, readd_fn,
		&g_blocking_wp, NULL, NULL,
		k_thread_priority_get(k_current_get()), 0, K_FOREVER);

	k_thread_cpu_pin(adder, coordinator_cpu);
	k_thread_start(adder);
	atomic_set(&g_blocking_release, 1);

	zassert_ok(k_thread_join(&g_remover_thread, K_MSEC(2000)));
	zassert_ok(k_thread_join(&g_writer_thread, K_MSEC(2000)));
	zassert_ok(k_thread_join(&g_adder_thread, K_MSEC(2000)));
	zassert_ok(g_remove_ret);
	zassert_equal(atomic_get(&g_remove_early), 0);
	zassert_ok(g_readd_ret);
	zassert_true(k_watchpoint_is_active(&g_blocking_wp));
	zassert_ok(k_watchpoint_remove(&g_blocking_wp));
}
#endif
