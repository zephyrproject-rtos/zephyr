/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/x86/multiboot.h>

/*
 * This is included by ia32/crt0.S and intel64/locore.S
 * at their 32-bit entry points to cover common ground.
 */

#ifdef CONFIG_MULTIBOOT_INFO
        /*
         * If we were loaded by a multiboot-compliant loader, then EAX
         * contains MULTIBOOT_EAX_MAGIC and EBX points to a valid 'struct
         * multiboot_info'; otherwise EBX is just junk. Check EAX early
	 * before it's clobbered and leave a sentinel (0) in EBX if invalid.
	 * The valid in EBX will be the argument to z_x86_prep_c(), so the
	 * subsequent code must, of course, be sure to preserve it meanwhile.
	 */

	cmpl $MULTIBOOT_EAX_MAGIC, %eax
	je 1f
	xorl %ebx, %ebx
1:
#endif

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/*
	 * Boot time measurements use TSC as a datum; zero it.
	 */

	xorl %eax, %eax
	xorl %edx, %edx
	movl $X86_TIME_STAMP_COUNTER_MSR, %ecx
	wrmsr
#endif

#ifdef CONFIG_PIC_DISABLE
	/*
	 * "Disable" legacy i8259 interrupt controllers. Note that we
	 * can't actually disable them, but we mask all their interrupt
	 * sources which is effectively the same thing (almost).
	 */

	movb $0xff, %al
	outb %al, $0x21
	outb %al, $0xA1
#endif

#ifdef CONFIG_MULTIBOOT
	jmp 1f

	.align 4
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_HEADER_FLAGS
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
#ifdef CONFIG_MULTIBOOT_FRAMEBUF
	.fill 5,4,0				/* (unused exec layout) */
	.long 0					/* linear graphics mode */
	.long CONFIG_MULTIBOOT_FRAMEBUF_X	/* width */
	.long CONFIG_MULTIBOOT_FRAMEBUF_Y	/* height */
	.long 32				/* depth */
#endif /* CONFIG_MULTIBOOT_FRAMEBUF */
1:
#endif
