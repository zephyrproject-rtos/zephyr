/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>

/**
 *
 * @brief Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine _SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy.
 *
 * This is a stub for more exception handling code to be added later.
 */
void z_arm_fault(z_arch_esf_t *esf, u32_t exc_return)
{
	z_arm_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}

void z_arm_fault_init(void)
{
}
