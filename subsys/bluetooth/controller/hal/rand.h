/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RAND_H_
#define _RAND_H_

void rand_init(u8_t *context, u8_t context_len);
size_t rand_get(size_t octets, u8_t *rand);
void isr_rand(void *param);

#endif /* _RAND_H_ */
