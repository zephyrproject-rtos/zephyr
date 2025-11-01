/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

const struct device *get_pwm_device(void);
const struct device *get_dma_device(void);

static void *pwm_dma_setup(void)
{
	const struct device *pwm_dev = get_pwm_device();
	const struct device *dma_dev = get_dma_device();

	zassert_true(device_is_ready(pwm_dev), "PWM device is not ready");
	k_object_access_grant(pwm_dev, k_current_get());

	zassert_true(device_is_ready(dma_dev), "DMA device is not ready");
	k_object_access_grant(dma_dev, k_current_get());

	return NULL;
}

ZTEST_SUITE(pwm_dma, NULL, pwm_dma_setup, NULL, NULL, NULL);
