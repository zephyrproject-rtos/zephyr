/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POSIX arch specific kernel interface header
 * This header contains the POSIX arch specific kernel interface.
 * It is included by the generic kernel interface header (include/arch/cpu.h)
 *
 */


#ifndef ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_H_

#include <stdlib.h>

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#include <zephyr/toolchain.h>
#include <zephyr/irq.h>
#include <zephyr/arch/posix/asm_inline.h>
#include <zephyr/arch/posix/thread.h>
#include <board_irq.h> /* Each board must define this */
#include <zephyr/sw_isr_table.h>
#include <zephyr/arch/posix/posix_soc_if.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_64BIT
#define ARCH_STACK_PTR_ALIGN 8
#else
#define ARCH_STACK_PTR_ALIGN 4
#endif

/* "native_posix" should really use CONFIG_LOG_MODE_IMMEDIATE=y but better safe
 * than sorry: debugging crashes is painful enough already, so try to be nice
 * and flush all messages. The deferred logging code may also want to enjoy
 * native_posix too.
 *
 * Other archs use Zephyr's elaborate "Fatal Errors" framework which takes care
 * of flushing logs but native_posix is simpler so we have to do it ourselves.
 */
static inline void posix_arch_log_immediate(void)
{
#if defined(CONFIG_LOG) && !defined(CONFIG_LOG_MODE_MINIMAL)
	/* We can't #include the higher-level "log_ctrl.h" in this low-level
	 * file here because this descends into .h dependency hell.  So let's
	 * use a one-line forward declaration instead. This void->void
	 * prototype does not look like it's going to change much in the
	 * future.
	 *
	 * We can't just declare "void log_panic(void);" here because system
	 * calls are complicated. As ARCH_EXCEPT() can't be possibly used in
	 * user mode, log_panic() is equivalent to just z_impl_log_panic().
	 * Exceptionally invoke the latter directly.
	 */
	extern void z_impl_log_panic(void);
	z_impl_log_panic();
#endif
}

/* Copied from kernel.h */
#define _EXCEPT_LOC() __ASSERT_PRINT("@ %s:%d\n", __FILE__, __LINE__)

/* Invoked by k_panic() and k_oops().
 *
 * _EXCEPT_LOC() and "ZEPHYR FATAL ERROR" mimic z_fatal_error()
 *
 * Many [Z]TESTs invoke k_panic(). The test framework expects that to
 * hang forever like hardware does; so don't exit / don't interfere.
 */
#if !defined(CONFIG_ZTEST) && !defined(CONFIG_TEST)
#define ARCH_EXCEPT(reason_p) do { \
	posix_arch_log_immediate(); \
	_EXCEPT_LOC(); \
	printk("ZEPHYR FATAL ERROR: %u\n", reason_p);  \
	abort(); CODE_UNREACHABLE; \
} while (false)
#endif

/* Exception context */
struct __esf {
	uint32_t dummy; /*maybe we will want to add something someday*/
};

typedef struct __esf z_arch_esf_t;

extern uint32_t sys_clock_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

extern uint64_t sys_clock_cycle_get_64(void);

static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key == false;
}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	return posix_irq_lock();
}


static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	posix_irq_unlock(key);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_H_ */
