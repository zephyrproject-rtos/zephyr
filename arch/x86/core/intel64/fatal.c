/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

void z_x86_exception(z_arch_esf_t *esf)
{
	switch (esf->vector) {
	case IV_PAGE_FAULT:
		z_x86_page_fault_handler(esf);
		break;
	default:
		z_x86_unhandled_cpu_exception(esf->vector, esf);
		CODE_UNREACHABLE;
	}
}
