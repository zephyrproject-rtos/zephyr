/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/*
 * PINMUX configuration settings
 */
#define PINMUX_NUM_PINS				20

/*
 * The name of the GPIO expander labelled as EXP0 in the schematic.
 */
#define PINMUX_GALILEO_EXP0_NAME 		CONFIG_GPIO_PCAL9535A_0_DEV_NAME
/*
 * The name of the GPIO expander labelled as EXP1 in the schematic.
 */
#define PINMUX_GALILEO_EXP1_NAME 		CONFIG_GPIO_PCAL9535A_1_DEV_NAME
/*
 * The name of the GPIO expander labelled as EXP2 in the schematic.
 */
#define PINMUX_GALILEO_EXP2_NAME 		CONFIG_GPIO_PCAL9535A_2_DEV_NAME
/*
 * The name of the PWM LED expander labelled as PWM0 in the schematic.
 */
#define PINMUX_GALILEO_PWM0_NAME 		CONFIG_PWM_PCA9685_0_DEV_NAME
/*
 * The name of the DesignWare GPIO with GPIO<0>..GPIO<7> in the schematic.
 */
#define PINMUX_GALILEO_GPIO_DW_NAME 		CONFIG_GPIO_DW_0_NAME
/*
 * The name of the Legacy Bridge Core Well GPIO with GPIO<8>..GPIO<9>
 * in the schematic.
 */
#define PINMUX_GALILEO_GPIO_INTEL_CW_NAME	CONFIG_GPIO_SCH_0_DEV_NAME
/*
 * The name of the Legacy Bridge Resume Well GPIO with
 * GPIO_SUS<0>..GPIO_SUS<5> in the schematic.
 */
#define PINMUX_GALILEO_GPIO_INTEL_RW_NAME	CONFIG_GPIO_SCH_1_DEV_NAME

#endif /* __INC_BOARD_H */
