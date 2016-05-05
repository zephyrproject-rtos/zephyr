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

void _new_thread(char *stack_memory, unsigned stack_size,
		 void *uk_task_ptr, _thread_entry_t thread_func,
		 _thread_arg_t arg1, _thread_arg_t arg2,
		 _thread_arg_t arg3,
		 int prio, unsigned options)
{
	/* STUB */
}
