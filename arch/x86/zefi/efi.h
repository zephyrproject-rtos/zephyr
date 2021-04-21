/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _EFI_H
#define _EFI_H

/* Minimal restatement of a minimal subset of the EFI runtime API */

#define __abi __attribute__((ms_abi))

typedef uintptr_t __abi (*efi_fn1_t)(void *);
typedef uintptr_t __abi (*efi_fn2_t)(void *, void *);
typedef uintptr_t __abi (*efi_fn3_t)(void *, void *, void *);
typedef uintptr_t __abi (*efi_fn4_t)(void *, void *, void *, void *);

struct efi_simple_text_output {
	efi_fn2_t Reset;
	efi_fn2_t OutputString;
	efi_fn2_t TestString;
	efi_fn4_t QueryMode;
	efi_fn2_t SetMode;
	efi_fn2_t SetAttribute;
	efi_fn1_t ClearScreen;
	efi_fn3_t SetCursorPosition;
	efi_fn2_t EnableCursor;
	struct simple_text_output_mode *Mode;
};

struct efi_system_table {
	uint64_t Signature;
	uint32_t Revision;
	uint32_t HeaderSize;
	uint32_t CRC32;
	uint32_t Reserved;
	uint16_t *FirmwareVendor;
	uint32_t FirmwareRevision;
	void *ConsoleInHandle;
	struct efi_simple_input *ConIn;
	void *ConsoleOutHandle;
	struct efi_simple_text_output *ConOut;
	void *StandardErrorHandle;
	struct efi_simple_text_output *StdErr;
	struct efi_runtime_services *RuntimeServices;
	struct efi_boot_services *BootServices;
	uint64_t NumberOfTableEntries;
	struct efi_configuration_table *ConfigurationTable;
};

#endif /* _EFI_H */
