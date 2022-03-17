/*
 * Copyright (c) 2022 Silicon Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinctrl.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
		struct soc_gpio_pin rxpin;
		struct soc_gpio_pin txpin;
		USART_TypeDef *base = (USART_TypeDef *)reg;
		uint8_t loc;
		int usart_num = USART_NUM(base);

		for (uint8_t i = 0U; i < pin_cnt; i++) {
			switch (GECKO_GET_FUN(pins[i])) {
				case GECKO_FUN_UART_RX:
					rxpin.port = GECKO_GET_PORT(pins[i]);
					rxpin.pin = GECKO_GET_PIN(pins[i]);
					rxpin.mode = gpioModeInput;
					rxpin.out = 1;
					break;
				case GECKO_FUN_UART_TX:
					txpin.port = GECKO_GET_PORT(pins[i]);
					txpin.pin = GECKO_GET_PIN(pins[i]);
					txpin.mode = gpioModePushPull;
					txpin.out = 1;
					break;
				case GECKO_FUN_UART_LOC:
					loc = GECKO_GET_LOC(pins[i]);
					break;
				default:
					break;
			}
		}

		soc_gpio_configure(&rxpin);
		soc_gpio_configure(&txpin);

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
			/* For SOCs with configurable pin locations (set in SOC Kconfig) */
			base->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
			base->ROUTELOC0 =
				(loc << _USART_ROUTELOC0_TXLOC_SHIFT) |
				(loc << _USART_ROUTELOC0_RXLOC_SHIFT);
			base->ROUTELOC1 = _USART_ROUTELOC1_RESETVALUE;
#elif defined(USART_ROUTE_RXPEN) && defined(USART_ROUTE_TXPEN)
			/* For olders SOCs with only one pin location */
			base->ROUTE = USART_ROUTE_RXPEN | USART_ROUTE_TXPEN
				| (loc << 8);
#elif defined(GPIO_USART_ROUTEEN_RXPEN) && defined(GPIO_USART_ROUTEEN_TXPEN)
			GPIO->USARTROUTE[usart_num].ROUTEEN =
				GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_RXPEN;
			GPIO->USARTROUTE[usart_num].TXROUTE =
				(txpin.pin << _GPIO_USART_TXROUTE_PIN_SHIFT) |
				(txpin.port << _GPIO_USART_TXROUTE_PORT_SHIFT);
			GPIO->USARTROUTE[usart_num].RXROUTE =
				(rxpin.pin << _GPIO_USART_RXROUTE_PIN_SHIFT) |
				(rxpin.port << _GPIO_USART_RXROUTE_PORT_SHIFT);
#endif /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */

#ifdef UART_GECKO_HW_FLOW_CONTROL
			/* Configure HW flow control (RTS, CTS) */
			if (config->hw_flowcontrol) {
				soc_gpio_configure(&config->pin_rts);
				soc_gpio_configure(&config->pin_cts);

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
				config->base->ROUTEPEN =
					USART_ROUTEPEN_RXPEN	|
					USART_ROUTEPEN_TXPEN	|
					USART_ROUTEPEN_RTSPEN |
					USART_ROUTEPEN_CTSPEN;

				config->base->ROUTELOC1 =
					(config->loc_rts << _USART_ROUTELOC1_RTSLOC_SHIFT) |
					(config->loc_cts << _USART_ROUTELOC1_CTSLOC_SHIFT);
#elif defined(GPIO_USART_ROUTEEN_RTSPEN) && defined(GPIO_USART_ROUTEEN_CTSPEN)
			GPIO->USARTROUTE[usart_num].ROUTEEN =
				GPIO_USART_ROUTEEN_TXPEN |
				GPIO_USART_ROUTEEN_RXPEN |
				GPIO_USART_ROUTEPEN_RTSPEN |
				GPIO_USART_ROUTEPEN_CTSPEN;

			GPIO->USARTROUTE[usart_num].RTSROUTE =
				(config->pin_rts.pin << _GPIO_USART_RTSROUTE_PIN_SHIFT) |
				(config->pin_rts.port << _GPIO_USART_RTSROUTE_PORT_SHIFT);
			GPIO->USARTROUTE[usart_num].CTSROUTE =
				(config->pin_cts.pin << _GPIO_USART_CTSROUTE_PIN_SHIFT) |
				(config->pin_cts.port << _GPIO_USART_CTSROUTE_PORT_SHIFT);
#endif /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */
			}
#endif /* UART_GECKO_HW_FLOW_CONTROL */
	return 0;
}
