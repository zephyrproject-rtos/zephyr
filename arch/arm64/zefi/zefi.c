/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * AArch64 EFI stub: copy embedded Zephyr segments, sync caches, disable
 * the MMU, and jump to __start. Keep all symbols static except efi_entry;
 * see arch/x86/zefi/README.txt for the PE/COFF linkage constraints.
 */
#include <stdint.h>
#include <stddef.h>
#include "efi.h"
#include "printf.h"
#include <zefi-segments.h>

#define PUTCHAR_BUFSZ 128

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

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

static __attribute__((section(".runtime_data_end")))
uint64_t runtime_data_end[1] = { 0x1111aa8888aa1111UL };

#define EXT_DATA_START ((void *)&runtime_data_end[1])

static struct efi_system_table *efi;

static void efi_putchar(int c)
{
	static uint16_t efibuf[PUTCHAR_BUFSZ + 1];
	static int n;

	if (c == '\n') {
		efi_putchar('\r');
	}

	efibuf[n] = c;
	++n;

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

	for (n_ct = 0; n_ct < (int)efi->NumberOfTableEntries; n_ct++) {
		if (efi_guid_compare(&ect_tmp->VendorGuid, guid)) {
			return ect_tmp->VendorTable;
		}

		ect_tmp++;
	}

	return NULL;
}

static void efi_prepare_boot_arg(void)
{
	efi_guid_t rsdp_guid_2 = ACPI_2_0_RSDP_EFI_GUID;
	efi_guid_t rsdp_guid_1 = ACPI_1_0_RSDP_EFI_GUID;
	void *acpi_rsdp;

	acpi_rsdp = efi_config_get_vendor_table_by_guid(&rsdp_guid_2);
	if (acpi_rsdp == NULL) {
		acpi_rsdp = efi_config_get_vendor_table_by_guid(&rsdp_guid_1);
	}

	if (acpi_rsdp != NULL) {
		zefi_printf("RSDP found at %p\n", acpi_rsdp);
	}
}

static void zefi_sync_cache_range(uint8_t *addr, int bytes)
{
	uintptr_t start = (uintptr_t)addr & ~0x3fUL;
	uintptr_t end = (uintptr_t)addr + (uintptr_t)bytes;
	uintptr_t p;

	for (p = start; p < end; p += 64) {
		__asm__ volatile("dc cvau, %0" :: "r"(p) : "memory");
	}
	__asm__ volatile("dsb ish" ::: "memory");

	for (p = start; p < end; p += 64) {
		__asm__ volatile("ic ivau, %0" :: "r"(p) : "memory");
	}
	__asm__ volatile("dsb ish" ::: "memory");
	__asm__ volatile("isb" ::: "memory");
}

static void zefi_disable_mmu(void)
{
	uint64_t el, sctlr;

	__asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
	switch (el >> 2) {
	case 2:
		__asm__ volatile("mrs %0, sctlr_el2" : "=r"(sctlr));
		sctlr &= ~1ULL;
		__asm__ volatile("msr sctlr_el2, %0" :: "r"(sctlr));
		break;
	case 1:
		__asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
		sctlr &= ~1ULL;
		__asm__ volatile("msr sctlr_el1, %0" :: "r"(sctlr));
		break;
	default:
		return;
	}

	__asm__ volatile("isb" ::: "memory");
}

uintptr_t __abi efi_entry(void *img_handle, struct efi_system_table *sys_tab)
{
	(void)img_handle;

	efi = sys_tab;
	z_putchar = efi_putchar;
	zefi_printf("*** Zephyr EFI Loader ***\n");

	efi_prepare_boot_arg();

	for (int i = 0; i < (int)ARRAY_SIZE(zefi_zsegs); i++) {
		int bytes = zefi_zsegs[i].sz;
		uint8_t *dst = (uint8_t *)(uintptr_t)zefi_zsegs[i].addr;

		zefi_printf("Zeroing %d bytes of memory at %p\n", bytes, dst);
		for (int j = 0; j < bytes; j++) {
			dst[j] = 0U;
		}
	}

	for (int i = 0; i < (int)ARRAY_SIZE(zefi_dsegs); i++) {
		int bytes = zefi_dsegs[i].sz;
		uint32_t off = zefi_dsegs[i].off;
		uint8_t *dst = (uint8_t *)(uintptr_t)zefi_dsegs[i].addr;
		uint8_t *src = &((uint8_t *)EXT_DATA_START)[off];

		zefi_printf("Copying %d data bytes to %p from image offset %d\n",
			    bytes, dst, (int)off);
		for (int j = 0; j < bytes; j++) {
			dst[j] = src[j];
		}

		zefi_sync_cache_range(dst, bytes);
	}

	unsigned char *code = (void *)zefi_entry;

	zefi_printf("Jumping to Entry Point: %p (%x %x %x %x)\n",
		    code, code[0], code[1], code[2], code[3]);

	/* The EFI console seems to be buffered; give it a little time
	 * to drain before we start banging on the same UART from the
	 * OS.
	 */
	for (volatile int i = 0; i < 50000000; i++) {
	}

	zefi_disable_mmu();
	__asm__ volatile("br %0" :: "r"(code) : "memory");

	return 0;
}

/* Dummy .reloc so EFI loaders accept the PE image (see gnu-efi). */
uint32_t relocation_dummy;
__asm__(".section .reloc\n"
	".align 4\n"
	"base_relocation_block:\n"
	".long 0\n"
	".long 0x0c\n"
	".long 0\n");
