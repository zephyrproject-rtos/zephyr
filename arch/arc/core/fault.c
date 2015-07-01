/* fault.c - common fault handler for ARCv2 */

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
 * DESCRIPTION
 * Common fault handler for ARCv2 processors.
 */

#include <toolchain.h>
#include <sections.h>

#include <nanokernel.h>
#include <nano_private.h>

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PR_EXC(...) printk(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK */

#if (CONFIG_FAULT_DUMP > 0)
#define FAULT_DUMP(esf, fault) _FaultDump(esf, fault)
#else
#define FAULT_DUMP(esf, fault) \
	do {                   \
		(void) esf;    \
		(void) fault;  \
	} while ((0))
#endif

#if (CONFIG_FAULT_DUMP > 0)
/*
 * _FaultDump - dump information regarding fault (FAULT_DUMP > 0)
 *
 * Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 1
 * (short form).
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void _FaultDump(const NANO_ESF *esf, int fault)
{
	ARG_UNUSED(esf);
	uint32_t exc_addr = _arc_v2_aux_reg_read(_ARC_V2_EFA);
	uint32_t ecr = _arc_v2_aux_reg_read(_ARC_V2_ECR);

	PR_EXC("Exception vector: 0x%x, cause code: 0x%x, parameter 0x%x\n",
	       _ARC_V2_ECR_VECTOR(ecr),
	       _ARC_V2_ECR_CODE(ecr),
	       _ARC_V2_ECR_PARAMETER(ecr));
	PR_EXC("Address 0x%x\n", exc_addr);
}
#endif /* CONFIG_FAULT_DUMP */

/*
 * _Fault - fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine _SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy.
 *
 * @return This function does not return.
 *
 * \NOMANUAL
 */

void _Fault(void)
{
	uint32_t ecr = _arc_v2_aux_reg_read(_ARC_V2_ECR);

	FAULT_DUMP(&_default_esf, ecr);

	_SysFatalErrorHandler(_NANO_ERR_HW_EXCEPTION, &_default_esf);
}
