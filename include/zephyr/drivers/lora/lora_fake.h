/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake LoRa driver API functions.
 * @ingroup lora_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LORA_LORA_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_LORA_LORA_FAKE_H_

#include <zephyr/drivers/lora.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake LoRa driver
 * @defgroup lora_fake Fake LoRa
 * @ingroup io_emulators
 * @ingroup lora_interface
 *
 * @driver_fake{lora_interface,CONFIG_LORA_FAKE,zephyr\,fake-lora}
 *
 * `fake_lora_recv()` is given a default `custom_fake` that initializes its
 * output parameters with zero values, re-installed on every reset. When
 * `CONFIG_LORA_FAKE_OPTIONAL_API` is enabled, `fake_lora_rssi()` and
 * `fake_lora_recv_duty_cycle()` receive equivalent delegates. Install your own
 * `custom_fake` in the test case to change what they report.
 *
 * @{
 */

/** @fake_of{lora_driver_api::config} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_config, const struct device *,
			const struct lora_modem_config *);

/** @fake_of{lora_driver_api::airtime} */
DECLARE_FAKE_VALUE_FUNC(uint32_t, fake_lora_airtime, const struct device *, uint32_t);

/** @fake_of{lora_driver_api::send} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_send, const struct device *, uint8_t *, uint32_t);

/** @fake_of{lora_driver_api::send_async} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_send_async, const struct device *, uint8_t *, uint32_t,
			struct k_poll_signal *);

/** @fake_of{lora_driver_api::recv} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_recv, const struct device *, uint8_t *, uint8_t, k_timeout_t,
			int16_t *, int8_t *);

/** @fake_of{lora_driver_api::recv_async} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_recv_async, const struct device *, lora_recv_cb, void *);

#if defined(CONFIG_LORA_FAKE_OPTIONAL_API) || defined(__DOXYGEN__)
/** @fake_of{lora_driver_api::cad} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_cad, const struct device *, k_timeout_t);

/** @fake_of{lora_driver_api::cad_async} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_cad_async, const struct device *, lora_cad_cb, void *);

/** @fake_of{lora_driver_api::rssi} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_rssi, const struct device *, int16_t *);

/** @fake_of{lora_driver_api::recv_duty_cycle} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_recv_duty_cycle, const struct device *, k_timeout_t,
			k_timeout_t, uint8_t *, uint8_t, k_timeout_t, int16_t *, int8_t *);

/** @fake_of{lora_driver_api::recv_duty_cycle_async} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_recv_duty_cycle_async, const struct device *, k_timeout_t,
			k_timeout_t, lora_recv_cb, void *);

/** @fake_of{lora_driver_api::test_cw} */
DECLARE_FAKE_VALUE_FUNC(int, fake_lora_test_cw, const struct device *, uint32_t, int8_t, uint16_t);
#endif /* defined(CONFIG_LORA_FAKE_OPTIONAL_API) || defined(__DOXYGEN__) */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_LORA_LORA_FAKE_H_ */
