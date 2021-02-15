/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ESPI_SPI_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_ESPI_SPI_EMUL_H_

/**
 * @file
 *
 * @brief Public APIs for the eSPI emulation drivers.
 */

#include <zephyr/types.h>
#include <device.h>

/**
 * @brief eSPI Emulation Interface
 * @defgroup espi_emul_interface eSPI Emulation Interface
 * @ingroup io_emulators
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define EMUL_ESPI_HOST_CHIPSEL 0

struct espi_emul;

/**
 * Passes eSPI virtual wires set request (virtual wire packet) to the emulator.
 * The emulator updates the state (level) of its virtual wire.
 *
 * @param emul Emulator instance
 * @param vw The signal to be set.
 * @param level The level of signal requested LOW(0) or HIGH(1).
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
typedef int (*emul_espi_api_set_vw)(struct espi_emul *emul,
				    enum espi_vwire_signal vw,
				    uint8_t level);

/**
 * Passes eSPI virtual wires get request (virtual wire packet) to the emulator.
 * The emulator returns the state (level) of its virtual wire.
 *
 * @param emul Emulator instance
 * @param vw The signal to be get.
 * @param level The level of the signal to be get.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
typedef int (*emul_espi_api_get_vw)(struct espi_emul *emul,
				    enum espi_vwire_signal vw,
				    uint8_t *level);

/**
 * Find an emulator present on a eSPI bus
 *
 * At present the function is used only to find an emulator of the host
 * device. It may be useful in systems with the SPI flash chips.
 *
 * @param dev eSPI emulation controller device
 * @param chipsel Chip-select value
 * @return emulator to use
 * @return NULL if not found
 */
typedef struct espi_emul *(*emul_find_emul)(const struct device *dev,
					    unsigned int chipsel);

/**
 * Triggers an event on the emulator of eSPI controller side which causes
 * calling specific callbacks.
 *
 * @param dev Device instance of emulated eSPI controller
 * @param evt Event to be triggered
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
typedef int (*emul_trigger_event)(const struct device *dev,
				  struct espi_event *evt);


/** Definition of the eSPI device emulator API */
struct emul_espi_device_api {
	emul_espi_api_set_vw set_vw;
	emul_espi_api_get_vw get_vw;
};

/** Node in a linked list of emulators for eSPI devices */
struct espi_emul {
	sys_snode_t node;
	/* API provided for this device */
	const struct emul_espi_device_api *api;
	/* eSPI chip-select of the emulated device */
	uint16_t chipsel;
};

/** Definition of the eSPI controller emulator API */
struct emul_espi_driver_api {
	/* The struct espi_driver_api has to be first in
	 * struct emul_espi_driver_api to make pointer casting working
	 */
	struct espi_driver_api espi_api;
	/* The rest, emulator specific functions */
	emul_trigger_event trigger_event;
	emul_find_emul find_emul;
};

/**
 * Register an emulated device on the controller
 *
 * @param dev Device that will use the emulator
 * @param name User-friendly name for this emulator
 * @param emul eSPI emulator to use
 * @return 0 indicating success (always)
 */
int espi_emul_register(const struct device *dev, const char *name,
		       struct espi_emul *emul);

/**
 * Sets the eSPI virtual wire on the host side, which will
 * trigger a proper event(and callbacks) on the emulated eSPI controller
 *
 * @param espi_dev eSPI emulation controller device
 * @param vw The signal to be set.
 * @param level The level of the signal to be set.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
int emul_espi_host_send_vw(const struct device *espi_dev,
			   enum espi_vwire_signal vw, uint8_t level);

/**
 * Perform port80 write on the emulated host side, which will
 * trigger a proper event(and callbacks) on the emulated eSPI controller
 *
 * @param espi_dev eSPI emulation controller device
 * @param data The date to be written.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
int emul_espi_host_port80_write(const struct device *espi_dev, uint32_t data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ESPI_SPI_EMUL_H_ */
