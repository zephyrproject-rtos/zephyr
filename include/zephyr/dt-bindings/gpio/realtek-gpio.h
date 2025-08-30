/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_

/** Enable input detect */
#define RTS5912_GPIO_INDETEN  BIT(8)
/** Set pin driving current */
#define RTS5912_GPIO_OUTDRV   BIT(9)
/** Set GPIO slew rate */
#define RTS5912_GPIO_SLEWRATE BIT(10)
/** Enable Schmitt-trigger */
#define RTS5912_GPIO_SCHEN    BIT(11)

#define RTS5912_GPIO_VOLTAGE_POS     12
#define RTS5912_GPIO_VOLTAGE_MASK    GENMASK(13, 12)
/** Set pin at the default voltage level */
#define RTS5912_GPIO_VOLTAGE_DEFAULT (0U << RTS5912_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 1.8 V */
#define RTS5912_GPIO_VOLTAGE_1V8     (1U << RTS5912_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 3.3 V */
#define RTS5912_GPIO_VOLTAGE_3V3     (2U << RTS5912_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 5.0 V */
#define RTS5912_GPIO_VOLTAGE_5V0     (3U << RTS5912_GPIO_VOLTAGE_POS)

/** Multi function */
#define RTS5912_GPIO_MFCTRL_POS  14
#define RTS5912_GPIO_MFCTRL_MASK GENMASK(15, 14)
/** 0x00:Function 0 0x01: Function 1 0x02: Function 2 0x03: Function 3 */
#define RTS5912_GPIO_MFCTRL_0    (0U << RTS5912_GPIO_MFCTRL_POS)
#define RTS5912_GPIO_MFCTRL_1    (1U << RTS5912_GPIO_MFCTRL_POS)
#define RTS5912_GPIO_MFCTRL_2    (2U << RTS5912_GPIO_MFCTRL_POS)
#define RTS5912_GPIO_MFCTRL_3    (3U << RTS5912_GPIO_MFCTRL_POS)
/** Interrupt Mask since rts5912 does not support GPIO_INT_LEVELS_LOGICAL*/
#define RTS5912_GPIO_INTR_MASK   (1U << 21 | 1U << 22 | 1U << 24 | 1U << 25 | 1U << 26)

/** GPIO number map between spec and zephyr */
#define RTS5912_GPIO_NUM_000 &gpioa 0
#define RTS5912_GPIO_NUM_001 &gpioa 1
#define RTS5912_GPIO_NUM_002 &gpioa 2
#define RTS5912_GPIO_NUM_003 &gpioa 3
#define RTS5912_GPIO_NUM_004 &gpioa 4
#define RTS5912_GPIO_NUM_009 &gpioa 9
#define RTS5912_GPIO_NUM_013 &gpioa 13
#define RTS5912_GPIO_NUM_014 &gpioa 14
#define RTS5912_GPIO_NUM_015 &gpioa 15
#define RTS5912_GPIO_NUM_016 &gpiob 0
#define RTS5912_GPIO_NUM_017 &gpiob 1
#define RTS5912_GPIO_NUM_018 &gpiob 2
#define RTS5912_GPIO_NUM_019 &gpiob 3
#define RTS5912_GPIO_NUM_020 &gpiob 4
#define RTS5912_GPIO_NUM_021 &gpiob 5
#define RTS5912_GPIO_NUM_022 &gpiob 6
#define RTS5912_GPIO_NUM_023 &gpiob 7
#define RTS5912_GPIO_NUM_025 &gpiob 9
#define RTS5912_GPIO_NUM_026 &gpiob 10
#define RTS5912_GPIO_NUM_027 &gpiob 11
#define RTS5912_GPIO_NUM_028 &gpiob 12
#define RTS5912_GPIO_NUM_029 &gpiob 13
#define RTS5912_GPIO_NUM_030 &gpiob 14
#define RTS5912_GPIO_NUM_031 &gpiob 15
#define RTS5912_GPIO_NUM_040 &gpioc 8
#define RTS5912_GPIO_NUM_041 &gpioc 9
#define RTS5912_GPIO_NUM_042 &gpioc 10
#define RTS5912_GPIO_NUM_043 &gpioc 11
#define RTS5912_GPIO_NUM_044 &gpioc 12
#define RTS5912_GPIO_NUM_045 &gpioc 13
#define RTS5912_GPIO_NUM_046 &gpioc 14
#define RTS5912_GPIO_NUM_047 &gpioc 15
#define RTS5912_GPIO_NUM_048 &gpiod 0
#define RTS5912_GPIO_NUM_049 &gpiod 1
#define RTS5912_GPIO_NUM_050 &gpiod 2
#define RTS5912_GPIO_NUM_051 &gpiod 3
#define RTS5912_GPIO_NUM_052 &gpiod 4
#define RTS5912_GPIO_NUM_053 &gpiod 5
#define RTS5912_GPIO_NUM_055 &gpiod 7
#define RTS5912_GPIO_NUM_056 &gpiod 8
#define RTS5912_GPIO_NUM_057 &gpiod 9
#define RTS5912_GPIO_NUM_058 &gpiod 10
#define RTS5912_GPIO_NUM_059 &gpiod 11
#define RTS5912_GPIO_NUM_060 &gpiod 12
#define RTS5912_GPIO_NUM_061 &gpiod 13
#define RTS5912_GPIO_NUM_064 &gpioe 0
#define RTS5912_GPIO_NUM_065 &gpioe 1
#define RTS5912_GPIO_NUM_066 &gpioe 2
#define RTS5912_GPIO_NUM_067 &gpioe 3
#define RTS5912_GPIO_NUM_068 &gpioe 4
#define RTS5912_GPIO_NUM_069 &gpioe 5
#define RTS5912_GPIO_NUM_070 &gpioe 6
#define RTS5912_GPIO_NUM_071 &gpioe 7
#define RTS5912_GPIO_NUM_074 &gpioe 10
#define RTS5912_GPIO_NUM_075 &gpioe 11
#define RTS5912_GPIO_NUM_076 &gpioe 12
#define RTS5912_GPIO_NUM_077 &gpioe 13
#define RTS5912_GPIO_NUM_078 &gpioe 14
#define RTS5912_GPIO_NUM_079 &gpioe 15
#define RTS5912_GPIO_NUM_080 &gpiof 0
#define RTS5912_GPIO_NUM_081 &gpiof 1
#define RTS5912_GPIO_NUM_083 &gpiof 3
#define RTS5912_GPIO_NUM_084 &gpiof 4
#define RTS5912_GPIO_NUM_085 &gpiof 5
#define RTS5912_GPIO_NUM_086 &gpiof 6
#define RTS5912_GPIO_NUM_087 &gpiof 7
#define RTS5912_GPIO_NUM_088 &gpiof 8
#define RTS5912_GPIO_NUM_089 &gpiof 9
#define RTS5912_GPIO_NUM_090 &gpiof 10
#define RTS5912_GPIO_NUM_091 &gpiof 11
#define RTS5912_GPIO_NUM_092 &gpiof 12
#define RTS5912_GPIO_NUM_093 &gpiof 13
#define RTS5912_GPIO_NUM_094 &gpiof 14
#define RTS5912_GPIO_NUM_095 &gpiof 15
#define RTS5912_GPIO_NUM_096 &gpiog 0
#define RTS5912_GPIO_NUM_097 &gpiog 1
#define RTS5912_GPIO_NUM_099 &gpiog 3
#define RTS5912_GPIO_NUM_100 &gpiog 4
#define RTS5912_GPIO_NUM_101 &gpiog 5
#define RTS5912_GPIO_NUM_102 &gpiog 6
#define RTS5912_GPIO_NUM_103 &gpiog 7
#define RTS5912_GPIO_NUM_104 &gpiog 8
#define RTS5912_GPIO_NUM_105 &gpiog 9
#define RTS5912_GPIO_NUM_106 &gpiog 10
#define RTS5912_GPIO_NUM_107 &gpiog 11
#define RTS5912_GPIO_NUM_108 &gpiog 12
#define RTS5912_GPIO_NUM_109 &gpiog 13
#define RTS5912_GPIO_NUM_111 &gpiog 15
#define RTS5912_GPIO_NUM_112 &gpioh 0
#define RTS5912_GPIO_NUM_113 &gpioh 1
#define RTS5912_GPIO_NUM_114 &gpioh 2
#define RTS5912_GPIO_NUM_115 &gpioh 3
#define RTS5912_GPIO_NUM_117 &gpioh 5
#define RTS5912_GPIO_NUM_118 &gpioh 6
#define RTS5912_GPIO_NUM_119 &gpioh 7
#define RTS5912_GPIO_NUM_120 &gpioh 8
#define RTS5912_GPIO_NUM_121 &gpioh 9
#define RTS5912_GPIO_NUM_122 &gpioh 10
#define RTS5912_GPIO_NUM_123 &gpioh 11
#define RTS5912_GPIO_NUM_124 &gpioh 12
#define RTS5912_GPIO_NUM_125 &gpioh 13
#define RTS5912_GPIO_NUM_126 &gpioh 14
#define RTS5912_GPIO_NUM_127 &gpioh 15
#define RTS5912_GPIO_NUM_128 &gpioi 0
#define RTS5912_GPIO_NUM_130 &gpioi 2
#define RTS5912_GPIO_NUM_131 &gpioi 3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_ */
