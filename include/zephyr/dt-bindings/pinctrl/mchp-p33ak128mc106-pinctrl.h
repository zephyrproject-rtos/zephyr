/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_P33AK128MC106_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_P33AK128MC106_PINCTRL_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Masks & Shifts */
#define DSPIC33_PORT_MASK  0x3U /* 2 bits for 0-3 */
#define DSPIC33_PORT_SHIFT 30

#define DSPIC33_PIN_MASK  0x3FU /* 6 bits for 0-47 */
#define DSPIC33_PIN_SHIFT 24

#define DSPIC33_FUNC_MASK  0xFFFFFFU /* 24 bits for 0-511 */
#define DSPIC33_FUNC_SHIFT 0

/* Encode: pack port, pin, function */
#define DSPIC33_PINMUX(port, pin, func)                                                            \
	((((port) & DSPIC33_PORT_MASK) << DSPIC33_PORT_SHIFT) |                                    \
	 (((pin) & DSPIC33_PIN_MASK) << DSPIC33_PIN_SHIFT) |                                       \
	 (((func) & DSPIC33_FUNC_MASK) << DSPIC33_FUNC_SHIFT))

/* Decode: extract port, pin, function */
#define DSPIC33_PINMUX_PORT(pinmux) (((pinmux) >> DSPIC33_PORT_SHIFT) & DSPIC33_PORT_MASK)
#define DSPIC33_PINMUX_PIN(pinmux)  (((pinmux) >> DSPIC33_PIN_SHIFT) & DSPIC33_PIN_MASK)
#define DSPIC33_PINMUX_FUNC(pinmux) (((pinmux) >> DSPIC33_FUNC_SHIFT) & DSPIC33_FUNC_MASK)

#define OFFSET_RPOR  0x3780
#define OFFSET_RPIN  0x3704
#define OFFSET_LATCH 0x0004
#define OFFSET_TRIS  0x0008
#define OFFSET_ANSEL 0x3440

/* Port definitions */
#define PORT_A 0
#define PORT_B 1
#define PORT_C 2
#define PORT_D 3

/* Input Function Macros (for RPINRx register configuration) */
#define INT1     0x3905
#define INT2     0x3906
#define INT3     0x3907
#define INT4     0x3908
#define T1CK     0x3909
#define REFI1    0x390A
#define REFI2    0x390B
#define ICM1     0x390C
#define ICM2     0x390D
#define ICM3     0x390E
#define ICM4     0x390F
#define OCFA     0x3918
#define OCFB     0x3919
#define OCFC     0x391A
#define OCFD     0x391B
#define PCI8     0x391C
#define PCI9     0x391D
#define PCI10    0x391E
#define PCI11    0x391F
#define QEIA1    0x3920
#define QEIB1    0x3921
#define QEINDX1  0x3922
#define QEIHOM1  0x3923
#define U1RX     0x3928
#define U1DSR    0x3929
#define U2RX     0x392A
#define U2DSR    0x392B
#define SDI1     0x392C
#define SCK1IN   0x392D
#define SS1      0x392E
#define SDI2     0x3930
#define SCK2IN   0x3931
#define SS2      0x3932
#define U3RX     0x3938
#define U3DSR    0x3939
#define SENT1    0x393E
#define SENT2    0x393F
#define SDI3     0x3940
#define SCK3IN   0x3941
#define SS3      0x3942
#define PCI12    0x3948
#define PCI13    0x3949
#define PCI14    0x394A
#define PCI15    0x394B
#define PCI16    0x394C
#define PCI17    0x394D
#define PCI18    0x394E
#define ADTRIG31 0x394F
#define BISS1SL  0x3950
#define BISS1GS  0x3951
#define CLCINA   0x3954
#define CLCINB   0x3955
#define CLCINC   0x3956
#define CLCIND   0x3957
#define U1CTS    0x3958
#define U2CTS    0x3959
#define U3CTS    0x395A

/* Output Function Macros (for RPnR register configuration) */
#define DEFAULT_PORT 0x00
#define PWM1H        0x01
#define PWM1L        0x02
#define PWM2H        0x03
#define PWM2L        0x04
#define PWM3H        0x05
#define PWM3L        0x06
#define PWM4H        0x07
#define PWM4L        0x08
#define U1TX         0x09
#define U1RTS        0x0A
#define U2TX         0x0B
#define U2RTS        0x0C
#define SDO1         0x0D
#define SCK1OUT      0x0E
#define SS1OUT       0x0F
#define SDO2         0x10
#define SCK2OUT      0x11
#define SS2OUT       0x12
#define SDO3         0x13
#define SCK3OUT      0x14
#define SS3OUT       0x15
#define REFO1        0x16
#define REFO2        0x17
#define OCM1         0x18
#define OCM2         0x19
#define OCM3         0x1A
#define OCM4         0x1B
#define CMP1         0x20
#define CMP2         0x21
#define CMP3         0x22
#define U3TX         0x24
#define U3RTS        0x25
#define PEVTA        0x2B
#define PEVTB        0x2C
#define QEICMP1      0x2D
#define CLC1OUT      0x2F
#define CLC2OUT      0x30
#define PEVTC        0x33
#define PEVTD        0x34
#define PEVTE        0x35
#define PEVTF        0x36
#define PTG_TRIG24   0x37
#define PTG_TRIG25   0x38
#define SENT1OUT     0x39
#define SENT2OUT     0x3A
#define BISS1MO      0x3F
#define BISS1MA      0x40
#define CLC3OUT      0x41
#define CLC4OUT      0x42
#define U1DTRn       0x43
#define U2DTRn       0x44
#define U3DTRn       0x45

#endif
