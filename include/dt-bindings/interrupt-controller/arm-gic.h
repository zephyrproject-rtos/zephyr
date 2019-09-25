/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DT_BINDING_ARM_GIC_H
#define __DT_BINDING_ARM_GIC_H

/* CPU Interrupt numbers */
#define	GIC_INT_VIRT_MAINT		25
#define	GIC_INT_HYP_TIMER		26
#define	GIC_INT_VIRT_TIMER		27
#define	GIC_INT_LEGACY_FIQ		28
#define	GIC_INT_PHYS_TIMER		29
#define	GIC_INT_NS_PHYS_TIMER	30
#define	GIC_INT_LEGACY_IRQ		31

#define	IRQ_TYPE_LEVEL		0x0
#define	IRQ_TYPE_EDGE		0x1

#define IRQ_DEFAULT_PRIORITY	0xa

#endif
