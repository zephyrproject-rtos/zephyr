/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "acpi.h"
#include "accommon.h"
#include "acapps.h"
#include <aecommon.h>

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/acpi/acpi.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ACPI, CONFIG_ACPI_LOG_LEVEL);

struct acpi {
	struct acpi_dev child_dev[CONFIG_ACPI_DEV_MAX];
	int num_dev;
	ACPI_PCI_ROUTING_TABLE pci_prt_table[CONFIG_ACPI_MAX_PRT_ENTRY];
	bool early_init;
	int status;
};

static struct acpi bus_ctx = {
	.status = AE_NOT_CONFIGURED,
};

static ACPI_TABLE_DESC acpi_tables[CONFIG_ACPI_MAX_INIT_TABLES];

static int acpi_init(void);

static int check_init_status(void)
{
	int ret;

	if (ACPI_SUCCESS(bus_ctx.status)) {
		return 0;
	}

	if (bus_ctx.status == AE_NOT_CONFIGURED) {
		ret = acpi_init();
	} else {
		LOG_ERR("ACPI init was not success\n");
		ret = -EIO;
	}
	return ret;
}

static void notify_handler(ACPI_HANDLE device, UINT32 value, void *ctx)
{
	ACPI_INFO(("Received a notify 0x%X", value));
}

static ACPI_STATUS region_handler(UINT32 Function, ACPI_PHYSICAL_ADDRESS address, UINT32 bit_width,
				  UINT64 *value, void *handler_ctx, void *region_ctx)
{
	return AE_OK;
}

static ACPI_STATUS region_init(ACPI_HANDLE RegionHandle, UINT32 Function, void *handler_ctx,
			       void **region_ctx)
{
	if (Function == ACPI_REGION_DEACTIVATE) {
		*region_ctx = NULL;
	} else {
		*region_ctx = RegionHandle;
	}

	return AE_OK;
}

static ACPI_STATUS install_handlers(void)
{
	ACPI_STATUS status;

	/* Install global notify handler */
	status = AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY, notify_handler,
					  NULL);
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While installing Notify handler"));
		goto exit;
	}

	status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_SYSTEM_MEMORY,
						region_handler, region_init, NULL);
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While installing an OpRegion handler"));
	}
exit:

	return status;
}

static ACPI_STATUS initialize_acpica(void)
{
	ACPI_STATUS status;

	/* Initialize the ACPI subsystem */
	status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While initializing ACPI"));
		goto exit;
	}

	/* Initialize the ACPI Table Manager and get all ACPI tables */
	if (!bus_ctx.early_init) {
		status = AcpiInitializeTables(NULL, 16, FALSE);
	} else {
		/* Copy the root table list to dynamic memory if already initialized */
		status = AcpiReallocateRootTable();
	}
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While initializing Table Manager"));
		goto exit;
	}

	/* Initialize the ACPI hardware */
	status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While enabling ACPI"));
		goto exit;
	}

	/* Install local handlers */
	status = install_handlers();
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While installing handlers"));
		goto exit;
	}

	/* Create the ACPI namespace from ACPI tables */
	status = AcpiLoadTables();
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While loading ACPI tables"));
		goto exit;
	}

	/* Complete the ACPI namespace object initialization */
	status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While initializing ACPI objects"));
	}
exit:

	return status;
}

static ACPI_NAMESPACE_NODE *acpi_name_lookup(char *name)
{
	char *path;
	ACPI_STATUS status;
	ACPI_NAMESPACE_NODE *node;

	LOG_DBG("");

	status = AcpiNsInternalizeName(name, &path);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("Invalid namestring: %s", name);
		return NULL;
	}

	status = AcpiNsLookup(NULL, path, ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
			      ACPI_NS_NO_UPSEARCH | ACPI_NS_DONT_OPEN_SCOPE, NULL, &node);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("Could not locate name: %s, %d", name, status);
		node = NULL;
	}

	ACPI_FREE(path);
	return node;
}

static ACPI_NAMESPACE_NODE *acpi_evaluate_method(char *bus_name, char *method)
{
	ACPI_NAMESPACE_NODE *node;
	ACPI_NAMESPACE_NODE *handle;
	ACPI_NAMESPACE_NODE *prt_node = NULL;

	LOG_DBG("%s", bus_name);

	handle = acpi_name_lookup(bus_name);
	if (!handle) {
		LOG_ERR("No ACPI node with given name: %s", bus_name);
		goto exit;
	}

	if (handle->Type != ACPI_TYPE_DEVICE) {
		LOG_ERR("No ACPI node foud with given name: %s", bus_name);
		goto exit;
	}

	node = ACPI_CAST_PTR(ACPI_NAMESPACE_NODE, handle);

	(void)AcpiGetHandle(node, method, ACPI_CAST_PTR(ACPI_HANDLE, &prt_node));

	if (!prt_node) {
		LOG_ERR("No entry for the ACPI node with given name: %s", bus_name);
		goto exit;
	}
	return node;
exit:
	return NULL;
}

static ACPI_STATUS acpi_enable_pic_mode(void)
{
	ACPI_STATUS status;
	ACPI_OBJECT_LIST arg_list;
	ACPI_OBJECT arg[1];

	arg_list.Count = 1;
	arg_list.Pointer = arg;

	arg[0].Type = ACPI_TYPE_INTEGER;
	arg[0].Integer.Value = 1;

	status = AcpiEvaluateObject(NULL, "\\_PIC", &arg_list, NULL);
	if (ACPI_FAILURE(status)) {
		LOG_WRN("error While executing \\_pic method: %d", status);
	}

	return status;
}

static int acpi_get_irq_table(struct acpi *bus, char *bus_name,
				ACPI_PCI_ROUTING_TABLE *rt_table, uint32_t rt_size)
{
	ACPI_BUFFER rt_buffer;
	ACPI_NAMESPACE_NODE *node;
	int status;

	LOG_DBG("%s", bus_name);

	node = acpi_evaluate_method(bus_name, METHOD_NAME__PRT);
	if (!node) {
		LOG_ERR("Evaluation failed for given device: %s", bus_name);
		return -ENODEV;
	}

	rt_buffer.Pointer = rt_table;
	rt_buffer.Length = ACPI_DEBUG_BUFFER_SIZE;

	status = AcpiGetIrqRoutingTable(node, &rt_buffer);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("unable to retrieve IRQ Routing Table: %s", bus_name);
		return -EIO;
	}

	for (int i = 0; i < CONFIG_ACPI_MAX_PRT_ENTRY; i++) {
		if (!bus->pci_prt_table[i].SourceIndex) {
			break;
		}
		if (IS_ENABLED(CONFIG_X86_64)) {
			/* mark the PRT irq numbers as reserved. */
			arch_irq_set_used(bus->pci_prt_table[i].SourceIndex);
		}
	}

	return 0;
}

static int acpi_retrieve_legacy_irq(struct acpi *bus)
{
	/* TODO: assume platform have only one PCH with single PCI bus (bus 0). */
	return acpi_get_irq_table(bus, "_SB.PC00", bus->pci_prt_table, sizeof(bus->pci_prt_table));
}

static ACPI_STATUS dev_resource_enum_callback(ACPI_HANDLE obj_handle, UINT32 level, void *ctx,
					      void **ret_value)
{
	ACPI_NAMESPACE_NODE *node;
	ACPI_BUFFER rt_buffer;
	struct acpi *bus = (struct acpi *)ctx;
	struct acpi_dev *child_dev;

	node = ACPI_CAST_PTR(ACPI_NAMESPACE_NODE, obj_handle);
	char *path_name;
	int status;
	ACPI_DEVICE_INFO *dev_info;

	LOG_DBG("%s %p\n", __func__, node);

	/* get device info such as HID, Class ID etc. */
	status = AcpiGetObjectInfo(obj_handle, &dev_info);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("AcpiGetObjectInfo failed: %s", AcpiFormatException(status));
		goto exit;
	}

	if (bus->num_dev >= CONFIG_ACPI_DEV_MAX) {
		return AE_NO_MEMORY;
	}

	child_dev = (struct acpi_dev *)&bus->child_dev[bus->num_dev++];
	child_dev->handle = obj_handle;
	child_dev->dev_info = dev_info;

	path_name = AcpiNsGetNormalizedPathname(node, TRUE);
	if (!path_name) {
		LOG_ERR("No memory for path_name\n");
		goto exit;
	} else {
		LOG_DBG("Device path: %s\n", path_name);
		child_dev->path = path_name;
	}

	rt_buffer.Pointer = NULL;
	rt_buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	status = AcpiGetCurrentResources(node, &rt_buffer);
	if (ACPI_FAILURE(status)) {
		LOG_DBG("AcpiGetCurrentResources failed: %s", AcpiFormatException(status));
	} else {
		child_dev->res_lst = rt_buffer.Pointer;
	}

exit:

	return status;
}

static int acpi_enum_devices(struct acpi *bus)
{
	LOG_DBG("");

	AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, ACPI_UINT32_MAX,
			  dev_resource_enum_callback, NULL, bus, NULL);

	return 0;
}

static int acpi_init(void)
{
	int status;

	LOG_DBG("");

	if (bus_ctx.status != AE_NOT_CONFIGURED) {
		LOG_DBG("acpi init already done");
		return bus_ctx.status;
	}

	/* For debug version only */
	ACPI_DEBUG_INITIALIZE();

	status = initialize_acpica();
	if (ACPI_FAILURE(status)) {
		LOG_ERR("Error in ACPI init:%d", status);
		goto exit;
	}

	/* Enable IO APIC mode */
	status = acpi_enable_pic_mode();
	if (ACPI_FAILURE(status)) {
		LOG_WRN("Error in enable pic mode acpi method:%d", status);
	}

	status = acpi_retrieve_legacy_irq(&bus_ctx);
	if (status) {
		LOG_ERR("Error in retrieve legacy interrupt info:%d", status);
		goto exit;
	}

	acpi_enum_devices(&bus_ctx);

exit:
	bus_ctx.status = status;

	return status;
}

static int acpi_early_init(void)
{
	ACPI_STATUS status;

	LOG_DBG("");

	if (bus_ctx.early_init) {
		LOG_DBG("acpi early init already done");
		return 0;
	}

	status = AcpiInitializeTables(acpi_tables, CONFIG_ACPI_MAX_INIT_TABLES, TRUE);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("Error in acpi table init:%d", status);
		return -EIO;
	}

	bus_ctx.early_init = true;

	return 0;
}

uint32_t acpi_legacy_irq_get(pcie_bdf_t bdf)
{
	uint32_t slot = PCIE_BDF_TO_DEV(bdf), pin;

	LOG_DBG("");

	if (check_init_status()) {
		return UINT_MAX;
	}

	pin = (pcie_conf_read(bdf, PCIE_CONF_INTR) >> 8) & 0x3;

	LOG_DBG("Device irq info: slot:%d pin:%d", slot, pin);

	for (int i = 0; i < CONFIG_ACPI_MAX_PRT_ENTRY; i++) {
		if (((bus_ctx.pci_prt_table[i].Address >> 16) & 0xffff) == slot &&
		    bus_ctx.pci_prt_table[i].Pin + 1 == pin) {
			LOG_DBG("[%d]Device irq info: slot:%d pin:%d irq:%d", i, slot, pin,
				bus_ctx.pci_prt_table[i].SourceIndex);
			return bus_ctx.pci_prt_table[i].SourceIndex;
		}
	}

	return UINT_MAX;
}

int acpi_current_resource_get(char *dev_name, ACPI_RESOURCE **res)
{
	ACPI_BUFFER rt_buffer;
	ACPI_NAMESPACE_NODE *node;
	int status;

	LOG_DBG("%s", dev_name);

	status = check_init_status();
	if (status) {
		return status;
	}

	node = acpi_evaluate_method(dev_name, METHOD_NAME__CRS);
	if (!node) {
		LOG_ERR("Evaluation failed for given device: %s", dev_name);
		status = -ENOTSUP;
		goto exit;
	}

	rt_buffer.Pointer = NULL;
	rt_buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	status = AcpiGetCurrentResources(node, &rt_buffer);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("AcpiGetCurrentResources failed: %s", AcpiFormatException(status));
		status = -ENOTSUP;
	} else {
		*res = rt_buffer.Pointer;
	}

exit:

	return status;
}

int acpi_possible_resource_get(char *dev_name, ACPI_RESOURCE **res)
{
	ACPI_BUFFER rt_buffer;
	ACPI_NAMESPACE_NODE *node;
	int status;

	LOG_DBG("%s", dev_name);

	status = check_init_status();
	if (status) {
		return status;
	}

	node = acpi_evaluate_method(dev_name, METHOD_NAME__PRS);
	if (!node) {
		LOG_ERR("Evaluation failed for given device: %s", dev_name);
		status = -ENOTSUP;
		goto exit;
	}

	rt_buffer.Pointer = NULL;
	rt_buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	AcpiGetPossibleResources(node, &rt_buffer);
	*res = rt_buffer.Pointer;

exit:

	return status;
}

int acpi_current_resource_free(ACPI_RESOURCE *res)
{
	ACPI_FREE(res);

	return 0;
}

int acpi_get_irq_routing_table(char *bus_name,
			       ACPI_PCI_ROUTING_TABLE *rt_table, size_t rt_size)
{
	int ret;

	ret = check_init_status();
	if (ret) {
		return ret;
	}

	return acpi_get_irq_table(&bus_ctx, bus_name, rt_table, rt_size);
}

ACPI_RESOURCE *acpi_resource_parse(ACPI_RESOURCE *res, int res_type)
{
	do {
		if (!res->Length) {
			LOG_DBG("Error: zero length found!\n");
			break;
		} else if (res->Type == res_type) {
			break;
		}
		res = ACPI_NEXT_RESOURCE(res);
	} while (res->Type != ACPI_RESOURCE_TYPE_END_TAG);

	if (res->Type == ACPI_RESOURCE_TYPE_END_TAG) {
		return NULL;
	}

	return res;
}

static int acpi_res_type(ACPI_RESOURCE *res)
{
	int type;

	switch (res->Type) {
	case ACPI_RESOURCE_TYPE_IO:
		type = ACPI_RESOURCE_TYPE_IO;
		break;
	case ACPI_RESOURCE_TYPE_FIXED_IO:
		type = ACPI_RESOURCE_TYPE_FIXED_IO;
		break;
	case ACPI_RESOURCE_TYPE_MEMORY24:
		type = ACPI_RESOURCE_TYPE_MEMORY24;
		break;
	case ACPI_RESOURCE_TYPE_MEMORY32:
		type = ACPI_RESOURCE_TYPE_MEMORY32;
		break;
	case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
		type = ACPI_RESOURCE_TYPE_FIXED_MEMORY32;
		break;
	case ACPI_RESOURCE_TYPE_ADDRESS16:
		type = ACPI_RESOURCE_TYPE_ADDRESS16;
		break;
	case ACPI_RESOURCE_TYPE_ADDRESS32:
		type = ACPI_RESOURCE_TYPE_ADDRESS32;
		break;
	case ACPI_RESOURCE_TYPE_ADDRESS64:
		type = ACPI_RESOURCE_TYPE_ADDRESS64;
		break;
	case ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64:
		type = ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64;
		break;
	default:
		type = ACPI_RESOURCE_TYPE_MAX;
	}

	return type;
}

int acpi_device_type_get(ACPI_RESOURCE *res)
{
	int type = ACPI_RESOURCE_TYPE_MAX;

	do {
		if (!res->Length) {
			LOG_ERR("Error: zero length found!\n");
			break;
		}
		type = acpi_res_type(res);
		if (type != ACPI_RESOURCE_TYPE_MAX) {
			break;
		}
		res = ACPI_NEXT_RESOURCE(res);
	} while (res->Type != ACPI_RESOURCE_TYPE_END_TAG);

	return type;
}

struct acpi_dev *acpi_device_get(char *hid, int inst)
{
	struct acpi_dev *child_dev;
	int i = 0, inst_id;

	LOG_DBG("");

	if (check_init_status()) {
		return NULL;
	}

	do {
		child_dev = &bus_ctx.child_dev[i];
		if (!child_dev->path) {
			LOG_DBG("NULL device path found\n");
			continue;
		}

		if (!child_dev->res_lst || !child_dev->dev_info ||
		    !child_dev->dev_info->HardwareId.Length) {
			continue;
		}

		if (!strcmp(hid, child_dev->dev_info->HardwareId.String)) {
			if (child_dev->dev_info->UniqueId.Length) {
				inst_id = atoi(child_dev->dev_info->UniqueId.String);
				if (inst_id == inst) {
					return child_dev;
				}
			} else {
				return child_dev;
			}
		}
	} while (i++ < bus_ctx.num_dev);

	return NULL;
}

struct acpi_dev *acpi_device_by_index_get(int index)
{
	return index < bus_ctx.num_dev ? &bus_ctx.child_dev[index] : NULL;
}

void *acpi_table_get(char *signature, int inst)
{
	int status;
	ACPI_TABLE_HEADER *table;

	if (!bus_ctx.early_init) {
		status = acpi_early_init();
		if (status) {
			LOG_ERR("ACPI early init failed");
			return NULL;
		}
	}

	status = AcpiGetTable(signature, inst, &table);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("ACPI get table failed: %d", status);
		return NULL;
	}

	return (void *)table;
}

static uint32_t acpi_get_subtable_entry_num(int type, struct acpi_subtable_header *subtable,
					    uintptr_t offset, uintptr_t base, uint32_t madt_len)
{
	uint32_t subtable_cnt = 0;

	while (offset < madt_len) {
		if (type == subtable->Type) {
			subtable_cnt++;
		}
		offset += subtable->Length;
		subtable = ACPI_ADD_PTR(ACPI_SUBTABLE_HEADER, base, offset);

		if (!subtable->Length) {
			break;
		}
	}

	return subtable_cnt;
}

int acpi_madt_entry_get(int type, struct acpi_subtable_header **tables, int *num_inst)
{
	struct acpi_table_header *madt = acpi_table_get("APIC", 0);
	uintptr_t base = POINTER_TO_UINT(madt);
	uintptr_t offset = sizeof(ACPI_TABLE_MADT);
	struct acpi_subtable_header *subtable;

	if (!madt) {
		return -EIO;
	}

	subtable = ACPI_ADD_PTR(ACPI_SUBTABLE_HEADER, base, offset);
	while (offset < madt->Length) {

		if (type == subtable->Type) {
			*tables = subtable;
			*num_inst = acpi_get_subtable_entry_num(type, subtable, offset, base,
								madt->Length);
			return 0;
		}

		offset += subtable->Length;
		subtable = ACPI_ADD_PTR(ACPI_SUBTABLE_HEADER, base, offset);
	}

	return -ENODEV;
}

int acpi_dmar_entry_get(enum AcpiDmarType type, struct acpi_subtable_header **tables)
{
	struct acpi_table_dmar *dmar = acpi_table_get("DMAR", 0);
	uintptr_t base = POINTER_TO_UINT(dmar);
	uintptr_t offset = sizeof(ACPI_TABLE_DMAR);
	struct acpi_dmar_header *subtable;

	if (!dmar) {
		LOG_ERR("error on get DMAR table\n");
		return -EIO;
	}

	subtable = ACPI_ADD_PTR(ACPI_DMAR_HEADER, base, offset);
	while (offset < dmar->Header.Length) {
		if (type == subtable->Type) {
			*tables = (struct acpi_subtable_header *)subtable;
			return 0;
		}
		offset += subtable->Length;
		subtable = ACPI_ADD_PTR(ACPI_DMAR_HEADER, base, offset);
	}

	return -ENODEV;
}

int acpi_drhd_get(enum AcpiDmarScopeType scope, struct acpi_dmar_device_scope *dev_scope,
	union acpi_dmar_id *dmar_id, int *num_inst, int max_inst)
{
	uintptr_t offset = sizeof(ACPI_DMAR_HARDWARE_UNIT);
	uint32_t i = 0;
	struct acpi_dmar_header *drdh;
	struct acpi_dmar_device_scope *subtable;
	struct acpi_dmar_pci_path *dev_path;
	int ret;
	uintptr_t base;
	int scope_size;

	ret = acpi_dmar_entry_get(ACPI_DMAR_TYPE_HARDWARE_UNIT,
				  (struct acpi_subtable_header **)&drdh);
	if (ret) {
		LOG_ERR("Error on retrieve DMAR table\n");
		return ret;
	}

	scope_size = drdh->Length - sizeof(ACPI_DMAR_HARDWARE_UNIT);
	base = (uintptr_t)((uintptr_t)drdh + offset);

	offset = 0;

	while (scope_size) {
		int num_path;

		subtable = ACPI_ADD_PTR(ACPI_DMAR_DEVICE_SCOPE, base, offset);
		if (!subtable->Length) {
			break;
		}

		if (scope == subtable->EntryType) {
			num_path = (subtable->Length - 6u) / 2u;
			dev_path = ACPI_ADD_PTR(ACPI_DMAR_PCI_PATH, subtable,
				sizeof(ACPI_DMAR_DEVICE_SCOPE));

			while (num_path--) {
				if (i >= max_inst) {
					LOG_ERR("DHRD not enough buffer size\n");
					return -ENOBUFS;
				}
				dmar_id[i].bits.bus = subtable->Bus;
				dmar_id[i].bits.device = dev_path[i].Device;
				dmar_id[i].bits.function = dev_path[i].Function;
				i++;
			}
			break;
		}

		offset += subtable->Length;

		if (scope_size < subtable->Length) {
			break;
		}
		scope_size -= subtable->Length;
	}

	*num_inst = i;
	if (!i) {
		LOG_ERR("Error on retrieve DRHD Info\n");
		return -ENODEV;
	}

	if (dev_scope && subtable) {
		memcpy(dev_scope, subtable, sizeof(struct acpi_dmar_device_scope));
	}

	return 0;
}

struct acpi_madt_local_apic *acpi_local_apic_get(uint32_t cpu_num)
{
	struct acpi_madt_local_apic *lapic;
	int cpu_cnt;

	if (acpi_madt_entry_get(ACPI_MADT_TYPE_LOCAL_APIC, (ACPI_SUBTABLE_HEADER **)&lapic,
				&cpu_cnt)) {
		/* Error on MAD table. */
		return NULL;
	}

	if ((cpu_num >= cpu_cnt) || !(lapic[cpu_num].LapicFlags & 1u)) {
		/* Proccessor not enabled. */
		return NULL;
	}

	return &lapic[cpu_num];
}
