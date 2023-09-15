/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_ACPI_H_
#define ZEPHYR_INCLUDE_DRIVERS_ACPI_H_
#include <acpica/source/include/acpi.h>
#include <zephyr/drivers/pcie/pcie.h>

#define ACPI_RES_INVALID ACPI_RESOURCE_TYPE_MAX

#define ACPI_DRHD_FLAG_INCLUDE_PCI_ALL			BIT(0)
#define ACPI_DMAR_FLAG_INTR_REMAP				BIT(0)
#define ACPI_DMAR_FLAG_X2APIC_OPT_OUT			BIT(1)
#define ACPI_DMAR_FLAG_DMA_CTRL_PLATFORM_OPT_IN	BIT(2)

#define ACPI_MMIO_GET(res) (res)->reg_base[0].mmio
#define ACPI_IO_GET(res) (res)->reg_base[0].port
#define ACPI_RESOURCE_SIZE_GET(res) (res)->reg_base[0].length
#define ACPI_RESOURCE_TYPE_GET(res) (res)->reg_base[0].type

#define ACPI_MULTI_MMIO_GET(res, idx) (res)->reg_base[idx].mmio
#define ACPI_MULTI_IO_GET(res, idx) (res)->reg_base[idx].port
#define ACPI_MULTI_RESOURCE_SIZE_GET(res, idx) (res)->reg_base[idx].length
#define ACPI_MULTI_RESOURCE_TYPE_GET(res, idx) (res)->reg_base[idx].type

#define ACPI_RESOURCE_COUNT_GET(res) (res)->mmio_max

enum acpi_res_type {
	/** IO mapped Resource type */
	ACPI_RES_TYPE_IO,
	/** Memory mapped Resource type */
	ACPI_RES_TYPE_MEM,
	/** Unknown Resource type */
	ACPI_RES_TYPE_UNKNOWN,
};

struct acpi_dev {
	ACPI_HANDLE handle;
	char *path;
	ACPI_RESOURCE *res_lst;
	int res_type;
	ACPI_DEVICE_INFO *dev_info;
};

union acpi_dmar_id {
	struct {
		uint16_t function: 3;
		uint16_t device: 5;
		uint16_t bus: 8;
	} bits;

	uint16_t raw;
};

struct acpi_mcfg {
	ACPI_TABLE_HEADER header;
	uint64_t _reserved;
	ACPI_MCFG_ALLOCATION pci_segs[];
} __packed;

struct acpi_irq_resource {
	uint32_t flags;
	union {
		uint16_t irq;
		uint16_t irqs[CONFIG_ACPI_IRQ_VECTOR_MAX];
	};
	uint8_t irq_vector_max;
};

struct acpi_reg_base {
	enum acpi_res_type type;
	union {
		uintptr_t mmio;
		uintptr_t port;
	};
	uint32_t length;
};

struct acpi_mmio_resource {
	struct acpi_reg_base reg_base[CONFIG_ACPI_MMIO_ENTRIES_MAX];
	uint8_t mmio_max;
};

/**
 * @brief Get the ACPI HID for a node
 *
 * @param node_id DTS node identifier
 * @return The HID of the ACPI node
 */
#define ACPI_DT_HID(node_id) DT_PROP(node_id, acpi_hid)

/**
 * @brief Get the ACPI UID for a node if one exist
 *
 * @param node_id DTS node identifier
 * @return The UID of the ACPI node else NULL if does not exist
 */
#define ACPI_DT_UID(node_id) DT_PROP_OR(node_id, acpi_uid, NULL)

/**
 * @brief check whether the node has ACPI HID property or not
 *
 * @param node_id DTS node identifier
 * @return 1 if the node has the HID, 0 otherwise.
 */
#define ACPI_DT_HAS_HID(node_id) DT_NODE_HAS_PROP(node_id, acpi_hid)

/**
 * @brief check whether the node has ACPI UID property or not
 *
 * @param node_id DTS node identifier
 * @return 1 if the node has the UID, 0 otherwise.
 */
#define ACPI_DT_HAS_UID(node_id) DT_NODE_HAS_PROP(node_id, acpi_uid)

/**
 * @brief Init legacy interrupt routing table information from ACPI.
 * Currently assume platform have only one PCI bus.
 *
 * @param hid the hardware id of the ACPI child device
 * @param uid the unique id of the ACPI child device. The uid can be
 * NULL if only one device with given hid present in the platform.
 * @return return 0 on success or error code
 */
int acpi_legacy_irq_init(const char *hid, const char *uid);

/**
 * @brief Retrieve a legacy interrupt number for a PCI device.
 *
 * @param bdf the BDF of endpoint/PCI device
 * @return return IRQ number or UINT_MAX if not found
 */
uint32_t acpi_legacy_irq_get(pcie_bdf_t bdf);

/**
 * @brief Retrieve the current resource settings of a device.
 *
 * @param dev_name the name of the device
 * @param res the list of acpi resource list
 * @return return 0 on success or error code
 */
int acpi_current_resource_get(char *dev_name, ACPI_RESOURCE **res);

/**
 * @brief Retrieve possible resource settings of a device.
 *
 * @param dev_name the name of the device
 * @param res the list of acpi resource list
 * @return return 0 on success or error code
 */
int acpi_possible_resource_get(char *dev_name, ACPI_RESOURCE **res);

/**
 * @brief Free current resource list memory which is retrieved by
 * acpi_current_resource_get().
 *
 * @param res the list of acpi resource list
 * @return return 0 on success or error code
 */
int acpi_current_resource_free(ACPI_RESOURCE *res);

/**
 * @brief Parse resource table for a given resource type.
 *
 * @param res the list of acpi resource list
 * @param res_type the acpi resource type
 * @return resource list for the given type on success or NULL
 */
ACPI_RESOURCE *acpi_resource_parse(ACPI_RESOURCE *res, int res_type);

/**
 * @brief Retrieve ACPI device info for given hardware id and unique id.
 *
 * @param hid the hardware id of the ACPI child device
 * @param uid the unique id of the ACPI child device. The uid can be
 * NULL if only one device with given HID present in the platform.
 * @return ACPI child device info on success or NULL
 */
struct acpi_dev *acpi_device_get(const char *hid, const char *uid);

/**
 * @brief Retrieve acpi device info from the index.
 *
 * @param index the device index of an acpi child device
 * @return acpi child device info on success or NULL
 */
struct acpi_dev *acpi_device_by_index_get(int index);

/**
 * @brief Parse resource table for irq info.
 *
 * @param res_lst the list of acpi resource list
 * @return irq resource list on success or NULL
 */
static inline ACPI_RESOURCE_IRQ *acpi_irq_res_get(ACPI_RESOURCE *res_lst)
{
	ACPI_RESOURCE *res = acpi_resource_parse(res_lst, ACPI_RESOURCE_TYPE_IRQ);

	return res ? &res->Data.Irq : NULL;
}

/**
 * @brief Parse resource table for irq info.
 *
 * @param child_dev the device object of the ACPI node
 * @param irq_res irq resource info
 * @return return 0 on success or error code
 */
int acpi_device_irq_get(struct acpi_dev *child_dev, struct acpi_irq_resource *irq_res);

/**
 * @brief Parse resource table for MMIO info.
 *
 * @param child_dev the device object of the ACPI node
 * @param mmio_res MMIO resource info
 * @return return 0 on success or error code
 */
int acpi_device_mmio_get(struct acpi_dev *child_dev, struct acpi_mmio_resource *mmio_res);

/**
 * @brief Parse resource table for identify resource type.
 *
 * @param res the list of acpi resource list
 * @return resource type on success or invalid resource type
 */
int acpi_device_type_get(ACPI_RESOURCE *res);

/**
 * @brief Retrieve acpi table for the given signature.
 *
 * @param signature pointer to the 4-character ACPI signature for the requested table
 * @param inst instance number for the requested table
 * @return acpi_table pointer to the acpi table on success else return NULL
 */
void *acpi_table_get(char *signature, int inst);

/**
 * @brief retrieve acpi MAD table for the given type.
 *
 * @param type type of requested MAD table
 * @param tables pointer to the MAD table
 * @param num_inst number of instance for the requested table
 * @return return 0 on success or error code
 */
int acpi_madt_entry_get(int type, ACPI_SUBTABLE_HEADER **tables, int *num_inst);

/**
 * @brief retrieve DMA remapping structure for the given type.
 *
 * @param type type of remapping structure
 * @param tables pointer to the dmar id structure
 * @return return 0 on success or error code
 */
int acpi_dmar_entry_get(enum AcpiDmarType type, ACPI_SUBTABLE_HEADER **tables);

/**
 * @brief retrieve acpi DRHD info for the given scope.
 *
 * @param scope scope of requested DHRD table
 * @param dev_scope pointer to the sub table (optional)
 * @param dmar_id pointer to the DHRD info
 * @param num_inst number of instance for the requested table
 * @param max_inst maximum number of entry for the given dmar_id buffer
 * @return return 0 on success or error code
 */
int acpi_drhd_get(enum AcpiDmarScopeType scope, ACPI_DMAR_DEVICE_SCOPE *dev_scope,
		  union acpi_dmar_id *dmar_id, int *num_inst, int max_inst);

typedef void (*dmar_foreach_subtable_func_t)(ACPI_DMAR_HEADER *subtable, void *arg);
typedef void (*dmar_foreach_devscope_func_t)(ACPI_DMAR_DEVICE_SCOPE *devscope, void *arg);

void acpi_dmar_foreach_subtable(ACPI_TABLE_DMAR *dmar, dmar_foreach_subtable_func_t func,
				void *arg);
void acpi_dmar_foreach_devscope(ACPI_DMAR_HARDWARE_UNIT *hu,
				dmar_foreach_devscope_func_t func, void *arg);

/**
 * @brief Retrieve IOAPIC id
 *
 * @param ioapic_id IOAPIC id
 * @return return 0 on success or error code
 */
int acpi_dmar_ioapic_get(uint16_t *ioapic_id);

/**
 * @brief Retrieve the 'n'th enabled local apic info.
 *
 * @param cpu_num the cpu number
 * @return local apic info on success or NULL otherwise
 */
ACPI_MADT_LOCAL_APIC *acpi_local_apic_get(int cpu_num);

/**
 * @brief invoke an ACPI method and return the result.
 *
 * @param path the path name of the ACPI object
 * @param arg_list the list of arguments to be pass down
 * @param ret_obj the ACPI result to be return
 * @return return 0 on success or error code
 */
int acpi_invoke_method(char *path, ACPI_OBJECT_LIST *arg_list, ACPI_OBJECT *ret_obj);

#endif
