/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common fault handler for ARCv2
 *
 * Common fault handler for ARCv2 processors.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <inttypes.h>

#include <kernel.h>
#include <kernel_structs.h>
#include <misc/printk.h>
#include <exc_handle.h>

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_arch_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_arch_user_string_nlen)
};
#endif

/*
 * @brief Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine _SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy.
 */
void _Fault(NANO_ESF *esf)
{
	u32_t vector, code, parameter;
	u32_t exc_addr = _arc_v2_aux_reg_read(_ARC_V2_EFA);
	u32_t ecr = _arc_v2_aux_reg_read(_ARC_V2_ECR);

#ifdef CONFIG_USERSPACE
	for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
		u32_t start = (u32_t)exceptions[i].start;
		u32_t end = (u32_t)exceptions[i].end;

		if (esf->pc >= start && esf->pc < end) {
			esf->pc = (u32_t)(exceptions[i].fixup);
			return;
		}
	}
#endif

	vector = _ARC_V2_ECR_VECTOR(ecr);
	code =  _ARC_V2_ECR_CODE(ecr);
	parameter = _ARC_V2_ECR_PARAMETER(ecr);


	/* exception raised by kernel */
	if (vector == 0x9 && parameter == _TRAP_S_CALL_RUNTIME_EXCEPT) {
		_NanoFatalErrorHandler(esf->r0, esf);
		return;
	}

	printk("Exception vector: 0x%x, cause code: 0x%x, parameter 0x%x\n",
	       vector, code, parameter);
	printk("Address 0x%x\n", exc_addr);
#ifdef CONFIG_ARC_STACK_CHECKING
	/* Vector 6 = EV_ProV. Regardless of code, parameter 2 means stack
	 * check violation
	 */
	if (vector == 6 && parameter == 2) {
		_NanoFatalErrorHandler(_NANO_ERR_STACK_CHK_FAIL, esf);
		return;
	}
#endif
	_NanoFatalErrorHandler(_NANO_ERR_HW_EXCEPTION, esf);
}
