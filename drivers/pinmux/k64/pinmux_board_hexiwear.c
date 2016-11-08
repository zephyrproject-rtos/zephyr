/* pinmux_board_hexiwear.c - pin out mapping for the NXP Hexiwear board */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
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

#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <sys_io.h>
#include <pinmux.h>
#include <pinmux/pinmux.h>
#include <pinmux/k64/pinmux.h>

/*
 * I/O pin configuration
 */

/*
 * Alter this table to change the default pin settings on the NXP Hexiwear
 * boards. Specifically, change the PINMUX_* values to represent the
 * functionality desired.
 */
static const struct pin_config mux_config[] = {
	/* pin,		selected mode */

	/* RGB */
	{ K64_PIN_PTC8, K64_PINMUX_FUNC_GPIO},
	{ K64_PIN_PTC9, K64_PINMUX_FUNC_GPIO},
	{ K64_PIN_PTD0, K64_PINMUX_FUNC_GPIO},

	/* I2C1 - accel/mag, gyro, pressure */
	{ K64_PIN_PTC10, (K64_PINMUX_ALT_2 | K64_PINMUX_OPEN_DRN_ENABLE)},
	{ K64_PIN_PTC11, (K64_PINMUX_ALT_2 | K64_PINMUX_OPEN_DRN_ENABLE)},

	/* FXOS8700 INT1 */
	{ K64_PIN_PTC1, K64_PINMUX_FUNC_GPIO},

	/* UART4 - BLE */
	{ K64_PIN_PTE25, K64_PINMUX_ALT_3 },
	{ K64_PIN_PTE24, K64_PINMUX_ALT_3 },
};

static int hexiwear_pin_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/* configure the pins from the default mapping above */
	for (int i = 0; i < ARRAY_SIZE(mux_config); i++) {
		_fsl_k64_set_pin(mux_config[i].pin_num, mux_config[i].mode);
	}

	return 0;
}

SYS_INIT(hexiwear_pin_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
