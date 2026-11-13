/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/div64.h>

LOG_MODULE_REGISTER(sys_clock_hw_cycles, CONFIG_KERNEL_LOG_LEVEL);

#if CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC > 0
struct sys_div64_recip z_sys_clock_cyc_recip =
	SYS_DIV64_RECIP_INIT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
#else
struct sys_div64_recip z_sys_clock_cyc_recip;
#endif
static struct k_spinlock lock;

void z_sys_clock_hw_cycles_per_sec_set(uint32_t hz)
{
	struct sys_div64_recip r;
	k_spinlock_key_t key;

	__ASSERT(hz != 0U, "zero system timer frequency");

	sys_div64_recip_init(&r, hz);

	key = k_spin_lock(&lock);
	z_div64_recip_copy(&z_sys_clock_cyc_recip, &r);
	k_spin_unlock(&lock, key);
}

const struct sys_div64_recip *z_impl_z_sys_clock_cyc_recip_get(struct sys_div64_recip *buf)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	z_div64_recip_copy(buf, &z_sys_clock_cyc_recip);
	k_spin_unlock(&lock, key);

	return buf;
}

#ifdef CONFIG_USERSPACE
static inline const struct sys_div64_recip *
z_vrfy_z_sys_clock_cyc_recip_get(struct sys_div64_recip *buf)
{
	struct sys_div64_recip r;

	(void)z_impl_z_sys_clock_cyc_recip_get(&r);
	K_OOPS(k_usermode_to_copy(buf, &r, sizeof(r)));

	return buf;
}
#include <zephyr/syscalls/z_sys_clock_cyc_recip_get_mrsh.c>
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
/*
 * Weak default: only publish the new frequency value.
 * System timer drivers that rescale the cycle counter, cache derived
 * constants, or need to reprogram hardware should override this symbol.
 */
void __weak z_sys_clock_hw_cycles_per_sec_update(uint32_t new_hz)
{
	if (new_hz == 0U) {
		return;
	}

	uint32_t old_hz = (uint32_t)z_clock_hw_cycles_per_sec;

	if (old_hz == new_hz) {
		return;
	}

	z_sys_clock_hw_cycles_per_sec_set(new_hz);

	LOG_WRN("system timer frequency updated at runtime (%u -> %u Hz) "
		"without a %s() override",
		old_hz, new_hz, __func__);
}
#endif /* CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE */

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#ifdef CONFIG_USERSPACE
static inline unsigned int z_vrfy_sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	return z_impl_sys_clock_hw_cycles_per_sec_runtime_get();
}
#include <zephyr/syscalls/sys_clock_hw_cycles_per_sec_runtime_get_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */
