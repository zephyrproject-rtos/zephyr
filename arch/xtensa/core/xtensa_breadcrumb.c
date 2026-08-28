/*
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa_breadcrumb.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

/* No-op default, overridden by SoCs that support fatal breadcrumbs. */
void __weak xtensa_fatal_breadcrumb(const _xtensa_irq_bsa_t *bsa, int cause)
{
	ARG_UNUSED(bsa);
	ARG_UNUSED(cause);
}
