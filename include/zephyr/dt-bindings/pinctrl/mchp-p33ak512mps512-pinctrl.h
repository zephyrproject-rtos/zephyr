/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_P33AK512MPS512_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_P33AK512MPS512_PINCTRL_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Masks & Shifts */
#define DSPIC33_PORT_MASK  0x7U /* 2 bits for 0-3 */
#define DSPIC33_PORT_SHIFT 29

#define DSPIC33_PIN_MASK  0x0FU /* 4 bits for 0-11 */
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

#define OFFSET_RPOR  0x3150
#define OFFSET_RPIN  0x30D4
#define OFFSET_LATCH 0x0004
#define OFFSET_TRIS  0x0008
#define OFFSET_ANSEL 0x3440

/* Port definitions */
#define PORT_A 0
#define PORT_B 1
#define PORT_C 2
#define PORT_D 3
#define PORT_E 4
#define PORT_F 5
#define PORT_G 6
#define PORT_H 7

/* Input Function Macros (for RPINRx register configuration) */
#define INT1     0x32D5
#define INT2     0x32D6
#define INT3     0x32D7
#define INT4     0x32D8
#define T1CK     0x32D9
#define T2CK     0x32DA
#define T3CK     0x32DB
#define TCKI1    0x32DC
#define ICM1     0x32DD
#define TCKI2    0x32DE
#define ICM2     0x32DF
#define TCKI3    0x32E0
#define ICM3     0x32E1
#define TCKI4    0x32E2
#define ICM4     0x32E3
#define TCKI5    0x32E4
#define ICM5     0x32E5
#define TCKI6    0x32E6
#define ICM6     0x32E7
#define TCKI7    0x32E8
#define ICM7     0x32E9
#define TCKI8    0x32EA
#define ICM8     0x32EB
#define TCKI9    0x32EC
#define ICM9     0x32ED
#define RTCCTMP  0x32EE
#define RTCCPCLK 0x32EF
#define OCFAR    0x32F0
#define OCFBR    0x32F1
#define OCFCR    0x32F2
#define OCFDR    0x32F3
#define PCI8     0x32F4
#define PCI9     0x32F5
#define PCI10    0x32F6
#define PCI11    0x32F7
#define QEA1     0x32F8
#define QEB1     0x32F9
#define INDX1    0x32FA
#define HOME1    0x32FB
#define QEA2     0x32FC
#define QEB2     0x32FD
#define INDX2    0x32FE
#define HOME2    0x32FF
#define QEA3     0x3300
#define QEB3     0x3301
#define INDX3    0x3302
#define HOME3    0x3303
#define QEA4     0x3304
#define QEB4     0x3305
#define INDX4    0x3306
#define HOME4    0x3307
#define U1RX     0x3308
#define U1DSR    0x3309
#define U2RX     0x330A
#define U2DSR    0x330B
#define U3RX     0x330C
#define U3DSR    0x330D
#define SDI1     0x330E
#define SDK1     0x330F
#define SS1      0x3310
#define SDI2     0x3311
#define SDK2     0x3312
#define SS2      0x3313
#define SDI3     0x3314
#define SDK3     0x3315
#define SS3      0x3316
#define SDI4     0x3317
#define SDK4     0x3318
#define SS4      0x3319
#define CAN1RX   0x331A
#define CAN2RX   0x331B
#define SENT1    0x331C
#define SENT2    0x331D
#define REFI1    0x331E
#define REFI2    0x331F
#define PCI12    0x3320
#define PCI13    0x3321
#define PCI14    0x3322
#define PCI15    0x3323
#define PCI16    0x3324
#define PCI17    0x3325
#define PCI18    0x3326
#define CLCINA   0x3327
#define CLCINB   0x3328
#define CLCINC   0x3329
#define CLCIND   0x332A
#define CLCINE   0x332B
#define CLCINF   0x332C
#define CLCING   0x332D
#define CLCINH   0x332E
#define CLCINI   0x332F
#define CLCINJ   0x3330
#define ADTRG    0x3331
#define U1CTS    0x3332
#define U2CTS    0x3333
#define U3CTS    0x3334
#define BISS1SL  0x3335
#define BISS1GS  0x3336
#define IOM0     0x3337
#define IOM1     0x3338
#define IOM2     0x3339
#define IOM3     0x333A
#define IOM4     0x333B
#define IOM5     0x333C
#define IOM6     0x333D
#define IOM7     0x333E
#define PCI19    0x333F
#define PCI20    0x3340
#define PCI21    0x3341
#define PCI22    0x3342


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
#define PWM5L        0x09
#define PWM5H        0x0A
#define PWM6L        0x0B
#define PWM6H        0x0C
#define PWM7L        0x0D
#define PWM7H        0x0E
#define PWM8L        0x0F
#define PWM8H        0x10
#define CAN1TX       0x11
#define CAN2TX       0x12
#define U1TX         0x13
#define U1RTS        0x14
#define U2TX         0x15
#define U2RTS        0x16
#define U3TX         0x17
#define U3RTS        0x18
#define SDO1         0x19
#define SCK1         0x1A
#define SS1OUT       0x1B
#define SDO2         0x1C
#define SCK2         0x1D
#define SS2OUT       0x1E
#define SDO3         0x1F
#define SCK3         0x20
#define SS3OUT       0x21
#define SDO4         0x22
#define SCK4         0x23
#define SS4OUT       0x24
#define REFO1        0x25
#define REFO2        0x26
#define OCM1         0x27
#define OCM2         0x28
#define OCM3         0x29
#define OCM4         0x2A
#define OCM5         0x2B
#define OCM6         0x2C
#define OCM7         0x2D
#define OCM8         0x2E
#define MCCP9A       0x2F
#define MCCP9B       0x30
#define MCCP9C       0x31
#define MCCP9D       0x32
#define MCCP9E       0x33
#define MCCP9F       0x34
#define CMP1         0x35
#define CMP2         0x36
#define CMP3         0x37
#define CMP4         0x38
#define CMP5         0x39
#define CMP6         0x3A
#define CMP7         0x3B   
#define CMP8         0x3C
#define PEVTA        0x3D
#define PEVTB        0x3E
#define PEVTC        0x3F
#define PEVTD        0x40
#define QEICMP1      0x41
#define QEICMP2      0x42
#define QEICMP3      0x43
#define QEICMP4      0x44
#define CLC1OUT      0x45
#define CLC2OUT      0x46
#define CLC3OUT      0x47
#define CLC4OUT      0x48
#define CLC5OUT      0x49
#define CLC6OUT      0x4A
#define CLC7OUT      0x4B
#define CLC8OUT      0x4C
#define CLC9OUT      0x4D
#define CLC10OUT     0x4E 
#define PTGTRG24     0x4F 
#define PTGTRG25     0x50   
#define SENT1OUT     0x51    
#define SENT2OUT     0x52     
#define BISSMO1      0x53    
#define BISSMA1      0x54    
#define U1DTR        0x55    
#define U2DTR        0x56
#define U3DTR        0x57
#define APWM1H       0x58
#define APWM1L       0x59
#define APWM2H       0x5A
#define APWM2L       0x5B
#define APWM3H       0x5C
#define APWM3L       0x5D
#define APWM4H       0x5E
#define APWM4L       0x5F
#define APEVTA       0x60
#define APEVTB       0x61

#endif
