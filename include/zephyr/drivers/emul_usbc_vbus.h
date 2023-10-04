/*
 * Copyright (c) 2023 Jonas Otto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
* @file
* @brief Backend APIs for the usbc vbus emulators.
*/

#ifndef ZEPHYR_INCLUDE_DRIVERS_EMUL_USBC_VBUS_H_
#define ZEPHYR_INCLUDE_DRIVERS_EMUL_USBC_VBUS_H_

#include <stdint.h>
#include <zephyr/drivers/emul.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief USBC VBUS backend emulator APIs
* @defgroup usbc_vbus_emulator_backend USBC VBUS backed emulator APIs
* @ingroup io_interfaces
* @{
*/

/**
* @cond INTERNAL_HIDDEN
*
* These are for internal use only, so skip these in public documentation.
*/
__subsystem struct usbc_vbus_emul_driver_api {
       int (*set_vbus_voltage)(const struct emul *emul, int mV);
};
/**
* @endcond
*/

/**
* @brief
*
*
* @param target Pointer to the emulator structure for the usbc vbus emulator instance.
* @param mV VBUs voltage in millivolts.
*
* @retval 0 If successful.
*/
static inline int emul_usbc_vbus_set_vbus_voltage(const struct emul *target, int mV)
{
       const struct usbc_vbus_emul_driver_api *backend_api =
	       (const struct usbc_vbus_emul_driver_api *)target->backend_api;

       return backend_api->set_vbus_voltage(target, mV);
}

#ifdef __cplusplus
}
#endif

/**
* @}
*/

#endif /* ZEPHYR_INCLUDE_DRIVERS_EMUL_USBC_VBUS_H_*/
