/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Structure offsets and sizes for efi_stub.S. This file is not linked into
 * Zephyr: zephyr_constants_library() turns its symbols into the generated
 * header arm64_efi_constants.h.
 */

#include <stdint.h>
#include <gen_offset.h>
#include <zephyr/arch/arm64/efi.h>
#include <efi.h>

/* Locals of arm64_efi_entry(), which the other functions in efi_stub.S use too */
struct efi_stub_frame {
	uint64_t base; /* AllocatePages() address */
	uint64_t map;  /* memory map buffer from AllocatePool() */
	uint64_t map_alloc_size;
	uint64_t map_size; /* GetMemoryMap() arguments */
	uint64_t map_key;
	uint64_t desc_size;
	uint32_t desc_version;
	uint16_t text[2]; /* efi_putc() string: a UCS-2 character and a NUL */
} __aligned(16);

GEN_ABS_SYM_BEGIN(_Arm64EfiConstantAbsSyms)

GEN_OFFSET_STRUCT(efi_system_table, ConOut);
GEN_OFFSET_STRUCT(efi_system_table, BootServices);
GEN_OFFSET_STRUCT(efi_simple_text_output, OutputString);

GEN_OFFSET_STRUCT(efi_boot_services, AllocatePages);
GEN_OFFSET_STRUCT(efi_boot_services, FreePages);
GEN_OFFSET_STRUCT(efi_boot_services, GetMemoryMap);
GEN_OFFSET_STRUCT(efi_boot_services, AllocatePool);
GEN_OFFSET_STRUCT(efi_boot_services, FreePool);
GEN_OFFSET_STRUCT(efi_boot_services, ExitBootServices);
GEN_OFFSET_STRUCT(efi_boot_services, CopyMem);

GEN_OFFSET_STRUCT(efi_memory_descriptor, Type);
GEN_OFFSET_STRUCT(efi_memory_descriptor, PhysicalStart);
GEN_OFFSET_STRUCT(efi_memory_descriptor, NumberOfPages);

GEN_OFFSET_STRUCT(arm64_efi_boot_info, system_table);
GEN_OFFSET_STRUCT(arm64_efi_boot_info, memmap);
GEN_OFFSET_STRUCT(arm64_efi_boot_info, memmap_size);
GEN_OFFSET_STRUCT(arm64_efi_boot_info, memmap_desc_size);
GEN_OFFSET_STRUCT(arm64_efi_boot_info, memmap_desc_version);

GEN_OFFSET_STRUCT(efi_stub_frame, base);
GEN_OFFSET_STRUCT(efi_stub_frame, map);
GEN_OFFSET_STRUCT(efi_stub_frame, map_alloc_size);
GEN_OFFSET_STRUCT(efi_stub_frame, map_size);
GEN_OFFSET_STRUCT(efi_stub_frame, map_key);
GEN_OFFSET_STRUCT(efi_stub_frame, desc_size);
GEN_OFFSET_STRUCT(efi_stub_frame, desc_version);
GEN_OFFSET_STRUCT(efi_stub_frame, text);
GEN_ABSOLUTE_SYM(__struct_efi_stub_frame_SIZEOF, sizeof(struct efi_stub_frame));

GEN_ABS_SYM_END
