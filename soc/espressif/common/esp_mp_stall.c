/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <esp_cpu.h>
#include <esp_attr.h>
#include <esp_rom_sys.h>
#include <esp_intr_alloc.h>
#include <soc/system_reg.h>

#include <esp_mp_stall.h>

#if defined(CONFIG_XTENSA)
#include <xtensa/corebits.h>
#endif

#define STALL_SPIN_MAX 100000000U

static volatile DRAM_ATTR uint32_t s_stall_req[CONFIG_MP_MAX_NUM_CPUS];
static volatile DRAM_ATTR uint32_t s_stall_ack[CONFIG_MP_MAX_NUM_CPUS];
static volatile DRAM_ATTR bool s_stall_enabled;
static volatile DRAM_ATTR bool s_cpu_up[CONFIG_MP_MAX_NUM_CPUS];

/* Taken above any flash or cache lock; re-entrant on the owning core. */
static DRAM_ATTR atomic_t s_pause_owner = ATOMIC_INIT(-1);
static volatile DRAM_ATTR uint32_t s_pause_nest;
static volatile DRAM_ATTR unsigned int s_pause_irq_key[CONFIG_MP_MAX_NUM_CPUS];

static ALWAYS_INLINE void IRAM_ATTR stall_trigger_clear(int core_id)
{
	if (core_id == 0) {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_2_REG, 0);
	} else {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_3_REG, 0);
	}
}

static ALWAYS_INLINE void IRAM_ATTR stall_trigger_set(int other, bool assert_line)
{
	if (other == 1) {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_3_REG,
			       assert_line ? SYSTEM_CPU_INTR_FROM_CPU_3 : 0);
	} else {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_2_REG,
			       assert_line ? SYSTEM_CPU_INTR_FROM_CPU_2 : 0);
	}
}

/* The parked core keeps its IRAM lines open so the radio handlers keep running. */
#if defined(CONFIG_XTENSA)
static ALWAYS_INLINE uint32_t IRAM_ATTR stall_irq_open(void)
{
	uint32_t ps;

	__asm__ volatile("rsr.ps %0" : "=r"(ps));
	__asm__ volatile("wsr.ps %0\n\trsync" : : "r"(ps & ~PS_INTLEVEL_MASK));
	return ps;
}

static ALWAYS_INLINE void IRAM_ATTR stall_irq_restore(uint32_t ps)
{
	__asm__ volatile("wsr.ps %0\n\trsync" : : "r"(ps));
}
#else
static ALWAYS_INLINE uint32_t IRAM_ATTR stall_irq_open(void)
{
	return 0;
}

static ALWAYS_INLINE void IRAM_ATTR stall_irq_restore(uint32_t state)
{
	ARG_UNUSED(state);
}
#endif

void IRAM_ATTR esp_mp_stall_isr(const void *arg)
{
	int core_id = esp_cpu_get_core_id();
	uint32_t irq_state;
	uint32_t masked;

	ARG_UNUSED(arg);

	stall_trigger_clear(core_id);

	/* Mask before acking: the requester suspends the cache once it sees the ack. */
	masked = esp_intr_noniram_mask_local();
	s_stall_ack[core_id] = 1;
	barrier_dmem_fence_full();

	irq_state = stall_irq_open();

	/* arch_spin_relax() asserts on open interrupts, so spin on plain nops. */
	while (s_stall_req[core_id] != 0U) {
		arch_nop();
	}

	stall_irq_restore(irq_state);

	barrier_dmem_fence_full();
	s_stall_ack[core_id] = 0;
	esp_intr_noniram_unmask_local(masked);
}

static void IRAM_ATTR stall_other_cpu(void)
{
	int other = (esp_cpu_get_core_id() == 0) ? 1 : 0;
	uint32_t spins = 0;

	if (!s_stall_enabled || !s_cpu_up[other]) {
		return;
	}

	/* s_stall_ack is owned by the parked core. */
	s_stall_req[other] = 1;

	barrier_dmem_fence_full();

	stall_trigger_set(other, true);

	while (s_stall_ack[other] == 0U) {
		arch_spin_relax();
		if (++spins > STALL_SPIN_MAX) {
			esp_rom_printf("esp_mp: cpu %d did not ack the stall\n", other);
			k_panic();
		}
	}
}

static void IRAM_ATTR release_other_cpu(void)
{
	int other = (esp_cpu_get_core_id() == 0) ? 1 : 0;
	uint32_t spins = 0;

	if (!s_stall_enabled || !s_cpu_up[other]) {
		return;
	}

	s_stall_req[other] = 0;

	barrier_dmem_fence_full();

	stall_trigger_set(other, false);

	while (s_stall_ack[other] != 0U) {
		arch_spin_relax();
		if (++spins > STALL_SPIN_MAX) {
			esp_rom_printf("esp_mp: cpu %d did not leave the stall\n", other);
			k_panic();
		}
	}
}

void IRAM_ATTR soc_mp_pause_others(void)
{
	unsigned int key;
	atomic_val_t me;

	if (!s_stall_enabled) {
		return;
	}

	key = arch_irq_lock();
	me = (atomic_val_t)esp_cpu_get_core_id();

	/* A parked core cannot be granted the pause. */
	if (s_stall_ack[me] != 0U) {
		esp_rom_printf("esp_mp: cpu %d requested a pause while parked\n", (int)me);
		k_panic();
	}

	if (atomic_get(&s_pause_owner) == me) {
		s_pause_nest++;
		arch_irq_unlock(key);
		return;
	}

	while (!atomic_cas(&s_pause_owner, -1, me)) {
		/* Open interrupts while waiting so the owner can park this core. */
		arch_irq_unlock(key);
		arch_nop();
		key = arch_irq_lock();
	}

	s_pause_irq_key[me] = key;
	stall_other_cpu();

	s_pause_nest = 1;
}

void IRAM_ATTR soc_mp_resume_others(void)
{
	unsigned int key;

	if (!s_stall_enabled) {
		return;
	}

	__ASSERT_NO_MSG(atomic_get(&s_pause_owner) == (atomic_val_t)esp_cpu_get_core_id());

	if (--s_pause_nest != 0) {
		return;
	}

	release_other_cpu();

	key = s_pause_irq_key[esp_cpu_get_core_id()];
	atomic_set(&s_pause_owner, -1);
	arch_irq_unlock(key);
}

void esp_mp_set_cpu_online(int cpu, bool online)
{
	barrier_dmem_fence_full();
	s_cpu_up[cpu] = online;
}

void esp_mp_stall_enable(void)
{
	barrier_dmem_fence_full();
	s_stall_enabled = true;
}

bool esp_mp_cpu_online(int cpu)
{
	return s_cpu_up[cpu];
}
