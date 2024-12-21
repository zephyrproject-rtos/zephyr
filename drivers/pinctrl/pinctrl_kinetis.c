/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nxp_kinetis_pinmux

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <fsl_clock.h>

LOG_MODULE_REGISTER(pinctrl_kinetis, CONFIG_PINCTRL_LOG_LEVEL);

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
#define PINCFG(mux) ((mux) & Z_PINCTRL_KINETIS_PCR_MASK)

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

		base->PCR[pin] = (base->PCR[pin] & (~Z_PINCTRL_KINETIS_PCR_MASK)) | mux;
	}
	return 0;
}

/* Kinetis pinmux driver binds to the same DTS nodes,
 * and handles clock init. Only bind to these nodes if pinmux driver
 * is disabled.
 */
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

#if DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_kinetis_sim))
#define PINCTRL_MCUX_DT_INST_CLOCK_SUBSYS(n)                                                       \
	CLK_GATE_DEFINE(DT_INST_CLOCKS_CELL(n, offset), DT_INST_CLOCKS_CELL(n, bits))
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_scg_k4)
#define PINCTRL_MCUX_DT_INST_CLOCK_SUBSYS(n)                                                       \
	(DT_INST_CLOCKS_CELL(n, mrcc_offset) == 0                                                  \
		 ? 0                                                                               \
		 : MAKE_MRCC_REGADDR(MRCC_BASE, DT_INST_CLOCKS_CELL(n, mrcc_offset)))
#else
#define PINCTRL_MCUX_DT_INST_CLOCK_SUBSYS(n) \
	DT_INST_CLOCKS_CELL(n, name)
#endif

#define PINCTRL_MCUX_INIT(n)						\
	static const struct pinctrl_mcux_config pinctrl_mcux_##n##_config = {\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)		\
				PINCTRL_MCUX_DT_INST_CLOCK_SUBSYS(n),	\
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
