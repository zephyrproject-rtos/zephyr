/**
 * @file bluetooth/smp.h
 * Security Manager Protocol implementation header
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Security management
 * @defgroup bt_smp Security management
 * @ingroup bluetooth
 * @{
 */

#include <bluetooth/bluetooth.h>

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SMP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SMP_H_

#define BT_SMP_IO_DISPLAY_ONLY			0x00
#define BT_SMP_IO_DISPLAY_YESNO			0x01
#define BT_SMP_IO_KEYBOARD_ONLY			0x02
#define BT_SMP_IO_NO_INPUT_OUTPUT		0x03
#define BT_SMP_IO_KEYBOARD_DISPLAY		0x04

#define BT_SMP_OOB_DATA_MASK			0x01
#define BT_SMP_OOB_NOT_PRESENT			0x00
#define BT_SMP_OOB_PRESENT			0x01

#define BT_SMP_MIN_ENC_KEY_SIZE			7
#define BT_SMP_MAX_ENC_KEY_SIZE			16

#define BT_SMP_DIST_ENC_KEY			0x01
#define BT_SMP_DIST_ID_KEY			0x02
#define BT_SMP_DIST_SIGN			0x04
#define BT_SMP_DIST_LINK_KEY			0x08

#define BT_SMP_DIST_MASK			0x0f

#define BT_SMP_AUTH_NONE			0x00
#define BT_SMP_AUTH_BONDING			0x01
#define BT_SMP_AUTH_MITM			0x04
#define BT_SMP_AUTH_SC				0x08
#define BT_SMP_AUTH_KEYPRESS			0x10
#define BT_SMP_AUTH_CT2				0x20

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pairing request parameters, Core Spec 5.1 Vol 3 Part H 3.5.1
 *
 * @param io_capability IO Capability, octet 1, valid range 0x00-0x04
 * @param oob_flag OOB data flag, octet 2, valid range 0x00-0x01
 * @param auth_req AuthReq, octet 3, bits:
 *   MSB  LSB
 *   ------0x  Bonding_Flags, range 0x00-0x01
 *   -----x--  MITM
 *   ----x---  SC
 *   ---x----  Keypress
 *   --x-----  CT2
 *   xx------  Reserved for future use
 * @param max_key_size Max Encryption Key Size, octet 4, valid range 0x07-0x10
 * @param init_key_dist Initiator Key Distribution/Generation, octet 5, bits:
 *   MSB  LSB
 *   -------x  EncKey
 *   ------x-  IdKey
 *   -----x--  SignKey
 *   ----x---  LinkKey
 *   xxxx----  Reserved for future use
 * @param resp_key_dist Responder Key Dist/Generation, octet 6, see above.
 *
 */
struct bt_smp_pairing {
	u8_t  io_capability;
	u8_t  oob_flag;
	u8_t  auth_req;
	u8_t  max_key_size;
	u8_t  init_key_dist;
	u8_t  resp_key_dist;
} __packed;

/**
 * @brief Return pairing req/rsp parameters for peer.
 *
 * @details Return pairing req parameters, except Code, for peer associated
 * with conn object. Can only be called during pairing while waiting for user
 * input.
 *
 * @param conn Connection object.
 * @param info Pairing info object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_smp_get_pairing_peer_info(const struct bt_conn *conn,
				 struct bt_smp_pairing *info);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SMP_H_ */
