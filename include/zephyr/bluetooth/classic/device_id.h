/** @file
 *  @brief Device Identification Protocol handling.
 */

/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_DEVICE_ID_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_DEVICE_ID_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Device Identification Register SDP Service.
 *
 *  Register device identification sdp service.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_did_register(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_DEVICE_ID_H_ */
