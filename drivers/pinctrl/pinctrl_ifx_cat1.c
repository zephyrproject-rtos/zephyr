/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Pin control driver for Infineon CAT1 MCU family.
 */

#include <zephyr/drivers/pinctrl.h>

#include <infineon_kconfig.h>
#include <cy_gpio.h>

#define GPIO_PORT_OR_NULL(node_id)                                                                 \
	COND_CODE_1(DT_NODE_EXISTS(node_id), ((GPIO_PRT_Type *)DT_REG_ADDR(node_id)), (NULL))

/* @brief Array containing pointers to each GPIO port.
 *
 * Entries will be NULL if the GPIO port is not enabled.
 */
static GPIO_PRT_Type *const gpio_ports[] = {
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt0)),  GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt1)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt2)),  GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt3)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt4)),  GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt5)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt6)),  GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt7)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt8)),  GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt9)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt10)), GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt11)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt12)), GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt13)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt14)), GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt15)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt16)), GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt17)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt18)), GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt19)),
	GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt20)), GPIO_PORT_OR_NULL(DT_NODELABEL(gpio_prt21))};

/* @brief This function returns gpio drive mode, according to.
 * bias and drive mode params defined in pinctrl node.
 *
 * @param flags - bias and drive mode flags from pinctrl node.
 */
static uint32_t soc_gpio_get_drv_mode(uint32_t flags)
{
	uint32_t drv_mode = CY_GPIO_DM_ANALOG;
	uint32_t flags_masked;

	flags_masked = ((flags & SOC_GPIO_FLAGS_MASK) >> SOC_GPIO_FLAGS_POS);

	if (flags_masked & SOC_GPIO_OPENDRAIN) {
		/* drive_open_drain */
		drv_mode = (flags_masked & SOC_GPIO_INPUTENABLE) ? CY_GPIO_DM_OD_DRIVESLOW
							   : CY_GPIO_DM_OD_DRIVESLOW_IN_OFF;

	} else if (flags_masked & SOC_GPIO_OPENSOURCE) {
		/* drive_open_source */
		drv_mode = (flags_masked & SOC_GPIO_INPUTENABLE) ? CY_GPIO_DM_OD_DRIVESHIGH
							   : CY_GPIO_DM_OD_DRIVESHIGH_IN_OFF;

	} else if (flags_masked & SOC_GPIO_PUSHPULL) {
		/* drive_push_pull */
		drv_mode = (flags_masked & SOC_GPIO_INPUTENABLE) ? CY_GPIO_DM_STRONG
							   : CY_GPIO_DM_STRONG_IN_OFF;

	} else if ((flags_masked & SOC_GPIO_PULLUP) && (flags_masked & SOC_GPIO_PULLDOWN)) {
		/* bias_pull_up and bias_pull_down */
		drv_mode = (flags_masked & SOC_GPIO_INPUTENABLE) ? CY_GPIO_DM_PULLUP_DOWN
							   : CY_GPIO_DM_PULLUP_DOWN_IN_OFF;

	} else if (flags_masked & SOC_GPIO_PULLUP) {
		/* bias_pull_up */
		drv_mode = (flags_masked & SOC_GPIO_INPUTENABLE) ? CY_GPIO_DM_PULLUP
							   : CY_GPIO_DM_PULLUP_IN_OFF;

	} else if (flags_masked & SOC_GPIO_PULLDOWN) {
		/* bias_pull_down */
		drv_mode = (flags_masked & SOC_GPIO_INPUTENABLE) ? CY_GPIO_DM_PULLDOWN
							   : CY_GPIO_DM_PULLDOWN_IN_OFF;
	} else if ((flags_masked & SOC_GPIO_HIGHZ) | (flags_masked & SOC_GPIO_INPUTENABLE)) {
		/* bias_pull_down */
		drv_mode = CY_GPIO_DM_HIGHZ;
	} else {
		/* nothing do here */
	}

	return drv_mode;
}

#if defined(CONFIG_SOC_SERIES_PSE84)
static uint32_t soc_gpio_get_drv_strength(uint32_t flags)
{
	uint32_t drv_strength_idx = 0;
	uint32_t drv_strength = CY_GPIO_DRIVE_1_8;
	uint32_t flags_masked;

	flags_masked = ((flags & SOC_GPIO_FLAGS_MASK) >> SOC_GPIO_FLAGS_POS);
	drv_strength_idx = (flags_masked & SOC_GPIO_DRIVESTRENGTH) >> SOC_GPIO_DRIVESTRENGTH_POS;

	switch (drv_strength_idx) {
	case 0:
		drv_strength = CY_GPIO_DRIVE_FULL;
		break;
	case 1:
		drv_strength = CY_GPIO_DRIVE_1_2;
		break;
	case 2:
		drv_strength = CY_GPIO_DRIVE_1_4;
		break;
	case 3:
		drv_strength = CY_GPIO_DRIVE_1_8;
		break;

	default:
		drv_strength = CY_GPIO_DRIVE_1_8;
		break;
	}

	return drv_strength;
}
#endif

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		uint32_t drv_mode = soc_gpio_get_drv_mode(pins[i].pincfg);
		uint32_t hsiom = CAT1_PINMUX_GET_HSIOM_FUNC(pins[i].pinmux);
		uint32_t port_num = CAT1_PINMUX_GET_PORT_NUM(pins[i].pinmux);
		uint32_t pin_num = CAT1_PINMUX_GET_PIN_NUM(pins[i].pinmux);

		/* Initialize pin */
#if defined(CY_PDL_TZ_ENABLED)
		Cy_GPIO_Pin_SecFastInit(gpio_ports[port_num], pin_num, drv_mode, 1, hsiom);
#else
		Cy_GPIO_Pin_FastInit(gpio_ports[port_num], pin_num, drv_mode, 1, hsiom);
#endif /* defined(CY_PDL_TZ_ENABLED) */

		/* Force output to enable pulls */
		switch (drv_mode) {
		case CY_GPIO_DM_PULLUP:
			Cy_GPIO_Write(gpio_ports[port_num], pin_num, 1);
			break;
		case CY_GPIO_DM_PULLDOWN:
			Cy_GPIO_Write(gpio_ports[port_num], pin_num, 0);
			break;
		default:
			/* Do nothing */
			break;
		}

#if defined(CONFIG_SOC_SERIES_PSE84)
		Cy_GPIO_SetDriveSel(gpio_ports[port_num], pin_num,
				    soc_gpio_get_drv_strength(pins[i].pincfg));
#endif
	}

	return 0;
}
