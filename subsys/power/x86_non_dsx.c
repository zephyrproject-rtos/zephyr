/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <devicetree/gpio.h>
#include <drivers/espi.h>
#include <power/ndsx_espi.h>
#include <power/x86_non_dsx.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(power);

/* Power sequencing GPIOs */
static struct gpio_config power_seq_gpios[] = {
	{
		POWER_SEQ_GPIO(PCH_EC_SLP_SUS_L),
	},
	{
		POWER_SEQ_GPIO(PCH_EC_SLP_S0_L),
	},
	{
		POWER_SEQ_GPIO(VR_EC_EC_RSMRST_ODL),
	},
	{
		POWER_SEQ_GPIO(VR_EC_ALL_SYS_PWRGD),
	},
#if POWER_SEQ_GPIO_PRESENT(VR_EC_DSW_PWROK)
	{
		POWER_SEQ_GPIO(VR_EC_DSW_PWROK),
	},
#endif
	{
		POWER_SEQ_GPIO(EC_PCH_RSMRST_L),
	},
#if POWER_SEQ_GPIO_PRESENT(EC_PCH_DSW_PWROK)
	{
		POWER_SEQ_GPIO(EC_PCH_DSW_PWROK),
	},
#endif
#if POWER_SEQ_GPIO_PRESENT(EC_PCH_SYS_PWROK)
	{
		POWER_SEQ_GPIO(EC_PCH_SYS_PWROK),
	},
#endif
#if POWER_SEQ_GPIO_PRESENT(EC_VR_PPVAR_VCCIN)
	{
		POWER_SEQ_GPIO(EC_VR_PPVAR_VCCIN),
	},
#endif
	{
		POWER_SEQ_GPIO(EC_PCH_PWR_BTN_ODL),
	},
#if POWER_SEQ_GPIO_PRESENT(EC_VR_EN_PP5000_A)
	{
		POWER_SEQ_GPIO(EC_VR_EN_PP5000_A),
	},
#endif
	{
		POWER_SEQ_GPIO(EC_VR_EN_PP3300_A),
	},
};

/* On power-on start boot up sequence */
static enum power_states_ndsx power_state = SYS_POWER_STATE_G3S5;

const struct gpio_config *get_gpio_config_from_net_name(const char *net_name)
{
	const struct gpio_config *gpio;
	int i;

	for (i = 0; i < ARRAY_SIZE(power_seq_gpios); i++) {
		gpio = &power_seq_gpios[i];
		if (!strcmp(gpio->net_name, net_name))
			return gpio;
	}

	LOG_ERR("Failed to find GPIO %s", net_name);
	return NULL;
}

static int gpio_get_level(const char *net_name)
{
	const struct gpio_config *gpio =
		get_gpio_config_from_net_name(net_name);

	if (gpio)
		return gpio_pin_get_raw(gpio->port, gpio->pin);

	return 0;
}

static void gpio_set_level(const char *net_name, int val)
{
	const struct gpio_config *gpio =
		get_gpio_config_from_net_name(net_name);

	if (gpio) {
		if (gpio_pin_set_raw(gpio->port, gpio->pin, val))
			LOG_ERR("Failed to set GPIO %s", net_name);
	}
}

static void enable_power_rails(bool enable)
{
	if (enable) {
#if POWER_SEQ_GPIO_PRESENT(EC_VR_EN_PP5000_A)
		gpio_set_level(GPIO_NET_NAME(EC_VR_EN_PP5000_A), 1);
#endif
		gpio_set_level(GPIO_NET_NAME(EC_VR_EN_PP3300_A), 1);
	} else {
		gpio_set_level(GPIO_NET_NAME(EC_VR_EN_PP3300_A), 0);
#if POWER_SEQ_GPIO_PRESENT(EC_VR_EN_PP5000_A)
		gpio_set_level(GPIO_NET_NAME(EC_VR_EN_PP5000_A), 0);
#endif
	}
}

static void powseq_gpio_init(void)
{
	struct gpio_config *gpio;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(power_seq_gpios); i++) {
		gpio = &power_seq_gpios[i];

		LOG_DBG("Configuring GPIO: net_name=%s, port_name=%s, \
			pin=0x%x, flag=0x%x", gpio->net_name, gpio->port_name,
			gpio->pin, gpio->flags);

		/* Get GPIO binding */
		gpio->port = device_get_binding(gpio->port_name);
		if (!gpio->port) {
			ret = -EINVAL;
			break;
		}

		/* Configure the GPIO */
		ret = gpio_pin_configure(gpio->port, gpio->pin, gpio->flags);
		if (ret != 0)
			break;
	}

	if (ret == 0)
		LOG_INF("Configuring GPIO complete");
	else
		LOG_ERR("Configuring GPIO failed: net_name=%s, port_name=%s, \
			pin=0x%x, flag=0x%x", gpio->net_name, gpio->port_name,
			gpio->pin, gpio->flags);
}

static void power_states_handler(void)
{
	switch (power_state) {
	case SYS_POWER_STATE_G3:
		/* Nothing to do */
		break;

	case SYS_POWER_STATE_S5:
		/* If A-rails are stable move to higher state */
		if (gpio_get_level(
#if POWER_SEQ_GPIO_PRESENT(VR_EC_DSW_PWROK)
			GPIO_NET_NAME(VR_EC_DSW_PWROK)
#else
			GPIO_NET_NAME(EC_VR_EN_PP3300_A)
#endif
			)) {
#if POWER_SEQ_GPIO_PRESENT(EC_PCH_DSW_PWROK)
			gpio_set_level(GPIO_NET_NAME(EC_PCH_DSW_PWROK), 1);
#endif
			power_state = SYS_POWER_STATE_S5S4;
		}
		break;

	case SYS_POWER_STATE_S4:
		/* AP is out of suspend to disk */
		if (vw_get_level(ESPI_VWIRE_SIGNAL_SLP_S4))
			power_state = SYS_POWER_STATE_S4S3;
		break;

	case SYS_POWER_STATE_S3:
		/* AP is out of suspend to RAM */
		if (vw_get_level(ESPI_VWIRE_SIGNAL_SLP_S3))
			power_state = SYS_POWER_STATE_S3S0;
		break;

	case SYS_POWER_STATE_S0:
		/* Stay in S0 */
		break;

	case SYS_POWER_STATE_G3S5:
		/* Enable AP power rails */
		enable_power_rails(true);
		power_state = SYS_POWER_STATE_S5;
		break;

	case SYS_POWER_STATE_S5S4:
		/* Check if the PCH has come out of suspend state */
		if (gpio_get_level(GPIO_NET_NAME(PCH_EC_SLP_SUS_L)) &&
			gpio_get_level(GPIO_NET_NAME(VR_EC_EC_RSMRST_ODL)))
			power_state = SYS_POWER_STATE_S4;
		break;

	case SYS_POWER_STATE_S4S3:
		power_state = SYS_POWER_STATE_S3;
		break;

	case SYS_POWER_STATE_S3S0:
		/* All the power rails must be stable */
		if (gpio_get_level(GPIO_NET_NAME(VR_EC_ALL_SYS_PWRGD)))
			power_state = SYS_POWER_STATE_S0;
		break;

	case SYS_POWER_STATE_S5G3:
	case SYS_POWER_STATE_S4S5:
	case SYS_POWER_STATE_S3S4:
	case SYS_POWER_STATE_S0S3:
		break;

	default:
		break;
	}
}

static void power_pass_thru_handler(const char *in_signal,
			const char *out_signal, uint32_t delay_ms)
{
	int in_sig_val = gpio_get_level(in_signal);

	if (in_sig_val != gpio_get_level(out_signal)) {
		if (in_sig_val)
			k_msleep(delay_ms);

		gpio_set_level(out_signal, in_sig_val);
	}
}

void espi_bus_reset(void)
{
	/* If SOC is up toggle the PM_PWRBTN pin */
	if (gpio_get_level(GPIO_NET_NAME(PCH_EC_SLP_SUS_L))) {
		LOG_INF("Toggle PM PWRBTN");

		gpio_set_level(GPIO_NET_NAME(EC_PCH_PWR_BTN_ODL), 0);
		k_msleep(POWER_EC_PCH_PM_PWRBTN_DELAY_MS);
		gpio_set_level(GPIO_NET_NAME(EC_PCH_PWR_BTN_ODL), 1);
	}
}

void pwrseq_thread(void *p1, void *p2, void *p3)
{
	int32_t t_wait_ms = *(int32_t *) p1;

	powseq_gpio_init();
	ndsx_espi_configure();

	while (true) {
		LOG_INF("In power state %d", power_state);

#if POWER_SEQ_GPIO_PRESENT(EC_PCH_DSW_PWROK)
		/* Handle DSW_PWROK passthrough */
		power_pass_thru_handler(
#if POWER_SEQ_GPIO_PRESENT(VR_EC_DSW_PWROK)
			GPIO_NET_NAME(VR_EC_DSW_PWROK),
#else
			GPIO_NET_NAME(EC_VR_EN_PP3300_A),
#endif
			GPIO_NET_NAME(EC_PCH_DSW_PWROK),
			POWER_EC_PCH_DSW_PWROK_DELAY_MS);
#endif

		/* Handle RSMRST passthrough */
		power_pass_thru_handler(GPIO_NET_NAME(VR_EC_EC_RSMRST_ODL),
				GPIO_NET_NAME(EC_PCH_RSMRST_L),
				POWER_EC_PCH_RSMRST_DELAY_MS);

#if POWER_SEQ_GPIO_PRESENT(EC_PCH_SYS_PWROK)
		/* Handle SYS_PWROK passthrough */
		power_pass_thru_handler(GPIO_NET_NAME(VR_EC_ALL_SYS_PWRGD),
			GPIO_NET_NAME(EC_PCH_SYS_PWROK),
			POWER_EC_PCH_SYS_PWROK_DELAY_MS);
#endif

#if POWER_SEQ_GPIO_PRESENT(EC_VR_PPVAR_VCCIN)
		/* Handle VCCIN passthrough */
		power_pass_thru_handler(GPIO_NET_NAME(VR_EC_ALL_SYS_PWRGD),
			GPIO_NET_NAME(EC_VR_PPVAR_VCCIN),
			POWER_EC_VR_EN_VCCIN_DELAY_MS);
#endif

		power_states_handler();

		k_msleep(t_wait_ms);
	}
}
