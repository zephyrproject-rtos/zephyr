/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/applet/applet.h>

#include "applet_shared.h"

LOG_MODULE_REGISTER(test_applet, LOG_LEVEL_INF);

/*
 * A larger alignment than strictly required eases debugging: it keeps the
 * addresses inside the extension stable between otherwise similar runs.
 */
#define ELF_ALIGN __aligned(4096)

#ifdef CONFIG_APPLET_LLEXT
static const uint8_t cpu_pinning_elf[] ELF_ALIGN = {
#include <cpu_pinning.inc>
};

static const uint8_t worker_elf[] ELF_ALIGN = {
#include <worker.inc>
};
#else
extern void cpu_pinning_main(void *arg);
extern void worker_main(void *arg);
extern void applet_main(void *arg);
extern void shared_mem_main(void *arg);
#endif /* CONFIG_APPLET_LLEXT */

/*
 * An applet body is reached either through an LLEXT export table lookup or as
 * a plain function pointer, depending on the build. Tests name one of these
 * descriptors and let the harness pick the right route.
 */
struct applet_image {
	const char *sym;
#ifdef CONFIG_APPLET_LLEXT
	const uint8_t *elf;
	size_t elf_size;
#else
	k_thread_entry_t entry;
#endif
};

#ifdef CONFIG_APPLET_LLEXT
#define APPLET_IMAGE_DEFINE(_var, _blob, _fn)                                                      \
	static const struct applet_image _var = {                                                  \
		.sym = STRINGIFY(_fn), .elf = _blob, .elf_size = sizeof(_blob),                    \
	}
#else
/* Applet bodies take a single argument; the other two are never read. */
#define APPLET_IMAGE_DEFINE(_var, _blob, _fn)                                                      \
	static const struct applet_image _var = {                                                  \
		.sym = STRINGIFY(_fn), .entry = (k_thread_entry_t)_fn,                             \
	}
#endif

APPLET_IMAGE_DEFINE(img_cpu_pinning, cpu_pinning_elf, cpu_pinning_main);
APPLET_IMAGE_DEFINE(img_worker, worker_elf, worker_main);
APPLET_IMAGE_DEFINE(img_shared_mem, worker_elf, shared_mem_main);

#ifdef CONFIG_APPLET_LLEXT
/* Only the LLEXT route looks an entry point up by name. */
APPLET_IMAGE_DEFINE(img_default_entry, worker_elf, applet_main);
#endif

static struct applet applet_1;
static struct applet applet_2;

#ifdef CONFIG_APPLET_LLEXT
#ifdef CONFIG_LLEXT_STORAGE_WRITABLE
/*
 * With writable storage the loader relocates the ELF in place, so a blob can
 * only ever be loaded once. Give every load its own scratch copy of the
 * pristine image. One buffer per descriptor is enough because at most two
 * applets are loaded at the same time, and it keeps a live applet's copy from
 * being overwritten by the next load.
 */
#define ELF_SCRATCH_SIZE 4096

static uint8_t elf_scratch[2][ELF_SCRATCH_SIZE] ELF_ALIGN;

static const void *elf_storage(const struct applet *applet_inst, const void *elf, size_t elf_size)
{
	uint8_t *buf = elf_scratch[applet_inst == &applet_2 ? 1 : 0];

	zassert_true(elf_size <= ELF_SCRATCH_SIZE, "ELF does not fit in the scratch buffer");
	memcpy(buf, elf, elf_size);

	return buf;
}
#else
static inline const void *elf_storage(const struct applet *applet_inst, const void *elf,
				      size_t elf_size)
{
	ARG_UNUSED(applet_inst);
	ARG_UNUSED(elf_size);

	return elf;
}
#endif /* CONFIG_LLEXT_STORAGE_WRITABLE */
#endif /* CONFIG_APPLET_LLEXT */

/* Stacks. Pool A backs applet_1, pool B backs applet_2 when both are live. */
#define POOL_B_THREADS 2

/* Upper bound on the slot-heap walk in test_thread_slot_exhaustion(). */
#define APPLET_TEST_SLOT_LIMIT 256

APPLET_THREAD_STACK_ARRAY_DEFINE(stacks_a, APPLET_TEST_MAX_THREADS,
				 CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT);
APPLET_THREAD_STACK_ARRAY_DEFINE(stacks_b, POOL_B_THREADS, CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT);

#define POOL_STACK_SIZE K_THREAD_STACK_SIZEOF(stacks_a[0])

enum stack_pool {
	POOL_A,
	POOL_B,
};

static k_thread_stack_t *pool_stack(enum stack_pool pool, unsigned int idx)
{
	return (pool == POOL_B) ? stacks_b[idx] : stacks_a[idx];
}

/*
 * Shared buffers handed to applets through a memory partition. Under
 * CONFIG_USERSPACE these end up in their own linker section; without it the
 * macros collapse to plain globals and the same test still exercises the
 * argument-passing path.
 */
K_APPMEM_PARTITION_DEFINE(applet_part_a);
K_APP_BMEM(applet_part_a) static struct applet_test_shared shared_a;

K_APPMEM_PARTITION_DEFINE(applet_part_b);
K_APP_BMEM(applet_part_b) static struct applet_test_shared shared_b;

#ifdef CONFIG_USERSPACE
#define APPLET_TEST_PART(_p) (&(_p))
#else
#define APPLET_TEST_PART(_p) NULL
#endif

/* Helper threads used by the concurrency tests. */
#define HELPER_STACK_SIZE 1536
#define NUM_HELPERS       2

K_THREAD_STACK_ARRAY_DEFINE(helper_stacks, NUM_HELPERS, HELPER_STACK_SIZE);
static struct k_thread helper_threads[NUM_HELPERS];
static int helper_ret[NUM_HELPERS];
static atomic_t poller_run;
static atomic_t poller_iterations;

#ifndef CONFIG_APPLET_FATAL_HANDLER
static ZTEST_DMEM volatile int expected_reason = -1;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	ARG_UNUSED(pEsf);
	printk("Caught system error -- reason %d\n", reason);

	if (expected_reason == -1) {
		printk("Was not expecting a crash\n");
		ztest_test_fail();
		return;
	}

	if (reason != expected_reason) {
		printk("Wrong reason, got %d but expected %d\n", reason, expected_reason);
		ztest_test_fail();
		return;
	}

	expected_reason = -1;
}
#endif /* !CONFIG_APPLET_FATAL_HANDLER */

/*
 * Shared harness.
 *
 * A test case describes an applet completely: which body to run, how many
 * threads to give it, what each thread gets as its argument and which
 * partition (if any) to share between them.
 */
struct applet_test {
	const char *name;
	const struct applet_image *image;
	struct applet_opts opts;
	enum stack_pool pool;
	unsigned int num_threads;
	void *args[APPLET_TEST_MAX_THREADS];
	struct k_mem_partition *part;
};

#define APPLET_TEST_CASE(_name, _img)                                                              \
	{                                                                                          \
		.name = (_name),                                                                   \
		.image = &(_img),                                                                  \
		.opts = APPLET_OPTS_DEFAULT,                                                       \
		.pool = POOL_A,                                                                    \
		.num_threads = 1,                                                                  \
	}

/**
 * Bring an applet up to APPLET_STATE_LOADED with all its threads attached.
 *
 * @return 0, or a negative errno. -ENOSPC means the target ran out of memory
 *         partitions, which callers turn into a skip.
 */
static int applet_test_setup(struct applet *applet_inst, const struct applet_test *tc)
{
	int ret;

#ifdef CONFIG_APPLET_LLEXT
	ret = applet_load_llext(applet_inst, tc->name,
				elf_storage(applet_inst, tc->image->elf, tc->image->elf_size),
				tc->image->elf_size, &tc->opts);
#else
	ret = applet_init(applet_inst, tc->name, &tc->opts);
#endif
	if (ret != 0) {
		return ret;
	}

	if (tc->part != NULL) {
		ret = applet_add_partition(applet_inst, tc->part);
		if (ret != 0) {
			applet_unload(applet_inst);
			return ret;
		}
	}

	for (unsigned int i = 0; i < tc->num_threads; i++) {
		k_thread_stack_t *stack = pool_stack(tc->pool, i);

#ifdef CONFIG_APPLET_LLEXT
		ret = applet_add_thread_sym(applet_inst, stack, POOL_STACK_SIZE, tc->image->sym,
					    tc->args[i], NULL);
#else
		ret = applet_add_thread(applet_inst, stack, POOL_STACK_SIZE, tc->image->entry,
					tc->args[i], NULL);
#endif
		if (ret != 0) {
			applet_unload(applet_inst);
			return ret;
		}
	}

	return 0;
}

/**
 * Set up, start and wait for an applet. The descriptor is left loaded so the
 * caller can inspect it; the suite's after hook unloads it.
 */
static int applet_test_run(struct applet *applet_inst, const struct applet_test *tc,
			   k_timeout_t timeout)
{
	int ret = applet_test_setup(applet_inst, tc);

	if (ret != 0) {
		return ret;
	}

	ret = applet_start(applet_inst);
	if (ret != 0) {
		return ret;
	}

	return applet_join(applet_inst, timeout);
}

/** Turn a partition shortage into a skip instead of a failure. */
static bool skip_if_no_partitions(int ret)
{
	if (ret == -ENOSPC) {
		TC_PRINT("Too many memory partitions for this particular hardware\n");
		ztest_test_skip();
		return true;
	}
	return false;
}

static void shared_prepare(struct applet_test_shared *shared, unsigned int num_threads)
{
	memset((void *)shared, 0, sizeof(*shared));

	for (unsigned int i = 0; i < APPLET_TEST_MAX_THREADS; i++) {
		shared->pattern[i] = APPLET_TEST_PATTERN;
	}
	for (unsigned int i = 0; i < num_threads; i++) {
		shared->slot[i].shared = shared;
		shared->slot[i].index = i;
	}
}

static void shared_verify(struct applet_test_shared *shared, unsigned int num_threads)
{
	for (unsigned int i = 0; i < num_threads; i++) {
		zassert_equal(shared->result[i], APPLET_TEST_MAGIC,
			      "thread %u did not see the shared partition (result 0x%08x)", i,
			      shared->result[i]);
	}
}

/** Poll until the applet reports DEAD, or give up after @p max_ms. */
static enum applet_state wait_for_dead(struct applet *applet_inst, int max_ms)
{
	enum applet_state state = applet_get_state(applet_inst);

	for (int waited = 0; waited < max_ms && state != APPLET_STATE_DEAD; waited += 10) {
		k_msleep(10);
		state = applet_get_state(applet_inst);
	}

	return state;
}

static void helper_spawn(unsigned int idx, k_thread_entry_t entry, void *arg)
{
	helper_ret[idx] = -EBUSY;
	k_thread_create(&helper_threads[idx], helper_stacks[idx],
			K_THREAD_STACK_SIZEOF(helper_stacks[idx]), entry, arg, NULL, NULL,
			K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
}

static void helper_wait(unsigned int idx)
{
	zassert_ok(k_thread_join(&helper_threads[idx], K_SECONDS(10)),
		   "helper thread %u did not finish", idx);
}

static void joiner_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	helper_ret[0] = applet_join((struct applet *)p1, K_FOREVER);
}

static void joiner2_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	helper_ret[1] = applet_join((struct applet *)p1, K_SECONDS(10));
}

/*
 * Hammers the read-only observers of an applet while another thread tears it
 * down. Every one of these takes the same internal mutex the mutating calls
 * do, so this is the test for that serialisation.
 */
static void poller_entry(void *p1, void *p2, void *p3)
{
	struct applet *applet_inst = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (atomic_get(&poller_run) != 0) {
		unsigned int count = applet_thread_count(applet_inst);

		for (unsigned int i = 0; i < count + 1U; i++) {
			(void)applet_thread_get(applet_inst, i);
		}
		(void)applet_get_state(applet_inst);

		atomic_inc(&poller_iterations);
		/*
		 * Sleep rather than yield: on native_sim the simulated clock only
		 * advances while the CPU is idle, so a permanently runnable poller
		 * would stop time and nothing else would ever wake.
		 */
		k_msleep(1);
	}

	helper_ret[1] = 0;
}

static void applet_before(void *f)
{
	ARG_UNUSED(f);

#ifndef CONFIG_APPLET_FATAL_HANDLER
	expected_reason = -1;
#endif
}

static void applet_after(void *f)
{
	ARG_UNUSED(f);

	/*
	 * Unconditional, so that a failing assertion cannot leave a descriptor
	 * registered on the subsystem's internal list. The descriptors are
	 * reused as-is by the next test; zeroing them here would throw away a
	 * memory domain that the arch layer still tracks.
	 */
	applet_unload(&applet_1);
	applet_unload(&applet_2);
}

/*
 * Lifecycle
 */

ZTEST(applet, test_init_unload)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	zassert_ok(applet_init(&applet_1, "init", &opts));
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_LOADED);
	zassert_equal(applet_thread_count(&applet_1), 0);
	zassert_is_null(applet_thread_get(&applet_1, 0));

	applet_unload(&applet_1);
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_UNLOADED);
	zassert_equal(applet_thread_count(&applet_1), 0);
}

ZTEST(applet, test_run_single_thread)
{
	struct applet_test tc = APPLET_TEST_CASE("single", img_worker);

	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_QUICK;

	int ret = applet_test_run(&applet_1, &tc, K_SECONDS(5));

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_run failed: %d", ret);
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_DEAD);
}

ZTEST(applet, test_state_transitions)
{
	struct applet_test tc = APPLET_TEST_CASE("states", img_worker);

	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_SLEEP;

	int ret = applet_test_setup(&applet_1, &tc);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_setup failed: %d", ret);

	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_LOADED);
	zassert_equal(applet_thread_count(&applet_1), 1);

	zassert_ok(applet_start(&applet_1));
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_RUNNING);

	/* Nothing reports the exit; the state is resolved by observing it. */
	zassert_equal(wait_for_dead(&applet_1, 5000), APPLET_STATE_DEAD,
		      "applet did not settle in DEAD");

	applet_unload(&applet_1);
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_UNLOADED);
}

ZTEST(applet, test_start_no_threads)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	zassert_ok(applet_init(&applet_1, "empty", &opts));
	zassert_equal(applet_start(&applet_1), -EINVAL, "start with no threads should fail");
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_LOADED);
}

ZTEST(applet, test_double_start_and_late_add)
{
	struct applet_test tc = APPLET_TEST_CASE("late", img_worker);

	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;

	int ret = applet_test_setup(&applet_1, &tc);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_setup failed: %d", ret);
	zassert_ok(applet_start(&applet_1));

	zassert_equal(applet_start(&applet_1), -EINVAL, "second start should fail");

#ifdef CONFIG_APPLET_LLEXT
	ret = applet_add_thread_sym(&applet_1, pool_stack(POOL_A, 1), POOL_STACK_SIZE,
				    img_worker.sym, NULL, NULL);
#else
	ret = applet_add_thread(&applet_1, pool_stack(POOL_A, 1), POOL_STACK_SIZE, img_worker.entry,
				NULL, NULL);
#endif
	zassert_equal(ret, -EINVAL, "adding a thread while running should fail");
	zassert_equal(applet_thread_count(&applet_1), 1);

	zassert_ok(applet_kill(&applet_1));
}

ZTEST(applet, test_unload_while_running)
{
	struct applet_test tc = APPLET_TEST_CASE("runaway", img_worker);

	tc.num_threads = 2;
	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;
	tc.args[1] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;

	int ret = applet_test_setup(&applet_1, &tc);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_setup failed: %d", ret);
	zassert_ok(applet_start(&applet_1));
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_RUNNING);

	/* Must kill the threads itself rather than leaving them behind. */
	applet_unload(&applet_1);
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_UNLOADED);
	zassert_equal(applet_thread_count(&applet_1), 0);
}

ZTEST(applet, test_unload_idempotent)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	applet_unload(&applet_1);
	zassert_ok(applet_init(&applet_1, "twice", &opts));
	applet_unload(&applet_1);
	applet_unload(&applet_1);
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_UNLOADED);
}

/*
 * Argument validation
 */

ZTEST(applet, test_invalid_args)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	zassert_equal(applet_init(NULL, "x", &opts), -EINVAL);
	zassert_equal(applet_init(&applet_1, NULL, &opts), -EINVAL);

	zassert_ok(applet_init(&applet_1, "args", &opts));

	zassert_equal(applet_add_thread(NULL, pool_stack(POOL_A, 0), POOL_STACK_SIZE,
					(k_thread_entry_t)k_yield, NULL, NULL),
		      -EINVAL);
	zassert_equal(applet_add_thread(&applet_1, NULL, POOL_STACK_SIZE, (k_thread_entry_t)k_yield,
					NULL, NULL),
		      -EINVAL);
	zassert_equal(applet_add_thread(&applet_1, pool_stack(POOL_A, 0), 0,
					(k_thread_entry_t)k_yield, NULL, NULL),
		      -EINVAL);
	zassert_equal(applet_add_thread(&applet_1, pool_stack(POOL_A, 0), POOL_STACK_SIZE, NULL,
					NULL, NULL),
		      -EINVAL);

#ifdef CONFIG_USERSPACE
	zassert_equal(applet_add_partition(NULL, &applet_part_a), -EINVAL);
#endif
	zassert_equal(applet_add_partition(&applet_1, NULL), -EINVAL);

	zassert_equal(applet_start(NULL), -EINVAL);
	zassert_equal(applet_join(NULL, K_NO_WAIT), -EINVAL);
	zassert_equal(applet_kill(NULL), -EINVAL);

	zassert_equal(applet_get_state(NULL), APPLET_STATE_UNLOADED);
	zassert_equal(applet_thread_count(NULL), 0);
	zassert_is_null(applet_thread_get(NULL, 0));

	applet_unload(NULL);
}

ZTEST(applet, test_join_kill_wrong_state)
{
	struct applet_test tc = APPLET_TEST_CASE("wrong-state", img_worker);

	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_QUICK;

	/* UNLOADED */
	zassert_equal(applet_join(&applet_1, K_NO_WAIT), -EINVAL);
	zassert_equal(applet_kill(&applet_1), -EINVAL);

	int ret = applet_test_setup(&applet_1, &tc);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_setup failed: %d", ret);

	/* LOADED but not started */
	zassert_equal(applet_join(&applet_1, K_NO_WAIT), -EINVAL);
	zassert_equal(applet_kill(&applet_1), -EINVAL);

	zassert_ok(applet_start(&applet_1));
	zassert_ok(applet_join(&applet_1, K_SECONDS(5)));

	/* DEAD: join is idempotent, kill no longer applies */
	zassert_ok(applet_join(&applet_1, K_NO_WAIT));
	zassert_equal(applet_kill(&applet_1), -EINVAL);
}

ZTEST(applet, test_name_truncation)
{
	char long_name[CONFIG_APPLET_NAME_MAX_LEN + 16];
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	memset(long_name, 'x', sizeof(long_name) - 1);
	long_name[sizeof(long_name) - 1] = '\0';

	zassert_ok(applet_init(&applet_1, long_name, &opts));
	zassert_equal(strlen(applet_1.name), CONFIG_APPLET_NAME_MAX_LEN,
		      "name should be truncated to CONFIG_APPLET_NAME_MAX_LEN");
}

/*
 * Threads
 */

ZTEST(applet, test_multi_thread)
{
	struct applet_test tc = APPLET_TEST_CASE("multi", img_worker);

	tc.num_threads = APPLET_TEST_MAX_THREADS;
	for (unsigned int i = 0; i < tc.num_threads; i++) {
		tc.args[i] = (void *)(uintptr_t)APPLET_TEST_CMD_SLEEP;
	}

	int ret = applet_test_setup(&applet_1, &tc);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	if (ret == -ENOMEM) {
		TC_PRINT("Applet slot heap too small for %u threads\n", tc.num_threads);
		ztest_test_skip();
		return;
	}
	zassert_ok(ret, "applet_test_setup failed: %d", ret);
	zassert_equal(applet_thread_count(&applet_1), tc.num_threads);

	/* Slots are handed back in attachment order and are all distinct. */
	for (unsigned int i = 0; i < tc.num_threads; i++) {
		struct k_thread *thread = applet_thread_get(&applet_1, i);

		zassert_not_null(thread, "thread %u missing", i);
		for (unsigned int j = 0; j < i; j++) {
			zassert_not_equal(thread, applet_thread_get(&applet_1, j),
					  "threads %u and %u alias", i, j);
		}
	}
	zassert_is_null(applet_thread_get(&applet_1, tc.num_threads));

	zassert_ok(applet_start(&applet_1));
	zassert_ok(applet_join(&applet_1, K_SECONDS(10)));
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_DEAD);
}

/*
 * The slot heap is the only bound on the thread count, so walk it to the end
 * and check that unloading gives every byte back.
 */
ZTEST(applet, test_thread_slot_exhaustion)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;
	unsigned int round_count[2] = {0};

	/*
	 * The threads are never started, so they can all share one stack: the
	 * only resource being consumed here is one bookkeeping slot per call.
	 */
	opts.user_mode = false;

	for (unsigned int round = 0; round < ARRAY_SIZE(round_count); round++) {
		unsigned int added = 0;

		zassert_ok(applet_init(&applet_1, "greedy", &opts));

		while (added < APPLET_TEST_SLOT_LIMIT) {
			int ret =
				applet_add_thread(&applet_1, pool_stack(POOL_A, 0), POOL_STACK_SIZE,
						  (k_thread_entry_t)k_yield, NULL, NULL);

			if (ret == -ENOMEM) {
				break;
			}
			zassert_ok(ret, "unexpected error %d after %u threads", ret, added);
			added++;
		}

		zassert_true(added > 0, "not a single thread slot could be allocated");
		zassert_true(added < APPLET_TEST_SLOT_LIMIT, "slot allocation never ran out");
		zassert_equal(applet_thread_count(&applet_1), added);

		applet_unload(&applet_1);
		round_count[round] = added;
	}

	zassert_equal(round_count[0], round_count[1],
		      "unload leaked slots: %u the first time, %u the second", round_count[0],
		      round_count[1]);
}

ZTEST(applet, test_thread_names_and_priority)
{
	struct applet_test tc = APPLET_TEST_CASE("named", img_worker);
	int ret;

	tc.opts.thread_priority = 7;
	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_QUICK;

	ret = applet_test_setup(&applet_1, &tc);
	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_setup failed: %d", ret);

	struct k_thread *thread = applet_thread_get(&applet_1, 0);

	zassert_not_null(thread);
	zassert_equal(k_thread_priority_get(thread), 7, "opts.thread_priority not applied");
	/* A NULL thread name falls back to the applet name. */
	zassert_str_equal(k_thread_name_get(thread), "named");
}

ZTEST(applet, test_cpu_pinning)
{
	/* The applet calls arch_curr_cpu(), which faults in user mode. */
	if (IS_ENABLED(CONFIG_USERSPACE) || CONFIG_MP_MAX_NUM_CPUS < 2) {
		ztest_test_skip();
		return;
	}

	struct applet_test tc = APPLET_TEST_CASE("cpu-pin", img_cpu_pinning);

	tc.opts.cpu = CONFIG_MP_MAX_NUM_CPUS - 1;
	tc.opts.entry_sym = img_cpu_pinning.sym;
	tc.args[0] = (void *)(uintptr_t)(CONFIG_MP_MAX_NUM_CPUS - 1);

	int ret = applet_test_run(&applet_1, &tc, K_SECONDS(5));

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_run failed: %d", ret);
}

/*
 * Memory domain
 */

ZTEST(applet, test_shared_partition)
{
	struct applet_test tc = APPLET_TEST_CASE("shared", img_shared_mem);

	tc.num_threads = APPLET_TEST_MAX_THREADS;
	tc.part = APPLET_TEST_PART(applet_part_a);

	shared_prepare(&shared_a, tc.num_threads);
	for (unsigned int i = 0; i < tc.num_threads; i++) {
		tc.args[i] = &shared_a.slot[i];
	}

	int ret = applet_test_run(&applet_1, &tc, K_SECONDS(10));

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_run failed: %d", ret);

	shared_verify(&shared_a, tc.num_threads);
}

ZTEST(applet, test_partition_errors)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

#ifdef CONFIG_USERSPACE
	zassert_equal(applet_add_partition(&applet_1, &applet_part_a), -EINVAL,
		      "add_partition on an unloaded applet should fail");
#endif

	zassert_ok(applet_init(&applet_1, "parts", &opts));
	zassert_equal(applet_add_partition(&applet_1, NULL), -EINVAL);
}

/*
 * Two applets alive at once, each with its own partition. Proves the domains
 * are per-applet and that a second descriptor does not disturb the first.
 */
ZTEST(applet, test_two_applets)
{
	struct applet_test tc_a = APPLET_TEST_CASE("shared-a", img_shared_mem);
	struct applet_test tc_b = APPLET_TEST_CASE("shared-b", img_shared_mem);
	int ret;

	tc_a.num_threads = POOL_B_THREADS;
	tc_a.part = APPLET_TEST_PART(applet_part_a);
	tc_b.num_threads = POOL_B_THREADS;
	tc_b.pool = POOL_B;
	tc_b.part = APPLET_TEST_PART(applet_part_b);

	shared_prepare(&shared_a, tc_a.num_threads);
	shared_prepare(&shared_b, tc_b.num_threads);
	for (unsigned int i = 0; i < POOL_B_THREADS; i++) {
		tc_a.args[i] = &shared_a.slot[i];
		tc_b.args[i] = &shared_b.slot[i];
	}

	ret = applet_test_setup(&applet_1, &tc_a);
	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet 1 setup failed: %d", ret);

	ret = applet_test_setup(&applet_2, &tc_b);
	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet 2 setup failed: %d", ret);

	zassert_ok(applet_start(&applet_1));
	zassert_ok(applet_start(&applet_2));

	zassert_ok(applet_join(&applet_1, K_SECONDS(10)));
	zassert_ok(applet_join(&applet_2, K_SECONDS(10)));

	shared_verify(&shared_a, tc_a.num_threads);
	shared_verify(&shared_b, tc_b.num_threads);
}

/*
 * Concurrency
 */

ZTEST(applet, test_join_timeout)
{
	struct applet_test tc = APPLET_TEST_CASE("timeout", img_worker);

	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;

	int ret = applet_test_setup(&applet_1, &tc);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_setup failed: %d", ret);
	zassert_ok(applet_start(&applet_1));

	zassert_equal(applet_join(&applet_1, K_MSEC(50)), -EAGAIN, "join should have timed out");
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_RUNNING);

	zassert_ok(applet_kill(&applet_1));
	zassert_ok(applet_join(&applet_1, K_NO_WAIT));
}

ZTEST(applet, test_kill_while_joining)
{
	struct applet_test tc = APPLET_TEST_CASE("kill-join", img_worker);

	tc.num_threads = 2;
	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;
	tc.args[1] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;

	int ret = applet_test_setup(&applet_1, &tc);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_setup failed: %d", ret);
	zassert_ok(applet_start(&applet_1));

	helper_spawn(0, joiner_entry, &applet_1);
	k_msleep(50);

	zassert_ok(applet_kill(&applet_1));

	helper_wait(0);
	zassert_ok(helper_ret[0], "blocked applet_join() did not return cleanly");
}

ZTEST(applet, test_concurrent_join)
{
	struct applet_test tc = APPLET_TEST_CASE("two-joiners", img_worker);

	tc.num_threads = 2;
	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_SLEEP;
	tc.args[1] = (void *)(uintptr_t)APPLET_TEST_CMD_SLEEP;

	int ret = applet_test_setup(&applet_1, &tc);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_test_setup failed: %d", ret);
	zassert_ok(applet_start(&applet_1));

	helper_spawn(0, joiner_entry, &applet_1);
	helper_spawn(1, joiner2_entry, &applet_1);

	zassert_ok(applet_join(&applet_1, K_SECONDS(10)), "join from the test thread failed");

	helper_wait(0);
	helper_wait(1);
	zassert_ok(helper_ret[0], "first concurrent join failed");
	zassert_ok(helper_ret[1], "second concurrent join failed");
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_DEAD);
}

/*
 * The hard case for the internal locking: one thread parked in
 * applet_join(K_FOREVER), one thread hammering the observers, and the test
 * thread pulling the applet apart underneath both of them.
 */
ZTEST(applet, test_unload_while_joining)
{
	struct applet_test tc = APPLET_TEST_CASE("teardown", img_worker);

	tc.num_threads = 2;
	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;
	tc.args[1] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;

	for (unsigned int round = 0; round < 3; round++) {
		int ret = applet_test_setup(&applet_1, &tc);

		if (skip_if_no_partitions(ret)) {
			return;
		}
		zassert_ok(ret, "round %u setup failed: %d", round, ret);
		zassert_ok(applet_start(&applet_1));

		atomic_set(&poller_run, 1);
		atomic_set(&poller_iterations, 0);
		helper_spawn(0, joiner_entry, &applet_1);
		helper_spawn(1, poller_entry, &applet_1);

		k_msleep(20);
		applet_unload(&applet_1);

		helper_wait(0);
		atomic_set(&poller_run, 0);
		helper_wait(1);

		zassert_ok(helper_ret[0], "round %u: join did not return cleanly", round);
		zassert_ok(helper_ret[1], "round %u: poller did not finish", round);
		zassert_true(atomic_get(&poller_iterations) > 0, "round %u: poller never ran",
			     round);
		zassert_equal(applet_get_state(&applet_1), APPLET_STATE_UNLOADED);
	}
}

/*
 * A join that blocks forever on one applet must not stall calls on another.
 */
ZTEST(applet, test_concurrent_applets)
{
	struct applet_test tc_long = APPLET_TEST_CASE("long", img_worker);
	struct applet_test tc_short = APPLET_TEST_CASE("short", img_worker);
	int ret;

	tc_long.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;
	tc_short.pool = POOL_B;
	tc_short.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_QUICK;

	ret = applet_test_setup(&applet_1, &tc_long);
	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "setup failed: %d", ret);
	zassert_ok(applet_start(&applet_1));

	helper_spawn(0, joiner_entry, &applet_1);
	k_msleep(20);

	for (unsigned int round = 0; round < 3; round++) {
		ret = applet_test_run(&applet_2, &tc_short, K_SECONDS(5));
		zassert_ok(ret, "round %u on the second applet failed: %d", round, ret);
		applet_unload(&applet_2);
	}

	zassert_ok(applet_kill(&applet_1));
	helper_wait(0);
	zassert_ok(helper_ret[0], "blocked join did not return cleanly");
}

/*
 * LLEXT-backed applets
 */

#ifdef CONFIG_APPLET_LLEXT

ZTEST(applet, test_llext_spawn)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	opts.entry_sym = img_worker.sym;
	opts.arg = (void *)(uintptr_t)APPLET_TEST_CMD_QUICK;

	int ret = applet_spawn(&applet_1, "spawn",
			       elf_storage(&applet_1, worker_elf, sizeof(worker_elf)),
			       sizeof(worker_elf), pool_stack(POOL_A, 0), POOL_STACK_SIZE, &opts);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_spawn failed: %d", ret);
	zassert_equal(applet_thread_count(&applet_1), 1);
	zassert_ok(applet_join(&applet_1, K_SECONDS(5)));
}

ZTEST(applet, test_llext_load_then_start)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	opts.entry_sym = img_worker.sym;
	opts.arg = (void *)(uintptr_t)APPLET_TEST_CMD_QUICK;

	int ret = applet_load(&applet_1, "load",
			      elf_storage(&applet_1, worker_elf, sizeof(worker_elf)),
			      sizeof(worker_elf), pool_stack(POOL_A, 0), POOL_STACK_SIZE, &opts);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_load failed: %d", ret);
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_LOADED,
		      "applet_load must not start the applet");
	zassert_equal(applet_thread_count(&applet_1), 1);

	zassert_ok(applet_start(&applet_1));
	zassert_ok(applet_join(&applet_1, K_SECONDS(5)));
}

/* opts left at NULL must resolve APPLET_ENTRY_SYM in the extension. */
ZTEST(applet, test_llext_default_entry_sym)
{
	int ret = applet_spawn(&applet_1, "default-sym",
			       elf_storage(&applet_1, worker_elf, sizeof(worker_elf)),
			       sizeof(worker_elf), pool_stack(POOL_A, 0), POOL_STACK_SIZE, NULL);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "spawn with default opts failed: %d", ret);
	zassert_ok(applet_join(&applet_1, K_SECONDS(5)));
}

ZTEST(applet, test_llext_unknown_sym)
{
	int ret = applet_load_llext(&applet_1, "nosym",
				    elf_storage(&applet_1, worker_elf, sizeof(worker_elf)),
				    sizeof(worker_elf), NULL);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_load_llext failed: %d", ret);

	ret = applet_add_thread_sym(&applet_1, pool_stack(POOL_A, 0), POOL_STACK_SIZE,
				    "no_such_symbol", NULL, NULL);
	zassert_equal(ret, -ENOENT, "unknown entry symbol should report -ENOENT");
	zassert_equal(applet_thread_count(&applet_1), 0);
}

ZTEST(applet, test_llext_bad_elf)
{
	static const uint8_t garbage[64] ELF_ALIGN = {0xde, 0xad, 0xbe, 0xef};

	zassert_equal(applet_load_llext(&applet_1, "bad", NULL, 32, NULL), -EINVAL);
	zassert_equal(applet_load_llext(&applet_1, "bad", garbage, 0, NULL), -EINVAL);
	zassert_equal(applet_load_llext(NULL, "bad", garbage, sizeof(garbage), NULL), -EINVAL);

	int ret = applet_load_llext(&applet_1, "bad", garbage, sizeof(garbage), NULL);

	zassert_not_equal(ret, 0, "a malformed ELF should not load");
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_UNLOADED,
		      "a failed load must leave the descriptor unloaded");
}

ZTEST(applet, test_llext_sym_on_native_applet)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	zassert_ok(applet_init(&applet_1, "native", &opts));
	zassert_equal(applet_add_thread_sym(&applet_1, pool_stack(POOL_A, 0), POOL_STACK_SIZE,
					    img_worker.sym, NULL, NULL),
		      -EINVAL, "symbol lookup on a native applet should fail");
}

/* Two threads out of one extension: bringup runs once, both entries resolve. */
ZTEST(applet, test_llext_two_entry_points)
{
	int ret = applet_load_llext(&applet_1, "two-entries",
				    elf_storage(&applet_1, worker_elf, sizeof(worker_elf)),
				    sizeof(worker_elf), NULL);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "applet_load_llext failed: %d", ret);

	zassert_ok(applet_add_thread_sym(&applet_1, pool_stack(POOL_A, 0), POOL_STACK_SIZE,
					 img_worker.sym, (void *)(uintptr_t)APPLET_TEST_CMD_SLEEP,
					 "worker"));
	zassert_ok(applet_add_thread_sym(&applet_1, pool_stack(POOL_A, 1), POOL_STACK_SIZE,
					 img_default_entry.sym,
					 (void *)(uintptr_t)APPLET_TEST_CMD_QUICK, "main"));
	zassert_equal(applet_thread_count(&applet_1), 2);

	zassert_ok(applet_start(&applet_1));
	zassert_ok(applet_join(&applet_1, K_SECONDS(10)));
}

#endif /* CONFIG_APPLET_LLEXT */

/*
 * Fatal error handling
 */

#ifdef CONFIG_APPLET_FATAL_HANDLER

static atomic_t fault_count;

static void fault_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_inc(&fault_count);
	k_oops();
}

static void spin_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_msleep(APPLET_TEST_SPIN_MS);
	}
}

/*
 * The faulting thread is attached first on purpose: the handler must not stop
 * walking the applet's thread list when it reaches the thread it runs on.
 */
static int fault_applet_setup(struct applet *applet_inst, const char *name,
			      enum applet_halt_on_fault mode, enum stack_pool pool)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;
	int ret;

	/* k_oops() and the fault counter both need supervisor mode. */
	opts.user_mode = false;
	opts.halt_on_fault = mode;

	ret = applet_init(applet_inst, name, &opts);
	if (ret != 0) {
		return ret;
	}

	ret = applet_add_thread(applet_inst, pool_stack(pool, 0), POOL_STACK_SIZE, fault_entry,
				NULL, "faulter");
	if (ret != 0) {
		return ret;
	}

	return applet_add_thread(applet_inst, pool_stack(pool, 1), POOL_STACK_SIZE, spin_entry,
				 NULL, "survivor");
}

ZTEST(applet, test_halt_thread_on_fault)
{
	atomic_set(&fault_count, 0);

	zassert_ok(
		fault_applet_setup(&applet_1, "halt-thread", APPLET_HALT_ON_FAULT_THREAD, POOL_A));
	zassert_ok(applet_start(&applet_1));

	k_msleep(200);

	zassert_equal(atomic_get(&fault_count), 1, "the faulting thread never ran");
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_RUNNING,
		      "only the faulting thread should have been aborted");

	zassert_ok(applet_kill(&applet_1));
}

ZTEST(applet, test_halt_applet_on_fault)
{
	struct applet_test tc = APPLET_TEST_CASE("bystander", img_worker);

	atomic_set(&fault_count, 0);

	tc.pool = POOL_B;
	tc.args[0] = (void *)(uintptr_t)APPLET_TEST_CMD_SPIN;

	int ret = applet_test_setup(&applet_2, &tc);

	if (skip_if_no_partitions(ret)) {
		return;
	}
	zassert_ok(ret, "bystander setup failed: %d", ret);
	zassert_ok(applet_start(&applet_2));

	zassert_ok(
		fault_applet_setup(&applet_1, "halt-applet", APPLET_HALT_ON_FAULT_APPLET, POOL_A));
	zassert_ok(applet_start(&applet_1));

	k_msleep(200);

	zassert_equal(atomic_get(&fault_count), 1, "the faulting thread never ran");
	zassert_equal(applet_get_state(&applet_1), APPLET_STATE_DEAD,
		      "every thread of the faulting applet should have been aborted");
	zassert_equal(applet_get_state(&applet_2), APPLET_STATE_RUNNING,
		      "an unrelated applet must be left alone");

	zassert_ok(applet_kill(&applet_2));
}

/*
 * APPLET_HALT_ON_FAULT_SYSTEM ends in k_fatal_halt(), so there is no way back
 * into ztest to report a result. It is covered by src/test_halt_system.c,
 * which replaces this file when CONFIG_TEST_APPLET_HALT_SYSTEM is set.
 */

#else /* !CONFIG_APPLET_FATAL_HANDLER */

ZTEST(applet, test_halt_thread_on_fault)
{
	ztest_test_skip();
}

ZTEST(applet, test_halt_applet_on_fault)
{
	ztest_test_skip();
}

#endif /* CONFIG_APPLET_FATAL_HANDLER */

ZTEST_SUITE(applet, NULL, NULL, applet_before, applet_after, NULL);
