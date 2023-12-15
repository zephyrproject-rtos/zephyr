/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "mock.h"
#include <zephyr/acpi/acpi.h>

#include <acpi.h>
#include <accommon.h>
#include <acapps.h>
#include <aclocal.h>
#include <acstruct.h>

void ACPI_INTERNAL_VAR_XFACE AcpiInfo(const char *Format, ...)
{
}

ACPI_STATUS AcpiInstallNotifyHandler(ACPI_HANDLE             Device,
				     UINT32                  HandlerType,
				     ACPI_NOTIFY_HANDLER     Handler,
				     void                    *Context)
{
	return AE_OK;
}

void ACPI_INTERNAL_VAR_XFACE AcpiException(const char	          *ModuleName,
					   UINT32                  LineNumber,
					   ACPI_STATUS             Status,
					   const char              *Format,
					   ...)
{
}

ACPI_STATUS ACPI_INIT_FUNCTION AcpiInitializeSubsystem(void)
{
	return AE_OK;
}

ACPI_STATUS ACPI_INIT_FUNCTION AcpiInitializeTables(ACPI_TABLE_DESC         *InitialTableArray,
						    UINT32                  InitialTableCount,
						    BOOLEAN                 AllowResize)
{
	return AE_OK;
}

ACPI_STATUS ACPI_INIT_FUNCTION AcpiEnableSubsystem(UINT32                  Flags)
{
	return AE_OK;
}

ACPI_STATUS ACPI_INIT_FUNCTION AcpiInitializeObjects(UINT32                  Flags)
{
	return AE_OK;
}

ACPI_STATUS ACPI_INIT_FUNCTION AcpiLoadTables(void)
{
	return AE_OK;
}

ACPI_STATUS AcpiNsInternalizeName(const char              *ExternalName,
				  char                    **ConvertedName)
{
	return AE_OK;
}

ACPI_STATUS AcpiNsLookup(ACPI_GENERIC_STATE      *ScopeInfo,
			 char                    *Pathname,
			 ACPI_OBJECT_TYPE        Type,
			 ACPI_INTERPRETER_MODE   InterpreterMode,
			 UINT32                  Flags,
			 ACPI_WALK_STATE         *WalkState,
			 ACPI_NAMESPACE_NODE     **ReturnNode)
{
	return AE_OK;
}

void AcpiOsFree(void                    *Mem)
{
}

ACPI_STATUS AcpiGetHandle(ACPI_HANDLE             Parent,
			  const char              *Pathname,
			  ACPI_HANDLE             *RetHandle)
{
	return AE_OK;
}

ACPI_STATUS AcpiEvaluateObject(ACPI_HANDLE             Handle,
			       ACPI_STRING             Pathname,
			       ACPI_OBJECT_LIST        *ExternalParams,
			       ACPI_BUFFER             *ReturnBuffer)
{
	return AE_OK;
}

ACPI_STATUS AcpiGetObjectInfo(ACPI_HANDLE             Handle,
			      ACPI_DEVICE_INFO        **ReturnBuffer)
{
	return AE_OK;
}

char *AcpiNsGetNormalizedPathname(ACPI_NAMESPACE_NODE     *Node,
				  BOOLEAN                 NoTrailing)
{
	return NULL;
}

ACPI_STATUS AcpiGetCurrentResources(ACPI_HANDLE             DeviceHandle,
				    ACPI_BUFFER             *RetBuffer)
{
	return AE_OK;
}

ACPI_STATUS AcpiWalkNamespace(ACPI_OBJECT_TYPE        Type,
			      ACPI_HANDLE             StartObject,
			      UINT32                  MaxDepth,
			      ACPI_WALK_CALLBACK      DescendingCallback,
			      ACPI_WALK_CALLBACK      AscendingCallback,
			      void                    *Context,
			      void                    **ReturnValue)
{
	return AE_OK;
}

ACPI_STATUS AcpiGetPossibleResources(ACPI_HANDLE             DeviceHandle,
				     ACPI_BUFFER             *RetBuffer)
{
	return AE_OK;
}

ACPI_STATUS AcpiGetTable(char                    *Signature,
			 UINT32                  Instance,
			 ACPI_TABLE_HEADER       **OutTable)
{
	return AE_OK;
}
