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

static struct {
	struct acpi_dev child_dev[CONFIG_ACPI_DEV_MAX];
	int num_dev;
#ifdef CONFIG_PCIE_PRT
	ACPI_PCI_ROUTING_TABLE pci_prt_table[CONFIG_ACPI_MAX_PRT_ENTRY];
#endif
	bool early_init;
	ACPI_STATUS status;
} acpi = {
	.status = AE_NOT_CONFIGURED,
};

static int acpi_init(void);

static int check_init_status(void)
{
	if (acpi.status == AE_NOT_CONFIGURED) {
		acpi.status = acpi_init();
	}

	if (ACPI_FAILURE(acpi.status)) {
		LOG_ERR("ACPI init was not success");
		return -EIO;
	}

	return 0;
}

static void notify_handler(ACPI_HANDLE device, UINT32 value, void *ctx)
{
	ACPI_INFO(("Received a notify 0x%X", value));
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
	if (!acpi.early_init) {
		status = AcpiInitializeTables(NULL, 16, FALSE);
		if (ACPI_FAILURE(status)) {
			ACPI_EXCEPTION((AE_INFO, status, "While initializing Table Manager"));
			goto exit;
		}
	}

	/* Create the ACPI namespace from ACPI tables */
	status = AcpiLoadTables();
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While loading ACPI tables"));
		goto exit;
	}

	/* Install local handlers */
	status = install_handlers();
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While installing handlers"));
		goto exit;
	}

	/* Initialize the ACPI hardware */
	status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "While enabling ACPI"));
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

static ACPI_STATUS dev_resource_enum_callback(ACPI_HANDLE obj_handle, UINT32 level, void *ctx,
					      void **ret_value)
{
	ACPI_NAMESPACE_NODE *node;
	ACPI_BUFFER rt_buffer;
	struct acpi_dev *child_dev;

	node = ACPI_CAST_PTR(ACPI_NAMESPACE_NODE, obj_handle);
	char *path_name;
	ACPI_STATUS status;
	ACPI_DEVICE_INFO *dev_info;

	LOG_DBG("%s %p", __func__, node);

	/* get device info such as HID, Class ID etc. */
	status = AcpiGetObjectInfo(obj_handle, &dev_info);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("AcpiGetObjectInfo failed: %s", AcpiFormatException(status));
		goto exit;
	}

	if (acpi.num_dev >= CONFIG_ACPI_DEV_MAX) {
		return AE_NO_MEMORY;
	}

	if (!(dev_info->Valid & ACPI_VALID_HID)) {
		goto exit;
	}

	child_dev = (struct acpi_dev *)&acpi.child_dev[acpi.num_dev++];
	child_dev->handle = obj_handle;
	child_dev->dev_info = dev_info;

	path_name = AcpiNsGetNormalizedPathname(node, TRUE);
	if (!path_name) {
		LOG_ERR("No memory for path_name");
		goto exit;
	} else {
		LOG_DBG("Device path: %s", path_name);
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

	return AE_OK;
}

static int acpi_enum_devices(void)
{
	LOG_DBG("");

	AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, ACPI_UINT32_MAX,
			  dev_resource_enum_callback, NULL, NULL, NULL);

	return 0;
}

static int acpi_early_init(void)
{
	ACPI_STATUS status;

	LOG_DBG("");

	if (acpi.early_init) {
		LOG_DBG("acpi early init already done");
		return 0;
	}

	status = AcpiInitializeTables(NULL, 16, FALSE);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("Error in acpi table init:%d", status);
		return -EIO;
	}

	acpi.early_init = true;

	return 0;
}

int acpi_current_resource_get(char *dev_name, ACPI_RESOURCE **res)
{
	ACPI_BUFFER rt_buffer;
	ACPI_NAMESPACE_NODE *node;
	ACPI_STATUS status;

	LOG_DBG("%s", dev_name);

	status = check_init_status();
	if (status) {
		return -EAGAIN;
	}

	node = acpi_evaluate_method(dev_name, METHOD_NAME__CRS);
	if (!node) {
		LOG_ERR("Evaluation failed for given device: %s", dev_name);
		return -ENOTSUP;
	}

	rt_buffer.Pointer = NULL;
	rt_buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	status = AcpiGetCurrentResources(node, &rt_buffer);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("AcpiGetCurrentResources failed: %s", AcpiFormatException(status));
		return -ENOTSUP;
	}

	*res = rt_buffer.Pointer;

	return 0;
}

int acpi_possible_resource_get(char *dev_name, ACPI_RESOURCE **res)
{
	ACPI_BUFFER rt_buffer;
	ACPI_NAMESPACE_NODE *node;
	ACPI_STATUS status;

	LOG_DBG("%s", dev_name);

	status = check_init_status();
	if (status) {
		return -EAGAIN;
	}

	node = acpi_evaluate_method(dev_name, METHOD_NAME__PRS);
	if (!node) {
		LOG_ERR("Evaluation failed for given device: %s", dev_name);
		return -ENOTSUP;
	}

	rt_buffer.Pointer = NULL;
	rt_buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	AcpiGetPossibleResources(node, &rt_buffer);
	*res = rt_buffer.Pointer;

	return 0;
}

int acpi_current_resource_free(ACPI_RESOURCE *res)
{
	ACPI_FREE(res);

	return 0;
}

#ifdef CONFIG_PCIE_PRT
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
		if (((acpi.pci_prt_table[i].Address >> 16) & 0xffff) == slot &&
		    acpi.pci_prt_table[i].Pin + 1 == pin) {
			LOG_DBG("[%d]Device irq info: slot:%d pin:%d irq:%d", i, slot, pin,
				acpi.pci_prt_table[i].SourceIndex);
			return acpi.pci_prt_table[i].SourceIndex;
		}
	}

	return UINT_MAX;
}

int acpi_legacy_irq_init(const char *hid, const char *uid)
{
	struct acpi_dev *child_dev = acpi_device_get(hid, uid);
	ACPI_PCI_ROUTING_TABLE *rt_table = acpi.pci_prt_table;
	ACPI_BUFFER rt_buffer;
	ACPI_NAMESPACE_NODE *node;
	ACPI_STATUS status;

	if (!child_dev) {
		LOG_ERR("no such PCI bus device %s %s", hid, uid);
		return -ENODEV;
	}

	node = acpi_evaluate_method(child_dev->path, METHOD_NAME__PRT);
	if (!node) {
		LOG_ERR("Evaluation failed for given device: %s", child_dev->path);
		return -ENODEV;
	}

	rt_buffer.Pointer = rt_table;
	rt_buffer.Length = ARRAY_SIZE(acpi.pci_prt_table) * sizeof(ACPI_PCI_ROUTING_TABLE);

	status = AcpiGetIrqRoutingTable(node, &rt_buffer);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("unable to retrieve IRQ Routing Table: %s", child_dev->path);
		return -EIO;
	}

	if (rt_table->Source[0]) {
		/*
		 * If Name path exist then PCI interrupts are configurable and are not hardwired to
		 * any specific interrupt inputs on the interrupt controller. OSPM can uses
		 * _PRS/_CRS/_SRS to configure interrupts. But currently leave existing PCI bus
		 * driver with arch_irq_allocate() menthod for allocate and configure interrupts
		 * without conflicting.
		 */
		return -ENOENT;
	}

	for (size_t i = 0; i < ARRAY_SIZE(acpi.pci_prt_table); i++) {
		if (!acpi.pci_prt_table[i].SourceIndex) {
			break;
		}
		if (IS_ENABLED(CONFIG_X86_64)) {
			/* mark the PRT irq numbers as reserved. */
			arch_irq_set_used(acpi.pci_prt_table[i].SourceIndex);
		}
	}

	return 0;
}
#endif /* CONFIG_PCIE_PRT */

ACPI_RESOURCE *acpi_resource_parse(ACPI_RESOURCE *res, int res_type)
{
	do {
		if (!res->Length) {
			LOG_DBG("zero length found!");
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

int acpi_device_irq_get(struct acpi_dev *child_dev, struct acpi_irq_resource *irq_res)
{
	ACPI_RESOURCE *res = acpi_resource_parse(child_dev->res_lst, ACPI_RESOURCE_TYPE_IRQ);

	if (!res) {
		res = acpi_resource_parse(child_dev->res_lst, ACPI_RESOURCE_TYPE_EXTENDED_IRQ);
		if (!res) {
			return -ENODEV;
		}

		if (res->Data.ExtendedIrq.InterruptCount > CONFIG_ACPI_IRQ_VECTOR_MAX) {
			return -ENOMEM;
		}

		memset(irq_res, 0, sizeof(struct acpi_irq_resource));
		irq_res->irq_vector_max = res->Data.ExtendedIrq.InterruptCount;
		for (int i = 0; i < irq_res->irq_vector_max; i++) {
			irq_res->irqs[i] = (uint16_t)res->Data.ExtendedIrq.Interrupts[i];
		}

		irq_res->flags = arch_acpi_encode_irq_flags(res->Data.ExtendedIrq.Polarity,
							    res->Data.ExtendedIrq.Triggering);
	} else {
		if (res->Data.Irq.InterruptCount > CONFIG_ACPI_IRQ_VECTOR_MAX) {
			return -ENOMEM;
		}

		irq_res->irq_vector_max = res->Data.Irq.InterruptCount;
		for (int i = 0; i < irq_res->irq_vector_max; i++) {
			irq_res->irqs[i] = (uint16_t)res->Data.Irq.Interrupts[i];
		}

		irq_res->flags = arch_acpi_encode_irq_flags(res->Data.ExtendedIrq.Polarity,
							    res->Data.ExtendedIrq.Triggering);
	}

	return 0;
}

int acpi_device_mmio_get(struct acpi_dev *child_dev, struct acpi_mmio_resource *mmio_res)
{
	ACPI_RESOURCE *res = child_dev->res_lst;
	struct acpi_reg_base *reg_base = mmio_res->reg_base;
	int mmio_cnt = 0;

	do {
		if (!res->Length) {
			LOG_DBG("Found Acpi resource with zero length!");
			break;
		}

		switch (res->Type) {
		case ACPI_RESOURCE_TYPE_IO:
			reg_base[mmio_cnt].type = ACPI_RES_TYPE_IO;
			reg_base[mmio_cnt].port = (uint32_t)res->Data.Io.Minimum;
			reg_base[mmio_cnt++].length = res->Data.Io.AddressLength;
			break;

		case ACPI_RESOURCE_TYPE_FIXED_IO:
			reg_base[mmio_cnt].type = ACPI_RES_TYPE_IO;
			reg_base[mmio_cnt].port = (uint32_t)res->Data.FixedIo.Address;
			reg_base[mmio_cnt++].length = res->Data.FixedIo.AddressLength;
			break;

		case ACPI_RESOURCE_TYPE_MEMORY24:
			reg_base[mmio_cnt].type = ACPI_RES_TYPE_MEM;
			reg_base[mmio_cnt].mmio = (uintptr_t)res->Data.Memory24.Minimum;
			reg_base[mmio_cnt++].length = res->Data.Memory24.AddressLength;
			break;

		case ACPI_RESOURCE_TYPE_MEMORY32:
			reg_base[mmio_cnt].type = ACPI_RES_TYPE_MEM;
			reg_base[mmio_cnt].mmio = (uintptr_t)res->Data.Memory32.Minimum;
			reg_base[mmio_cnt++].length = res->Data.Memory32.AddressLength;
			break;

		case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
			reg_base[mmio_cnt].type = ACPI_RES_TYPE_MEM;
			reg_base[mmio_cnt].mmio = (uintptr_t)res->Data.FixedMemory32.Address;
			reg_base[mmio_cnt++].length = res->Data.FixedMemory32.AddressLength;
			break;
		}

		res = ACPI_NEXT_RESOURCE(res);
		if (mmio_cnt >= CONFIG_ACPI_MMIO_ENTRIES_MAX &&
			 res->Type != ACPI_RESOURCE_TYPE_END_TAG) {
			return -ENOMEM;
		}
	} while (res->Type != ACPI_RESOURCE_TYPE_END_TAG);

	if (!mmio_cnt) {
		return -ENODEV;
	}

	mmio_res->mmio_max = mmio_cnt;

	return 0;
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
			LOG_ERR("Error: zero length found!");
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

struct acpi_dev *acpi_device_get(const char *hid, const char *uid)
{
	struct acpi_dev *child_dev;
	int i = 0;

	LOG_DBG("");

	if (check_init_status()) {
		return NULL;
	}

	do {
		child_dev = &acpi.child_dev[i];
		if (!child_dev->path) {
			LOG_DBG("NULL device path found");
			continue;
		}

		if (!child_dev->res_lst || !child_dev->dev_info ||
		    !child_dev->dev_info->HardwareId.Length) {
			continue;
		}

		if (!strcmp(hid, child_dev->dev_info->HardwareId.String)) {
			if (uid && child_dev->dev_info->UniqueId.Length) {
				if (!strcmp(child_dev->dev_info->UniqueId.String, uid)) {
					return child_dev;
				}
			} else {
				return child_dev;
			}
		}
	} while (i++ < acpi.num_dev);

	return NULL;
}

struct acpi_dev *acpi_device_by_index_get(int index)
{
	return index < acpi.num_dev ? &acpi.child_dev[index] : NULL;
}

void *acpi_table_get(char *signature, int inst)
{
	ACPI_STATUS status;
	ACPI_TABLE_HEADER *table;

	if (!acpi.early_init) {
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

static uint32_t acpi_get_subtable_entry_num(int type, ACPI_SUBTABLE_HEADER *subtable,
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

int acpi_madt_entry_get(int type, ACPI_SUBTABLE_HEADER **tables, int *num_inst)
{
	ACPI_TABLE_HEADER *madt = acpi_table_get("APIC", 0);
	uintptr_t base = POINTER_TO_UINT(madt);
	uintptr_t offset = sizeof(ACPI_TABLE_MADT);
	ACPI_SUBTABLE_HEADER *subtable;

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

int acpi_dmar_entry_get(enum AcpiDmarType type, ACPI_SUBTABLE_HEADER **tables)
{
	struct acpi_table_dmar *dmar = acpi_table_get("DMAR", 0);
	uintptr_t base = POINTER_TO_UINT(dmar);
	uintptr_t offset = sizeof(ACPI_TABLE_DMAR);
	ACPI_DMAR_HEADER *subtable;

	if (!dmar) {
		LOG_ERR("error on get DMAR table");
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

void acpi_dmar_foreach_subtable(ACPI_TABLE_DMAR *dmar,
				dmar_foreach_subtable_func_t func, void *arg)
{
	uint16_t length = dmar->Header.Length;
	uintptr_t offset = sizeof(ACPI_TABLE_DMAR);

	__ASSERT_NO_MSG(length >= offset);

	while (offset < length) {
		ACPI_DMAR_HEADER *subtable = ACPI_ADD_PTR(ACPI_DMAR_HEADER, dmar, offset);

		__ASSERT_NO_MSG(subtable->Length >= sizeof(*subtable));
		__ASSERT_NO_MSG(subtable->Length <= length - offset);

		func(subtable, arg);

		offset += subtable->Length;
	}
}

void acpi_dmar_foreach_devscope(ACPI_DMAR_HARDWARE_UNIT *hu,
				dmar_foreach_devscope_func_t func, void *arg)
{
	uint16_t length = hu->Header.Length;
	uintptr_t offset = sizeof(ACPI_DMAR_HARDWARE_UNIT);

	__ASSERT_NO_MSG(length >= offset);

	while (offset < length) {
		ACPI_DMAR_DEVICE_SCOPE *devscope = ACPI_ADD_PTR(ACPI_DMAR_DEVICE_SCOPE,
								hu, offset);

		__ASSERT_NO_MSG(devscope->Length >= sizeof(*devscope));
		__ASSERT_NO_MSG(devscope->Length <= length - offset);

		func(devscope, arg);

		offset += devscope->Length;
	}
}

static void devscope_handler(ACPI_DMAR_DEVICE_SCOPE *devscope, void *arg)
{
	ACPI_DMAR_PCI_PATH *dev_path;
	union acpi_dmar_id pci_path;

	ARG_UNUSED(arg); /* may be unused */

	if (devscope->EntryType == ACPI_DMAR_SCOPE_TYPE_IOAPIC) {
		uint16_t *ioapic_id = arg;

		dev_path = ACPI_ADD_PTR(ACPI_DMAR_PCI_PATH, devscope,
					sizeof(ACPI_DMAR_DEVICE_SCOPE));

		/* Get first entry */
		pci_path.bits.bus = devscope->Bus;
		pci_path.bits.device = dev_path->Device;
		pci_path.bits.function = dev_path->Function;

		*ioapic_id = pci_path.raw;
	}
}

static void subtable_handler(ACPI_DMAR_HEADER *subtable, void *arg)
{
	ARG_UNUSED(arg); /* may be unused */

	if (subtable->Type == ACPI_DMAR_TYPE_HARDWARE_UNIT) {
		ACPI_DMAR_HARDWARE_UNIT *hu;

		hu = CONTAINER_OF(subtable, ACPI_DMAR_HARDWARE_UNIT, Header);
		acpi_dmar_foreach_devscope(hu, devscope_handler, arg);
	}
}

int acpi_dmar_ioapic_get(uint16_t *ioapic_id)
{
	ACPI_TABLE_DMAR *dmar = acpi_table_get("DMAR", 0);
	uint16_t found_ioapic = USHRT_MAX;

	if (dmar == NULL) {
		return -ENODEV;
	}

	acpi_dmar_foreach_subtable(dmar, subtable_handler, &found_ioapic);
	if (found_ioapic != USHRT_MAX) {
		*ioapic_id = found_ioapic;
		return 0;
	}

	return -ENOENT;
}

int acpi_drhd_get(enum AcpiDmarScopeType scope, ACPI_DMAR_DEVICE_SCOPE *dev_scope,
		  union acpi_dmar_id *dmar_id, int *num_inst, int max_inst)
{
	uintptr_t offset = sizeof(ACPI_DMAR_HARDWARE_UNIT);
	uint32_t i = 0;
	ACPI_DMAR_HEADER *drdh;
	ACPI_DMAR_DEVICE_SCOPE *subtable;
	ACPI_DMAR_PCI_PATH *dev_path;
	int ret;
	uintptr_t base;
	int scope_size;

	ret = acpi_dmar_entry_get(ACPI_DMAR_TYPE_HARDWARE_UNIT,
				  (ACPI_SUBTABLE_HEADER **)&drdh);
	if (ret) {
		LOG_ERR("Error on retrieve DMAR table");
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
					LOG_ERR("DHRD not enough buffer size");
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
		LOG_ERR("Error on retrieve DRHD Info");
		return -ENODEV;
	}

	if (dev_scope && subtable) {
		memcpy(dev_scope, subtable, sizeof(struct acpi_dmar_device_scope));
	}

	return 0;
}

#define ACPI_CPU_FLAGS_ENABLED 0x01u

ACPI_MADT_LOCAL_APIC *acpi_local_apic_get(int cpu_num)
{
	ACPI_MADT_LOCAL_APIC *lapic;
	int cpu_cnt;
	int idx;

	if (acpi_madt_entry_get(ACPI_MADT_TYPE_LOCAL_APIC, (ACPI_SUBTABLE_HEADER **)&lapic,
				&cpu_cnt)) {
		/* Error on MAD table. */
		return NULL;
	}

	for (idx = 0; cpu_num >= 0 && idx < cpu_cnt; idx++) {
		if (lapic[idx].LapicFlags & ACPI_CPU_FLAGS_ENABLED) {
			if (cpu_num == 0) {
				return &lapic[idx];
			}

			cpu_num--;
		}
	}

	return NULL;
}

int acpi_invoke_method(char *path, ACPI_OBJECT_LIST *arg_list, ACPI_OBJECT *ret_obj)
{
	ACPI_STATUS status;
	ACPI_BUFFER ret_buff;

	ret_buff.Length = sizeof(*ret_obj);
	ret_buff.Pointer = ret_obj;

	status = AcpiEvaluateObject(NULL, path, arg_list, &ret_buff);
	if (ACPI_FAILURE(status)) {
		LOG_ERR("error While executing %s method: %d", path, status);
		return -EIO;
	}

	return 0;
}

static int acpi_init(void)
{
	ACPI_STATUS status;

	LOG_DBG("");

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

	acpi_enum_devices();

exit:
	return status;
}
