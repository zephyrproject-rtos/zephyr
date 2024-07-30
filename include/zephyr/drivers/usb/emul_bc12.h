/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Backend APIs for the BC1.2 emulators.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USB_EMUL_BC12_H_
#define ZEPHYR_INCLUDE_DRIVERS_USB_EMUL_BC12_H_

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/usb/usb_bc12.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BC1.2 backend emulator APIs
 * @defgroup b12_emulator_backend BC1.2 backed emulator APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in public documentation.
 */
__subsystem struct bc12_emul_driver_api {
	int (*set_charging_partner)(const struct emul *emul, enum bc12_type partner_type);
	int (*set_pd_partner)(const struct emul *emul, bool connected);
};
/**
 * @endcond
 */

/**
 * @brief Set the charging partner type connected to the BC1.2 device.
 *
 * The corresponding BC1.2 emulator updates the vendor specific registers
 * to simulate connection of the specified charging partner type.  The emulator
 * also generates an interrupt for processing by the real driver, if supported.
 *
 * @param target Pointer to the emulator structure for the BC1.2 emulator instance.
 * @param partner_type The simulated partner type.  Set to BC12_TYPE_NONE to
 * disconnect the charging partner.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if the partner type is not supported.
 */
static inline int bc12_emul_set_charging_partner(const struct emul *target,
						 enum bc12_type partner_type)
{
	const struct bc12_emul_driver_api *backend_api =
		(const struct bc12_emul_driver_api *)target->backend_api;

	return backend_api->set_charging_partner(target, partner_type);
}

/**
 * @brief Set the portable device partner state.
 *
 * The corresponding BC1.2 emulator updates the vendor specific registers
 * to simulate connection or disconnection of a portable device partner.
 * The emulator also generates an interrupt for processing by the real driver,
 * if supported.
 *
 * @param target Pointer to the emulator structure for the BC1.2 emulator instance.
 * @param connected If true, emulate a connection of a portable device partner. If
 * false, emulate a disconnect event.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if the connection/disconnection of PD partner is not supported.
 */
static inline int bc12_emul_set_pd_partner(const struct emul *target, bool connected)
{
	const struct bc12_emul_driver_api *backend_api =
		(const struct bc12_emul_driver_api *)target->backend_api;

	return backend_api->set_pd_partner(target, connected);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_USB_EMUL_BC12_H_ */
