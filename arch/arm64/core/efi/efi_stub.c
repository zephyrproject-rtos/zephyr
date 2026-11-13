/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * EFI stub: lets UEFI start the Zephyr image as an EFI application.
 *
 * UEFI loads the image wherever it wants and calls arm64_efi_entry() with
 * the MMU on. Zephyr is linked to run from the zephyr,sram region, so the
 * stub reserves that region, copies the image there, exits boot services,
 * and jumps to __start with the MMU off.
 *
 * arm64_efi_stub() runs from where UEFI loaded the image. The code and data
 * it uses are at their link offsets but not at their link addresses, so it
 * must reach them PC-relatively (-mcmodel=small) and must not call into
 * the rest of Zephyr.
 */

#include <stdint.h>
#include <stddef.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/arm64/efi.h>
#include <efi.h>

#define SRAM_BASE     DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define SRAM_SIZE     DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#define EFI_PAGE_SIZE 4096U

/* In .data, so it is part of the copied image and BSS init leaves it alone */
static struct arm64_efi_boot_info boot_info __attribute__((section(".data.arm64_efi_boot_info")));
static uint8_t memmap_buf[CONFIG_ARM64_EFI_MEMMAP_SIZE] __noinit __aligned(8);

extern char __start[];
extern char __data_region_end[];
FUNC_NORETURN void arm64_efi_enter_zephyr(uintptr_t start, uintptr_t end, uintptr_t entry);
efi_status_t arm64_efi_stub(void *image_handle, struct efi_system_table *st, uintptr_t load,
			    uintptr_t link);

const struct arm64_efi_boot_info *arm64_efi_boot_info_get(void)
{
	return (boot_info.system_table != NULL) ? &boot_info : NULL;
}

static void efi_print(struct efi_system_table *st, const char *s)
{
	uint16_t buf[64];
	size_t n = 0;

	for (; *s != '\0'; s++) {
		if (*s == '\n') {
			buf[n++] = '\r';
		}
		buf[n++] = *s;
		if (n >= ARRAY_SIZE(buf) - 2) {
			buf[n] = 0;
			st->ConOut->OutputString(st->ConOut, buf);
			n = 0;
		}
	}
	buf[n] = 0;
	st->ConOut->OutputString(st->ConOut, buf);
}

/* Print the low <digits> hex digits of val */
static void efi_print_hex(struct efi_system_table *st, uint64_t val, int digits)
{
	char s[19];

	s[0] = '0';
	s[1] = 'x';
	for (int i = 0; i < digits; i++) {
		s[2 + i] = "0123456789abcdef"[(val >> (4 * (digits - 1 - i))) & 0xf];
	}
	s[2 + digits] = '\0';
	efi_print(st, s);
}

/* Print the memory map entries in [base, end) that are not free */
static void print_conflicts(struct efi_system_table *st, uint64_t base, uint64_t end)
{
	struct efi_boot_services *bs = st->BootServices;
	uintptr_t size = 0, key, desc_size;
	uint32_t desc_ver;
	uint8_t *map;

	if (bs->GetMemoryMap(&size, NULL, &key, &desc_size, &desc_ver) != EFI_BUFFER_TOO_SMALL) {
		return;
	}
	/* Room for the entries AllocatePool() may add */
	size += 4 * desc_size;
	if (bs->AllocatePool(EfiLoaderData, size, (void **)&map) != EFI_SUCCESS) {
		return;
	}
	if (bs->GetMemoryMap(&size, (void *)map, &key, &desc_size, &desc_ver) == EFI_SUCCESS) {
		for (uintptr_t off = 0; off < size; off += desc_size) {
			struct efi_memory_descriptor *d = (void *)(map + off);
			uint64_t d_end = d->PhysicalStart + d->NumberOfPages * EFI_PAGE_SIZE;

			if (d->Type == EfiConventionalMemory || d_end <= base ||
			    d->PhysicalStart >= end) {
				continue;
			}
			efi_print(st, "  type ");
			efi_print_hex(st, d->Type, 2);
			efi_print(st, " at ");
			efi_print_hex(st, d->PhysicalStart, 16);
			efi_print(st, "-");
			efi_print_hex(st, d_end - 1, 16);
			efi_print(st, "\n");
		}
	}
	bs->FreePool(map);
}

efi_status_t arm64_efi_stub(void *image_handle, struct efi_system_table *st, uintptr_t load,
			    uintptr_t link)
{
	struct efi_boot_services *bs = st->BootServices;
	uint64_t base = SRAM_BASE;
	uintptr_t pages = DIV_ROUND_UP(SRAM_SIZE, EFI_PAGE_SIZE);
	size_t image_size = (uintptr_t)__data_region_end - (uintptr_t)__text_region_start;
	struct arm64_efi_boot_info *info;
	uintptr_t map_size, key, desc_size;
	uint32_t desc_ver;
	efi_status_t status;

/* Where a symbol ends up once the image is at its link address */
#define LINK_ADDR(sym) ((uintptr_t)(sym) - load + link)

	status = bs->AllocatePages(AllocateAddress, EfiLoaderData, pages, &base);
	if (status != EFI_SUCCESS) {
		efi_print(st, "Zephyr: RAM at ");
		efi_print_hex(st, SRAM_BASE, 16);
		efi_print(st, "-");
		efi_print_hex(st, SRAM_BASE + SRAM_SIZE - 1, 16);
		efi_print(st, " is not free:\n");
		print_conflicts(st, SRAM_BASE, SRAM_BASE + SRAM_SIZE);
		return status;
	}

	/* UEFI loads at a page-aligned address and zero-fills the image up to a page boundary */
	for (size_t i = 0; i < DIV_ROUND_UP(image_size, 8); i++) {
		((uint64_t *)link)[i] = ((uint64_t *)load)[i];
	}

	for (int tries = 0; tries < 2; tries++) {
		map_size = sizeof(memmap_buf);
		status = bs->GetMemoryMap(&map_size, (void *)LINK_ADDR(memmap_buf), &key,
					  &desc_size, &desc_ver);
		if (status != EFI_SUCCESS) {
			if (tries == 0 && status == EFI_BUFFER_TOO_SMALL) {
				efi_print(st, "Zephyr: memory map needs ");
				efi_print_hex(st, map_size, 8);
				efi_print(st, " bytes, more than CONFIG_ARM64_EFI_MEMMAP_SIZE\n");
			}
			if (tries == 0) {
				bs->FreePages(base, pages);
			}
			return status;
		}
		/*
		 * Fails if the memory map changed since GetMemoryMap(). After a
		 * failure, only GetMemoryMap() and ExitBootServices() may be called.
		 */
		status = bs->ExitBootServices(image_handle, key);
		if (status == EFI_SUCCESS) {
			break;
		}
	}
	if (status != EFI_SUCCESS) {
		return status;
	}

	info = (void *)LINK_ADDR(&boot_info);
	info->system_table = st;
	info->memmap = (void *)LINK_ADDR(memmap_buf);
	info->memmap_size = map_size;
	info->memmap_desc_size = desc_size;
	info->memmap_desc_version = desc_ver;

	arm64_efi_enter_zephyr(link, LINK_ADDR(_image_ram_end), LINK_ADDR(__start));
}
