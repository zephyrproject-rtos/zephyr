/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/tc_capture.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_DEFINE(worker_stack, STACK_SIZE);
static struct k_thread worker;

/*
 * The tc_capture API can only be used from kernel context, so the user
 * mode cases keep the test body in kernel mode and run just the print
 * call in a worker thread that drops to user mode when CONFIG_USERSPACE
 * is enabled (and runs as a plain kernel thread otherwise).
 */
static void run_in_user_thread(k_thread_entry_t entry)
{
	k_thread_create(&worker, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack), entry, NULL,
			NULL, NULL, k_thread_priority_get(k_current_get()),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_thread_join(&worker, K_FOREVER);
}

/* Assert that exactly the expected bytes were captured since the last clear */
static void expect_captured(const char *expected)
{
	static char captured[CONFIG_TEST_CAPTURE_BUFFER_SIZE + 1];

	(void)tc_capture_get(captured, sizeof(captured));
	zassert_str_equal(captured, expected);
}

static void call_vprintk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
}

static void call_vprintk_unlocked(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk_unlocked(fmt, ap);
	va_end(ap);
}

static void printk_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("user %s %d %x\n", "context", -42, 0xdead);
}

static void printk_long_worker(void *p1, void *p2, void *p3)
{
	const char *part = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("%s%s%s\n", part, part, part);
}

static void printk_unlocked_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk_unlocked("user %s %d %x\n", "unlocked", -42, 0xdead);
}

static void vprintk_unlocked_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	call_vprintk_unlocked("user %s %d %x\n", "vprintk", -42, 0xdead);
}

static void k_str_out_worker(void *p1, void *p2, void *p3)
{
	char raw[] = "raw 100% bytes\n";

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_str_out(raw, sizeof(raw) - 1U);
}

static void printk_before(void *data)
{
	ARG_UNUSED(data);

	tc_capture_start();
}

static void printk_after(void *data)
{
	ARG_UNUSED(data);

	tc_capture_stop();
}

/**
 * @brief Printk output path tests
 * @defgroup lib_printk_tests Printk
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Verify that printk() output reaches the console from kernel
 * context.
 *
 * @details
 * Console output is recorded with the tc_capture helper. The test
 * calls printk() from a kernel mode thread and verifies the formatted
 * string arrives at the console unmodified.
 *
 * Test steps:
 * - Clear the capture buffer.
 * - Call printk() with a format string and arguments.
 * - Compare the captured output to the expected string.
 *
 * Expected result:
 * - Exactly the formatted string is captured.
 *
 * @see printk()
 */
ZTEST(lib_printk, test_printk_kernel_context_output)
{
	tc_capture_clear();
	printk("kernel %s %d %x\n", "context", -42, 0xdead);
	expect_captured("kernel context -42 dead\n");
}

/**
 * @brief Verify that printk() output reaches the console from user
 * context.
 *
 * @details
 * From a user mode thread, printk() cannot touch the console hook
 * directly: it must buffer the formatted output and hand it to the
 * kernel through the k_str_out() syscall. The test runs printk() in a
 * user mode worker thread and verifies the output still arrives at
 * the console unmodified.
 *
 * Test steps:
 * - Clear the capture buffer.
 * - Run printk() in a user mode worker thread and join it.
 * - Compare the captured output to the expected string.
 *
 * Expected result:
 * - Exactly the formatted string is captured, with no fault.
 *
 * @see printk()
 * @see k_str_out()
 */
ZTEST(lib_printk, test_printk_user_context_output)
{
	tc_capture_clear();
	run_in_user_thread(printk_worker);
	expect_captured("user context -42 dead\n");
}

/**
 * @brief Verify that user context printk() output longer than the
 * intermediate buffer is not lost or reordered.
 *
 * @details
 * The user mode printk() path accumulates characters in a fixed-size
 * buffer (CONFIG_PRINTK_BUFFER_SIZE) flushed through k_str_out()
 * whenever it fills. Output longer than the buffer therefore takes
 * multiple flushes. The test emits a string much longer than the
 * buffer from a user mode worker thread and verifies the console
 * receives all of it, in order.
 *
 * Test steps:
 * - Clear the capture buffer.
 * - Run printk() in a user mode worker thread with output longer
 *   than CONFIG_PRINTK_BUFFER_SIZE.
 * - Compare the captured output to the expected string.
 *
 * Expected result:
 * - The whole string is captured in order across flushes.
 *
 * @see printk()
 * @see k_str_out()
 */
ZTEST(lib_printk, test_printk_user_context_long_output)
{
	static const char part[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	char expected[3 * (sizeof(part) - 1U) + 2U];

	(void)snprintk(expected, sizeof(expected), "%s%s%s\n", part, part, part);

	tc_capture_clear();
	k_thread_create(&worker, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack),
			printk_long_worker, (void *)part, NULL, NULL,
			k_thread_priority_get(k_current_get()), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);
	k_thread_join(&worker, K_FOREVER);
	expect_captured(expected);
}

/**
 * @brief Verify that printk_unlocked() output reaches the console
 * from kernel context.
 *
 * @details
 * printk_unlocked() bypasses the printk spinlock so it stays usable
 * from fatal error and assertion paths. The test calls it from a
 * kernel mode thread and verifies the formatted string arrives at the
 * console unmodified.
 *
 * Test steps:
 * - Clear the capture buffer.
 * - Call printk_unlocked() with a format string and arguments.
 * - Compare the captured output to the expected string.
 *
 * Expected result:
 * - Exactly the formatted string is captured.
 *
 * @see printk_unlocked()
 */
ZTEST(lib_printk, test_printk_unlocked_kernel_context_output)
{
	tc_capture_clear();
	printk_unlocked("kernel %s %d %x\n", "unlocked", -42, 0xdead);
	expect_captured("kernel unlocked -42 dead\n");
}

/**
 * @brief Verify that printk_unlocked() works from user context.
 *
 * @details
 * Assertion reporting uses the unlocked printk variant, and an
 * assertion can fire in a user mode thread, so printk_unlocked() must
 * not touch kernel memory when called from user context: it has to
 * take the same buffered k_str_out() syscall path as printk(). A
 * regression here faults on the first emitted character (memory
 * protection violation reading the console hook) instead of printing.
 *
 * Test steps:
 * - Clear the capture buffer.
 * - Run printk_unlocked() in a user mode worker thread and join it.
 * - Compare the captured output to the expected string.
 *
 * Expected result:
 * - Exactly the formatted string is captured, with no fault.
 *
 * @see printk_unlocked()
 * @see k_str_out()
 */
ZTEST(lib_printk, test_printk_unlocked_user_context_output)
{
	tc_capture_clear();
	run_in_user_thread(printk_unlocked_worker);
	expect_captured("user unlocked -42 dead\n");
}

/**
 * @brief Verify that vprintk() produces the same output as printk().
 *
 * @details
 * vprintk() is the va_list form backing printk(). The test routes the
 * same format and arguments through a variadic wrapper calling
 * vprintk() and verifies the console receives the same formatted
 * string printk() would produce.
 *
 * Test steps:
 * - Clear the capture buffer.
 * - Call vprintk() through a variadic wrapper.
 * - Compare the captured output to the expected string.
 *
 * Expected result:
 * - Exactly the formatted string is captured.
 *
 * @see vprintk()
 */
ZTEST(lib_printk, test_printk_vprintk_output)
{
	tc_capture_clear();
	call_vprintk("kernel %s %d %x\n", "vprintk", -42, 0xdead);
	expect_captured("kernel vprintk -42 dead\n");
}

/**
 * @brief Verify that vprintk_unlocked() works from user context.
 *
 * @details
 * vprintk_unlocked() is the va_list form used directly by
 * assert_print(). The test calls it through a variadic wrapper in a
 * user mode worker thread and verifies the output arrives at the
 * console instead of faulting on kernel memory.
 *
 * Test steps:
 * - Clear the capture buffer.
 * - Run vprintk_unlocked() in a user mode worker thread and join it.
 * - Compare the captured output to the expected string.
 *
 * Expected result:
 * - Exactly the formatted string is captured, with no fault.
 *
 * @see vprintk_unlocked()
 */
ZTEST(lib_printk, test_printk_vprintk_unlocked_user_context_output)
{
	tc_capture_clear();
	run_in_user_thread(vprintk_unlocked_worker);
	expect_captured("user vprintk -42 dead\n");
}

/**
 * @brief Verify that k_str_out() passes bytes through to the console
 * unmodified.
 *
 * @details
 * k_str_out() is the syscall the user mode printk() path flushes
 * through. The test calls it directly from a user mode worker thread
 * with a fixed string, including a '%' character, and verifies the
 * exact bytes reach the console with no formatting applied.
 *
 * Test steps:
 * - Clear the capture buffer.
 * - Run k_str_out() in a user mode worker thread with a fixed byte
 *   string and join it.
 * - Compare the captured output to the input bytes.
 *
 * Expected result:
 * - The exact bytes are captured, '%' included.
 *
 * @see k_str_out()
 */
ZTEST(lib_printk, test_printk_k_str_out_passthrough)
{
	tc_capture_clear();
	run_in_user_thread(k_str_out_worker);
	expect_captured("raw 100% bytes\n");
}

/**
 * @}
 */

ZTEST_SUITE(lib_printk, NULL, NULL, printk_before, printk_after, NULL);
