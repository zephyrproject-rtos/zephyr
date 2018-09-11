/** @file
 *  @brief GATT Battery Service
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Notify battery charge level to peer.
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_gatt_bas_notify(void);

#ifdef __cplusplus
}
#endif
