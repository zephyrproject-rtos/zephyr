/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MPC_MPC_H_
#define ZEPHYR_INCLUDE_DRIVERS_MPC_MPC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory Protection Controller (MPC) Interface
 * @defgroup mpc_interface MPC Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Configure all MPC instances to grant NS access to their configured ranges.
 *
 * Walks every enabled MPC instance in the devicetree and applies the regions
 * described by its child nodes.  Called once from the Secure image before
 * transitioning to the Non-Secure world.
 */
void mpc_configure_all(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MPC_MPC_H_ */
