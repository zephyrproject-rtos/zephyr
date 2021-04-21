/*
 * Copyright (c) 2018 Ding Tao <miyatsu@qq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file irq_relay.S
 *
 * @brief IRQ relay vector table and relay handler for Cortex-M0 or
 *        Armv8-M baseline SoCs
 *
 * In certain ARMv6-M and Armv8-M baseline cores the vector table address can
 * not be changed. Once the * vector table is occupied by bootloader, there
 * will be no IRQ support in the chainloaded image.
 *
 * This program will link into bootloader, once an interrupt is coming,
 * the bootloader can forward the interrupt to the chainloaded image. This
 * will support DFU on those cores.
 *
 * Note: Currently support mcuboot only.
 * */

#include <toolchain.h>
#include <linker/sections.h>

_ASM_FILE_PROLOGUE

GDATA(_vector_table_pointer)
GDATA(z_main_stack)

SECTION_FUNC(TEXT, __vector_relay_handler)
	mrs	r0, ipsr;
	lsls	r0, r0, $0x02;

	ldr	r1, =_vector_table_pointer;
	ldr	r1, [r1];
	adds	r1, r1, r0;
	ldr	r1, [r1];

	/**
	 * The size of IRQ vector is 4 bytes, the offset within vector table
	 * is the IRQ number times 4 (aka r0 << 2). As know as the r1 stored
	 * the offset of real vector table, thus the (r1 = r1 + r0 << 2) will
	 * be the real irq handle vector.
	 * */

	bx	r1;

GTEXT(__vector_relay_handler)

SECTION_FUNC(vector_relay_table, __vector_relay_table)
	.word z_main_stack + CONFIG_MAIN_STACK_SIZE

	.word z_arm_reset

	.word __vector_relay_handler	/* nmi */
	.word __vector_relay_handler	/* hard fault */
	.word __vector_relay_handler
	.word __vector_relay_handler
	.word __vector_relay_handler
	.word __vector_relay_handler
	.word __vector_relay_handler
	.word __vector_relay_handler
	.word __vector_relay_handler
	.word __vector_relay_handler	/* svc */
	.word __vector_relay_handler
	.word __vector_relay_handler
	.word __vector_relay_handler	/* pendsv */
	.word __vector_relay_handler
	/* End of system exception */

	.rept CONFIG_NUM_IRQS
	.word __vector_relay_handler
	.endr

GDATA(__vector_relay_table)
