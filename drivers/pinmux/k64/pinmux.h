/* pinmux.h - Freescale K64 pinmux header */

/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Header file for Freescale K64 pin multiplexing.
 */

#ifndef __INCLUDE_PINMUX_K64_H
#define __INCLUDE_PINMUX_K64_H


#define K64_PINMUX_NUM_PINS		32		/* # of I/O pins per port */

/* Port Control Register offsets */

#define K64_PINMUX_CTRL_OFFSET(pin)	(pin * 4)

/*
 * The following pin settings match the K64 PORT module's
 * Pin Control Register bit fields.
 */

/*
 * Pin interrupt configuration:
 * At reset, interrupts are disabled for all pins.
 */

#define K64_PINMUX_INT_MASK			(0xF << 16)		/* interrupt config. */
#define K64_PINMUX_INT_DISABLE		(0x0 << 16)		/* disable interrupt */
#define K64_PINMUX_INT_LOW			(0x8 << 16)		/* active-low interrupt */
#define K64_PINMUX_INT_RISING		(0x9 << 16)		/* rising-edge interrupt */
#define K64_PINMUX_INT_FALLING		(0xA << 16)		/* falling-edge interrupt */
#define K64_PINMUX_INT_BOTH_EDGE	(0xB << 16)		/* either edge interrupt */
#define K64_PINMUX_INT_HIGH			(0xC << 16)		/* active-high interrupt */

/*
 * Pin function identification:
 * At reset, the setting for PTA0/1/2/3/4 is function 7;
 * the remaining pins are set to function 0.
 */

#define K64_PINMUX_ALT_MASK			(0x7 << 8)
#define K64_PINMUX_ALT_0			(0x0 << 8)
#define K64_PINMUX_ALT_1			(0x1 << 8)
#define K64_PINMUX_ALT_2			(0x2 << 8)
#define K64_PINMUX_ALT_3			(0x3 << 8)
#define K64_PINMUX_ALT_4			(0x4 << 8)
#define K64_PINMUX_ALT_5			(0x5 << 8)
#define K64_PINMUX_ALT_6			(0x6 << 8)
#define K64_PINMUX_ALT_7			K64_PINMUX_ALT_MASK

#define K64_PINMUX_FUNC_GPIO		K64_PINMUX_ALT_1
#define K64_PINMUX_FUNC_DISABLED	K64_PINMUX_ALT_0
#define K64_PINMUX_FUNC_ANALOG		K64_PINMUX_ALT_0
#define K64_PINMUX_FUNC_ETHERNET	K64_PINMUX_ALT_4

/*
 * Pin drive strength configuration, for output:
 * At reset, the setting for PTA0/1/2/3/4/5 is high drive strength;
 * the remaining pins are set to low drive strength.
 */

#define K64_PINMUX_DRV_STRN_MASK	(0x1 << 6)		/* drive strength select */
#define K64_PINMUX_DRV_STRN_LOW		(0x0 << 6)		/* low drive strength */
#define K64_PINMUX_DRV_STRN_HIGH	(0x1 << 6)		/* high drive strength */

/*
 * Pin open drain configuration, for output:
 * At reset, open drain is disabled for all pins.
 */

#define K64_PINMUX_OPEN_DRN_MASK	(0x1 << 5)		/* open drain enable */
#define K64_PINMUX_OPEN_DRN_DISABLE	(0x0 << 5)		/* disable open drain */
#define K64_PINMUX_OPEN_DRN_ENABLE	(0x1 << 5)		/* enable open drain */

/*
 * Pin slew rate configuration, for output:
 * At reset, fast slew rate is set for all pins.
 */

#define K64_PINMUX_SLEW_RATE_MASK	(0x1 << 2)	/* slew rate select */
#define K64_PINMUX_SLEW_RATE_FAST	(0x0 << 2)	/* fast slew rate */
#define K64_PINMUX_SLEW_RATE_SLOW	(0x1 << 2)	/* slow slew rate */

/*
 * Pin pull-up/pull-down configuration, for input:
 * At reset, the setting for PTA1/2/3/4/5 is pull-up; PTA0 is pull-down;
 * pull-up/pull-down is disabled for the remaining pins.
 */

#define K64_PINMUX_PULL_EN_MASK		(0x1 << 1)	/* pullup/pulldown enable */
#define K64_PINMUX_PULL_DISABLE		(0x0 << 1)	/* disable pullup/pulldown */
#define K64_PINMUX_PULL_ENABLE		(0x1 << 1)	/* enable pullup/pulldown */

#define K64_PINMUX_PULL_SEL_MASK	(0x1 << 0)	/* pullup/pulldown select */
#define K64_PINMUX_PULL_DN			(0x0 << 0)	/* select pulldown */
#define K64_PINMUX_PULL_UP			(0x1 << 0)	/* select pullup */


/*
 * Pin identification, by port and pin
 */

#define K64_PIN_PTA0	0
#define K64_PIN_PTA1	1
#define K64_PIN_PTA2	2
#define K64_PIN_PTA3	3
#define K64_PIN_PTA4	4
#define K64_PIN_PTA5	5
#define K64_PIN_PTA6	6
#define K64_PIN_PTA7	7
#define K64_PIN_PTA8	8
#define K64_PIN_PTA9	9
#define K64_PIN_PTA10	10
#define K64_PIN_PTA11	11
#define K64_PIN_PTA12	12
#define K64_PIN_PTA13	13
#define K64_PIN_PTA14	14
#define K64_PIN_PTA15	15
#define K64_PIN_PTA16	16
#define K64_PIN_PTA17	17
#define K64_PIN_PTA18	18
#define K64_PIN_PTA19	19
#define K64_PIN_PTA20	20
#define K64_PIN_PTA21	21
#define K64_PIN_PTA22	22
#define K64_PIN_PTA23	23
#define K64_PIN_PTA24	24
#define K64_PIN_PTA25	25
#define K64_PIN_PTA26	26
#define K64_PIN_PTA27	27
#define K64_PIN_PTA28	28
#define K64_PIN_PTA29	29
#define K64_PIN_PTA30	30
#define K64_PIN_PTA31	31

#define K64_PIN_PTB0	32
#define K64_PIN_PTB1	33
#define K64_PIN_PTB2	34
#define K64_PIN_PTB3	35
#define K64_PIN_PTB4	36
#define K64_PIN_PTB5	37
#define K64_PIN_PTB6	38
#define K64_PIN_PTB7	39
#define K64_PIN_PTB8	40
#define K64_PIN_PTB9	41
#define K64_PIN_PTB10	42
#define K64_PIN_PTB11	43
#define K64_PIN_PTB12	44
#define K64_PIN_PTB13	45
#define K64_PIN_PTB14	46
#define K64_PIN_PTB15	47
#define K64_PIN_PTB16	48
#define K64_PIN_PTB17	49
#define K64_PIN_PTB18	50
#define K64_PIN_PTB19	51
#define K64_PIN_PTB20	52
#define K64_PIN_PTB21	53
#define K64_PIN_PTB22	54
#define K64_PIN_PTB23	55
#define K64_PIN_PTB24	56
#define K64_PIN_PTB25	57
#define K64_PIN_PTB26	58
#define K64_PIN_PTB27	59
#define K64_PIN_PTB28	60
#define K64_PIN_PTB29	61
#define K64_PIN_PTB30	62
#define K64_PIN_PTB31	63

#define K64_PIN_PTC0	64
#define K64_PIN_PTC1	65
#define K64_PIN_PTC2	66
#define K64_PIN_PTC3	67
#define K64_PIN_PTC4	68
#define K64_PIN_PTC5	69
#define K64_PIN_PTC6	70
#define K64_PIN_PTC7	71
#define K64_PIN_PTC8	72
#define K64_PIN_PTC9	73
#define K64_PIN_PTC10	74
#define K64_PIN_PTC11	75
#define K64_PIN_PTC12	76
#define K64_PIN_PTC13	77
#define K64_PIN_PTC14	78
#define K64_PIN_PTC15	79
#define K64_PIN_PTC16	80
#define K64_PIN_PTC17	81
#define K64_PIN_PTC18	82
#define K64_PIN_PTC19	83
#define K64_PIN_PTC20	84
#define K64_PIN_PTC21	85
#define K64_PIN_PTC22	86
#define K64_PIN_PTC23	87
#define K64_PIN_PTC24	88
#define K64_PIN_PTC25	89
#define K64_PIN_PTC26	90
#define K64_PIN_PTC27	91
#define K64_PIN_PTC28	92
#define K64_PIN_PTC29	93
#define K64_PIN_PTC30	94
#define K64_PIN_PTC31	95

#define K64_PIN_PTD0	96
#define K64_PIN_PTD1	97
#define K64_PIN_PTD2	98
#define K64_PIN_PTD3	99
#define K64_PIN_PTD4	100
#define K64_PIN_PTD5	101
#define K64_PIN_PTD6	102
#define K64_PIN_PTD7	103
#define K64_PIN_PTD8	104
#define K64_PIN_PTD9	105
#define K64_PIN_PTD10	106
#define K64_PIN_PTD11	107
#define K64_PIN_PTD12	108
#define K64_PIN_PTD13	109
#define K64_PIN_PTD14	110
#define K64_PIN_PTD15	111
#define K64_PIN_PTD16	112
#define K64_PIN_PTD17	113
#define K64_PIN_PTD18	114
#define K64_PIN_PTD19	115
#define K64_PIN_PTD20	116
#define K64_PIN_PTD21	117
#define K64_PIN_PTD22	118
#define K64_PIN_PTD23	119
#define K64_PIN_PTD24	120
#define K64_PIN_PTD25	121
#define K64_PIN_PTD26	122
#define K64_PIN_PTD27	123
#define K64_PIN_PTD28	124
#define K64_PIN_PTD29	125
#define K64_PIN_PTD30	126
#define K64_PIN_PTD31	127

#define K64_PIN_PTE0	128
#define K64_PIN_PTE1	129
#define K64_PIN_PTE2	130
#define K64_PIN_PTE3	131
#define K64_PIN_PTE4	132
#define K64_PIN_PTE5	133
#define K64_PIN_PTE6	134
#define K64_PIN_PTE7	135
#define K64_PIN_PTE8	136
#define K64_PIN_PTE9	137
#define K64_PIN_PTE10	138
#define K64_PIN_PTE11	139
#define K64_PIN_PTE12	140
#define K64_PIN_PTE13	141
#define K64_PIN_PTE14	142
#define K64_PIN_PTE15	143
#define K64_PIN_PTE16	144
#define K64_PIN_PTE17	145
#define K64_PIN_PTE18	146
#define K64_PIN_PTE19	147
#define K64_PIN_PTE20	148
#define K64_PIN_PTE21	149
#define K64_PIN_PTE22	150
#define K64_PIN_PTE23	151
#define K64_PIN_PTE24	152
#define K64_PIN_PTE25	153
#define K64_PIN_PTE26	154
#define K64_PIN_PTE27	155
#define K64_PIN_PTE28	156
#define K64_PIN_PTE29	157
#define K64_PIN_PTE30	158
#define K64_PIN_PTE31	159

int _fsl_k64_set_pin(uint32_t pin_id, uint32_t func);

int _fsl_k64_get_pin(uint32_t pin_id, uint32_t *func);


#endif /* __INCLUDE_PINMUX_K64_H */
