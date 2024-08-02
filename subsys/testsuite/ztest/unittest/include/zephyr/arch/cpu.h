/*
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SUBSYS_TESTSUITE_ZTEST_INCLUDE_ARCH_CPU_H
#define ZEPHYR_SUBSYS_TESTSUITE_ZTEST_INCLUDE_ARCH_CPU_H

/* This file exists as a hack around Zephyr's dependencies */

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Architecture thread structure */
struct _callee_saved {
#ifdef CONFIG_CPP
	/* C++ does not allow empty structs, add an extra 1 byte */
	uint8_t c;
#endif
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
#ifdef CONFIG_CPP
	/* C++ does not allow empty structs, add an extra 1 byte */
	uint8_t c;
#endif
};

typedef struct _thread_arch _thread_arch_t;

/* Architecture functions */
static inline uint32_t arch_k_cycle_get_32(void)
{
	return 0;
}

static inline uint64_t arch_k_cycle_get_64(void)
{
	return 0;
}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	return 0;
}

static inline void arch_irq_unlock(unsigned int key)
{
	ARG_UNUSED(key);
}

static inline bool arch_irq_unlocked(unsigned int key)
{
	return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <zephyr/arch/arch_interface.h>


#endif /* ZEPHYR_SUBSYS_TESTSUITE_ZTEST_INCLUDE_ARCH_CPU_H */
