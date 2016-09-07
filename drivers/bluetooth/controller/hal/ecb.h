/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ECB_H_
#define _ECB_H_

typedef void (*ecb_fp) (uint32_t status, uint8_t *cipher_be,
			  void *context);

struct ecb {
	uint8_t in_key_be[16];
	uint8_t in_clear_text_be[16];
	uint8_t out_cipher_text_be[16];
	/* if not null reverse copy into in_key_be */
	uint8_t *in_key_le;
	/* if not null reverse copy into in_clear_text_be */
	uint8_t *in_clear_text_le;
	ecb_fp fp_ecb;
	void *context;
};

void ecb_encrypt(uint8_t const *const key_le,
		 uint8_t const *const clear_text_le,
		 uint8_t * const cipher_text_le,
		 uint8_t * const cipher_text_be);
uint32_t ecb_encrypt_nonblocking(struct ecb *ecb);
void ecb_isr(void);

uint32_t ecb_ut(void);

#endif /* _ECB_H_ */
