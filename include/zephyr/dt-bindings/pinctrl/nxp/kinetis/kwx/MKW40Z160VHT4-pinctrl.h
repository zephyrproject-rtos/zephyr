/*
 * Copyright (c) 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is handcoded based on what pin configurations are actually
 * used by boards in Zephyr.  At this time that is only the Hexiwear KW40Z.
 */

#ifndef _ZEPHYR_DTS_BINDING_MKW40Z160VHT4_
#define _ZEPHYR_DTS_BINDING_MKW40Z160VHT4_

#define KINETIS_MUX(port, pin, mux)		\
	(((((port) - 'A') & 0xF) << 28) |	\
	(((pin) & 0x3F) << 22) |		\
	(((mux) & 0x7) << 8))


#define ADC0_SE1_PTB1 KINETIS_MUX('B', 1, 0)
#define	GPIOB_PTB1 KINETIS_MUX('B', 1, 0)
#define GPIOC_PTC6 KINETIS_MUX('C', 6, 1)
#define UART0_RX_PTC6 KINETIS_MUX('C', 6, 4)
#define GPIOC_PTC7 KINETIS_MUX('C', 7, 1)
#define UART0_TX_PTC7 KINETIS_MUX('C', 7, 4)

#endif
