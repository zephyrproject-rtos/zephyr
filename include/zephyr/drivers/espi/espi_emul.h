/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ESPI_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_ESPI_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/espi/espi.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

/**
 * @file
 *
 * @brief Public APIs for the eSPI emulation drivers.
 */

/**
 * @brief eSPI Emulation Interface
 * @defgroup espi_emul_interface eSPI Emulation Interface
 * @ingroup io_emulators
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define ESPI_EMUL_KBC8042_PORT_IN_STATUS 0x64
#define ESPI_EMUL_KBC8042_PORT_IN_DATA	 0x60
#define ESPI_EMUL_KBC8042_PORT_OUT_DATA	 0x60
#define ESPI_EMUL_KBC8042_PORT_OUT_CMD	 0x64

#define KBC8042_STATUS_OBF BIT(0)
#define KBC8042_STATUS_IBF BIT(1)
#define KBC8042_STATUS_F0  BIT(2)
#define KBC8042_STATUS_A2  BIT(3)
#define KBC8042_STATUS_ST0 BIT(4)
#define KBC8042_STATUS_ST1 BIT(5)
#define KBC8042_STATUS_ST2 BIT(6)
#define KBC8042_STATUS_ST3 BIT(7)

#define KBC8042_STATUS_OBF BIT(0)
#define KBC8042_STATUS_IBF BIT(1)
#define KBC8042_STATUS_F0  BIT(2)
#define KBC8042_STATUS_A2  BIT(3)
#define KBC8042_STATUS_ST0 BIT(4)
#define KBC8042_STATUS_ST1 BIT(5)
#define KBC8042_STATUS_ST2 BIT(6)
#define KBC8042_STATUS_ST3 BIT(7)

struct espi_emul;

struct espi_emul_driver_api {
	struct espi_driver_api espi_api;

	/**
	 * Perform the I/O read from host through the eSPI bus.
	 * This function doesn't exist in real eSPI API since it is
	 * handled internally by the MCUs if needed. It is used by some
	 * peripherals like 8042 keyboard controller.
	 */
	int (*host_io_read)(const struct device *dev, uint8_t length, uint16_t addr, uint32_t *reg);
	/**
	 * Perform the I/O write from host through the eSPI bus.
	 * This function doesn't exist in real eSPI API since it is
	 * handled internally by the MCUs if needed. It is used by some
	 * peripherals like 8042 keyboard controller.
	 */
	int (*host_io_write)(const struct device *dev, uint8_t length, uint16_t addr, uint32_t reg);
};

/** Node in a linked list of emulators for eSPI devices */
struct espi_emul {
	sys_snode_t node;
	/** Target emulator - REQUIRED for all emulated bus nodes of any type */
	const struct emul *target;
	/** API provided for this device */
	const struct espi_emul_device_api *api;
	/** eSPI chip-select of the emulated device */
	uint16_t chipsel;
};

/** Data about the virtual wire */
struct espi_emul_vw_data {
	/* Virtual wire signal */
	enum espi_vwire_signal sig;
	/* The level(state) of the virtual wire */
	bool level;
};

/**
 * Defines the API used to communicate with the emulated bus and device.
 * Uses the functions defined in struct espi_driver_api used by eSPI
 * subsystem with changed parameter from struct device to espi_emul and adds
 * some functions that are normally handled internally by the MCU.
 */
struct espi_emul_device_api {
	int (*config)(const struct espi_emul *dev, struct espi_cfg *cfg);
	bool (*get_channel_status)(const struct espi_emul *dev, enum espi_channel ch);
	/* Logical Channel 0 APIs */
	int (*read_request)(const struct espi_emul *dev, struct espi_request_packet *req);
	int (*write_request)(const struct espi_emul *dev, struct espi_request_packet *req);
	int (*read_lpc_request)(const struct espi_emul *dev, enum lpc_peripheral_opcode op,
				uint32_t *data);
	int (*write_lpc_request)(const struct espi_emul *dev, enum lpc_peripheral_opcode op,
				 uint32_t *data);
	/* Logical Channel 1 APIs */
	int (*send_vwire)(const struct espi_emul *dev, enum espi_vwire_signal vw, uint8_t level);
	int (*receive_vwire)(const struct espi_emul *dev, enum espi_vwire_signal vw,
			     uint8_t *level);
	/* Logical Channel 2 APIs */
	int (*send_oob)(const struct espi_emul *dev, struct espi_oob_packet *pckt);
	int (*receive_oob)(const struct espi_emul *dev, struct espi_oob_packet *pckt);
	/* Logical Channel 3 APIs */
	int (*flash_read)(const struct espi_emul *dev, struct espi_flash_packet *pckt);
	int (*flash_write)(const struct espi_emul *dev, struct espi_flash_packet *pckt);
	int (*flash_erase)(const struct espi_emul *dev, struct espi_flash_packet *pckt);
	/* Callbacks and traffic intercept */
	int (*manage_callback)(const struct espi_emul *dev, struct espi_callback *callback,
			       bool set);

	/**
	 * Sends the registered callbacks. It may be used by host side of
	 * the bus to emulate the reception of some events that are normally
	 * sent through the bus and handled by the controller.
	 */
	int (*raise_event)(const struct espi_emul *dev, struct espi_event ev);

	/**
	 * Retrieve the pointer to the ACPI shared memory region.
	 */
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	uintptr_t (*get_acpi_shm)(const struct espi_emul *dev);
#endif
};

/**
 * Register an emulated device on the controller
 *
 * @param dev Device that will use the emulator
 * @param emul eSPI emulator to use
 * @return 0 indicating success (always)
 */
int espi_emul_register(const struct device *dev, struct espi_emul *emul);

/**
 * Perform the I/O read from host through the emulated eSPI bus.
 * This function doesn't exist in real eSPI API since it is
 * handled internally by the MCUs if needed. It is used by some
 * peripherals like 8042 keyboard controller.
 *
 * @param dev Device that will handle the operation
 * @param length number of bytes to read (1/2/4 supported)
 * @param addr I/O address to read
 * @param reg Pointer to value which will store the data
 * @return 0 if IO was successful, -EINVAL if address is not supported
 */
int espi_emul_host_io_read(const struct device *dev, uint8_t length, uint16_t addr, uint32_t *reg);

/**
 * Perform the I/O write from host through the eSPI bus.
 * This function doesn't exist in real eSPI API since it is
 * handled internally by the MCUs if needed. It is used by some
 * peripherals like 8042 keyboard controller.
 *
 * @param dev Device that will handle the operation
 * @param length number of bytes to write (1/2/4 supported)
 * @param addr I/O address to write
 * @param reg Data to write
 * @return 0 if IO was successful, -EINVAL if address is not supported
 */
int espi_emul_host_io_write(const struct device *dev, uint8_t length, uint16_t addr, uint32_t reg);

/**
 * Perform the set_vwire but from the host side of the bus.
 * This value then will be sent from the host through the emulated eSPI bus
 * to notify the controller about the level change.
 * If the callback for the reception of vwire change is set, it will be called.
 *
 * @param dev Device that will handle the operation
 * @param vw virtual wire which value will be changed
 * @param level logic level on virtual wire
 * @return 0 if IO was successful, -EIO if there's no emulator, -EPERM if
 *           the direction of vwire is invalid
 */
int espi_emul_host_set_vwire(const struct device *dev, enum espi_vwire_signal vw, uint8_t level);

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
/**
 * Retrieve the pointer to the ACPI shared memory region.
 *
 * @param dev Device that will handle the operation
 * @return pointer to the shared memory region with size specified by
 *         CONFIG_ESPI_EMUL_HOST_ACPI_SHM_REGION_SIZE
 */
uintptr_t espi_emul_host_get_acpi_shm(const struct device *dev);
#endif

/**
 * Manage the callbacks of the eSPI host.
 * It may be used to verify if calling the send_vwire function on the
 * controller bus results in the reception of it on the host.
 *
 * @param dev Device that will handle the operation
 * @param callback structure describing managed callback
 * @param set enable or disable the callback
 * @return 0 if IO was successful, -EIO if there's no emulator, or other values
 *           returned by the emulator
 */
int espi_emul_host_manage_callback(const struct device *dev, struct espi_callback *callback,
				   bool set);

/**
 * Sends the registered callbacks. It may be used by the host side of the bus
 * to emulate the reception of some events that are normally sent through
 * the bus and handled by the MCU with interrupts.
 *
 * @param dev Device that will handle the operation
 * @param ev structure describing event about which the secondary side
 *           wants to inform
 * @return always 0
 */
int espi_emul_raise_event(const struct device *dev, struct espi_event ev);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ESPI_EMUL_H_ */
