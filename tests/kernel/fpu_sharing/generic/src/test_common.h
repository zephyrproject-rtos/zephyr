/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MAX_TESTS	500

/*
 * Test Thread Parameters
 */
#define THREAD_STACK_SIZE	(1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define THREAD_HIGH_PRIORITY	5
#define THREAD_LOW_PRIORITY	10

#if defined(CONFIG_X86) && defined(CONFIG_X86_SSE)
#define THREAD_FP_FLAGS		(K_FP_REGS | K_SSE_REGS)
#else
#define THREAD_FP_FLAGS		(K_FP_REGS)
#endif
