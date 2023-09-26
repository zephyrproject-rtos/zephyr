/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright (c) 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_pinmux

#include <zephyr/drivers/pinctrl.h>

#include <fsl_clock.h>
#include <soc.h>

/* Port register addresses. */
static PORT_Type *ports[] = {
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(porta)),
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portb)),
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portc)),
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(portd)),
	(PORT_Type *)DT_REG_ADDR(DT_NODELABEL(porte)),
};

#define PIN(mux) (((mux) & 0xFC00000) >> 22)
#define PORT(mux) (((mux) & 0xF0000000) >> 28)
#define PINCFG(mux) ((mux) & Z_PINCTRL_RV32M1_PCR_MASK)

struct pinctrl_rv32m1_config {
	clock_ip_name_t clock_ip_name;
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	for (uint8_t i = 0; i < pin_cnt; i++) {
		PORT_Type *base = ports[PORT(pins[i])];
		uint8_t pin = PIN(pins[i]);
		uint16_t mux = PINCFG(pins[i]);

		base->PCR[pin] = (base->PCR[pin] & (~Z_PINCTRL_RV32M1_PCR_MASK)) | mux;
	}
	return 0;
}

static int pinctrl_rv32m1_init(const struct device *dev)
{
	const struct pinctrl_rv32m1_config *config = dev->config;

	CLOCK_EnableClock(config->clock_ip_name);

	return 0;
}

#define PINCTRL_RV32M1_INIT(n)						\
	static const struct pinctrl_rv32m1_config pinctrl_rv32m1_##n##_config = {\
		.clock_ip_name = INST_DT_CLOCK_IP_NAME(n),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &pinctrl_rv32m1_init,			\
			    NULL,					\
			    NULL, &pinctrl_rv32m1_##n##_config,		\
			    PRE_KERNEL_1,				\
			    CONFIG_PINCTRL_RV32M1_INIT_PRIORITY,	\
			    NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_RV32M1_INIT)
