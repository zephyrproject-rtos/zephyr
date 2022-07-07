/*
 * Copyright (c) 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nxp_kinetis_pinmux

#include <zephyr/drivers/pinctrl.h>
#include <fsl_clock.h>

/* Port register addresses. */
static PORT_Type *ports[] = {
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(porta)),
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portb)),
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portc)),
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 5
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portd)),
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(porte)),
#endif
};

#define PIN(mux) (((mux) & 0xFC00000) >> 22)
#define PORT(mux) (((mux) & 0xF0000000) >> 28)
#define PINCFG(mux) ((mux) & Z_PINCTRL_KINETIS_PCR_MASK)

struct pinctrl_mcux_config {
	clock_ip_name_t clock_ip_name;
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	for (uint8_t i = 0; i < pin_cnt; i++) {
		PORT_Type *base = ports[PORT(pins[i])];
		uint8_t pin = PIN(pins[i]);
		uint16_t mux = PINCFG(pins[i]);

		base->PCR[pin] = (base->PCR[pin] & (~Z_PINCTRL_KINETIS_PCR_MASK)) | mux;
	}
	return 0;
}

/* Kinetis pinmux driver binds to the same DTS nodes,
 * and handles clock init. Only bind to these nodes if pinmux driver
 * is disabled.
 */
#ifndef CONFIG_PINMUX
static int pinctrl_mcux_init(const struct device *dev)
{
	const struct pinctrl_mcux_config *config = dev->config;

	CLOCK_EnableClock(config->clock_ip_name);

	return 0;
}

#if DT_NODE_HAS_STATUS(DT_INST(0, nxp_kinetis_ke1xf_sim), okay)
#define INST_DT_CLOCK_IP_NAME(n) \
	DT_REG_ADDR(DT_INST_PHANDLE(n, clocks)) + DT_INST_CLOCKS_CELL(n, name)
#else
#define INST_DT_CLOCK_IP_NAME(n) \
	CLK_GATE_DEFINE(DT_INST_CLOCKS_CELL(n, offset), \
			DT_INST_CLOCKS_CELL(n, bits))
#endif

#define PINCTRL_MCUX_INIT(n)						\
	static const struct pinctrl_mcux_config pinctrl_mcux_##n##_config = {\
		.clock_ip_name = INST_DT_CLOCK_IP_NAME(n),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &pinctrl_mcux_init,				\
			    NULL,					\
			    NULL, &pinctrl_mcux_##n##_config,		\
			    PRE_KERNEL_1,				\
			    0,						\
			    NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_MCUX_INIT)
#endif
