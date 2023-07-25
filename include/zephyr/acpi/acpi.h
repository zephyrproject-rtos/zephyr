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

struct acpi_dev {
	ACPI_HANDLE handle;
	char *path;
	char hid[CONFIG_ACPI_HID_LEN_MAX];
	ACPI_RESOURCE *res_lst;
	int res_type;
	ACPI_DEVICE_INFO *dev_info;
};

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
 * @brief Retrieve IRQ routing table of a bus.
 *
 * @param bus_name the name of the bus
 * @param rt_table the IRQ routing table
 * @param rt_size the the size of IRQ routing table
 * @return return 0 on success or error code
 */
int acpi_get_irq_routing_table(char *bus_name,
			       ACPI_PCI_ROUTING_TABLE *rt_table, size_t rt_size);

/**
 * @brief Parse resource table for a given resource type.
 *
 * @param res the list of acpi resource list
 * @param res_type the acpi resource type
 * @return resource list for the given type on success or NULL
 */
ACPI_RESOURCE *acpi_resource_parse(ACPI_RESOURCE *res, int res_type);

/**
 * @brief Retrieve acpi device info for given hardware id and unique id.
 *
 * @param hid the hardware id of the acpi child device
 * @param inst the unique id of the acpi child device
 * @return acpi child device info on success or NULL
 */
struct acpi_dev *acpi_device_get(char *hid, int inst);

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
 * @param acpi_table pointer to the acpi table
 * @return return 0 on success or error code
 */
int acpi_table_get(char *signature, int inst, void **acpi_table);

#endif
