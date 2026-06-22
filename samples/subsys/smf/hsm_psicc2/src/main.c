/*
 * Copyright (c) 2024 Glenn Andrews
 * State Machine example copyright (c) Miro Samek
 *
 * Implementation of the statechart in Figure 2.11 of
 * Practical UML Statecharts in C/C++, 2nd Edition by Miro Samek
 * https://www.state-machine.com/psicc2
 * Used with permission of the author.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "hsm_psicc2_thread.h"

int main(void)
{
	printk("State Machine Framework Demo\n");
	printk("See PSiCC2 Fig 2.11 for the statechart\n");
	printk("https://www.state-machine.com/psicc2\n\n");
	hsm_psicc2_thread_run();
	return 0;
}
