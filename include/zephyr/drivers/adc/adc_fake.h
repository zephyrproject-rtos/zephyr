/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_FAKE_H_

#include <zephyr/drivers/adc.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int,
			adc_fake_channel_setup,
			const struct device *,
			const struct adc_channel_cfg *);

DECLARE_FAKE_VALUE_FUNC(int,
			adc_fake_read,
			const struct device *,
			const struct adc_sequence *);

DECLARE_FAKE_VALUE_FUNC(int,
			adc_fake_read_async,
			const struct device *,
			const struct adc_sequence *,
			struct k_poll_signal *);

DECLARE_FAKE_VOID_FUNC(adc_fake_submit,
		       const struct device *,
		       struct rtio_iodev_sqe *);

DECLARE_FAKE_VALUE_FUNC(int,
			adc_fake_get_decoder,
			const struct device *,
			const struct adc_decoder_api **);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_FAKE_H_ */
