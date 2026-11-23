/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AWS_CERTS_H_
#define _AWS_CERTS_H_

#include <stdint.h>

extern const uint8_t ca_cert[];
extern const uint32_t ca_cert_len;

extern const uint8_t private_key[];
extern const uint32_t private_key_len;

extern const uint8_t public_cert[];
extern const uint32_t public_cert_len;

#endif /* _AWS_CERTS_H_ */
