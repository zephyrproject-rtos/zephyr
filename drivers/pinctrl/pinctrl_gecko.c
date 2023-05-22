/*
 * Copyright (c) 2022 Silicon Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

#include <soc_gpio.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	USART_TypeDef *base = (USART_TypeDef *)reg;
	LEUART_TypeDef *lebase = (LEUART_TypeDef *)reg;
	#ifdef _SILICON_LABS_32B_SERIES_2
	int usart_num = USART_NUM(base);
	#endif /*_SILICON_LABS_32B_SERIES_2*/
	uint8_t loc;

	struct soc_gpio_pin usartpin = {0, 0, 0, 0};

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		switch (GECKO_GET_FUN(pins[i])) {
		case GECKO_FUN_UART_RX:
#ifdef CONFIG_UART_GECKO
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModeInput;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_RXPEN;
			GPIO->USARTROUTE[usart_num].RXROUTE =
				(usartpin.pin << _GPIO_USART_RXROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_RXROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_UART_TX:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModePushPull;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_TXPEN;
			GPIO->USARTROUTE[usart_num].TXROUTE =
				(usartpin.pin << _GPIO_USART_TXROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_TXROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_UART_RTS:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModePushPull;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_RTSPEN;
			GPIO->USARTROUTE[usart_num].RTSROUTE =
				(usartpin.pin << _GPIO_USART_RTSROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_RTSROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_UART_CTS:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModeInput;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);

#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_CTSPEN;
			GPIO->USARTROUTE[usart_num].CTSROUTE =
				(usartpin.pin << _GPIO_USART_CTSROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_CTSROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_UART_RX_LOC:
			loc = GECKO_GET_LOC(pins[i]);
#ifdef _SILICON_LABS_32B_SERIES_1
			base->ROUTEPEN |= USART_ROUTEPEN_RXPEN;
			base->ROUTELOC0 &= ~_USART_ROUTELOC0_RXLOC_MASK;
			base->ROUTELOC0 |= (loc << _USART_ROUTELOC0_RXLOC_SHIFT);
#elif defined _SILICON_LABS_32B_SERIES_0
			base->ROUTE |= (USART_ROUTE_RXPEN | (loc << 8));
#endif	/*_SILICON_LABS_32B_SERIES_1*/
			break;

		case GECKO_FUN_UART_TX_LOC:
			loc = GECKO_GET_LOC(pins[i]);
#ifdef _SILICON_LABS_32B_SERIES_1
			base->ROUTEPEN |= USART_ROUTEPEN_TXPEN;
			base->ROUTELOC0 &= ~_USART_ROUTELOC0_TXLOC_MASK;
			base->ROUTELOC0 |= (loc << _USART_ROUTELOC0_TXLOC_SHIFT);
#elif defined _SILICON_LABS_32B_SERIES_0
			base->ROUTE |= (USART_ROUTE_TXPEN | (loc << 8));
#endif	/*_SILICON_LABS_32B_SERIES_1*/
			break;

		case GECKO_FUN_UART_RTS_LOC:
#ifdef _SILICON_LABS_32B_SERIES_1
			loc = GECKO_GET_LOC(pins[i]);
			base->ROUTEPEN |= USART_ROUTEPEN_RTSPEN;
			base->ROUTELOC1 &= ~_USART_ROUTELOC1_RTSLOC_MASK;
			base->ROUTELOC1 |= (loc << _USART_ROUTELOC1_RTSLOC_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_1*/
			break;

		case GECKO_FUN_UART_CTS_LOC:
#ifdef _SILICON_LABS_32B_SERIES_1
			loc = GECKO_GET_LOC(pins[i]);
			base->ROUTEPEN |= USART_ROUTEPEN_CTSPEN;
			base->ROUTELOC1 &= ~_USART_ROUTELOC1_CTSLOC_MASK;
			base->ROUTELOC1 |= (loc << _USART_ROUTELOC1_CTSLOC_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_1*/
			break;

		case GECKO_FUN_LEUART_RX_LOC:
#ifdef _SILICON_LABS_32B_SERIES_1
			loc = GECKO_GET_LOC(pins[i]);
			lebase->ROUTEPEN |= LEUART_ROUTEPEN_RXPEN;
			lebase->ROUTELOC0 &= ~_LEUART_ROUTELOC0_RXLOC_MASK;
			lebase->ROUTELOC0 |= (loc << _LEUART_ROUTELOC0_RXLOC_SHIFT);
#endif /*_SILICON_LABS_32B_SERIES_1*/
			break;

		case GECKO_FUN_LEUART_TX_LOC:
#ifdef _SILICON_LABS_32B_SERIES_1
			loc = GECKO_GET_LOC(pins[i]);
			lebase->ROUTEPEN |= LEUART_ROUTEPEN_TXPEN;
			lebase->ROUTELOC0 &= ~_LEUART_ROUTELOC0_TXLOC_MASK;
			lebase->ROUTELOC0 |= (loc << _LEUART_ROUTELOC0_TXLOC_SHIFT);
#endif /*_SILICON_LABS_32B_SERIES_1*/
			break;
#endif /* CONFIG_UART_GECKO */

/**********************************************SPI**********************************************/
#ifdef CONFIG_SPI_GECKO
		case GECKO_FUN_SPIM_SCK:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModePushPull;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_CLKPEN;
			GPIO->USARTROUTE[usart_num].CLKOUTE =
				(usartpin.pin << _GPIO_USART_CLKROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_CLKROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_SPIM_MOSI:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModePushPull;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_TXPEN;
			GPIO->USARTROUTE[usart_num].TXROUTE =
				(usartpin.pin << _GPIO_USART_TXROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_TXROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_SPIM_MISO:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModeInput;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
					usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_RXPEN;
			GPIO->USARTROUTE[usart_num].RXROUTE =
				(usartpin.pin << _GPIO_USART_RXROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_RXROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_SPIM_CS:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModePushPull;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_CSPEN;
			GPIO->USARTROUTE[usart_num].CSROUTE =
				(usartpin.pin << _GPIO_USART_CSROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_CSROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_SPIS_SCK:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModeInput;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
					usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_CLKPEN;
			GPIO->USARTROUTE[usart_num].CLKROUTE =
				(usartpin.pin << _GPIO_USART_CLKROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_CLKROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_SPIS_MOSI:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModeInput;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_TXPEN;
			GPIO->USARTROUTE[usart_num].TXROUTE =
				(usartpin.pin << _GPIO_USART_TXROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_TXROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_SPIS_MISO:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModePushPull;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_RXPEN;
			GPIO->USARTROUTE[usart_num].RXROUTE =
				(usartpin.pin << _GPIO_USART_RXROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_RXROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_SPIS_CS:
			usartpin.port = GECKO_GET_PORT(pins[i]);
			usartpin.pin = GECKO_GET_PIN(pins[i]);
			usartpin.mode = gpioModeInput;
			usartpin.out = 1;
			GPIO_PinModeSet(usartpin.port, usartpin.pin, usartpin.mode,
							       usartpin.out);
#ifdef _SILICON_LABS_32B_SERIES_2
			GPIO->USARTROUTE[usart_num].ROUTEEN |= GPIO_USART_ROUTEEN_CSPEN;
			GPIO->USARTROUTE[usart_num].CSROUTE =
				(usartpin.pin << _GPIO_USART_CSROUTE_PIN_SHIFT) |
				(usartpin.port << _GPIO_USART_CSROUTE_PORT_SHIFT);
#endif	/*_SILICON_LABS_32B_SERIES_2*/
			break;

		case GECKO_FUN_SPI_SCK_LOC:
			loc = GECKO_GET_LOC(pins[i]);
#ifdef _SILICON_LABS_32B_SERIES_1
			base->ROUTEPEN |= USART_ROUTEPEN_CLKPEN;
			base->ROUTELOC0 &= ~_USART_ROUTELOC0_CLKLOC_MASK;
			base->ROUTELOC0 |= (loc << _USART_ROUTELOC0_CLKLOC_SHIFT);
#elif defined _SILICON_LABS_32B_SERIES_0
			base->ROUTE |= (USART_ROUTE_CLKPEN | (loc << 8));
#endif	/*_SILICON_LABS_32B_SERIES_1*/
			break;

		case GECKO_FUN_SPI_MOSI_LOC:
			loc = GECKO_GET_LOC(pins[i]);
#ifdef _SILICON_LABS_32B_SERIES_1
			base->ROUTEPEN |= USART_ROUTEPEN_TXPEN;
			base->ROUTELOC0 &= ~_USART_ROUTELOC0_TXLOC_MASK;
			base->ROUTELOC0 |= (loc << _USART_ROUTELOC0_TXLOC_SHIFT);
#elif defined _SILICON_LABS_32B_SERIES_0
			base->ROUTE |= (USART_ROUTE_TXPEN | (loc << 8));
#endif	/*_SILICON_LABS_32B_SERIES_1*/
			break;

		case GECKO_FUN_SPI_MISO_LOC:
#ifdef _SILICON_LABS_32B_SERIES_1
			loc = GECKO_GET_LOC(pins[i]);
			base->ROUTEPEN |= USART_ROUTEPEN_RXPEN;
			base->ROUTELOC0 &= ~_USART_ROUTELOC0_RXLOC_MASK;
			base->ROUTELOC0 |= (loc << _USART_ROUTELOC0_RXLOC_SHIFT);
#elif defined _SILICON_LABS_32B_SERIES_0
			base->ROUTE |= (USART_ROUTE_RXPEN | (loc << 8));
#endif	/*_SILICON_LABS_32B_SERIES_1*/
			break;

		case GECKO_FUN_SPI_CS_LOC:
#ifdef _SILICON_LABS_32B_SERIES_1
			loc = GECKO_GET_LOC(pins[i]);
			base->ROUTEPEN |= USART_ROUTEPEN_CSPEN;
			base->ROUTELOC0 &= ~_USART_ROUTELOC0_CSLOC_MASK;
			base->ROUTELOC0 |= (loc << _USART_ROUTELOC0_CSLOC_SHIFT);
#elif defined _SILICON_LABS_32B_SERIES_0
			base->ROUTE |= (USART_ROUTE_CSPEN | (loc << 8));
#endif	/*_SILICON_LABS_32B_SERIES_1*/
			break;
#endif /* CONFIG_SPI_GECKO */
		default:
			return -ENOTSUP;
		}
	}
	return 0;
}
