/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LORAWAN_SE_H
#define LORAWAN_SE_H

#include <zephyr.h>
#include <secure-element-nvm.h>

struct lorawan_se {
	/**
	 * Initialization of Secure Element driver
	 *
	 * @param nvm	Pointer to the non-volatile memory data
	 *		structure.
	 * @return	Status of the operation
	 */
	int (*init)(SecureElementNvmData_t *data);

	/**
	 * Updates the data stored by the Secure Element driver
	 *
	 * @param nvm	Pointer to the non-volatile memory data
	 *		structure.
	 * @return	Status of the operation
	 */
	int (*update)(SecureElementNvmData_t *data);

	/**
	 * Sets a key
	 *
	 * @param keyID		Key identifier
	 * @param key		Key value
	 * @return		Status of the operation
	 */
	int (*set_key)(KeyIdentifier_t keyID, uint8_t *key);

	/**
	 * Computes a CMAC of a message using provided initial Bx block
	 *
	 * @param micBxBuffer	Buffer containing the initial Bx block
	 * @param buffer	Data buffer
	 * @param size		Data buffer size
	 * @param keyID		Key identifier to determine the AES
	 *			key to be used
	 * @param cmac		Computed cmac
	 * @return		Status of the operation
	 */
	int (*compute_cmac)(uint8_t *micBxBuffer,
			    uint8_t *buffer, uint16_t size,
			    KeyIdentifier_t keyID, uint32_t *cmac);

	/**
	 * Verifies a CMAC (computes and compare with expected cmac)
	 *
	 * @param buffer	Data buffer
	 * @param size		Data buffer size
	 * @param expectedCmac	Expected cmac
	 * @param keyID		Key identifier to determine the AES
	 *			key to be used
	 * @return		Status of the operation
	 */
	int (*verify_cmac)(uint8_t *buffer, uint16_t size,
			   uint32_t expectedCmac, KeyIdentifier_t keyID);

	/**
	 * Encrypt a buffer
	 *
	 * @param buffer	Data buffer
	 * @param size		Data buffer size
	 * @param keyID		Key identifier to determine the AES
	 *			key to be used
	 * @param encBuffer	Encrypted buffer
	 * @return		Status of the operation
	 */
	int (*encrypt)(uint8_t *buffer, uint16_t size,
		       KeyIdentifier_t keyID, uint8_t *encBuffer);

	/**
	 * Derives and store a key
	 *
	 * @param input		Input data from which the key
	 *			is derived ( 16 byte )
	 * @param rootKeyID	Key identifier of the root key to use
	 *			to perform the derivation
	 * @param targetKeyID	Key identifier of the key which will be derived
	 * @return		Status of the operation
	 */
	int (*derive)(uint8_t *input,
		      KeyIdentifier_t rootKeyID,
		      KeyIdentifier_t targetKeyID);

	/**
	 * Process JoinAccept message.
	 *
	 * \param encJoinAccept		Received encrypted JoinAccept message
	 * \param encJoinAcceptSize	Received encrypted JoinAccept
	 *				message Size
	 * \param decJoinAccept		Decrypted and validated JoinAccept
	 *				message
	 * \param versionMinor		Detected LoRaWAN specification version
	 *				minor field.
	 *					0 -> LoRaWAN 1.0.x
	 *					1 -> LoRaWAN 1.1.x
	 * @return			Status of the operation
	 */
	int (*process_join_accept)(JoinReqIdentifier_t joinReqType,
				   uint8_t *joinEui,
				   uint16_t devNonce,
				   uint8_t *encJoinAccept,
				   uint8_t encJoinAcceptSize,
				   uint8_t *decJoinAccept,
				   uint8_t *versionMinor);

	/**
	 * Sets the DevEUI
	 *
	 * @param devEui	Pointer to the 8-byte devEUI
	 * @return		Status of the operation
	 */
	int (*set_deveui)(uint8_t *devEui);

	/**
	 * Gets the DevEUI
	 *
	 * @return		Pointer to the 8-byte devEUI
	 */
	uint8_t *(*get_deveui)(void);

	/**
	 * Sets the JoinEUI
	 *
	 * @param joinEui	Pointer to the 8-byte joinEui
	 * @return		Status of the operation
	 */
	int (*set_joineui)(uint8_t *joinEui);

	/**
	 * Gets the DevEUI
	 *
	 * @return		Pointer to the 8-byte joinEui
	 */
	uint8_t *(*get_joineui)(void);

	/**
	 * Sets the pin
	 *
	 * @param pin		Pointer to the 4-byte pin
	 * @return		Status of the operation
	 */
	int (*set_pin)(uint8_t *pin);

	/**
	 * Gets the Pin
	 *
	 * @return		Pointer to the 4-byte pin
	 */
	uint8_t *(*get_pin)(void);
};

/**
 * Registers an active Secure Element that will be used by the LoRaWAN stack
 *
 * The pointer must remain valid all the time
 *
 * @param se	The secure element
 */
void lorawan_se_register(const struct lorawan_se *se);

/**
 * Updates the data stored by the Secure Element driver
 *
 * @param data	Pointer to the non-volatile memory data
 *		structure.
 * @return	Status of the operation
 */
int lorawan_se_update(SecureElementNvmData_t *data);

#if defined(CONFIG_LORAWAN_SE_SETTINGS_USE_ENCRYPTED)
int lorawan_se_set_enc_key(const uint8_t *enc_key, size_t size);
#endif

#endif /* LORAWAN_SE_H */
