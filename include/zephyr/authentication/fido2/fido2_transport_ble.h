/*
 * Copyright (c) 2026 Jan Philipp Schmale <jan-philipp.schmale@teratron.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FIDO2 Bluetooth Low Energy transport definitions.
 * @ingroup fido2
 */

#ifndef ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_TRANSPORT_BLE_H_
#define ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_TRANSPORT_BLE_H_

#include <zephyr/bluetooth/uuid.h>

/** FIDO BLE service UUID value. */
#define FIDO2_BLE_SERVICE_UUID_VAL 0xFFFDU

/** FIDO BLE service UUID. */
#define BT_UUID_FIDO2_SERVICE BT_UUID_DECLARE_16(FIDO2_BLE_SERVICE_UUID_VAL)

/** FIDO BLE Control Point characteristic UUID value. */
#define FIDO2_BLE_CONTROL_POINT_UUID_VAL                                                           \
	BT_UUID_128_ENCODE(0xF1D0FFF1, 0xDEAA, 0xECEE, 0xB42F, 0xC9BA7ED623BB)

/** FIDO BLE Status characteristic UUID value. */
#define FIDO2_BLE_STATUS_UUID_VAL                                                                  \
	BT_UUID_128_ENCODE(0xF1D0FFF2, 0xDEAA, 0xECEE, 0xB42F, 0xC9BA7ED623BB)

/** FIDO BLE Control Point Length characteristic UUID value. */
#define FIDO2_BLE_CONTROL_POINT_LENGTH_UUID_VAL                                                    \
	BT_UUID_128_ENCODE(0xF1D0FFF3, 0xDEAA, 0xECEE, 0xB42F, 0xC9BA7ED623BB)

/** FIDO BLE Service Revision Bitfield characteristic UUID value. */
#define FIDO2_BLE_REVISION_BITFIELD_UUID_VAL                                                       \
	BT_UUID_128_ENCODE(0xF1D0FFF4, 0xDEAA, 0xECEE, 0xB42F, 0xC9BA7ED623BB)

/** FIDO BLE Control Point characteristic UUID. */
#define BT_UUID_FIDO2_BLE_CONTROL_POINT BT_UUID_DECLARE_128(FIDO2_BLE_CONTROL_POINT_UUID_VAL)

/** FIDO BLE Status characteristic UUID. */
#define BT_UUID_FIDO2_BLE_STATUS BT_UUID_DECLARE_128(FIDO2_BLE_STATUS_UUID_VAL)

/** FIDO BLE Control Point Length characteristic UUID. */
#define BT_UUID_FIDO2_BLE_CONTROL_POINT_LENGTH                                                     \
	BT_UUID_DECLARE_128(FIDO2_BLE_CONTROL_POINT_LENGTH_UUID_VAL)

/** FIDO BLE Service Revision Bitfield characteristic UUID. */
#define BT_UUID_FIDO2_BLE_REVISION_BITFIELD                                                        \
	BT_UUID_DECLARE_128(FIDO2_BLE_REVISION_BITFIELD_UUID_VAL)

/** Supported FIDO BLE service revision bitfield. */
#define FIDO2_BLE_REVISION 0x20U

#endif /* ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_TRANSPORT_BLE_H_ */
