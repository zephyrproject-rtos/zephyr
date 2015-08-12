/* offload to fiber kernel service */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

#include <micro_private.h>
#include <sections.h>

/**
 *
 * @brief Process an "offload to fiber" request
 *
 * This routine simply invokes the requested function from within the context
 * of the _k_server() fiber and saves the result.
 *
 * @return N/A
 */

void _k_offload_to_fiber(struct k_args *A)
{
	A->Args.u1.rval = (*A->Args.u1.func)(A->Args.u1.argp);
}

/**
 *
 * @brief Issue a custom call from within the microkernel server fiber
 *
 * @func: function to call from within the microkernel server fiber
 * @argp: argument to pass to custom function
 *
 * This routine issues a request to execute a function from within the context
 * of the microkernel server fiber.
 *
 * @return return value from custom <func> call
 */

int task_offload_to_fiber(int (*func)(), void *argp)
{
	struct k_args A;

	A.Comm = _K_SVC_OFFLOAD_TO_FIBER;
	A.Args.u1.func = func;
	A.Args.u1.argp = argp;
	KERNEL_ENTRY(&A);
	return A.Args.u1.rval;
}
