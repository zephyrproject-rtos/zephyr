/* fiber_abort.c - ARM Cortex-M fiber_abort() routine */

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
The ARM Cortex-M architecture provides its own fiber_abort() to deal with
different CPU modes (handler vs thread) when a fiber aborts. When its entry
point returns or when it aborts itself, the CPU is in thread mode and must
call _Swap() (which triggers a service call), but when in handler mode, the
CPU must exit handler mode to cause the context switch, and thus must queue
the PendSV exception.
 */

#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#include <micro_private_types.h>
#endif

#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>
#include <nanokernel.h>
#include <arch/cpu.h>

/**
 *
 * @brief Abort the currently executing fiber
 *
 * Possible reasons for a fiber aborting:
 *
 * - the fiber explicitly aborts itself by calling this routine
 * - the fiber implicitly aborts by returning from its entry point
 * - the fiber encounters a fatal exception
 *
 * @return N/A
 */

void fiber_abort(void)
{
	_context_exit(_nanokernel.current);
	if (_ScbIsInThreadMode()) {
		_nano_fiber_swap();
	} else {
		_ScbPendsvSet();
	}
}
