/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <string.h>

#include <kernel_internal.h>

#include <zephyr/kernel.h>

#if defined(CONFIG_THREAD_MAX_NAME_LEN)
#define THREAD_MAX_NAM_LEN CONFIG_THREAD_MAX_NAME_LEN
#else
#define THREAD_MAX_NAM_LEN 10
#endif

static void shell_stack_dump(const struct k_thread *thread, void *user_data)
{
	const struct shell *sh = (const struct shell *)user_data;
	unsigned int pcnt;
	size_t unused;
	size_t size = thread->stack_info.size;
	const char *tname;
	int ret;

	ret = k_thread_stack_space_get(thread, &unused);
	if (ret) {
		shell_print(sh,
			    "Unable to determine unused stack size (%d)\n",
			    ret);
		return;
	}

	tname = k_thread_name_get((struct k_thread *)thread);

	/* Calculate the real size reserved for the stack */
	pcnt = ((size - unused) * 100U) / size;

	shell_print(sh,
		    "%p %-" STRINGIFY(THREAD_MAX_NAM_LEN) "s "
		    "(real size %4zu):\tunused %4zu\tusage %4zu / %4zu (%2u %%)",
		    thread, tname ? tname : "NA", size, unused, size - unused, size, pcnt);
}

K_KERNEL_STACK_ARRAY_DECLARE(z_interrupt_stacks, CONFIG_MP_MAX_NUM_CPUS,
			     CONFIG_ISR_STACK_SIZE);

static int cmd_kernel_thread_stacks(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	char pad[THREAD_MAX_NAM_LEN] = { 0 };

	memset(pad, ' ', MAX((THREAD_MAX_NAM_LEN - strlen("IRQ 00")), 1));

	/*
	 * Use the unlocked version as the callback itself might call
	 * arch_irq_unlock.
	 */
	k_thread_foreach_unlocked(shell_stack_dump, (void *)sh);

	/* Placeholder logic for interrupt stack until we have better
	 * kernel support, including dumping arch-specific exception-related
	 * stack buffers.
	 */
	unsigned int num_cpus = arch_num_cpus();

	for (int i = 0; i < num_cpus; i++) {
		size_t unused;
		const uint8_t *buf = K_KERNEL_STACK_BUFFER(z_interrupt_stacks[i]);
		size_t size = K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[i]);
		int err = z_stack_space_get(buf, size, &unused);

		(void)err;
		__ASSERT_NO_MSG(err == 0);

		shell_print(sh,
			    "%p IRQ %02d %s(real size %4zu):\tunused %4zu\tusage %4zu / %4zu (%2zu %%)",
			    &z_interrupt_stacks[i], i, pad, size, unused, size - unused, size,
			    ((size - unused) * 100U) / size);
	}

	return 0;
}

KERNEL_THREAD_CMD_ADD(stacks, NULL, "List threads stack usage.", cmd_kernel_thread_stacks);
