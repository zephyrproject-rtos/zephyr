/* main.c - test IRQs installed in vector table */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
Set up three software IRQs: the ISR for each will print that it runs and then
release a semaphore. The task then verifies it can obtain all three
semaphores.

The ISRs are installed at build time, directly in the vector table.
 */

#if !defined(CONFIG_CPU_CORTEX_M3_M4)
  #error project can only run on Cortex-M3/M4
#endif

#include <arch/cpu.h>
#include <tc_util.h>
#include <sections.h>

struct nano_sem sem[3];

/**
 *
 * @brief ISR for IRQ0
 *
 * @return N/A
 */

void isr0(void)
{
	printk("%s ran!\n", __FUNCTION__);
	nano_isr_sem_give(&sem[0]);
	_IntExit();
}

/**
 *
 * @brief ISR for IRQ1
 *
 * @return N/A
 */

void isr1(void)
{
	printk("%s ran!\n", __FUNCTION__);
	nano_isr_sem_give(&sem[1]);
	_IntExit();
}

/**
 *
 * @brief ISR for IRQ2
 *
 * @return N/A
 */

void isr2(void)
{
	printk("%s ran!\n", __FUNCTION__);
	nano_isr_sem_give(&sem[2]);
	_IntExit();
}

/**
 *
 * @brief Task entry point
 *
 * @return N/A
 */

void main(void)
{
	TC_START("Test Cortex-M3 IRQ installed directly in vector table");

	for (int ii = 0; ii < 3; ii++) {
		irq_enable(ii);
		irq_priority_set(ii, _EXC_IRQ_DEFAULT_PRIO);
		nano_sem_init(&sem[ii]);
	}

	int rv;
	rv = nano_task_sem_take(&sem[0]) ||
		 nano_task_sem_take(&sem[1]) ||
		 nano_task_sem_take(&sem[2]) ? TC_FAIL : TC_PASS;

	if (TC_FAIL == rv) {
		goto get_out;
	}

	for (int ii = 0; ii < 3; ii++) {
		_NvicSwInterruptTrigger(ii);
	}

	rv = nano_task_sem_take(&sem[0]) &&
		 nano_task_sem_take(&sem[1]) &&
		 nano_task_sem_take(&sem[2]) ? TC_PASS : TC_FAIL;

get_out:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}

typedef void (*vth)(void); /* Vector Table Handler */
vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS] = {
	isr0, isr1, isr2
	};
