/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_CORE_H_
#define ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_CORE_H_

#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/random/random.h>
#include <zephyr/net/net_log.h>

#include <psa/crypto.h>
#include <mbedtls/platform_util.h>

#include "ssh_defs.h"

#if defined(BUILD_VERSION) && !IS_EMPTY(BUILD_VERSION)
#define ZEPHYR_SSH_VERSION STRINGIFY(BUILD_VERSION)
#else
#define ZEPHYR_SSH_VERSION KERNEL_VERSION_STRING
#endif

enum ssh_kex_alg {
	SSH_KEX_ALG_CURVE25519_SHA256,
};

enum ssh_host_key_alg {
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_256
	SSH_HOST_KEY_ALG_RSA_SHA2_256,
#endif
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_512
	SSH_HOST_KEY_ALG_RSA_SHA2_512,
#endif
};

enum ssh_encryption_alg {
	SSH_ENCRYPTION_ALG_AES128_CTR,
};

enum ssh_mac_alg {
	SSH_MAC_ALG_HMAC_SHA2_256,
};

enum ssh_compression_alg {
	SSH_COMPRESSION_ALG_NONE,
};

enum ssh_server_sig_algs {
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_512
	SSH_SERVER_SIG_ALG_RSA_SHA2_512,
#endif
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_256
	SSH_SERVER_SIG_ALG_RSA_SHA2_256,
#endif
};

static inline int ssh_mbedtls_rand(void *rng_state, unsigned char *output, size_t len)
{
	ARG_UNUSED(rng_state);
	return sys_csrand_get(output, len);
}

#define ssh_zeroize(buf, len) mbedtls_platform_zeroize((buf), (len))

#endif
