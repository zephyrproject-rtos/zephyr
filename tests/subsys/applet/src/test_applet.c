/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/applet/applet.h>

LOG_MODULE_REGISTER(test_applet, LOG_LEVEL_INF);

#ifdef CONFIG_APPLET_LLEXT
static const uint8_t cpu_pinning_elf[] __aligned(4096) = {
#include <cpu_pinning.inc>
};
#else
extern void cpu_pinning_main(void *arg);
#endif /* CONFIG_APPLET_LLEXT */

static ZTEST_DMEM volatile int expected_reason = -1;

APPLET_THREAD_STACK_DEFINE(applet_1_thread_a_stack, CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT);
APPLET_THREAD_STACK_DEFINE(applet_1_thread_b_stack, CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT);
static struct applet applet_1;

APPLET_THREAD_STACK_DEFINE(applet_2_thread_stack, CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT);
static struct applet applet_2;

static void applet_before(void *f)
{
	ARG_UNUSED(f);
	expected_reason = -1;

	memset(&applet_1, 0, sizeof(struct applet));
	memset(&applet_1_thread_a_stack, 0, CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT);
	memset(&applet_1_thread_b_stack, 0, CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT);
	memset(&applet_2, 0, sizeof(struct applet));
	memset(&applet_2_thread_stack, 0, CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT);
}

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

ZTEST(applet, test_cpu_pinning)
{
	if (IS_ENABLED(CONFIG_USERSPACE) || CONFIG_MP_MAX_NUM_CPUS < 2) {
		ztest_test_skip();
		return;
	}

	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	opts.arg = (void *)(uintptr_t)(CONFIG_MP_MAX_NUM_CPUS - 1);
	opts.cpu = CONFIG_MP_MAX_NUM_CPUS - 1;

	int ret;

#ifdef CONFIG_APPLET_LLEXT
	ret = applet_spawn(&applet_1, "CPU pinning llext", cpu_pinning_elf, sizeof(cpu_pinning_elf),
			   applet_1_thread_a_stack, K_THREAD_STACK_SIZEOF(applet_1_thread_a_stack),
			   &opts);
	if (ret != 0) {
		LOG_ERR("applet_spawn failed: %d", ret);
		ztest_test_fail();
	}
#else
	ret = applet_init(&applet_1, "CPU pinning native", &opts);
	if (ret != 0) {
		LOG_ERR("applet_init failed: %d", ret);
		ztest_test_fail();
	}

	ret = applet_add_thread(&applet_1, applet_1_thread_a_stack,
				K_THREAD_STACK_SIZEOF(applet_1_thread_a_stack),
				(k_thread_entry_t)cpu_pinning_main, opts.arg, NULL);
	if (ret != 0) {
		LOG_ERR("applet_add_thread failed: %d", ret);
		ztest_test_fail();
	}

	ret = applet_start(&applet_1);
	if (ret != 0) {
		LOG_ERR("applet_start failed: %d", ret);
		ztest_test_fail();
	}
#endif

	ret = applet_join(&applet_1, K_SECONDS(5));
	if (ret != 0) {
		LOG_ERR("applet_join failed: %d", ret);
		applet_kill(&applet_1);
		ztest_test_fail();
	}
}

ZTEST(applet, test_halt_thread_on_fault)
{
}

ZTEST(applet, test_halt_applet_on_fault)
{
}

ZTEST(applet, test_halt_system_on_fault)
{
}

ZTEST_SUITE(applet, NULL, NULL, applet_before, NULL, NULL);
