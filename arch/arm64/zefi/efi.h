/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal EFI types for the AArch64 zefi stub. AArch64 UEFI uses the
 * standard AAPCS64 calling convention, so __abi is empty (unlike x86_64
 * ms_abi).
 */

#ifndef ZEFI_ARCH_ARM64_EFI_H_
#define ZEFI_ARCH_ARM64_EFI_H_

#ifndef _ASMLANGUAGE

#include <stdbool.h>
#include <stdint.h>

#define __abi

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

#ifndef BITS_PER_LONG
#define BITS_PER_LONG 64
#endif

typedef uintptr_t efi_status_t;

#define EFI_STATUS(_status)		(_status | BIT((BITS_PER_LONG - 1)))

#define EFI_SUCCESS			0
#define EFI_BUFFER_TOO_SMALL		EFI_STATUS(5)

typedef struct {
	union {
		struct {
			uint32_t Data1;
			uint16_t Data2;
			uint16_t Data3;
			uint8_t Data4[8];
		};
		struct {
			uint64_t Part1;
			uint64_t Part2;
		};
	};
} efi_guid_t;

struct efi_table_header {
	uint64_t Signature;
	uint32_t Revision;
	uint32_t HeaderSize;
	uint32_t CRC32;
	uint32_t Reserved;
};

struct efi_simple_text_output_mode {
	int32_t MaxMode;
	int32_t Mode;
	int32_t Attribute;
	int32_t CursorColumn;
	int32_t CursorRow;
	bool CursorVisible;
};

struct efi_simple_text_output;

typedef efi_status_t __abi(*efi_text_string_t)(
	struct efi_simple_text_output *This,
	uint16_t *String);

struct efi_simple_text_output {
	void *Reset;
	efi_text_string_t OutputString;
	void *TestString;
	void *QueryMode;
	void *SetMode;
	void *SetAttribute;
	void *ClearScreen;
	void *SetCursorPosition;
	void *EnableCursor;
	struct efi_simple_text_output_mode *Mode;
};

struct efi_configuration_table {
	efi_guid_t VendorGuid;
	void *VendorTable;
};

struct efi_boot_services {
	struct efi_table_header Hdr;
	void *RaiseTPL;
	void *RestoreTPL;
	void *AllocatePages;
	void *FreePages;
	void *GetMemoryMap;
	void *AllocatePool;
	void *FreePool;
	void *CreateEvent;
	void *SetTimer;
	void *WaitForEvent;
	void *SignalEvent;
	void *CloseEvent;
	void *CheckEvent;
	void *InstallProtocolInterface;
	void *ReinstallProtocolInterface;
	void *UninstallProtocolInterface;
	void *HandleProtocol;
	void *Reserved;
	void *RegisterProtocolNotify;
	void *LocateHandle;
	void *LocateDevicePath;
	void *InstallConfigurationTable;
	void *LoadImage;
	void *StartImage;
	void *Exit;
	void *UnloadImage;
	void *ExitBootServices;
};

struct efi_system_table {
	struct efi_table_header Hdr;
	uint16_t *FirmwareVendor;
	uint32_t FirmwareRevision;
	void *ConsoleInHandle;
	void *ConIn;
	void *ConsoleOutHandle;
	struct efi_simple_text_output *ConOut;
	void *StandardErrorHandle;
	struct efi_simple_text_output *StdErr;
	void *RuntimeServices;
	struct efi_boot_services *BootServices;
	uint64_t NumberOfTableEntries;
	struct efi_configuration_table *ConfigurationTable;
};

#endif /* _ASMLANGUAGE */

#endif /* ZEFI_ARCH_ARM64_EFI_H_ */
