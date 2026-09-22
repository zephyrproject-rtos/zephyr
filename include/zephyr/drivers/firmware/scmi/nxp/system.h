/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_SCMI_NXP_SYSTEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_SCMI_NXP_SYSTEM_H_

/**
 * @file
 * @ingroup scmi_nxp_system
 * @brief NXP SCMI System Protocol Extensions
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NXP vendor-specific extensions to the SCMI system power protocol
 * @defgroup scmi_nxp_system NXP System Power Protocol
 * @ingroup scmi_protocols
 * @{
 */

/**
 * @name NXP Vendor System Power States
 * @{
 */
/** Wake the system */
#define SCMI_NXP_SYSTEM_POWER_STATE_WAKE           0x80000000U
/** Full system shutdown */
#define SCMI_NXP_SYSTEM_POWER_STATE_FULL_SHUTDOWN  0x80000001U
/** Full system reset */
#define SCMI_NXP_SYSTEM_POWER_STATE_FULL_RESET     0x80000002U
/** Full system suspend */
#define SCMI_NXP_SYSTEM_POWER_STATE_FULL_SUSPEND   0x80000003U
/** Full system wake */
#define SCMI_NXP_SYSTEM_POWER_STATE_FULL_WAKE      0x80000004U
/** Group shutdown */
#define SCMI_NXP_SYSTEM_POWER_STATE_GRP_SHUTDOWN   0x80000005U
/** Group reset */
#define SCMI_NXP_SYSTEM_POWER_STATE_GRP_RESET      0x80000006U
/** Subsystem reset */
#define SCMI_NXP_SYSTEM_POWER_STATE_SUBSYS_RESET   0x80000007U
/** Set system mode */
#define SCMI_NXP_SYSTEM_POWER_STATE_MODE           0xC0000000U
/** @} */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_SCMI_NXP_SYSTEM_H_ */
