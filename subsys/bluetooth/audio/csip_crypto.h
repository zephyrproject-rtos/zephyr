/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/types.h>

#define BT_CSIP_CRYPTO_KEY_SIZE   16
#define BT_CSIP_CRYPTO_SALT_SIZE  16
#define BT_CSIP_CRYPTO_PRAND_SIZE 3
#define BT_CSIP_CRYPTO_HASH_SIZE  3

/**
 * @brief Private Set Unique identifier hash function sih.
 *
 * The RSI hash function sih is used to generate a hash value that is
 * used in RSIs - Used by the Coordinated Set Identification service and
 * profile.
 *
 * @param sirk  16-byte SIRK in little-endian byte order.
 * @param r     3-byte random value in little-endian byte order.
 * @param out   3-byte output buffer in little-endian byte order.
 *
 * @return int 0 on success, any other value indicates a failure.
 */
int bt_csip_sih(const uint8_t sirk[BT_CSIP_SIRK_SIZE], uint8_t r[BT_CSIP_CRYPTO_PRAND_SIZE],
		uint8_t out[BT_CSIP_CRYPTO_HASH_SIZE]);

/**
 * @brief SIRK encryption function sef
 *
 * The SIRK encryption function sef is used by the server to encrypt the SIRK
 * with a key K. The value of K depends on the transport on which the pairing
 * between the client and the server was performed.
 *
 * If the pairing was performed on Basic Rate/Enhanced Data Rate (BR/EDR), K is equal to the
 * Link Key shared by the server and the client.
 *    K = Link Key.
 *
 * If the pairing was performed on Bluetooth Low Energy (LE), K is equal to the LTK shared by the
 * server and client. That is,
 *    K = LTK
 *
 * @param k         16-byte key in little-endian byte order.
 * @param sirk      16-byte unencrypted SIRK key in little-endian byte order.
 * @param out_sirk  16-byte encrypted SIRK key in little-endian byte order.
 *
 * @return int 0 on success, any other value indicates a failure.
 */
int bt_csip_sef(const uint8_t k[BT_CSIP_CRYPTO_KEY_SIZE], const uint8_t sirk[BT_CSIP_SIRK_SIZE],
		uint8_t out_sirk[BT_CSIP_SIRK_SIZE]);

/**
 * @brief SIRK decryption function sdf
 *
 * The SIRK decryption function sdf is used by the client to decrypt the SIRK
 * with a key K. The value of K depends on the transport on which the pairing
 * between the client and the server was performed.
 *
 * If the pairing was performed on Basic Rate/Enhanced Data Rate (BR/EDR), K is equal to the
 * Link Key shared by the server and the client.
 *    K = Link Key.
 *
 * If the pairing was performed on Bluetooth Low Energy (LE), K is equal to the LTK shared by the
 * server and client. That is,
 *    K = LTK
 *
 * @param k         16-byte key in little-endian byte order.
 * @param sirk      16-byte encrypted SIRK in little-endian byte order.
 * @param out_sirk  16-byte decrypted SIRK in little-endian byte order.
 *
 * @return int 0 on success, any other value indicates a failure.
 */
int bt_csip_sdf(const uint8_t k[BT_CSIP_CRYPTO_KEY_SIZE], const uint8_t enc_sirk[BT_CSIP_SIRK_SIZE],
		uint8_t out_sirk[BT_CSIP_SIRK_SIZE]);
