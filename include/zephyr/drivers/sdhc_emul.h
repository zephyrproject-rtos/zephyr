/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SDHC emulation bus interface
 *
 * Defines the emulator bus types and APIs used by the SDHC
 * emulation controller to communicate with emulated card devices.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SDHC_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SDHC_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

/**
 * @brief SDHC Emulation Interface
 * @defgroup sdhc_emul_interface SDHC Emulation Interface
 * @ingroup io_emulators
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

struct sdhc_emul_api;

/** Node in a linked list of emulators for SDHC card devices */
struct sdhc_emul {
	/** Linked list node for emulator registry */
	sys_snode_t node;

	/** Target emulator - REQUIRED for all emulated bus nodes */
	const struct emul *target;

	/** API provided for this device */
	const struct sdhc_emul_api *api;

	/**
	 * A mock API that if not NULL will take precedence over the actual API.
	 * If set, a return value of -ENOSYS will revert back to the default api.
	 */
	struct sdhc_emul_api *mock_api;

	/** Card slot index (typically 0 for single-slot controllers) */
	uint16_t slot;
};

/**
 * Passes an SDHC request to the card emulator.
 *
 * @param target The device Emulator instance.
 * @param cmd SDHC command to process.
 * @param data Optional data transfer associated with the command.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOTSUP Command not supported.
 */
typedef int (*sdhc_emul_request_t)(const struct emul *target,
				   struct sdhc_command *cmd,
				   struct sdhc_data *data);

/**
 * Check if the emulated card is busy.
 *
 * @param target The device Emulator instance.
 *
 * @retval 0 Card is not busy.
 * @retval 1 Card is busy.
 */
typedef int (*sdhc_emul_card_busy_t)(const struct emul *target);

/**
 * Check if the emulated card is present.
 *
 * @param target The device Emulator instance.
 *
 * @retval 0 Card not present.
 * @retval 1 Card present.
 */
typedef int (*sdhc_emul_card_present_t)(const struct emul *target);

/**
 * Register an emulated card device on the controller
 *
 * @param dev Device that will use the emulator (the controller)
 * @param emul SDHC card emulator to register
 * @return 0 indicating success (always)
 */
int sdhc_emul_register(const struct device *dev, struct sdhc_emul *emul);

/**
 * Find the first SDHC emulator registered on a controller.
 *
 * @param dev SDHC emulation controller device.
 *
 * @return First emulator, or NULL when no card slot is registered.
 */
struct sdhc_emul *sdhc_emul_get_card(const struct device *dev);

/**
 * Raise an interrupt from an emulated SDHC card through its controller.
 *
 * @param dev SDHC emulation controller device.
 * @param sources Interrupt source bits, using SDHC_INT_* values.
 */
void sdhc_emul_raise_interrupt(const struct device *dev, int sources);

/** Definition of the emulator API for SDHC card devices */
struct sdhc_emul_api {
	/** Pass an SDHC request to the card emulator */
	sdhc_emul_request_t request;

	/** Check if the emulated card is busy */
	sdhc_emul_card_busy_t card_busy;

	/** Check if the emulated card is present */
	sdhc_emul_card_present_t card_present;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SDHC_EMUL_H_ */
