/*
 * Copyright (c) 2019, NXP
 * Copyright (c) 2025, HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>

#include <psa/crypto.h>

int z_impl_sys_csrand_get(void *dst, size_t outlen)
{
	return psa_generate_random((uint8_t *)dst, outlen) == PSA_SUCCESS ? 0 : -EIO;
}
