/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_css_v2

#include "mcuxClCss_Rng.h"

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/init.h>
#include <zephyr/random/rand32.h>

static int entropy_mcux_css_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);
	uint8_t status = 0;

	MCUX_CSSL_FP_FUNCTION_CALL_BEGIN(result, token, mcuxClCss_Prng_GetRandom(buffer, length));
	if ((MCUX_CSSL_FP_FUNCTION_CALLED(mcuxClCss_Prng_GetRandom) != token) ||
	    (result != MCUXCLCSS_STATUS_OK)) {
		status = -EAGAIN;
	}
	MCUX_CSSL_FP_FUNCTION_CALL_END();

	__ASSERT_NO_MSG(!status);

	return status;
}

static const struct entropy_driver_api entropy_mcux_css_api_funcs = {
	.get_entropy = entropy_mcux_css_get_entropy
};

static int entropy_mcux_css_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	uint8_t status = 0;

	MCUX_CSSL_FP_FUNCTION_CALL_BEGIN(result, token, mcuxClCss_Enable_Async());
	if ((MCUX_CSSL_FP_FUNCTION_CALLED(mcuxClCss_Enable_Async) != token) ||
	    (result != MCUXCLCSS_STATUS_OK_WAIT)) {
		status = -ENODEV;
	}
	MCUX_CSSL_FP_FUNCTION_CALL_END();

	__ASSERT_NO_MSG(!status);

	return status;
}

DEVICE_DT_INST_DEFINE(0, entropy_mcux_css_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_mcux_css_api_funcs);
