/* entropy_nxp_ele.c - NXP ELE entropy driver */

/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ele_trng

#include <zephyr/logging/log.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include "fsl_sss_mgmt.h"
#include "fsl_sss_sscp.h"
#include "fsl_sscp_mu.h"

#define ENTROPY_ELE_MAX_WAIT     (0xFFFFFFFFu)
#define ENTROPY_ELE_QUALITY_HIGH (0u)

struct entropy_ele_data_str {
	struct k_sem sem_lock;
};

static int ele_get_session_local(sscp_context_t *sscp_ctx, sss_sscp_session_t *session)
{
	int result = -EIO;

	__ASSERT_NO_MSG(session != NULL);

	if (ELEMU_mu_wait_for_ready(ELEMUA, ENTROPY_ELE_MAX_WAIT) != kStatus_Success) {
		goto exit;
	}

	if (sscp_mu_init(sscp_ctx, (ELEMU_Type *)(uintptr_t)ELEMUA) != kStatus_SSCP_Success) {
		goto exit;
	}

	if (sss_sscp_open_session(session, CONFIG_MCUX_SECURE_SUBSYSTEM_SESSION_ID,
				  kType_SSS_Ele200, sscp_ctx) != kStatus_SSS_Success) {
		goto exit;
	}

	result = 0;
exit:
	return result;
}

static int entropy_ele_get_entropy_internal(uint8_t *buf, uint16_t len)
{
	sss_sscp_session_t session = {0};
	sscp_context_t sscp_ctx = {0};
	sss_sscp_rng_t rng_ctx = {0};
	int result = -EIO;

	if (ele_get_session_local(&sscp_ctx, &session) != 0) {
		goto exit;
	}

	if (sss_sscp_rng_context_init(&session, &rng_ctx, ENTROPY_ELE_QUALITY_HIGH) !=
	    kStatus_SSS_Success) {
		goto exit;
	}

	if (sss_sscp_rng_get_random(&rng_ctx, buf, len) != kStatus_SSS_Success) {
		goto exit;
	}

	if (sss_sscp_rng_free(&rng_ctx) != kStatus_SSS_Success) {
		goto exit;
	}

	result = 0;
exit:
	return result;
}

static struct entropy_ele_data_str entropy_ele_data;

static int entropy_ele_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	int result = -EIO;

	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(&entropy_ele_data == dev->data);

	k_sem_take(&entropy_ele_data.sem_lock, K_FOREVER);

	result = entropy_ele_get_entropy_internal(buf, len);

	k_sem_give(&entropy_ele_data.sem_lock);

	return result;
}

static int entropy_ele_init(const struct device *dev)
{
	int result = -EIO;

	__ASSERT_NO_MSG(&entropy_ele_data == dev->data);

	k_sem_init(&entropy_ele_data.sem_lock, 1, 1);

	k_sem_take(&entropy_ele_data.sem_lock, K_FOREVER);

	/* We request 0 data length just to initialize the TRNG */
	result = entropy_ele_get_entropy_internal(NULL, 0);

	k_sem_give(&entropy_ele_data.sem_lock);

	return result;
}

static DEVICE_API(entropy, entropy_ele_api_funcs) = {.get_entropy = entropy_ele_get_entropy};

DEVICE_DT_INST_DEFINE(0, entropy_ele_init, NULL, &entropy_ele_data, NULL, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_ele_api_funcs);
