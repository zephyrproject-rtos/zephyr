/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock.h"

#include <zephyr/acpi/acpi.h>
#include <acpi.h>
#include <accommon.h>

#include <zephyr/fff.h>

FAKE_VOID_FUNC_VARARG(AcpiInfo, const char *, ...);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiInstallNotifyHandler, ACPI_HANDLE,
		UINT32, ACPI_NOTIFY_HANDLER, void *);

FAKE_VOID_FUNC_VARARG(AcpiException, const char	*, UINT32, ACPI_STATUS,
		      const char *, ...);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiInitializeSubsystem);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiInitializeTables, ACPI_TABLE_DESC *, UINT32,
		BOOLEAN);
FAKE_VALUE_FUNC(ACPI_STATUS, AcpiEnableSubsystem, UINT32);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiInitializeObjects, UINT32);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiLoadTables);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiNsInternalizeName, const char *, char **);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiNsLookup, ACPI_GENERIC_STATE *, char *,
		ACPI_OBJECT_TYPE, ACPI_INTERPRETER_MODE, UINT32,
		ACPI_WALK_STATE *, struct acpi_namespace_node **);

FAKE_VOID_FUNC(AcpiOsFree, void *);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiGetHandle, ACPI_HANDLE, const char *,
		ACPI_HANDLE *);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiEvaluateObject, ACPI_HANDLE, ACPI_STRING,
		ACPI_OBJECT_LIST *, ACPI_BUFFER *);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiGetObjectInfo, ACPI_HANDLE,
		struct acpi_device_info **);

FAKE_VALUE_FUNC(char *, AcpiNsGetNormalizedPathname, ACPI_NAMESPACE_NODE *,
		BOOLEAN);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiGetCurrentResources, ACPI_HANDLE, ACPI_BUFFER *);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiWalkNamespace, ACPI_OBJECT_TYPE, ACPI_HANDLE,
		UINT32, ACPI_WALK_CALLBACK, ACPI_WALK_CALLBACK, void *, void **);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiGetPossibleResources, ACPI_HANDLE, ACPI_BUFFER *);

FAKE_VALUE_FUNC(ACPI_STATUS, AcpiGetTable, char *, UINT32,
		struct acpi_table_header **);

FAKE_VALUE_FUNC(uint32_t, arch_acpi_encode_irq_flags, uint8_t, uint8_t);
