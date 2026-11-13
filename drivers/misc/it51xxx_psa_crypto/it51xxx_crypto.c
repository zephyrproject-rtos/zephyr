/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "it51xxx_crypto.h"

#if CONFIG_IT51XXX_PSA_CRYPTO_SHA
#include "it51xxx_crypto_sha.h"
#endif /* CONFIG_IT51XXX_PSA_CRYPTO_SHA */

#if CONFIG_IT51XXX_PSA_CRYPTO_RSA
#include "it51xxx_crypto_rsa.h"
#endif /* CONFIG_IT51XXX_PSA_CRYPTO_RSA */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT ite_it51xxx_crypto

Z_GENERIC_SECTION(.__hwcrypto_dlm_block) union hwcrypto_dlm_block hwcrypto_dlm;

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "it51xxx: supported one psa-crypto controller only");

/* The legacy sha driver and psa-crypto driver can not be enable simultaneously
 * because they share the same dlm block (hwcrypto_dlm_block).
 */
#define LEGACY_SHA_DT_DRV_COMPAT ite_it51xxx_sha
BUILD_ASSERT(!DT_HAS_COMPAT_STATUS_OKAY(LEGACY_SHA_DT_DRV_COMPAT),
	     "legacy sha and psa-crypto devices cannot be enabled simultaneously");

struct k_mutex it51xxx_psa_crypto_mutex;

mm_reg_t it51xxx_crypto_get_base(void)
{
	return (mm_reg_t)DT_REG_ADDR(DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT));
}

int it51xxx_crypto_lock(k_timeout_t timeout)
{
	return k_mutex_lock(&it51xxx_psa_crypto_mutex, timeout);
}

void it51xxx_crypto_unlock(void)
{
	k_mutex_unlock(&it51xxx_psa_crypto_mutex);
}

psa_status_t it51xxx_psa_crypto_init(void)
{
	psa_status_t status = PSA_SUCCESS;

	if (k_mutex_init(&it51xxx_psa_crypto_mutex)) {
		return PSA_ERROR_GENERIC_ERROR;
	}

#if CONFIG_IT51XXX_PSA_CRYPTO_SHA
	status = it51xxx_crypto_sha_init();
	if (status) {
		return status;
	}
#endif /* CONFIG_IT51XXX_PSA_CRYPTO_SHA */

#if CONFIG_IT51XXX_PSA_CRYPTO_RSA
	status = it51xxx_crypto_rsa_init();
	if (status) {
		return status;
	}
#endif /* CONFIG_IT51XXX_PSA_CRYPTO_RSA */

	return status;
}

psa_status_t it51xxx_psa_crypto_free(void)
{
	return PSA_SUCCESS;
}
