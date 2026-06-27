/** @file
 *  @brief Bluetooth Network Encapsulation Protocol (BNEP)
 */

/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_BNEP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_BNEP_H_

#include <zephyr/bluetooth/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

/** BNEP L2CAP PSM */
#define BT_L2CAP_PSM_BNEP BT_UUID_BNEP_VAL

/** BNEP Personal Area Network User (PANU) service */
#define BT_BNEP_SVC_PANU 0x02
/** BNEP Network Access Point (NAP) service */
#define BT_BNEP_SVC_NAP  0x04
/** BNEP Group Networking (GN) service */
#define BT_BNEP_SVC_GN   0x10

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_BNEP_H_ */
