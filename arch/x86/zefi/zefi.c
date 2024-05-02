/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "efi.h"
#include "printf.h"
#include <zefi-segments.h>
#include <zephyr/arch/x86/efi.h>

#define PUTCHAR_BUFSZ 128

/* EFI GUID for RSDP
 * See "Finding the RSDP on UEFI Enabled Systems" in ACPI specs.
 */
#define ACPI_1_0_RSDP_EFI_GUID						\
	{								\
		.Data1 = 0xeb9d2d30,					\
		.Data2 = 0x2d88,					\
		.Data3 = 0x11d3,					\
		.Data4 = { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d }, \
	}

#define ACPI_2_0_RSDP_EFI_GUID						\
	{								\
		.Data1 = 0x8868e871,					\
		.Data2 = 0xe4f1,					\
		.Data3 = 0x11d3,					\
		.Data4 = { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 }, \
	}

/* The linker places this dummy last in the data memory.  We can't use
 * traditional linker address symbols because we're relocatable; the
 * linker doesn't know what the runtime address will be.  The compiler
 * has to emit code to find this thing's address at runtime via an
 * offset from RIP.  It's a qword so we can guarantee alignment of the
 * stuff after.
 */
static __attribute__((section(".runtime_data_end")))
uint64_t runtime_data_end[1] = { 0x1111aa8888aa1111L };

#define EXT_DATA_START ((void *) &runtime_data_end[1])

static struct efi_system_table *efi;
static struct efi_boot_arg efi_arg;

static void efi_putchar(int c)
{
	static uint16_t efibuf[PUTCHAR_BUFSZ + 1];
	static int n;

	if (c == '\n') {
		efi_putchar('\r');
	}

	efibuf[n++] = c;

	if (c == '\n' || n == PUTCHAR_BUFSZ) {
		efibuf[n] = 0U;
		efi->ConOut->OutputString(efi->ConOut, efibuf);
		n = 0;
	}
}

static inline bool efi_guid_compare(efi_guid_t *s1, efi_guid_t *s2)
{
	return ((s1->Part1 == s2->Part1) && (s1->Part2 == s2->Part2));
}

static void *efi_config_get_vendor_table_by_guid(efi_guid_t *guid)
{
	struct efi_configuration_table *ect_tmp;
	int n_ct;

	if (efi == NULL) {
		return NULL;
	}

	ect_tmp = efi->ConfigurationTable;

	for (n_ct = 0; n_ct < efi->NumberOfTableEntries; n_ct++) {
		if (efi_guid_compare(&ect_tmp->VendorGuid, guid)) {
			return ect_tmp->VendorTable;
		}

		ect_tmp++;
	}

	return NULL;
}

static void efi_prepare_boot_arg(void)
{
	efi_guid_t rsdp_guid_1 = ACPI_1_0_RSDP_EFI_GUID;
	efi_guid_t rsdp_guid_2 = ACPI_2_0_RSDP_EFI_GUID;

	/* Let's lookup for most recent ACPI table first */
	efi_arg.acpi_rsdp = efi_config_get_vendor_table_by_guid(&rsdp_guid_2);
	if (efi_arg.acpi_rsdp == NULL) {
		efi_arg.acpi_rsdp =
			efi_config_get_vendor_table_by_guid(&rsdp_guid_1);
	}

	if (efi_arg.acpi_rsdp != NULL) {
		printf("RSDP found at %p\n", efi_arg.acpi_rsdp);
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

	hpet[32] &= ~4;
}

/* FIXME: if you check the generated code, "ms_abi" calls like this
 * have to SPILL HALF OF THE SSE REGISTER SET TO THE STACK on entry
 * because of the way the conventions collide.  Is there a way to
 * prevent/suppress that?
 */
uintptr_t __abi efi_entry(void *img_handle, struct efi_system_table *sys_tab)
{
	(void)img_handle;

	efi = sys_tab;
	z_putchar = efi_putchar;
	printf("*** Zephyr EFI Loader ***\n");

	efi_prepare_boot_arg();

	for (int i = 0; i < sizeof(zefi_zsegs)/sizeof(zefi_zsegs[0]); i++) {
		int bytes = zefi_zsegs[i].sz;
		uint8_t *dst = (uint8_t *)zefi_zsegs[i].addr;

		printf("Zeroing %d bytes of memory at %p\n", bytes, dst);
		for (int j = 0; j < bytes; j++) {
			dst[j] = 0U;
		}
	}

	for (int i = 0; i < sizeof(zefi_dsegs)/sizeof(zefi_dsegs[0]); i++) {
		int bytes = zefi_dsegs[i].sz;
		int off = zefi_dsegs[i].off;
		uint8_t *dst = (uint8_t *)zefi_dsegs[i].addr;
		uint8_t *src = &((uint8_t *)EXT_DATA_START)[off];

		printf("Copying %d data bytes to %p from image offset %d\n",
		       bytes, dst, zefi_dsegs[i].off);
		for (int j = 0; j < bytes; j++) {
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
		if (((long)dst & 0xfff) == 0 && dst < (uint8_t *)0x100000L) {
			for (int i = 0; i < 8; i++) {
				dst[i] = 0x90; /* 0x90 == 1-byte NOP */
			}
		}
	}

	unsigned char *code = (void *)zefi_entry;

	efi_arg.efi_systab = efi;
	__asm__ volatile("movq %%cr3, %0" : "=r"(efi_arg.efi_cr3));

	printf("Jumping to Entry Point: %p (%x %x %x %x %x %x %x)\n",
	       code, code[0], code[1], code[2], code[3],
	       code[4], code[5], code[6]);

	disable_hpet();

	/* The EFI console seems to be buffered, give it a little time
	 * to drain before we start banging on the same UART from the
	 * OS.
	 */
	for (volatile int i = 0; i < 50000000; i++) {
	}

	__asm__ volatile("cli; movq %0, %%rbx; jmp *%1"
			 :: "r"(&efi_arg), "r"(code) : "rbx");

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
