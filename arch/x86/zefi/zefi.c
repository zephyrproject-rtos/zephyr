/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include "efi.h"
#include "printf.h"
#include <zefi-segments.h>

#define PUTCHAR_BUFSZ 128

/* The linker places this dummy last in the data memory.  We can't use
 * traditional linker address symbols because we're relocatable; the
 * linker doesn't know what the runtime address will be.  The compiler
 * has to emit code to find this thing's address at runtime via an
 * offset from RIP.  It's a qword so we can guarantee alignment of the
 * stuff after.
 */
static __attribute__((section(".runtime_data_end")))
uint64_t runtime_data_end[1] = { 0x1111aa8888aa1111ULL };

#define EXT_DATA_START ((void *) &runtime_data_end[1])

static struct efi_system_table *efi;

static void efi_putchar(int c)
{
	static uint16_t efibuf[PUTCHAR_BUFSZ + 1];
	static int n;

	if (c == (int)'\n') {
		efi_putchar((int)'\r');
	}

	efibuf[n] = (uint16_t)c;
	++n;

	if ((c == (int)'\n') || (n == PUTCHAR_BUFSZ)) {
		efibuf[n] = 0U;
		efi->ConOut->OutputString(efi->ConOut, efibuf);
		n = 0;
	}
}

/* Existing x86_64 EFI environments have a bad habit of leaving the
 * HPET timer running.  This then fires later on, once the OS has
 * started.  If the timing isn't right, it can happen before the OS
 * HPET driver gets a chance to disable it.  And because we do the
 * handoff (necessarily) with interrupts disabled, it's not actually
 * possible for the OS to reliably disable it in time anyway.
 *
 * Basically: it's our job as the bootloader to ensure that no
 * interrupt sources are live before entering the OS. Clear the
 * interrupt enable bit of HPET timer zero.
 */
static void disable_hpet(void)
{
	uint64_t *hpet = (uint64_t *)0xfed00000L;

	hpet[32] &= ~4ULL;
}

/* FIXME: if you check the generated code, "ms_abi" calls like this
 * have to SPILL HALF OF THE SSE REGISTER SET TO THE STACK on entry
 * because of the way the conventions collide.  Is there a way to
 * prevent/suppress that?
 */
uintptr_t __abi efi_entry(void *img_handle, struct efi_system_table *sys_tab)
{
	efi = sys_tab;
	z_putchar = efi_putchar;
	printf("*** Zephyr EFI Loader ***\n");

	for (size_t i = 0; i < (sizeof(zefi_zsegs)/sizeof(zefi_zsegs[0])); i++) {
		uint32_t bytes = zefi_zsegs[i].sz;
		uint8_t *dst = (uint8_t *)zefi_zsegs[i].addr;

		printf("Zeroing %u bytes of memory at %p\n", bytes, dst);
		for (uint32_t j = 0; j < bytes; j++) {
			dst[j] = 0U;
		}
	}

	for (size_t i = 0; i < (sizeof(zefi_dsegs)/sizeof(zefi_dsegs[0])); i++) {
		uint32_t bytes = zefi_dsegs[i].sz;
		uint32_t off = zefi_dsegs[i].off;
		uint8_t *dst = (uint8_t *)zefi_dsegs[i].addr;
		uint8_t *src = &((uint8_t *)EXT_DATA_START)[off];

		printf("Copying %u data bytes to %p from image offset %u\n",
		       bytes, dst, zefi_dsegs[i].off);
		for (uint32_t j = 0; j < bytes; j++) {
			dst[j] = src[j];
		}

		/* Page-aligned blocks below 1M are the .locore
		 * section, which has a jump in its first bytes for
		 * the benefit of 32 bit entry.  Those have to be
		 * written over with NOP instructions. (See comment
		 * about OUTRAGEOUS HACK in locore.S) before Zephyr
		 * starts, because the very first thing it does is
		 * install its own page table that disallows writes.
		 */
		if ((((uintptr_t)dst & 0xfff) == 0) && ((uintptr_t)dst < 0x100000ULL)) {
			for (int i = 0; i < 8; i++) {
				dst[i] = 0x90; /* 0x90 == 1-byte NOP */
			}
		}
	}

	unsigned char *code = (void *)zefi_entry;

	printf("Jumping to Entry Point: %p (%x %x %x %x %x %x %x)\n",
	       code, code[0], code[1], code[2], code[3],
	       code[4], code[5], code[6]);

	disable_hpet();

	/* The EFI console seems to be buffered, give it a little time
	 * to drain before we start banging on the same UART from the
	 * OS.
	 */
	for (volatile int i = 0; i < 50000000; i += 1) {
	}

	__asm__ volatile("cli; jmp *%0" :: "r"(code));

	return 0;
}

/* Trick cribbed shamelessly from gnu-efi.  We need to emit a ".reloc"
 * section into the image with a single dummy entry for the EFI loader
 * to think we're a valid PE file, gcc won't because it thinks we're
 * ELF.
 */
uint32_t relocation_dummy;
__asm__(".section .reloc\n"
	"base_relocation_block:\n"
	".long relocation_dummy - base_relocation_block\n"
	".long 0x0a\n"
	".word	0\n");
