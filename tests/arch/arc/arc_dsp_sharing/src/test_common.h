/*
 * Copyright (c) 2022 Synopsys
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

#define THREAD_DSP_FLAGS		(K_DSP_REGS | K_AGU_REGS)
