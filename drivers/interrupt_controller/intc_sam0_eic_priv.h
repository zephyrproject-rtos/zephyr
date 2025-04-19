/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_SAM0_EIC_PRIV_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_SAM0_EIC_PRIV_H_

#include <errno.h>
#include <zephyr/types.h>
#include <soc.h>

/* MCLK registers */

#if defined(CONFIG_SOC_SERIES_SAMD20) || defined(CONFIG_SOC_SERIES_SAMD21) ||                      \
	defined(CONFIG_SOC_SERIES_SAMR21)

#define APBAMASK_OFFSET  0x18
#define APBAMASK_EIC_BIT 6

#else

#define APBAMASK_OFFSET 0x14

#if defined(CONFIG_SOC_SERIES_SAML21) || defined(CONFIG_SOC_SERIES_SAMR34) ||                      \
	defined(CONFIG_SOC_SERIES_SAMR35)
#define APBAMASK_EIC_BIT 9
#else
#define APBAMASK_EIC_BIT 10
#endif

#endif

/* GCLK registers */

#define CLKCTRL_OFFSET 0x02
#define PCHCTRL_OFFSET 0x80

#if defined(CONFIG_SOC_SERIES_SAMD20) || defined(CONFIG_SOC_SERIES_SAMD21) ||                      \
	defined(CONFIG_SOC_SERIES_SAMR21)

#define CLKCTRL_GEN_GCLK0 0
#define CLKCTRL_CLKEN     BIT(14)

#if defined(CONFIG_SOC_SERIES_SAMD20)
#define CLKCTRL_ID_EIC 3
#else
#define CLKCTRL_ID_EIC 5
#endif

#else

#define PCHCTRL_GEN_GCLK0 0
#define PCHCTRL_CHEN      BIT(6)

#if defined(CONFIG_SOC_SERIES_SAMD20) || defined(CONFIG_SOC_SERIES_SAMD21)

#define GCLK_ID 2

#elif defined(CONFIG_SOC_SERIES_SAML21) || defined(CONFIG_SOC_SERIES_SAMR34) ||                    \
	defined(CONFIG_SOC_SERIES_SAMR35)

#define GCLK_ID 3

#else
#define GCLK_ID 4
#endif

#endif

/* EIC registers */

#define CFG_FILTEN0     8
#define CFG_SENSE0_BOTH 3
#define CFG_SENSE0_FALL 2
#define CFG_SENSE0_HIGH 4
#define CFG_SENSE0_LOW  5
#define CFG_SENSE0_RISE 1

#define EIC_EXTINT_NUM 16
#define EIC_ENABLE_BIT 1

#if !defined(CONFIG_SOC_SERIES_SAMD20) && !defined(CONFIG_SOC_SERIES_SAMD21) &&                    \
	!defined(CONFIG_SOC_SERIES_SAMR21)

#define CFG_OFFSET      0x1C
#define INTENCLR_OFFSET 0x0C
#define INTENSET_OFFSET 0x10
#define INTFLAG_OFFSET  0x14
#define SYNCBUSY_OFFSET 0x04

#else /* SOC_SERIES_SAMD20 || SOC_SERIES_SAMD21 || SOC_SERIES_SAMR21 */

#define CFG_OFFSET      0x18
#define INTENCLR_OFFSET 0x08
#define INTENSET_OFFSET 0x0C
#define INTFLAG_OFFSET  0x10
#define STATUS_OFFSET   0x01

#define SYNCBUSY_BIT 7

#endif

/*
 * Unfortunately the ASF headers define the EIC mappings somewhat painfully:
 * the macros have both the port letter and are only defined if that pin
 * has an EIC channel.  So we can't just use a macro expansion here, because
 * some of them might be undefined for a port and we can't test for another
 * macro definition inside a macro.
 */

#if defined(CONFIG_SOC_SAMC20E15A) || defined(CONFIG_SOC_SAMC20E16A) ||                            \
	defined(CONFIG_SOC_SAMC20E17A) || defined(CONFIG_SOC_SAMC20E18A) ||                        \
	defined(CONFIG_SOC_SAMC21E15A) || defined(CONFIG_SOC_SAMC21E16A) ||                        \
	defined(CONFIG_SOC_SAMC21E17A) || defined(CONFIG_SOC_SAMC21E18A) ||                        \
	defined(CONFIG_SOC_SAMD20E14) || defined(CONFIG_SOC_SAMD20E15) ||                          \
	defined(CONFIG_SOC_SAMD20E16) || defined(CONFIG_SOC_SAMD20E17) ||                          \
	defined(CONFIG_SOC_SAMD20E18) || defined(CONFIG_SOC_SAMD21E15A) ||                         \
	defined(CONFIG_SOC_SAMD21E16A) || defined(CONFIG_SOC_SAMD21E17A) ||                        \
	defined(CONFIG_SOC_SAMD21E18A) || defined(CONFIG_SOC_PIC32CM6408MC00032) ||                \
	defined(CONFIG_SOC_PIC32CM1216MC00032)

#define EIC_PORTA_EXTINT_BITS 0xDBCFCEFF
#define EIC_PORTB_EXTINT_BITS 0x00000000
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 1

#elif defined(CONFIG_SOC_SAMC20G15A) || defined(CONFIG_SOC_SAMC20G16A) ||                          \
	defined(CONFIG_SOC_SAMC20G17A) || defined(CONFIG_SOC_SAMC20G18A) ||                        \
	defined(CONFIG_SOC_SAMC21G15A) || defined(CONFIG_SOC_SAMC21G16A) ||                        \
	defined(CONFIG_SOC_SAMC21G17A) || defined(CONFIG_SOC_SAMC21G18A) ||                        \
	defined(CONFIG_SOC_SAMD20G14) || defined(CONFIG_SOC_SAMD20G15) ||                          \
	defined(CONFIG_SOC_SAMD20G16) || defined(CONFIG_SOC_SAMD20G17) ||                          \
	defined(CONFIG_SOC_SAMD20G18) || defined(CONFIG_SOC_SAMD21G15A) ||                         \
	defined(CONFIG_SOC_SAMD21G16A) || defined(CONFIG_SOC_SAMD21G17A) ||                        \
	defined(CONFIG_SOC_SAMD21G18A) || defined(CONFIG_SOC_PIC32CM6408MC00048) ||                \
	defined(CONFIG_SOC_PIC32CM1216MC00048)

#define EIC_PORTA_EXTINT_BITS 0xDBFFFEFF
#define EIC_PORTB_EXTINT_BITS 0x00C00F0C
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#elif defined(CONFIG_SOC_SAMC20J15A) || defined(CONFIG_SOC_SAMC20J16A) ||                          \
	defined(CONFIG_SOC_SAMC20J17A) || defined(CONFIG_SOC_SAMC20J18A) ||                        \
	defined(CONFIG_SOC_SAMC21J15A) || defined(CONFIG_SOC_SAMC21J16A) ||                        \
	defined(CONFIG_SOC_SAMC21J17A) || defined(CONFIG_SOC_SAMC21J18A) ||                        \
	defined(CONFIG_SOC_SAMD20J14) || defined(CONFIG_SOC_SAMD20J15) ||                          \
	defined(CONFIG_SOC_SAMD20J16) || defined(CONFIG_SOC_SAMD20J17) ||                          \
	defined(CONFIG_SOC_SAMD20J18) || defined(CONFIG_SOC_SAMD21J15A) ||                         \
	defined(CONFIG_SOC_SAMD21J16A) || defined(CONFIG_SOC_SAMD21J17A) ||                        \
	defined(CONFIG_SOC_SAMD21J18A)

#define EIC_PORTA_EXTINT_BITS 0xDBFFFEFF
#define EIC_PORTB_EXTINT_BITS 0xC0C3FFFF
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#elif defined(CONFIG_SOC_SAMD51J18A) || defined(CONFIG_SOC_SAMD51J19A) ||                          \
	defined(CONFIG_SOC_SAMD51J20A) || defined(CONFIG_SOC_SAME51J18A) ||                        \
	defined(CONFIG_SOC_SAME51J19A) || defined(CONFIG_SOC_SAME51J20A) ||                        \
	defined(CONFIG_SOC_SAME53J18A) || defined(CONFIG_SOC_SAME53J19A) ||                        \
	defined(CONFIG_SOC_SAME53J20A) || defined(CONFIG_SOC_SAML21J16B) ||                        \
	defined(CONFIG_SOC_SAML21J17B) || defined(CONFIG_SOC_SAML21J17BU) ||                       \
	defined(CONFIG_SOC_SAML21J18B) || defined(CONFIG_SOC_SAML21J18BU)

#define EIC_PORTA_EXTINT_BITS 0xCBFFFEFF
#define EIC_PORTB_EXTINT_BITS 0xC0C3FFFF
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#elif defined(CONFIG_SOC_SAMD51N19A) || defined(CONFIG_SOC_SAMD51N20A) ||                          \
	defined(CONFIG_SOC_SAME51N19A) || defined(CONFIG_SOC_SAME51N20A) ||                        \
	defined(CONFIG_SOC_SAME53N19A) || defined(CONFIG_SOC_SAME53N20A) ||                        \
	defined(CONFIG_SOC_SAME54N19A) || defined(CONFIG_SOC_SAME54N20A)

#define EIC_PORTA_EXTINT_BITS 0xCBFFFEFF
#define EIC_PORTB_EXTINT_BITS 0xC3FFFFFF
#define EIC_PORTC_EXTINT_BITS 0x1F3FFCEF
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 3

#elif defined(CONFIG_SOC_SAMR34J16B) || defined(CONFIG_SOC_SAMR34J17B) ||                          \
	defined(CONFIG_SOC_SAMR34J18B) || defined(CONFIG_SOC_SAMR35J16B) ||                        \
	defined(CONFIG_SOC_SAMR35J17B) || defined(CONFIG_SOC_SAMR35J18B)

#define EIC_PORTA_EXTINT_BITS 0xDBCFFEF3
#define EIC_PORTB_EXTINT_BITS 0xC0C3800D
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#elif defined(CONFIG_SOC_SAMD51G18A) || defined(CONFIG_SOC_SAMD51G19A) ||                          \
	defined(CONFIG_SOC_SAML21G16B) || defined(CONFIG_SOC_SAML21G17B) ||                        \
	defined(CONFIG_SOC_SAML21G18B)

#define EIC_PORTA_EXTINT_BITS 0xCBFFFEFF
#define EIC_PORTB_EXTINT_BITS 0x00C00F0C
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#elif defined(CONFIG_SOC_SAML21E15B) || defined(CONFIG_SOC_SAML21E16B) ||                          \
	defined(CONFIG_SOC_SAML21E17B) || defined(CONFIG_SOC_SAML21E18B)

#define EIC_PORTA_EXTINT_BITS 0xCBCFCEFF
#define EIC_PORTB_EXTINT_BITS 0x00000000
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 1

#elif defined(CONFIG_SOC_SAMD51P19A) || defined(CONFIG_SOC_SAMD51P20A) ||                          \
	defined(CONFIG_SOC_SAME54P19A) || defined(CONFIG_SOC_SAME54P20A)

#define EIC_PORTA_EXTINT_BITS 0xCBFFFEFF
#define EIC_PORTB_EXTINT_BITS 0xFFFFFFFF
#define EIC_PORTC_EXTINT_BITS 0xDFFFFCFF
#define EIC_PORTD_EXTINT_BITS 0x00301F03

#define NUM_PORT_GROUPS 4

#elif defined(CONFIG_SOC_SAMD20G17U) || defined(CONFIG_SOC_SAMD20G18U) ||                          \
	defined(CONFIG_SOC_SAMD21G17AU) || defined(CONFIG_SOC_SAMD21G18AU)

#define EIC_PORTA_EXTINT_BITS 0xDBFFFEFF
#define EIC_PORTB_EXTINT_BITS 0x0000031C
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#elif defined(CONFIG_SOC_SAMC20J17AU) || defined(CONFIG_SOC_SAMC20J18AU) ||                        \
	defined(CONFIG_SOC_SAMC21J17AU) || defined(CONFIG_SOC_SAMC21J18AU)

#define EIC_PORTA_EXTINT_BITS 0xDBFFFEFF
#define EIC_PORTB_EXTINT_BITS 0x00C0FF0F
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#elif defined(CONFIG_SOC_SAMC20N17A) || defined(CONFIG_SOC_SAMC20N18A) ||                          \
	defined(CONFIG_SOC_SAMC21N17A) || defined(CONFIG_SOC_SAMC21N18A)

#define EIC_PORTA_EXTINT_BITS 0xDBFFFEFF
#define EIC_PORTB_EXTINT_BITS 0xC3FFFFFF
#define EIC_PORTC_EXTINT_BITS 0x1F3FFFEF
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 3

#elif defined(CONFIG_SOC_SAMR21E16A) || defined(CONFIG_SOC_SAMR21E17A) ||                          \
	defined(CONFIG_SOC_SAMR21E18A)

#define EIC_PORTA_EXTINT_BITS 0xDB1FCEC0
#define EIC_PORTB_EXTINT_BITS 0xC003C301
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#elif defined(CONFIG_SOC_SAMR21G16A) || defined(CONFIG_SOC_SAMR21G17A) ||                          \
	defined(CONFIG_SOC_SAMR21G18A)

#define EIC_PORTA_EXTINT_BITS 0xDBDFFEF3
#define EIC_PORTB_EXTINT_BITS 0xC0C3C30D
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#elif defined(CONFIG_SOC_SAMR21E19A)

#define EIC_PORTA_EXTINT_BITS 0xDBDFDEC1
#define EIC_PORTB_EXTINT_BITS 0xC0C3C301
#define EIC_PORTC_EXTINT_BITS 0x00000000
#define EIC_PORTD_EXTINT_BITS 0x00000000

#define NUM_PORT_GROUPS 2

#else

#error "Undefined EIC_PORTx_EXTINT_BITS macros!"

#endif

#if defined(CONFIG_SOC_SERIES_SAMD51) || defined(CONFIG_SOC_SERIES_SAME51) ||                      \
	defined(CONFIG_SOC_SERIES_SAME53) || defined(CONFIG_SOC_SERIES_SAME54)

#define EIC_PORTA_EXTINT_NUM                                                                       \
	{                                                                                          \
		0, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8,   \
			9, 0, 11, 8, 0, 14, 15                                                     \
	}
#define EIC_PORTC_EXTINT_NUM                                                                       \
	{                                                                                          \
		0, 1, 2, 3, 4, 5, 6, 9, 0, 1, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8,   \
			9, 10, 11, 12, 0, 14, 15                                                   \
	}

#else

#define EIC_PORTA_EXTINT_NUM                                                                       \
	{                                                                                          \
		0, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 12,  \
			13, 0, 15, 8, 0, 10, 11                                                    \
	}
#define EIC_PORTC_EXTINT_NUM                                                                       \
	{                                                                                          \
		8, 9, 10, 11, 4, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 6, 7,   \
			0, 1, 2, 3, 4, 0, 14, 15                                                   \
	}

#endif /* SOC_SERIES_SAMD51 || SOC_SERIES_SAME51 || SOC_SERIES_SAME53 || SOC_SERIES_SAME54 */

#define EIC_PORTB_EXTINT_NUM                                                                       \
	{                                                                                          \
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8,   \
			9, 12, 13, 14, 15, 14, 15                                                  \
	}
#define EIC_PORTD_EXTINT_NUM                                                                       \
	{                                                                                          \
		0, 1, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0, 0, 10, 11, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0                                                              \
	}

static const uint8_t sam0_eic_porta_channel[] = EIC_PORTA_EXTINT_NUM;
#if EIC_PORTB_EXTINT_BITS
static const uint8_t sam0_eic_portb_channel[] = EIC_PORTB_EXTINT_NUM;
#endif
#if EIC_PORTC_EXTINT_BITS
static const uint8_t sam0_eic_portc_channel[] = EIC_PORTC_EXTINT_NUM;
#endif
#if EIC_PORTD_EXTINT_BITS
static const uint8_t sam0_eic_portd_channel[] = EIC_PORTD_EXTINT_NUM;
#endif

static inline int sam0_eic_map_to_line(int port, int pin)
{
	int ch = -ENOTSUP;

	switch (port) {
	case 0:
		if (IS_BIT_SET(EIC_PORTA_EXTINT_BITS, pin)) {
			ch = sam0_eic_porta_channel[pin];
		}
		break;
#if EIC_PORTB_EXTINT_BITS
	case 1:
		if (IS_BIT_SET(EIC_PORTB_EXTINT_BITS, pin)) {
			ch = sam0_eic_portb_channel[pin];
		}
		break;
#endif
#if EIC_PORTC_EXTINT_BITS
	case 2:
		if (IS_BIT_SET(EIC_PORTC_EXTINT_BITS, pin)) {
			ch = sam0_eic_portc_channel[pin];
		}
		break;
#endif
#if EIC_PORTD_EXTINT_BITS
	case 3:
		if (IS_BIT_SET(EIC_PORTD_EXTINT_BITS, pin)) {
			ch = sam0_eic_portd_channel[pin];
		}
		break;
#endif
	}

	return ch;
}

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_SAM0_EIC_PRIV_H_ */
