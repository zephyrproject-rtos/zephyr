/* offload to fiber kernel service */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

#include <micro_private.h>
#include <sections.h>

/**
 * @brief Process an "offload to fiber" request
 *
 * This routine simply invokes the requested function from within the context
 * of the _k_server() fiber and saves the result.
 * @param A Arguments
 *
 * @return N/A
 */
void _k_offload_to_fiber(struct k_args *A)
{
	A->args.u1.rval = (*A->args.u1.func)(A->args.u1.argp);
}

int task_offload_to_fiber(int (*func)(), void *argp)
{
	struct k_args A;

	A.Comm = _K_SVC_OFFLOAD_TO_FIBER;
	A.args.u1.func = func;
	A.args.u1.argp = argp;
	KERNEL_ENTRY(&A);
	return A.args.u1.rval;
}
