/*
 * Copyright (C) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_JWT_JWT_H_
#define ZEPHYR_SUBSYS_JWT_JWT_H_

#include <zephyr/data/jwt.h>

int jwt_sign_impl(struct jwt_builder *builder, const unsigned char *der_key,
		  size_t der_key_len, unsigned char *sig, size_t sig_size);

#endif /* ZEPHYR_SUBSYS_JWT_JWT_H_ */
