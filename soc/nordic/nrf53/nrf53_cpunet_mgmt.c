/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Nordic Semiconductor nRF53 processors family management helper for the network CPU.
 */

#include <nrf53_cpunet_mgmt.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/notify.h>
#include <zephyr/sys/onoff.h>
#include <soc_secure.h>

#include <hal/nrf_reset.h>
#include <hal/nrf_spu.h>

static struct onoff_manager cpunet_mgr;

#ifdef CONFIG_SOC_NRF_GPIO_FORWARDER_FOR_NRF5340

#define GPIOS_PSEL_BY_IDX(node_id, prop, idx) \
	NRF_DT_GPIOS_TO_PSEL_BY_IDX(node_id, prop, idx),
#define ALL_GPIOS_IN_NODE(node_id) \
	DT_FOREACH_PROP_ELEM(node_id, gpios, GPIOS_PSEL_BY_IDX)
#define ALL_GPIOS_IN_FORWARDER(node_id) \
	DT_FOREACH_CHILD(node_id, ALL_GPIOS_IN_NODE)

static void assign_pins(nrf_gpio_pin_sel_t sel)
{
	static const uint8_t forwarded_psels[] = {
		DT_FOREACH_STATUS_OKAY(nordic_nrf_gpio_forwarder, ALL_GPIOS_IN_FORWARDER)
	};

	for (int i = 0; i < ARRAY_SIZE(forwarded_psels); i++) {
		soc_secure_gpio_pin_mcu_select(forwarded_psels[i], sel);
	}
}
#endif /* CONFIG_SOC_NRF_GPIO_FORWARDER_FOR_NRF5340 */

static void onoff_start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
#ifdef CONFIG_SOC_NRF_GPIO_FORWARDER_FOR_NRF5340
	/* Forward assigned pins to the network core. */
	assign_pins(NRF_GPIO_PIN_SEL_NETWORK);
#endif

	nrf_reset_network_force_off(NRF_RESET, false);

	notify(mgr, 0);
}

static void onoff_stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	nrf_reset_network_force_off(NRF_RESET, true);

#ifdef CONFIG_SOC_NRF_GPIO_FORWARDER_FOR_NRF5340
	/* Assign pins back to app core. */
	assign_pins(NRF_GPIO_PIN_SEL_APP);
#endif

	notify(mgr, 0);
}

static int nrf53_cpunet_mgmt_init(void)
{
	static const struct onoff_transitions transitions = {
		.start = onoff_start,
		.stop = onoff_stop
	};

	return onoff_manager_init(&cpunet_mgr, &transitions);
}

SYS_INIT(nrf53_cpunet_mgmt_init, PRE_KERNEL_1, 0);

void nrf53_cpunet_enable(bool on)
{
	int ret;
	int ignored;
	struct onoff_client cli;

	if (on) {
		sys_notify_init_spinwait(&cli.notify);

		ret = onoff_request(&cpunet_mgr, &cli);
		__ASSERT_NO_MSG(ret >= 0);

		/* The transition is synchronous and shall take effect immediately. */
		ret = sys_notify_fetch_result(&cli.notify, &ignored);
	} else {
		ret = onoff_release(&cpunet_mgr);
	}

	__ASSERT_NO_MSG(ret >= 0);
}

#ifdef CONFIG_SOC_NRF53_CPUNET_ENABLE
static int nrf53_cpunet_init(void)
{
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 */
	nrf_spu_extdomain_set((NRF_SPU_Type *)DT_REG_ADDR(DT_NODELABEL(spu)), 0, true, false);
#endif /* !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) */

#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	/*
	 * Building Zephyr with CONFIG_TRUSTED_EXECUTION_SECURE=y implies
	 * building also a Non-Secure image. The Non-Secure image will, in
	 * this case do the remainder of actions to properly configure and
	 * boot the Network MCU.
	 */

	nrf53_cpunet_enable(true);
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	return 0;
}

SYS_INIT(nrf53_cpunet_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif /* CONFIG_SOC_NRF53_CPUNET_ENABLE */
