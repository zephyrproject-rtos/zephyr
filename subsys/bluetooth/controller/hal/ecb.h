/*
 * Copyright (c) 2016-2024 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

typedef void (*ecb_fp) (uint32_t status, uint8_t *cipher_be, void *context);

struct ecb {
	uint8_t in_key_be[16];
	uint8_t in_clear_text_be[16];
	uint8_t out_cipher_text_be[16];

	/* if not null reverse copy into in_key_be */
	uint8_t *in_key_le;
	/* if not null reverse copy into in_clear_text_be */
	uint8_t *in_clear_text_le;

	ecb_fp  fp_ecb;
	void    *context;
};

/**
 * @brief Encrypt one block with AES-128, operands most significant octet first.
 *
 * @param key_be         Key, 16 octets.
 * @param clear_text_be  Plaintext, 16 octets.
 * @param cipher_text_be Ciphertext, 16 octets.
 *
 * @retval 0       Success.
 * @retval -EAGAIN The AES block is in use elsewhere; nothing is written. Returned only to an
 *                 LLL or ULL caller: a thread waits for the block instead.
 */
int ecb_encrypt_be(uint8_t const *const key_be, uint8_t const *const clear_text_be,
		   uint8_t * const cipher_text_be);

/**
 * @brief Encrypt one block with AES-128, inputs least significant octet first.
 *
 * @param key_le         Key, 16 octets.
 * @param clear_text_le  Plaintext, 16 octets.
 * @param cipher_text_le Ciphertext, least significant octet first, or NULL.
 * @param cipher_text_be Ciphertext, most significant octet first, or NULL.
 *
 * @retval 0       Success.
 * @retval -EAGAIN The AES block is in use elsewhere; nothing is written. Returned only to an
 *                 LLL or ULL caller: a thread waits for the block instead.
 */
int ecb_encrypt(uint8_t const *const key_le, uint8_t const *const clear_text_le,
		uint8_t * const cipher_text_le, uint8_t * const cipher_text_be);

void ecb_encrypt_nonblocking(struct ecb *e);

int ecb_ut(void);
