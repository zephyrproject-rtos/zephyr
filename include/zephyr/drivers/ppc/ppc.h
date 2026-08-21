/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PPC_PPC_H_
#define ZEPHYR_INCLUDE_DRIVERS_PPC_PPC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Peripheral Protection Controller (PPC) Interface
 * @defgroup ppc_interface PPC Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Configure all PPC instances to grant NS access to all peripheral ports.
 *
 * Walks every enabled PPC instance in the devicetree and opens the peripheral
 * regions it is configured to expose.  Called once from the Secure image just
 * before transitioning to the Non-Secure world.
 */
void ppc_configure_ns_all(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PPC_PPC_H_ */
