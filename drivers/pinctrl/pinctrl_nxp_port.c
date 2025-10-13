/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nxp_port_pinmux

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <fsl_clock.h>

LOG_MODULE_REGISTER(pinctrl_nxp_port, CONFIG_PINCTRL_LOG_LEVEL);

/* Port register addresses. */
static PORT_Type *ports[] = {
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(porta)),
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portb)),
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portc)),
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 3
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portd)),
#endif
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 4
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(porte)),
#endif
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 5
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portf)),
#endif
};

#define PIN(mux) (((mux) & 0xFC00000) >> 22)
#define PORT(mux) (((mux) & 0xF0000000) >> 28)
#define PINCFG(mux) ((mux) & Z_PINCTRL_NXP_PORT_PCR_MASK)

struct pinctrl_mcux_config {
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	for (uint8_t i = 0; i < pin_cnt; i++) {
		PORT_Type *base = ports[PORT(pins[i])];
		uint8_t pin = PIN(pins[i]);
		uint16_t mux = PINCFG(pins[i]);

		base->PCR[pin] = (base->PCR[pin] & (~Z_PINCTRL_NXP_PORT_PCR_MASK)) | mux;
	}
	return 0;
}

static int pinctrl_mcux_init(const struct device *dev)
{
	const struct pinctrl_mcux_config *config = dev->config;
	int err;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_ERR("failed to enable clock (err %d)", err);
		return -EINVAL;
	}

	return 0;
}

#define PINCTRL_MCUX_INIT(n)						\
	CLOCK_CONTROL_DT_SPEC_INST_DEFINE(n, clocks);			\
	static const struct pinctrl_mcux_config pinctrl_mcux_##n##_config = {\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = CLOCK_CONTROL_DT_SPEC_INST_GET(n, clocks), \
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &pinctrl_mcux_init,				\
			    NULL,					\
			    NULL, &pinctrl_mcux_##n##_config,		\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_MCUX_INIT)
