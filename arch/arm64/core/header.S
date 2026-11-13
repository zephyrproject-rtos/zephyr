/*
 * Copyright (c) 2020 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

#if CONFIG_MMU_PAGE_SIZE == 4096 || defined(CONFIG_ARM_MPU)
#define HEADER_PGSIZE	1
#elif CONFIG_MMU_PAGE_SIZE == 16384
#define HEADER_PGSIZE	2
#elif CONFIG_MMU_PAGE_SIZE == 65536
#define HEADER_PGSIZE	3
#else
#define HEADER_PGSIZE 0
#warning "Can't determine page size for header flags"
#endif

#define HEADER_FLAGS	(HEADER_PGSIZE << 1)

_ASM_FILE_PROLOGUE

SECTION_SUBSEC_FUNC(image_header,_image_header_section,_image_header)
#ifdef CONFIG_BUILD_OUTPUT_EFI
	/* "MZ" PE/COFF signature, encoding "ccmp x18, #0, #0xd, pl": only changes flags */
	.long	0xfa405a4d
	b	__start
#else
	b	__start				// branch to kernel start
	.long	0				// reserved
#endif
	.quad	_aarch64_text_offset		// Image load offset from start
						// of RAM, little-endian

	.quad	_aarch64_image_size		// Effective size of kernel
						// image, little-endian

	.quad	HEADER_FLAGS			// Informative flags,
						// little-endian

	.quad	0				// reserved
	.quad	0				// reserved
	.quad	0				// reserved
	.ascii	"ARM\x64"			// Magic number
#ifdef CONFIG_BUILD_OUTPUT_EFI
	.long	pe_header - _image_header	// PE header offset
#else
	.long	0				// reserved
#endif

#ifdef CONFIG_BUILD_OUTPUT_EFI
/*
 * PE/COFF header for UEFI. The first 4 KiB of the file are the PE headers
 * and the rest is one section. The linker computes the sizes in efi.ld.
 * Field names are those of the PE format specification.
 */
pe_header:
	.ascii	"PE\0\0"
	.short	0xaa64				// Machine: ARM64
	.short	1				// NumberOfSections
	.long	0				// TimeDateStamp
	.long	0				// PointerToSymbolTable
	.long	0				// NumberOfSymbols
	.short	section_table - optional_header	// SizeOfOptionalHeader
	.short	0x0206				// Characteristics: executable, no line
						// numbers, no debug info
optional_header:
	.short	0x20b				// Magic: PE32+
	.byte	0				// MajorLinkerVersion
	.byte	0				// MinorLinkerVersion
	.long	__efi_section_size		// SizeOfCode
	.long	0				// SizeOfInitializedData
	.long	0				// SizeOfUninitializedData
	.long	__efi_entry_offset		// AddressOfEntryPoint
	.long	0x1000				// BaseOfCode
	.quad	0				// ImageBase
	.long	0x1000				// SectionAlignment
	.long	0x200				// FileAlignment
	.short	0				// MajorOperatingSystemVersion
	.short	0				// MinorOperatingSystemVersion
	.short	0				// MajorImageVersion
	.short	0				// MinorImageVersion
	.short	0				// MajorSubsystemVersion
	.short	0				// MinorSubsystemVersion
	.long	0				// Win32VersionValue
	.long	__efi_image_size		// SizeOfImage
	.long	0x1000				// SizeOfHeaders
	.long	0				// CheckSum
	.short	10				// Subsystem: EFI application
	.short	0x100				// DllCharacteristics: NX compatible
	.quad	0				// SizeOfStackReserve
	.quad	0				// SizeOfStackCommit
	.quad	0				// SizeOfHeapReserve
	.quad	0				// SizeOfHeapCommit
	.long	0				// LoaderFlags
	.long	(section_table - data_directories) / 8	// NumberOfRvaAndSizes
data_directories:
	.quad	0				// Export Table
	.quad	0				// Import Table
	.quad	0				// Resource Table
	.quad	0				// Exception Table
	.quad	0				// Certificate Table
	.quad	0				// Base Relocation Table: none, the stub
						// runs at any address as is
section_table:
	.ascii	".text\0\0\0"			// Name
	.long	__efi_section_size		// VirtualSize
	.long	0x1000				// VirtualAddress
	.long	__efi_section_size		// SizeOfRawData
	.long	0x1000				// PointerToRawData
	.long	0				// PointerToRelocations
	.long	0				// PointerToLinenumbers
	.short	0				// NumberOfRelocations
	.short	0				// NumberOfLinenumbers
	.long	0x60000020			// Characteristics: code, execute, read

	/*
	 * PE loaders may copy only the header structures out of the first
	 * 4 KiB, not the rest, so nothing else may live there.
	 */
	.balign	0x1000
#endif /* CONFIG_BUILD_OUTPUT_EFI */
