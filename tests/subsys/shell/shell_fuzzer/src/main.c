/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Interactive shell fuzzer
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/irq.h>

/* Fuzz input received from LLVM via "interrupt" */
extern uint8_t *posix_fuzz_buf, posix_fuzz_sz;

K_SEM_DEFINE(fuzz_sem, 0, K_SEM_MAX_LIMIT);

static void fuzz_isr(const void *arg)
{
	k_sem_give(&fuzz_sem);
}

/* Forward-declare the libFuzzer's mutator callback. */
size_t LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

/* The custom mutator:
 * make sure every string is terminated
 */
size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed)
{
	Size = LLVMFuzzerMutate(Data, Size, MaxSize);
	Data[Size - 1] = 0;
	return Size;
}

int main(void)
{
	printk("Shell libfuzzer test %s\n", CONFIG_BOARD);

	IRQ_CONNECT(CONFIG_ARCH_POSIX_FUZZ_IRQ, 0, fuzz_isr, NULL, 0);
	irq_enable(CONFIG_ARCH_POSIX_FUZZ_IRQ);

	const struct shell *sh = shell_backend_dummy_get_ptr();

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));

	while (true) {
		k_sem_take(&fuzz_sem, K_FOREVER);

		/* Execute the fuzz case we got from LLVM and passed
		 * through an interrupt to this thread.
		 */
		if (posix_fuzz_sz == 0 || posix_fuzz_sz == 1) {
			/* discard 0 size vectors as it is no a valid string
			 * discard 1 size vectors as they do not have termination
			 */
			continue;
		}
		shell_execute_cmd(sh, posix_fuzz_buf);
	}
	return 0;
}
