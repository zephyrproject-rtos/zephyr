/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nano_private.h>
#include <drivers/loapic.h>

#ifdef CONFIG_SYS_POWER_MANAGEMENT

#if defined(CONFIG_NANOKERNEL) && defined(CONFIG_TICKLESS_IDLE)
extern void _power_save_idle_exit(void);
#else
extern void _sys_power_save_idle_exit(int32_t ticks);
#endif

#endif

#ifdef CONFIG_NESTED_INTERRUPTS
static inline void enable_nested_interrupts(void)
{
	__asm__ volatile("sti");
}

static inline void disable_nested_interrupts(void)
{
	__asm__ volatile("cli");
}
#else
static inline void enable_nested_interrupts(void){};
static inline void disable_nested_interrupts(void){};
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
extern void _sys_k_event_logger_interrupt(void);
#else
#define _sys_k_event_logger_interrupt()
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
extern void _sys_k_event_logger_exit_sleep(void);
#else
#define _sys_k_event_logger_exit_sleep()
#endif

typedef void (*int_handler_t) (int context);

void _execute_handler(int_handler_t function, int context)
{
	_int_latency_start();

	_sys_k_event_logger_interrupt();

	_sys_k_event_logger_exit_sleep();

#ifdef CONFIG_NESTED_INTERRUPTS
	if (!_nanokernel.nested)
#endif
	{
		/* move to the interrupt stack and push current stack
		 * pointer onto interrupt stack
		 */
		__asm__ volatile ("movl %%esp, %%edx \n\t"
				  "movl %0, %%esp \n\t"
				  "pushl %%edx \n\t"
				  :
				  :"m" (_nanokernel.common_isp)
				  :"%edx"
				 );
	}
	_nanokernel.nested++;

#ifdef CONFIG_SYS_POWER_MANAGEMENT

#if defined(CONFIG_NANOKERNEL) && defined(CONFIG_TICKLESS_IDLE)
	_power_save_idle_exit();
#else
	if (_nanokernel.idle) {
		_sys_power_save_idle_exit(_nanokernel.idle);
		_nanokernel.idle = 0;
	}
#endif

#endif
	_int_latency_stop();
	enable_nested_interrupts();

	(*function)(context);
	_loapic_eoi();

	disable_nested_interrupts();
	_nanokernel.nested--;

	/* Are we returning to a task or fiber context? If so we need
	 * to do some work based on the context that was interrupted
	 */
#ifdef CONFIG_NESTED_INTERRUPTS
	if (!_nanokernel.nested)
#endif
	{
		/* switch to kernel stack */
		__asm__ volatile ("popl %esp");

		/* if the interrupted context was a task we need to
		 * swap back to the interrupted context
		 */
		if ((_nanokernel.current->flags & PREEMPTIBLE) &&
			_nanokernel.fiber) {
			/* move flags into arg0 we can't use local
			 * variables here since the stack may have
			 * changed.
			 */
			__asm__ volatile ("pushfl \n\t"
					  "popl %eax \n\t"
					  "call _Swap");
		}
	}
}

void _SpuriousIntHandler(void)
{
	__asm__ volatile("cld"); /* clear direction flag */

	/*
	 * The task's regular stack is being used, but push the value of ESP
	 * anyway so that _ExcExit can "recover the stack pointer"
	 * without determining whether the exception occurred while CPL=3
	 */
	__asm__ volatile ("pushl %esp");

again:
	_NanoFatalErrorHandler(_NANO_ERR_SPURIOUS_INT, &_default_esf);
	/* The handler should no return but if it does call it again */
	goto again;
}

void _SpuriousIntNoErrCodeHandler(void)
{
	__asm__ volatile ("pushl %eax");
	_SpuriousIntHandler();
}
