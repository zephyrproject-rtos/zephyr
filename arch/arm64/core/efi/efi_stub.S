/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * EFI stub: lets UEFI start the Zephyr image as an EFI application.
 *
 * UEFI loads the image wherever it wants and calls arm64_efi_entry() with
 * the MMU on. Zephyr is linked to run from a fixed RAM region, so the stub
 * reserves that region, copies the image there, exits boot services, and
 * jumps to __start with the MMU off.
 *
 * The stub runs from where UEFI loaded the image, not from its link address.
 * It is written in assembly so that no compiler option can add calls into
 * Zephyr or accesses to Zephyr data. "adr" and "adrp" give the load address
 * of a symbol, "ldr =" gives its link address. The stub only calls UEFI
 * services and the helpers in this file.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm64/macro.inc>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/sram.h>
#include <zephyr/arm64_efi_constants.h>

/* Values from the UEFI specification */
#define EFI_ALLOCATE_ADDRESS    2	/* EFI_ALLOCATE_TYPE AllocateAddress */
#define EFI_LOADER_DATA         2	/* EFI_MEMORY_TYPE EfiLoaderData */
#define EFI_CONVENTIONAL_MEMORY 7	/* EFI_MEMORY_TYPE EfiConventionalMemory */
#define EFI_BUFFER_TOO_SMALL    0x8000000000000005	/* EFI_STATUS */
#define EFI_PAGE_SHIFT          12

/* The RAM the linker places Zephyr in */
#define SRAM_BASE  DT_CHOSEN_SRAM_ADDR
#define SRAM_END   (DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE)
#define SRAM_PAGES ((DT_CHOSEN_SRAM_SIZE + (1 << EFI_PAGE_SHIFT) - 1) >> EFI_PAGE_SHIFT)

/*
 * Room for memory map entries UEFI may add between sizing the map and
 * ExitBootServices(), AllocatePool() itself being one source of them.
 */
#define MAP_SLACK_ENTRIES 32

/* Members of the structures in efi_constants.c and efi.h */
#define ST(m)    __struct_efi_system_table_##m##_OFFSET
#define BS(m)    __struct_efi_boot_services_##m##_OFFSET
#define CON(m)   __struct_efi_simple_text_output_##m##_OFFSET
#define MD(m)    __struct_efi_memory_descriptor_##m##_OFFSET
#define BI(m)    __struct_arm64_efi_boot_info_##m##_OFFSET
#define FRAME(m) __struct_efi_stub_frame_##m##_OFFSET
#define FRAME_SIZE __struct_efi_stub_frame_SIZEOF

/*
 * Registers arm64_efi_entry() sets up and the rest of this file relies on.
 * A function that uses x25 to x28 saves them.
 */
#define x_image     x19	/* image handle */
#define x_systab    x20	/* system table */
#define x_bootsvc   x21	/* boot services */
#define x_status    x22	/* EFI status to return */
#define x_ebs_tries x23	/* ExitBootServices() attempts left */
#define x_frame     x24	/* struct efi_stub_frame, the locals of arm64_efi_entry() */

/* Address operands for "[...]" and "add": register and member offset */
#define x_ST(m)    x_systab, #ST(m)
#define x_BS(m)    x_bootsvc, #BS(m)
#define x_FRAME(m) x_frame, #FRAME(m)

_ASM_FILE_PROLOGUE

/* Clean and invalidate [\start, \end) to the PoC. Clobbers \start, x9 and x10. */
.macro dcache_civac_range, start, end
	mrs	x9, ctr_el0
	ubfx	x9, x9, #16, #4		/* DminLine: log2 of words per line */
	mov	x10, #4
	lsl	x9, x10, x9		/* line size in bytes */
	sub	x10, x9, #1
	bic	\start, \start, x10
1:	dc	civac, \start
	add	\start, \start, x9
	cmp	\start, \end
	b.lo	1b
	dsb	sy
.endm

/* Call the boot service at offset \fn. Clobbers x9. */
.macro efi_bs_call, fn
	ldr	x9, [x_bootsvc, #\fn]
	blr	x9
.endm

/*
 * Jump to the firmware function at [\base, \off], which then returns
 * straight to our caller. A BR through x16 may land on the "bti c" of
 * firmware built with branch target identification.
 */
.macro efi_tail_call, base, off
	ldr	x16, [\base, \off]
	br	x16
.endm

/*
 * Print \fmt on the UEFI console with efi_printf(). Each '#' in \fmt prints
 * the next of \a1 to \a4 in hex. The arguments are registers other than x0
 * to x3. The string follows the call, and efi_printf() returns past it.
 */
.macro efi_print, fmt:req, a1, a2, a3, a4
	.ifnb \a1
	mov	x0, \a1
	.endif
	.ifnb \a2
	mov	x1, \a2
	.endif
	.ifnb \a3
	mov	x2, \a3
	.endif
	.ifnb \a4
	mov	x3, \a4
	.endif
	bl	efi_printf
	.asciz	"\fmt"
	.balign	4
.endm

/*
 * UEFI entry point, set as AddressOfEntryPoint in the PE header.
 * x0: image handle, x1: system table.
 * Returns an EFI status to UEFI on failure.
 */
GTEXT(arm64_efi_entry)
SECTION_FUNC(TEXT, arm64_efi_entry)
	stp	x29, x30, [sp, #-16]!
	mov	x29, sp
	stp	x19, x20, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	sub	sp, sp, #FRAME_SIZE
	mov	x_frame, sp
	mov	x_image, x0
	mov	x_systab, x1
	ldr	x_bootsvc, [x_ST(BootServices)]

	/* Reserve the RAM Zephyr is linked into */
	ldr	x9, =SRAM_BASE
	str	x9, [x_FRAME(base)]
	mov	x0, #EFI_ALLOCATE_ADDRESS
	mov	x1, #EFI_LOADER_DATA
	ldr	x2, =SRAM_PAGES
	add	x3, x_FRAME(base)
	efi_bs_call BS(AllocatePages)
	cbnz	x0, .Lalloc_failed

	/* Copy the image from where UEFI loaded it to its link address */
	ldr	x0, =__text_region_start
	adrp	x1, __text_region_start
	add	x1, x1, :lo12:__text_region_start
	ldr	x2, =__efi_file_end
	sub	x2, x2, x0
	efi_bs_call BS(CopyMem)

	/* UEFI leaves the map buffer alone after ExitBootServices(): it is EfiLoaderData */
	bl	alloc_map
	cbnz	x0, .Lmap_failed

	/*
	 * Fails if the memory map changed since GetMemoryMap(). After the
	 * first call, the firmware may have shut down part of boot services:
	 * only the memory allocation services may be called, so nothing is
	 * printed on failure.
	 */
	mov	x_ebs_tries, #2
.Lexit_boot_services:
	mov	x0, x_image
	ldr	x1, [x_FRAME(map_key)]
	efi_bs_call BS(ExitBootServices)
	cbz	x0, .Lexited
	subs	x_ebs_tries, x_ebs_tries, #1
	b.eq	.Lexit_failed
	bl	get_map
	cbz	x0, .Lexit_boot_services
.Lexit_failed:
	mov	x_status, x0
	b	.Lfree_pages

.Lexited:
	/* For arm64_efi_boot_info_get(), at its link address */
	ldr	x0, =z_arm64_efi_boot_info
	str	x_systab, [x0, #BI(system_table)]
	ldr	x1, [x_FRAME(map)]
	str	x1, [x0, #BI(memmap)]
	ldr	x1, [x_FRAME(map_size)]
	str	x1, [x0, #BI(memmap_size)]
	ldr	x1, [x_FRAME(desc_size)]
	str	x1, [x0, #BI(memmap_desc_size)]
	ldr	w1, [x_FRAME(desc_version)]
	str	w1, [x0, #BI(memmap_desc_version)]
	b	enter_zephyr

.Lmap_failed:
	mov	x_status, x0
	efi_print "Zephyr: no memory map, status #\n", x_status
.Lfree_pages:
	ldr	x0, =SRAM_BASE
	ldr	x1, =SRAM_PAGES
	efi_bs_call BS(FreePages)
	b	.Lfree_map

.Lalloc_failed:
	/* List the memory map entries over Zephyr's RAM that are not free */
	mov	x_status, x0
	ldr	x9, =SRAM_BASE
	ldr	x10, =(SRAM_END - 1)
	efi_print "Zephyr: RAM at #-# is not free:\n", x9, x10
	bl	alloc_map
	cbnz	x0, .Lfree_map

#define x_entry   x25	/* memory map entry */
#define x_map_end x26	/* end of the memory map */
#define x_MD(m)   x_entry, #MD(m)
	ldr	x_entry, [x_FRAME(map)]
	ldr	x_map_end, [x_FRAME(map_size)]
	add	x_map_end, x_entry, x_map_end
1:	cmp	x_entry, x_map_end
	b.hs	.Lfree_map
	ldr	w9, [x_MD(Type)]
	cmp	w9, #EFI_CONVENTIONAL_MEMORY
	b.eq	2f
	ldr	x10, [x_MD(PhysicalStart)]
	ldr	x11, [x_MD(NumberOfPages)]
	add	x11, x10, x11, lsl #EFI_PAGE_SHIFT
	ldr	x12, =SRAM_BASE
	cmp	x11, x12
	b.ls	2f
	ldr	x12, =SRAM_END
	cmp	x10, x12
	b.hs	2f
	sub	x11, x11, #1
	efi_print "  type # at #-#\n", x9, x10, x11
2:	ldr	x9, [x_FRAME(desc_size)]
	add	x_entry, x_entry, x9
	b	1b
#undef x_entry
#undef x_map_end
#undef x_MD

.Lfree_map:
	ldr	x0, [x_FRAME(map)]
	cbz	x0, 1f
	efi_bs_call BS(FreePool)
1:	mov	x0, x_status

.Lreturn:
	add	sp, sp, #FRAME_SIZE
	ldp	x25, x26, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x29, x30, [sp], #16
	ret

/*
 * Allocate a buffer for the memory map with AllocatePool(), with room for
 * MAP_SLACK_ENTRIES more entries, and get the map into it with get_map.
 * FRAME(map) stays 0 if AllocatePool() fails. Returns an EFI status.
 */
alloc_map:
	stp	x29, x30, [sp, #-16]!
	mov	x29, sp
	str	xzr, [x_FRAME(map)]
	/* With a zero size, GetMemoryMap() only returns the sizes */
	str	xzr, [x_FRAME(map_size)]
	add	x0, x_FRAME(map_size)
	mov	x1, xzr
	add	x2, x_FRAME(map_key)
	add	x3, x_FRAME(desc_size)
	add	x4, x_FRAME(desc_version)
	efi_bs_call BS(GetMemoryMap)
	ldr	x9, =EFI_BUFFER_TOO_SMALL
	cmp	x0, x9
	b.ne	1f
	ldr	x1, [x_FRAME(map_size)]
	ldr	x9, [x_FRAME(desc_size)]
	mov	x10, #MAP_SLACK_ENTRIES
	madd	x1, x9, x10, x1
	str	x1, [x_FRAME(map_alloc_size)]
	mov	x0, #EFI_LOADER_DATA
	add	x2, x_FRAME(map)
	efi_bs_call BS(AllocatePool)
	cbnz	x0, 1f
	ldp	x29, x30, [sp], #16
	b	get_map
1:	ldp	x29, x30, [sp], #16
	ret

/*
 * Get the memory map into the alloc_map buffer. Returns an EFI status:
 * GetMemoryMap() returns straight to the caller.
 */
get_map:
	ldr	x9, [x_FRAME(map_alloc_size)]
	str	x9, [x_FRAME(map_size)]
	add	x0, x_FRAME(map_size)
	ldr	x1, [x_FRAME(map)]
	add	x2, x_FRAME(map_key)
	add	x3, x_FRAME(desc_size)
	add	x4, x_FRAME(desc_version)
	efi_tail_call x_BS(GetMemoryMap)

/*
 * Print the NUL-terminated ASCII string at the return address on the UEFI
 * console, with "\n" printed as "\r\n" and each '#' replaced by the next of
 * x0 to x3 in hex. Returns to the first 4-byte boundary past the string.
 */
#define x_fmt      x25	/* next format character */
#define x_next_arg x26	/* next argument */
#define x_value    x27	/* argument being printed */
#define x_shift    x28	/* shift of its next hex digit */
efi_printf:
	stp	x25, x26, [sp, #-16]!
	stp	x27, x28, [sp, #-16]!
	/* The arguments, in order from sp */
	stp	x2, x3, [sp, #-16]!
	stp	x0, x1, [sp, #-16]!
	mov	x_fmt, x30
	mov	x_next_arg, sp

1:	ldrb	w0, [x_fmt], #1
	cbz	w0, 4f
	cmp	w0, #'#'
	b.eq	3f
	cmp	w0, #'\n'
	b.ne	2f
	mov	w0, #'\r'
	bl	efi_putc
	mov	w0, #'\n'
2:	bl	efi_putc
	b	1b

3:	/* Print "0x" and the argument without leading zeros */
	ldr	x_value, [x_next_arg], #8
	mov	w0, #'0'
	bl	efi_putc
	mov	w0, #'x'
	bl	efi_putc
	orr	x9, x_value, #1		/* at least one digit */
	clz	x_shift, x9
	mov	x9, #63
	sub	x_shift, x9, x_shift
	and	x_shift, x_shift, #~3
5:	lsr	x9, x_value, x_shift
	and	x9, x9, #0xf
	adr	x10, .Lhex_digits
	ldrb	w0, [x10, x9]
	bl	efi_putc
	subs	x_shift, x_shift, #4
	b.hs	5b
	b	1b

4:	add	x9, x_fmt, #3		/* x_fmt is past the NUL */
	and	x9, x9, #~3
	add	sp, sp, #32		/* the arguments */
	ldp	x27, x28, [sp], #16
	ldp	x25, x26, [sp], #16
	ret	x9
#undef x_fmt
#undef x_next_arg
#undef x_value
#undef x_shift

.Lhex_digits:
	.ascii	"0123456789abcdef"

/*
 * Print the ASCII character w0 on the UEFI console. Storing w0 as 32 bits
 * writes the UCS-2 character and its terminating NUL. OutputString()
 * returns straight to the caller.
 */
efi_putc:
	str	w0, [x_FRAME(text)]
	add	x1, x_FRAME(text)
	ldr	x0, [x_ST(ConOut)]
	efi_tail_call x0, #CON(OutputString)

/*
 * Runs after ExitBootServices(), still on the UEFI page tables.
 * Cleans the copied image to the PoC, turns off the MMU and the data cache,
 * and jumps to __start the way a bootloader would.
 */
enter_zephyr:
	msr	DAIFSet, #0xf
	ldr	x0, =__text_region_start
	ldr	x1, =_image_ram_end
	ldr	x4, =__start

	dcache_civac_range x0, x1

	/*
	 * The code from .Lmmu_off on runs with the MMU off, fetched from memory
	 * UEFI wrote with the data cache on. Push it to the PoC too.
	 */
	adr	x0, .Lmmu_off
	adr	x1, .Lmmu_off_end
	dcache_civac_range x0, x1
	ic	iallu
	dsb	sy
	isb

.Lmmu_off:
	mov	x1, #(SCTLR_M_BIT | SCTLR_C_BIT)
	switch_el x0, .Lel3, .Lel2, .Lel1
.Lel2:
	mrs	x0, sctlr_el2
	bic	x0, x0, x1
	msr	sctlr_el2, x0
	isb
	/* Drop whatever UEFI set in HCR_EL2 (VHE, stage 2, traps): EL1 is AArch64, that's all */
	mov	x0, #HCR_RW_BIT
	msr	hcr_el2, x0
	isb
	b	.Ljump
.Lel1:
	mrs	x0, sctlr_el1
	bic	x0, x0, x1
	msr	sctlr_el1, x0
	isb
.Ljump:
	mov	x0, xzr
	mov	x1, xzr
	mov	x2, xzr
	mov	x3, xzr
	br	x4

	/* UEFI never runs at EL3 */
.Lel3:	b	.Lel3
.Lmmu_off_end:
