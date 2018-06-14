/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

typedef void (*ecb_fp) (u32_t status, u8_t *cipher_be, void *context);

struct ecb {
	u8_t   in_key_be[16];
	u8_t   in_clear_text_be[16];
	u8_t   out_cipher_text_be[16];
	/* if not null reverse copy into in_key_be */
	u8_t   *in_key_le;
	/* if not null reverse copy into in_clear_text_be */
	u8_t   *in_clear_text_le;
	ecb_fp fp_ecb;
	void   *context;
};

void ecb_encrypt_be(u8_t const *const key_be, u8_t const *const clear_text_be,
		    u8_t * const cipher_text_be);
void ecb_encrypt(u8_t const *const key_le, u8_t const *const clear_text_le,
		 u8_t * const cipher_text_le, u8_t * const cipher_text_be);
u32_t ecb_encrypt_nonblocking(struct ecb *ecb);
void isr_ecb(void *param);

u32_t ecb_ut(void);
