/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/kernel/smp.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/zsr.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <soc.h>
#include <esp_cpu.h>
#include <esp_attr.h>
#include <esp_rom_sys.h>
#include <hal/cpu_utility_ll.h>
#include <soc/system_reg.h>
#include <soc/interrupts.h>

#include <ipi.h>

#include <esp_mp_stall.h>

struct cpustart_rec {
	arch_cpustart_t fn;
	void *arg;
	int vecbase;
	uint32_t ccount;
	volatile int *alive;
};

static volatile struct cpustart_rec *start_rec;
static volatile DRAM_ATTR bool s_appcpu_ready;

/* Referenced by name from the asm entry stub. */
void *appcpu_top;

#define IPI_LINE_ALLOC(node, handler)                                                              \
	esp_intr_alloc(DT_IRQ_BY_IDX(DT_NODELABEL(node), 0, irq),                                  \
		ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(node), 0, priority)) |                \
			ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(node), 0, flags)) |         \
			ESP_INTR_FLAG_IRAM,                                                        \
		handler, NULL, NULL)

/* C entry of the application core, reached from the asm stub. Never returns. */
__used void appcpu_entry(void)
{
	_cpu_t *cpu = &_kernel.cpus[1];
	arch_cpustart_t fn;
	void *arg;
	uint32_t ps;

	/* Interrupts stay disabled until the kernel secondary init has run. */
	__asm__ volatile("rsr.PS %0" : "=r"(ps));
	ps &= ~XCHAL_PS_EXCM_MASK;
	ps = (ps & ~XCHAL_PS_INTLEVEL_MASK) | (XCHAL_EXCM_LEVEL << 0);
	__asm__ volatile("wsr.PS %0" : : "r"(ps));

	__asm__ volatile("wsr.INTENABLE %0" : : "r"(0));
	__asm__ volatile("wsr.VECBASE %0" : : "r"(start_rec->vecbase));
	__asm__ volatile("rsync");

	/* Seed CCOUNT from the primary core: the timer driver shares one last_count. */
	__asm__ volatile("wsr.CCOUNT %0" : : "r"(start_rec->ccount));
	__asm__ volatile("rsync");

	__asm__ volatile("wsr %0, " ZSR_CPU_STR : : "r"(cpu));

	/* The start record lives on the primary core's stack: copy it before
	 * publishing the alive flag.
	 */
	fn = start_rec->fn;
	arg = start_rec->arg;
	barrier_dmem_fence_full();
	*start_rec->alive = 1;
	fn(arg);
}

/* Naked entry: reset the register window and switch to the interrupt stack
 * before any C code runs.
 */
extern void z_appcpu_asm_entry(void);
__asm__(".align 4\n\t"
	".global z_appcpu_asm_entry\n\t"
	"z_appcpu_asm_entry:\n\t"
	"movi a0, 0\n\t"
	"wsr.WINDOWBASE a0\n\t"
	"movi a0, 1\n\t"
	"wsr.WINDOWSTART a0\n\t"
	"rsync\n\t"
	"movi a1, appcpu_top\n\t"
	"l32i a1, a1, 0\n\t"
	"call4 appcpu_entry\n\t");

/* The explicit reset matters on a warm boot, where the clock is already
 * enabled and the hal skips its own reset.
 */
static void release_appcpu(void *entry_point)
{
	esp_cpu_unstall(1);
	esp_cpu_reset(1);

	cpu_utility_ll_enable_clock_and_reset_app_cpu();
	cpu_utility_ll_enable_clock_and_reset_app_cpu_int_matrix();

	esp_rom_ets_set_appcpu_boot_addr((void *)entry_point);
}

static void IRAM_ATTR crosscore_isr(void *arg)
{
	ARG_UNUSED(arg);

	if (esp_core_id() == 0) {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_0_REG, 0);
	} else {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_1_REG, 0);
	}

	z_sched_ipi();
}

void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz, arch_cpustart_t fn, void *arg)
{
	volatile struct cpustart_rec sr;
	int vb;
	volatile int alive_flag;
	uint32_t ccount;

	__ASSERT(cpu_num == 1, "ESP32S3 supports only two CPUs");

	__asm__ volatile("rsr.VECBASE %0\n\t" : "=r"(vb));

	alive_flag = 0;

	sr.fn = fn;
	sr.arg = arg;
	sr.vecbase = vb;
	sr.alive = &alive_flag;

	appcpu_top = K_KERNEL_STACK_BUFFER(stack) + sz;

	start_rec = &sr;

	__asm__ volatile("rsr.CCOUNT %0" : "=r"(sr.ccount));

	release_appcpu((void *)z_appcpu_asm_entry);

	/* Keep publishing CCOUNT until the core has seeded its own. */
	while (alive_flag == 0) {
		__asm__ volatile("rsr.CCOUNT %0" : "=r"(ccount));
		sr.ccount = ccount;
	}
}

IRAM_ATTR bool arch_cpu_active(int cpu_num)
{
	return esp_mp_cpu_online(cpu_num);
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	ARG_UNUSED(cpu_bitmap);

	if (esp_core_id() == 0) {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_1_REG, SYSTEM_CPU_INTR_FROM_CPU_1);
	} else {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_0_REG, SYSTEM_CPU_INTR_FROM_CPU_0);
	}
}

void arch_sched_broadcast_ipi(void)
{
	arch_sched_directed_ipi(IPI_ALL_CPUS_MASK);
}

int esp_mp_stall_isr_register(int core_id)
{
	if (core_id == 0) {
		return IPI_LINE_ALLOC(ipi2, (intr_handler_t)esp_mp_stall_isr);
	}

	return IPI_LINE_ALLOC(ipi3, (intr_handler_t)esp_mp_stall_isr);
}

static int register_ipi(void)
{
	int core_id = esp_core_id();
	unsigned int key = arch_irq_lock();
	int ret;

	if (core_id == 0) {
		ret = IPI_LINE_ALLOC(ipi0, crosscore_isr);
	} else {
		ret = IPI_LINE_ALLOC(ipi1, crosscore_isr);
	}

	if (ret == 0) {
		ret = esp_mp_stall_isr_register(core_id);
	}

	arch_irq_unlock(key);

	if (ret != 0) {
		return ret;
	}

	/* Online is published before the stall gate is armed. */
	esp_mp_set_cpu_online(core_id, true);

	/* CPU1 registers last and arms the stall. */
	if (core_id == 1) {
		esp_mp_stall_enable();
	}

	return 0;
}

static int cpu0_ipi_init(void)
{
	return register_ipi();
}
SYS_INIT(cpu0_ipi_init, PRE_KERNEL_2, 0);

/* Runs on CPU1 before it enters the scheduler. */
static void appcpu_ready(void *arg)
{
	ARG_UNUSED(arg);

	if (register_ipi() != 0) {
		k_panic();
	}

	barrier_dmem_fence_full();
	s_appcpu_ready = true;
}

/* CPU1 must be online before the Wi-Fi driver creates its pinned task. */
static int smp_bringup_appcpu(void)
{
	if (CONFIG_MP_MAX_NUM_CPUS < 2) {
		return 0;
	}

	k_smp_cpu_start(1, appcpu_ready, NULL);

	while (!s_appcpu_ready) {
		arch_nop();
	}
	barrier_dmem_fence_full();

	return 0;
}
SYS_INIT(smp_bringup_appcpu, POST_KERNEL, 0);
