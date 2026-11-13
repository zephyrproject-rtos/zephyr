/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BEKEN_BK7258_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BEKEN_BK7258_PINCTRL_H_

/*
 * Peripheral functions a pad can be switched to, by index in its select
 * field. The signal behind each index is per pad, and is listed in the
 * GPIO_DEV_MAP table of the vendor SDK, one row per pad, FUNC0 first:
 * https://github.com/bekencorp/bk_idk/blob/release/v2.0.1/middleware/soc/bk7258/soc/gpio_map.h
 */
#define BK7258_FUNC0 0x0
#define BK7258_FUNC1 0x1
#define BK7258_FUNC2 0x2
#define BK7258_FUNC3 0x3
#define BK7258_FUNC4 0x4
#define BK7258_FUNC5 0x5
#define BK7258_FUNC6 0x6
#define BK7258_FUNC7 0x7

/*
 * A pinmux cell carries the pad number, 0 to 55, in bits 5:0 and the
 * function index in bits 11:8.
 */
#define BK7258_PIN_SHIFT  0U
#define BK7258_PIN_MASK   0x3fU
#define BK7258_FUNC_SHIFT 8U
#define BK7258_FUNC_MASK  0xfU

#define BK7258_PINMUX(pin, func)                                                                   \
	((((pin) & BK7258_PIN_MASK) << BK7258_PIN_SHIFT) |                                         \
	 (((BK7258_##func) & BK7258_FUNC_MASK) << BK7258_FUNC_SHIFT))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BEKEN_BK7258_PINCTRL_H_ */
