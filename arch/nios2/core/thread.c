/*
 * Copyright (c) 2016 Intel Corporation
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

#include <nanokernel.h>
#include <nano_private.h>

tNANO _nanokernel = {0};

/**
 *
 * @brief Create a new kernel execution context
 *
 * This function initializes a thread control structure (TCS) for a
 * new kernel execution context. A fake stack frame is created as if
 * the context had been "swapped out" via _Swap()
 *
 * @param stack_memory pointer to the context stack area
 * @param stack_size size of contexts stack area
 * @param thread_func new contexts entry function
 * @param parameter1 first entry function parameter
 * @param parameter2 second entry function parameter
 * @param parameter3 third entry function parameter
 * @param priority Priority of the new context
 * @param options Additional options for the context
 *
 * @return none
 *
 * \NOMANUAL
 */

void _new_thread(char *stack_memory, unsigned stack_size,
		 _thread_entry_t thread_func, void *parameter1,
		 void *parameter2, void *parameter3, int priority,
		 unsigned options)
{
	/* STUB */
}
