/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include "disco_box.h"

#include <errno.h>
#include <string.h>

/* The DISCO box requires raw Poly1305, which has no PSA API. */
#ifndef MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS
#define MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS
#endif
#include <mbedtls/constant_time.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/private/poly1305.h>
#include <zephyr/sys/byteorder.h>

/*
 * Salsa20 and HSalsa20 follow Daniel J. Bernstein's "Salsa20 specification"
 * and "Extending the Salsa20 nonce", section 2. The NaCl construction
 * reserves the first 32 keystream bytes as the one-time Poly1305 key and
 * authenticates the ciphertext.
 * https://cr.yp.to/snuffle/spec.pdf
 * https://cr.yp.to/snuffle/xsalsa-20081128.pdf
 * https://fossil.sowaswie.de/nacl/wiki?name=hints
 */

#define SALSA20_STATE_WORDS 16
#define SALSA20_BLOCK_SIZE  64
#define HSALSA20_INPUT_SIZE 16
#define POLY1305_KEY_SIZE   32

struct xsalsa20_stream {
	uint8_t subkey[DISCO_BOX_KEY_SIZE];
	uint8_t first_block[SALSA20_BLOCK_SIZE];
};

static uint32_t rotate_left(uint32_t value, unsigned int shift)
{
	return (value << shift) | (value >> (32 - shift));
}

static void quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
	*b ^= rotate_left(*a + *d, 7);
	*c ^= rotate_left(*b + *a, 9);
	*d ^= rotate_left(*c + *b, 13);
	*a ^= rotate_left(*d + *c, 18);
}

static void salsa20_rounds(uint32_t state[SALSA20_STATE_WORDS])
{
	for (unsigned int i = 0; i < 10; i++) {
		quarter_round(&state[0], &state[4], &state[8], &state[12]);
		quarter_round(&state[5], &state[9], &state[13], &state[1]);
		quarter_round(&state[10], &state[14], &state[2], &state[6]);
		quarter_round(&state[15], &state[3], &state[7], &state[11]);

		quarter_round(&state[0], &state[1], &state[2], &state[3]);
		quarter_round(&state[5], &state[6], &state[7], &state[4]);
		quarter_round(&state[10], &state[11], &state[8], &state[9]);
		quarter_round(&state[15], &state[12], &state[13], &state[14]);
	}
}

static void salsa20_init(uint32_t state[SALSA20_STATE_WORDS],
			 const uint8_t key[DISCO_BOX_KEY_SIZE])
{
	static const uint8_t sigma[] = "expand 32-byte k";

	for (unsigned int i = 0; i < 4; i++) {
		state[5 * i] = sys_get_le32(sigma + 4 * i);
		state[1 + i] = sys_get_le32(key + 4 * i);
		state[11 + i] = sys_get_le32(key + 16 + 4 * i);
	}
}

static void hsalsa20(uint8_t output[DISCO_BOX_KEY_SIZE], const uint8_t key[DISCO_BOX_KEY_SIZE],
		     const uint8_t input[HSALSA20_INPUT_SIZE])
{
	static const uint8_t output_words[] = {0, 5, 10, 15, 6, 7, 8, 9};
	uint32_t state[SALSA20_STATE_WORDS];

	salsa20_init(state, key);
	for (unsigned int i = 0; i < 4; i++) {
		state[6 + i] = sys_get_le32(input + 4 * i);
	}
	salsa20_rounds(state);
	for (size_t i = 0; i < sizeof(output_words); i++) {
		sys_put_le32(state[output_words[i]], output + 4 * i);
	}
	mbedtls_platform_zeroize(state, sizeof(state));
}

static void salsa20_block(uint8_t block[SALSA20_BLOCK_SIZE], const uint8_t key[DISCO_BOX_KEY_SIZE],
			  const uint8_t nonce[8], uint64_t counter)
{
	uint32_t initial[SALSA20_STATE_WORDS];
	uint32_t state[SALSA20_STATE_WORDS];

	salsa20_init(initial, key);
	initial[6] = sys_get_le32(nonce);
	initial[7] = sys_get_le32(nonce + 4);
	initial[8] = (uint32_t)counter;
	initial[9] = (uint32_t)(counter >> 32);
	memcpy(state, initial, sizeof(state));
	salsa20_rounds(state);
	for (unsigned int i = 0; i < SALSA20_STATE_WORDS; i++) {
		sys_put_le32(state[i] + initial[i], block + 4 * i);
	}
	mbedtls_platform_zeroize(state, sizeof(state));
	mbedtls_platform_zeroize(initial, sizeof(initial));
}

static void xsalsa20_start(struct xsalsa20_stream *stream, const uint8_t key[DISCO_BOX_KEY_SIZE],
			   const uint8_t nonce[DISCO_BOX_NONCE_SIZE])
{
	hsalsa20(stream->subkey, key, nonce);
	salsa20_block(stream->first_block, stream->subkey, nonce + HSALSA20_INPUT_SIZE, 0);
}

static void xor_block(uint8_t *output, const uint8_t *input, const uint8_t *block, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		output[i] = input[i] ^ block[i];
	}
}

static void xsalsa20_xor(const struct xsalsa20_stream *stream,
			const uint8_t nonce[DISCO_BOX_NONCE_SIZE], const uint8_t *input,
			uint8_t *output, size_t len)
{
	const size_t first_available = SALSA20_BLOCK_SIZE - POLY1305_KEY_SIZE;
	size_t offset = len < first_available ? len : first_available;
	uint8_t block[SALSA20_BLOCK_SIZE];

	xor_block(output, input, stream->first_block + POLY1305_KEY_SIZE, offset);

	for (uint64_t counter = 1; offset < len; counter++) {
		size_t remaining = len - offset;
		size_t count = remaining < sizeof(block) ? remaining : sizeof(block);

		salsa20_block(block, stream->subkey, nonce + HSALSA20_INPUT_SIZE, counter);
		xor_block(output + offset, input + offset, block, count);
		offset += count;
	}
	mbedtls_platform_zeroize(block, sizeof(block));
}

void disco_box_precompute(uint8_t key[DISCO_BOX_KEY_SIZE],
			  const uint8_t shared[DISCO_BOX_KEY_SIZE])
{
	static const uint8_t zero[HSALSA20_INPUT_SIZE];

	hsalsa20(key, shared, zero);
}

int disco_box_open(const uint8_t key[DISCO_BOX_KEY_SIZE],
		   const uint8_t nonce[DISCO_BOX_NONCE_SIZE], const uint8_t *box, size_t box_len,
		   uint8_t *plaintext)
{
	struct xsalsa20_stream stream;
	uint8_t tag[DISCO_BOX_TAG_SIZE];
	int ret;

	if (box_len < DISCO_BOX_TAG_SIZE) {
		return -EBADMSG;
	}
	const uint8_t *ciphertext = box + DISCO_BOX_TAG_SIZE;
	size_t len = box_len - DISCO_BOX_TAG_SIZE;

	xsalsa20_start(&stream, key, nonce);
	if (mbedtls_poly1305_mac(stream.first_block, ciphertext, len, tag) != 0) {
		ret = -EIO;
	} else if (mbedtls_ct_memcmp(tag, box, sizeof(tag)) != 0) {
		ret = -EBADMSG;
	} else {
		xsalsa20_xor(&stream, nonce, ciphertext, plaintext, len);
		ret = 0;
	}
	mbedtls_platform_zeroize(tag, sizeof(tag));
	mbedtls_platform_zeroize(&stream, sizeof(stream));
	return ret;
}

int disco_box_seal(const uint8_t key[DISCO_BOX_KEY_SIZE],
		   const uint8_t nonce[DISCO_BOX_NONCE_SIZE], const uint8_t *plaintext, size_t len,
		   uint8_t *box)
{
	struct xsalsa20_stream stream;
	int ret;

	if (len > SIZE_MAX - DISCO_BOX_TAG_SIZE) {
		return -EOVERFLOW;
	}
	uint8_t *ciphertext = box + DISCO_BOX_TAG_SIZE;

	xsalsa20_start(&stream, key, nonce);
	xsalsa20_xor(&stream, nonce, plaintext, ciphertext, len);
	ret = mbedtls_poly1305_mac(stream.first_block, ciphertext, len, box);
	mbedtls_platform_zeroize(&stream, sizeof(stream));
	if (ret != 0) {
		mbedtls_platform_zeroize(box, DISCO_BOX_TAG_SIZE + len);
		return -EIO;
	}
	return 0;
}
