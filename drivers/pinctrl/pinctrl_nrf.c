/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinctrl.h>

#include <hal/nrf_gpio.h>
#if CONFIG_NRF_UARTE_PERIPHERAL
#include <hal/nrf_uarte.h>
#endif

#if 0 && CONFIG_NRF_UARTE_PERIPHERAL
static void pinctrl_nrf_uarte_config(NRF_UARTE_Type *uarte,
				     const uint32_t *pincfgs,
				     size_t pincfgs_cnt)
{
	uint32_t pseltxd = NRF_UARTE_PSEL_DISCONNECTED;
	uint32_t pselrxd = NRF_UARTE_PSEL_DISCONNECTED;
	uint32_t pselrts = NRF_UARTE_PSEL_DISCONNECTED;
	uint32_t pselcts = NRF_UARTE_PSEL_DISCONNECTED;

	for (size_t i = 0U; i < pincfgs_cnt; i++) {
		switch (PINCTRL_NRF_GET_FUN(pincfgs[i])) {
		case PINCTRL_NRF_FUN_UART_TX:
			pseltxd = PINCTRL_NRF_GET_PIN(pincfgs[i]);
			break;
		case PINCTRL_NRF_FUN_UART_RX:
			pselrxd = PINCTRL_NRF_GET_PIN(pincfgs[i]);
			break;
		case PINCTRL_NRF_FUN_UART_RTS:
			pselrts = PINCTRL_NRF_GET_PIN(pincfgs[i]);
			break;
		case PINCTRL_NRF_FUN_UART_CTS:
			pselcts = PINCTRL_NRF_GET_PIN(pincfgs[i]);
			break;
		default:
			break;
		}
	}

	nrf_uarte_txrx_pins_set(uarte, pseltxd, pselrxd);
	nrf_uarte_hwfc_pins_set(uarte, pselrts, pselcts);

	nrf_gpio_pin_write(pseltxd, 1);
}
#endif

int z_impl_pinctrl_pin_configure(const pinctrl_soc_pins_t *pin_spec)
{
#if 0
	/* configure peripheral pin selection */
	switch (dev->pinctrl->reg) {
#if CONFIG_NRF_UARTE_PERIPHERAL
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
	case DT_REG_ADDR(DT_NODELABEL(uart0)):
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	case DT_REG_ADDR(DT_NODELABEL(uart1)):
#endif
		pinctrl_nrf_uarte_config((NRF_UARTE_Type *)dev->pinctrl->reg,
					 state->pincfgs,
					 state->pincfgs_cnt);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	/* configure pins GPIO settings */
	for (size_t i = 0U; i < state->pincfgs_cnt; i++) {
		nrf_gpio_cfg(PINCTRL_NRF_GET_PIN(state->pincfgs[i]),
			     PINCTRL_NRF_GET_DIR(state->pincfgs[i]),
			     PINCTRL_NRF_GET_INP(state->pincfgs[i]),
			     PINCTRL_NRF_GET_PULL(state->pincfgs[i]),
			     NRF_GPIO_PIN_S0S1,
			     NRF_GPIO_PIN_NOSENSE);
	}
#endif

	return 0;
}
