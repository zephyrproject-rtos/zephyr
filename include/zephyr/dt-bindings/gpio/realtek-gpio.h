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

/**
 * @brief Map GPIO signal name to devicetree binding.
 *
 * RTS5912 documentation uses octal GPIO pin
 * numbering. These macros do not require the user to do the transfer for gpio.
 *
 * Example DT usage:
 *
 * @code{.dts}
 * gpios = <RTS5912_GPIO102 (GPIO_OUTPUT)>;
 * @endcode
 *
 * @{
 */

#define RTS5912_GPIO000 &gpioa 0
#define RTS5912_GPIO001 &gpioa 1
#define RTS5912_GPIO002 &gpioa 2
#define RTS5912_GPIO003 &gpioa 3
#define RTS5912_GPIO004 &gpioa 4
#define RTS5912_GPIO009 &gpioa 9
#define RTS5912_GPIO013 &gpioa 13
#define RTS5912_GPIO014 &gpioa 14
#define RTS5912_GPIO015 &gpioa 15
#define RTS5912_GPIO016 &gpiob 0
#define RTS5912_GPIO017 &gpiob 1
#define RTS5912_GPIO018 &gpiob 2
#define RTS5912_GPIO019 &gpiob 3
#define RTS5912_GPIO020 &gpiob 4
#define RTS5912_GPIO021 &gpiob 5
#define RTS5912_GPIO022 &gpiob 6
#define RTS5912_GPIO023 &gpiob 7
#define RTS5912_GPIO025 &gpiob 9
#define RTS5912_GPIO026 &gpiob 10
#define RTS5912_GPIO027 &gpiob 11
#define RTS5912_GPIO028 &gpiob 12
#define RTS5912_GPIO029 &gpiob 13
#define RTS5912_GPIO030 &gpiob 14
#define RTS5912_GPIO031 &gpiob 15
#define RTS5912_GPIO040 &gpioc 8
#define RTS5912_GPIO041 &gpioc 9
#define RTS5912_GPIO042 &gpioc 10
#define RTS5912_GPIO043 &gpioc 11
#define RTS5912_GPIO044 &gpioc 12
#define RTS5912_GPIO045 &gpioc 13
#define RTS5912_GPIO046 &gpioc 14
#define RTS5912_GPIO047 &gpioc 15
#define RTS5912_GPIO048 &gpiod 0
#define RTS5912_GPIO049 &gpiod 1
#define RTS5912_GPIO050 &gpiod 2
#define RTS5912_GPIO051 &gpiod 3
#define RTS5912_GPIO052 &gpiod 4
#define RTS5912_GPIO053 &gpiod 5
#define RTS5912_GPIO055 &gpiod 7
#define RTS5912_GPIO056 &gpiod 8
#define RTS5912_GPIO057 &gpiod 9
#define RTS5912_GPIO058 &gpiod 10
#define RTS5912_GPIO059 &gpiod 11
#define RTS5912_GPIO060 &gpiod 12
#define RTS5912_GPIO061 &gpiod 13
#define RTS5912_GPIO064 &gpioe 0
#define RTS5912_GPIO065 &gpioe 1
#define RTS5912_GPIO066 &gpioe 2
#define RTS5912_GPIO067 &gpioe 3
#define RTS5912_GPIO068 &gpioe 4
#define RTS5912_GPIO069 &gpioe 5
#define RTS5912_GPIO070 &gpioe 6
#define RTS5912_GPIO071 &gpioe 7
#define RTS5912_GPIO074 &gpioe 10
#define RTS5912_GPIO075 &gpioe 11
#define RTS5912_GPIO076 &gpioe 12
#define RTS5912_GPIO077 &gpioe 13
#define RTS5912_GPIO078 &gpioe 14
#define RTS5912_GPIO079 &gpioe 15
#define RTS5912_GPIO080 &gpiof 0
#define RTS5912_GPIO081 &gpiof 1
#define RTS5912_GPIO083 &gpiof 3
#define RTS5912_GPIO084 &gpiof 4
#define RTS5912_GPIO085 &gpiof 5
#define RTS5912_GPIO086 &gpiof 6
#define RTS5912_GPIO087 &gpiof 7
#define RTS5912_GPIO088 &gpiof 8
#define RTS5912_GPIO089 &gpiof 9
#define RTS5912_GPIO090 &gpiof 10
#define RTS5912_GPIO091 &gpiof 11
#define RTS5912_GPIO092 &gpiof 12
#define RTS5912_GPIO093 &gpiof 13
#define RTS5912_GPIO094 &gpiof 14
#define RTS5912_GPIO095 &gpiof 15
#define RTS5912_GPIO096 &gpiog 0
#define RTS5912_GPIO097 &gpiog 1
#define RTS5912_GPIO099 &gpiog 3
#define RTS5912_GPIO100 &gpiog 4
#define RTS5912_GPIO101 &gpiog 5
#define RTS5912_GPIO102 &gpiog 6
#define RTS5912_GPIO103 &gpiog 7
#define RTS5912_GPIO104 &gpiog 8
#define RTS5912_GPIO105 &gpiog 9
#define RTS5912_GPIO106 &gpiog 10
#define RTS5912_GPIO107 &gpiog 11
#define RTS5912_GPIO108 &gpiog 12
#define RTS5912_GPIO109 &gpiog 13
#define RTS5912_GPIO111 &gpiog 15
#define RTS5912_GPIO112 &gpioh 0
#define RTS5912_GPIO113 &gpioh 1
#define RTS5912_GPIO114 &gpioh 2
#define RTS5912_GPIO115 &gpioh 3
#define RTS5912_GPIO117 &gpioh 5
#define RTS5912_GPIO118 &gpioh 6
#define RTS5912_GPIO119 &gpioh 7
#define RTS5912_GPIO120 &gpioh 8
#define RTS5912_GPIO121 &gpioh 9
#define RTS5912_GPIO122 &gpioh 10
#define RTS5912_GPIO123 &gpioh 11
#define RTS5912_GPIO124 &gpioh 12
#define RTS5912_GPIO125 &gpioh 13
#define RTS5912_GPIO126 &gpioh 14
#define RTS5912_GPIO127 &gpioh 15
#define RTS5912_GPIO128 &gpioi 0
#define RTS5912_GPIO130 &gpioi 2
#define RTS5912_GPIO131 &gpioi 3

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_ */
