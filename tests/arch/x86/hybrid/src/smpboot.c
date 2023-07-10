/* Copyright (c) 2023 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cpuid.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel_structs.h>

BUILD_ASSERT(CONFIG_SMP);
BUILD_ASSERT(CONFIG_X86_64);

#define CPUID_MASK_WORD  0xFF
#define CPUID_MASK_NIBLE 0x0F
#define STACKSZ          2048
char stack[STACKSZ];

volatile bool mp_flag[CONFIG_MP_MAX_NUM_CPUS];

struct k_thread cpu_thr;
K_THREAD_STACK_DEFINE(thr_stack, STACKSZ);

extern void z_smp_start_cpu(int id);

static int cpu_info_get(struct x86_cpu_info *info)
{
	uint32_t eax, ebx, ecx, edx;

	if (!__get_cpuid(0x01, &eax, &ebx, &ecx, &edx)) {
		return -EIO;
	}
	info->family = (((eax >> 20u) & CPUID_MASK_WORD) << 4u) | ((eax >> 8u) & CPUID_MASK_NIBLE);
	info->model = (((eax >> 16u) & CPUID_MASK_NIBLE) << 4u) | ((eax >> 4u) & CPUID_MASK_NIBLE);
	info->stepping = eax & CPUID_MASK_NIBLE;
	info->apic_id = (ebx >> 24u) & CPUID_MASK_WORD;

	if (!__get_cpuid(0x1A, &eax, &ebx, &ecx, &edx)) {
		printk("cpuid 0x07 failed, many to support hybrid core or older CPU version\n");
		return -EIO;
	}
	info->type = (enum x86_cpu_type)(eax >> 24);

	if (!__get_cpuid(0x07, &eax, &ebx, &ecx, &edx)) {
		printk("cpuid 0x07 failed, many to support hybrid core or older CPU version\n");
		return -EIO;
	}
	info->hybrid = (edx >> 15) & 0x01;

	return 0;
}

static void print_cpu_info(struct x86_cpu_info *cpu_info)
{
	printk("CPU info[family:%x model:%x stepping:%x type:%s]\n", cpu_info->family,
	       cpu_info->model, cpu_info->stepping, cpu_info->bsp ? "BSP" : "AP");
	printk("Schedule thread on CPU Type:%s, cpu id:%d ",
	       cpu_info->type == CPU_TYPE_CORE ? "CORE" : "ATOM", cpu_info->cpu_id);

	if (cpu_info->hybrid) {
		printk("Hybrid Core\n");
	} else {
		printk("\n");
	}
}

static void thread_fn(void *a, void *b, void *c)
{
	struct x86_cpu_info actual_cpu_info = {
		0,
	};
	struct x86_cpu_info *expected_cpu_info = (struct x86_cpu_info *)a;

	if (cpu_info_get(&actual_cpu_info)) {
		zassert_true(false, "get cpu info failed");
		return;
	}

	zassert_true(actual_cpu_info.cpu_id < CONFIG_MP_MAX_NUM_CPUS, "invalid cpu id received\n");

	zassert_true(expected_cpu_info->apic_id == 0 ||
			     expected_cpu_info->apic_id == actual_cpu_info.apic_id,
		     "running on wrong cpu");

	if (expected_cpu_info->type == actual_cpu_info.type) {
		mp_flag[expected_cpu_info->cpu_id] = true;
	}
}

ZTEST(intel_hybrid_boot, test_on_hybrid_ap_cores)
{
	unsigned int num_cpus = arch_num_cpus();

	if (arch_num_cpus() < 2) {
		ztest_test_skip();
	}

	for (int i = 1; i < num_cpus; i++) {
		struct x86_cpu_info *cpu_info = arch_cpu_info_get(i);

		if (cpu_info == NULL) {
			zassert_true(false, "No cpu info for the given cpu id");
			return;
		}

		if (!cpu_info->hybrid) {
			printk("Platform does not support hybrid core !\n");
			ztest_test_skip();
			return;
		}

		print_cpu_info(cpu_info);

		k_thread_create(&cpu_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack), thread_fn,
				(void *)cpu_info, NULL, NULL, 0, 0, K_FOREVER);
		k_thread_cpu_mask_clear(&cpu_thr);
		k_thread_cpu_mask_enable(&cpu_thr, cpu_info->cpu_id);
		k_thread_start(&cpu_thr);
	}

	/* Give enough time to execute threads on all CPUs */
	k_msleep(500u);

	for (int i = 1; i < num_cpus; i++) {
		/* Verify all thread executed */
		zassert_true(mp_flag[i], "thread fail to execute on AP Proccessor:%d", i);
	}
}

ZTEST(intel_hybrid_boot, test_on_hybrid_bsp_core)
{
	struct x86_cpu_info *cpu_info = arch_cpu_info_get(0);

	if (cpu_info == NULL) {
		zassert_true(false, "No cpu info for the given cpu id");
		ztest_test_skip();
		return;
	}

	if (!cpu_info->hybrid) {
		printk("Platform does not support hybrid core!\n");
		ztest_test_skip();
		return;
	}

	print_cpu_info(cpu_info);

	k_thread_create(&cpu_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack), thread_fn,
			(void *)cpu_info, NULL, NULL, 1, 0, K_NO_WAIT);

	k_msleep(100u);
	zassert_true(mp_flag[0], "thread fail to execute on BSP Proccessor");
}

ZTEST_SUITE(intel_hybrid_boot, NULL, NULL, NULL, NULL, NULL);
