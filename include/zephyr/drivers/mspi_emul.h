/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MSPI_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_MSPI_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

/**
 * @file
 *
 * @brief Public APIs for the MSPI emulation drivers.
 */

/**
 * @brief MSPI Emulation Interface
 * @defgroup mspi_emul_interface MSPI Emulation Interface
 * @ingroup io_emulators
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

struct mspi_emul;

/**
 * Find an emulator present on a MSPI bus
 *
 * At present the function is used only to find an emulator of the host
 * device. It may be useful in systems with the SPI flash chips.
 *
 * @param dev MSPI emulation controller device
 * @param dev_idx Device index from device tree.
 * @return mspi_emul to use
 * @return NULL if not found
 */
typedef struct mspi_emul *(*mspi_emul_find_emul)(const struct device *dev,
						 uint16_t dev_idx);

/**
 * Triggers an event on the emulator of MSPI controller side which causes
 * calling specific callbacks.
 *
 * @param dev MSPI emulation controller device
 * @param evt_type Event type to be triggered @see mspi_bus_event
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
typedef int (*mspi_emul_trigger_event)(const struct device *dev,
				       enum mspi_bus_event evt_type);

/**
 * Loopback MSPI transceive request to the device emulator
 * as no real hardware attached
 *
 * @param target The device Emulator instance
 * @param packets Pointer to the buffers of command, addr, data and etc.
 * @param num_packet The number of packets in packets.
 * @param async Indicate whether this is a asynchronous request.
 * @param timeout Maximum Time allowed for this request
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
typedef int (*emul_mspi_dev_api_transceive)(const struct emul *target,
					    const struct mspi_xfer_packet *packets,
					    uint32_t num_packet,
					    bool async,
					    uint32_t timeout);

/** Definition of the MSPI device emulator API */
struct emul_mspi_device_api {
	emul_mspi_dev_api_transceive transceive;
};

/** Node in a linked list of emulators for MSPI devices */
struct mspi_emul {
	sys_snode_t                  node;
	/** Target emulator - REQUIRED for all emulated bus nodes of any type */
	const struct emul            *target;
	/** API provided for this device */
	const struct emul_mspi_device_api *api;
	/** device index */
	uint16_t                     dev_idx;
};

/** Definition of the MSPI controller emulator API */
struct emul_mspi_driver_api {
	/* The struct mspi_driver_api has to be first in
	 * struct emul_mspi_driver_api to make pointer casting working
	 */
	struct mspi_driver_api       mspi_api;
	/* The rest, emulator specific functions */
	mspi_emul_trigger_event      trigger_event;
	mspi_emul_find_emul          find_emul;
};

/**
 * Register an emulated device on the controller
 *
 * @param dev MSPI emulation controller device
 * @param emul MSPI device emulator to be registered
 * @return 0 indicating success (always)
 */
int mspi_emul_register(const struct device *dev, struct mspi_emul *emul);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MSPI_EMUL_H_ */
