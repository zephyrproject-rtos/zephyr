/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/lora/lora_fake.h>
#include <zephyr/fff.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

#define DT_DRV_COMPAT zephyr_fake_lora

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_config, const struct device *,
		       const struct lora_modem_config *);

DEFINE_FAKE_VALUE_FUNC(uint32_t, fake_lora_airtime, const struct device *, uint32_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_send, const struct device *, uint8_t *, uint32_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_send_async, const struct device *, uint8_t *, uint32_t,
		       struct k_poll_signal *);

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_recv, const struct device *, uint8_t *, uint8_t, k_timeout_t,
		       int16_t *, int8_t *);

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_recv_async, const struct device *, lora_recv_cb, void *);

#ifdef CONFIG_LORA_FAKE_OPTIONAL_API
DEFINE_FAKE_VALUE_FUNC(int, fake_lora_cad, const struct device *, k_timeout_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_cad_async, const struct device *, lora_cad_cb, void *);

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_rssi, const struct device *, int16_t *);

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_recv_duty_cycle, const struct device *, k_timeout_t,
		       k_timeout_t, uint8_t *, uint8_t, k_timeout_t, int16_t *, int8_t *);

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_recv_duty_cycle_async, const struct device *, k_timeout_t,
		       k_timeout_t, lora_recv_cb, void *);

DEFINE_FAKE_VALUE_FUNC(int, fake_lora_test_cw, const struct device *, uint32_t, int8_t, uint16_t);
#endif /* CONFIG_LORA_FAKE_OPTIONAL_API */

static int fake_lora_recv_delegate(const struct device *dev, uint8_t *data, uint8_t size,
				   k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	ARG_UNUSED(size);
	ARG_UNUSED(timeout);

	if (rssi != NULL) {
		*rssi = 0;
	}

	if (snr != NULL) {
		*snr = 0;
	}

	return 0;
}

#ifdef CONFIG_LORA_FAKE_OPTIONAL_API
static int fake_lora_rssi_delegate(const struct device *dev, int16_t *rssi)
{
	ARG_UNUSED(dev);

	if (rssi != NULL) {
		*rssi = 0;
	}

	return 0;
}

static int fake_lora_recv_duty_cycle_delegate(const struct device *dev, k_timeout_t rx_period,
					      k_timeout_t sleep_period, uint8_t *data, uint8_t size,
					      k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_period);
	ARG_UNUSED(sleep_period);
	ARG_UNUSED(data);
	ARG_UNUSED(size);
	ARG_UNUSED(timeout);

	if (rssi != NULL) {
		*rssi = 0;
	}

	if (snr != NULL) {
		*snr = 0;
	}

	return 0;
}
#endif /* CONFIG_LORA_FAKE_OPTIONAL_API */

static void fake_lora_install_default_delegates(void)
{
	fake_lora_recv_fake.custom_fake = fake_lora_recv_delegate;
#ifdef CONFIG_LORA_FAKE_OPTIONAL_API
	fake_lora_rssi_fake.custom_fake = fake_lora_rssi_delegate;
	fake_lora_recv_duty_cycle_fake.custom_fake = fake_lora_recv_duty_cycle_delegate;
#endif /* CONFIG_LORA_FAKE_OPTIONAL_API */
}

#ifdef CONFIG_ZTEST
static void fake_lora_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(fake_lora_config);
	RESET_FAKE(fake_lora_airtime);
	RESET_FAKE(fake_lora_send);
	RESET_FAKE(fake_lora_send_async);
	RESET_FAKE(fake_lora_recv);
	RESET_FAKE(fake_lora_recv_async);
#ifdef CONFIG_LORA_FAKE_OPTIONAL_API
	RESET_FAKE(fake_lora_cad);
	RESET_FAKE(fake_lora_cad_async);
	RESET_FAKE(fake_lora_rssi);
	RESET_FAKE(fake_lora_recv_duty_cycle);
	RESET_FAKE(fake_lora_recv_duty_cycle_async);
	RESET_FAKE(fake_lora_test_cw);
#endif /* CONFIG_LORA_FAKE_OPTIONAL_API */

	/* Re-install default delegates */
	fake_lora_install_default_delegates();
}

ZTEST_RULE(fake_lora_reset_rule, fake_lora_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static int fake_lora_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Install default delegates */
	fake_lora_install_default_delegates();

	return 0;
}

static DEVICE_API(lora, fake_lora_driver_api) = {
	.config = fake_lora_config,
	.airtime = fake_lora_airtime,
	.send = fake_lora_send,
	.send_async = fake_lora_send_async,
	.recv = fake_lora_recv,
	.recv_async = fake_lora_recv_async,
#ifdef CONFIG_LORA_FAKE_OPTIONAL_API
	.cad = fake_lora_cad,
	.cad_async = fake_lora_cad_async,
	.rssi = fake_lora_rssi,
	.recv_duty_cycle = fake_lora_recv_duty_cycle,
	.recv_duty_cycle_async = fake_lora_recv_duty_cycle_async,
	.test_cw = fake_lora_test_cw,
#endif /* CONFIG_LORA_FAKE_OPTIONAL_API */
};

#define FAKE_LORA_INIT(inst)                                                                       \
	DEVICE_DT_INST_DEFINE(inst, fake_lora_init, NULL, NULL, NULL, POST_KERNEL,                 \
			      CONFIG_LORA_INIT_PRIORITY, &fake_lora_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_LORA_INIT)
