/** @file
 *  @brief Internal API GATT GAP service validation.
 */

/*
 * Copyright (c) 2025 Koppel Electronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_GATT_GAP_SVC_VALIDATE_H
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_GATT_GAP_SVC_VALIDATE_H


/** @brief Validate GATT database contains exactly one GAP service.
 *
 * This function iterates through the GATT attribute database and verifies
 * that exactly one Generic Access Profile (GAP) service is present. The
 * Bluetooth specification requires that a GATT server expose exactly one
 * GAP service.
 *
 * @return 0 on success (exactly one GAP service found).
 * @return -EINVAL if zero or multiple GAP services are found.
 */
int gatt_gap_svc_validate(void);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_GATT_GAP_SVC_VALIDATE_H */
