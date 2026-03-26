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

#define EFI_LOADED_IMAGE_PROTOCOL_GUID					\
	{								\
		.Data1 = 0x5b1b31a1,					\
		.Data2 = 0x9562,					\
		.Data3 = 0x11d2,					\
		.Data4 = { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
	}

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID				\
	{								\
		.Data1 = 0x9042a9de,					\
		.Data2 = 0x23dc,					\
		.Data3 = 0x4a38,					\
		.Data4 = { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a }, \
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

/*
 * Get GOP from ConOut handle and save info for Zephyr display driver;
 * only compiled when CONFIG_DISPLAY_EFI_GOP is set.
 *
 * When started from UEFI Shell, ConOut may not have GOP (returns
 * EFI_UNSUPPORTED). Fall back to LocateProtocol to find GOP from
 * the system in that case.
 */
static void efi_init_gop_save(void)
{
#if defined(CONFIG_DISPLAY_EFI_GOP)
	struct efi_gop *gop = NULL;
	efi_status_t st;
	struct efi_boot_services *bs;
	efi_guid_t gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

	if (efi == NULL || efi->BootServices == NULL) {
		return;
	}
	bs = efi->BootServices;

	/* Try ConOut first (works when booting .efi directly). */
	if (efi->ConsoleOutHandle != NULL && bs->HandleProtocol != NULL) {
		st = bs->HandleProtocol(efi->ConsoleOutHandle,
					(efi_guid_t *)&gop_guid, (void **)&gop);
		if (st == EFI_SUCCESS && gop != NULL && gop->Mode != NULL) {
			printf("GOP found from ConOut\n");
		} else {
			gop = NULL;
		}
	}

	/*
	 * Fallback via LocateProtocol: when started from UEFI Shell,
	 * ConOut often does not expose GOP.
	 */
	if ((gop == NULL || gop->Mode == NULL) && bs->LocateProtocol != NULL) {
		st = bs->LocateProtocol((efi_guid_t *)&gop_guid, NULL, (void **)&gop);
		if (st != EFI_SUCCESS || gop == NULL) {
			printf("GOP not found by LocateProtocol: (0x%lx)\n",
			       (unsigned long)st);
			return;
		}
	}

	if (gop == NULL || gop->Mode == NULL) {
		printf("GOP not found via ConOut or LocateProtocol\n");
		return;
	}

	{
		struct efi_gop_mode *mode = gop->Mode;
		struct efi_gop_mode_info *info = NULL;
		uintptr_t size_of_info = 0;
		uint32_t width, height, pitch;

		/*
		 * Use QueryMode to get mode info; avoids dereferencing
		 * mode->Info which may point to unmapped firmware memory
		 * when started from UEFI Shell.
		 */
		if (gop->QueryMode != NULL) {
			st = gop->QueryMode(gop, mode->Mode, &size_of_info, &info);
			if (st != 0 || info == NULL) {
				printf("GOP QueryMode failed (0x%lx)\n",
				       (unsigned long)st);
				return;
			}
		} else {
			info = mode->Info;
		}

		if (info == NULL ||
		    (info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor &&
		     info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor)) {
			return;
		}

		width = info->HorizontalResolution;
		height = info->VerticalResolution;
		pitch = info->PixelsPerScanLine;

		if (mode->FrameBufferBase == 0 || mode->FrameBufferSize == 0) {
			printf("GOP framebuffer invalid\n");
			return;
		}

		/* Save GOP info for Zephyr display driver. */
		efi_arg.gop_fb_base = mode->FrameBufferBase;
		efi_arg.gop_fb_size = mode->FrameBufferSize;
		efi_arg.gop_width = width;
		efi_arg.gop_height = height;
		efi_arg.gop_pitch = pitch;
		efi_arg.gop_pixel_format = info->PixelFormat;
	}
#endif /* CONFIG_DISPLAY_EFI_GOP */
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
#ifndef CONFIG_DYNAMIC_BOOTARGS
	(void)img_handle;
#endif /* CONFIG_DYNAMIC_BOOTARGS */

	efi = sys_tab;
	z_putchar = efi_putchar;
	printf("*** Zephyr EFI Loader ***\n");

	efi_prepare_boot_arg();

	efi_init_gop_save();

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

#ifdef CONFIG_DYNAMIC_BOOTARGS
	char *dst_bootargs = (char *)zefi_bootargs;
	struct efi_loaded_image_protocol *loaded_image;
	efi_guid_t loaded_image_protocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	efi_status_t loaded_image_status = sys_tab->BootServices->HandleProtocol(
		img_handle,
		&loaded_image_protocol,
		(void **)&loaded_image
	);

	if (loaded_image_status == EFI_SUCCESS) {
		uint16_t *src_bootargs = (uint16_t *)loaded_image->LoadOptions;

		while (*src_bootargs != '\0' &&
		       dst_bootargs + 1 <
			       (char *)zefi_bootargs + CONFIG_BOOTARGS_ARGS_BUFFER_SIZE) {
			*dst_bootargs++ = *src_bootargs++ & 0x7f;
		}
		*dst_bootargs = '\0';
	} else {
		*dst_bootargs = '\0';
	}
#endif /* CONFIG_DYNAMIC_BOOTARGS */

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
