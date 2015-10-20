/* "do nothing" kernel service */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
This module provides a "do nothing" kernel service.

This service is primarily used by other kernel services that need a way to
resume the execution of a kernel request that could not be completed in a
single invocation of the _k_server fiber. However, it can also be used by
a task to measure the overhead involved in issuing a kernel service request.
 */

#include <micro_private.h>
#include <toolchain.h>
#include <sections.h>

/**
 *
 * @brief Perform "do nothing" kernel request
 *
 * @return N/A
 */
void _k_nop(struct k_args *A)
{
	ARG_UNUSED(A);
}

/**
 *
 * @brief "do nothing" kernel request
 *
 * This routine is a request for the _k_server to run a "do nothing" routine.
 *
 * @return N/A
 */
void _task_nop(void)
{
	struct k_args A;

	A.Comm = _K_SVC_NOP;
	KERNEL_ENTRY(&A);
}
