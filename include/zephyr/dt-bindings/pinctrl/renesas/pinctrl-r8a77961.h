/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_R8A77961_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_R8A77961_H_

#include "pinctrl-rcar-common.h"

/* Pins declaration */
#define PIN_NONE       -1

#define PIN_SD0_CLK    RCAR_GP_PIN(3, 0)
#define PIN_SD0_CMD    RCAR_GP_PIN(3, 1)
#define PIN_SD0_DATA0  RCAR_GP_PIN(3, 2)
#define PIN_SD0_DATA1  RCAR_GP_PIN(3, 3)
#define PIN_SD0_DATA2  RCAR_GP_PIN(3, 4)
#define PIN_SD0_DATA3  RCAR_GP_PIN(3, 5)
#define PIN_SD0_CD     RCAR_GP_PIN(3, 12)
#define PIN_SD0_WP     RCAR_GP_PIN(3, 13)

#define PIN_SD1_CLK    RCAR_GP_PIN(3, 6)
#define PIN_SD1_CMD    RCAR_GP_PIN(3, 7)
/*
 * note: the next data pins shared with SD2,
 *       and for SD2 they represent DATA4-DATA7
 */
#define PIN_SD1_DATA0  RCAR_GP_PIN(3, 8)
#define PIN_SD1_DATA1  RCAR_GP_PIN(3, 9)
#define PIN_SD1_DATA2  RCAR_GP_PIN(3, 10)
#define PIN_SD1_DATA3  RCAR_GP_PIN(3, 11)

#define PIN_SD1_CD     RCAR_GP_PIN(3, 14)
#define PIN_SD1_WP     RCAR_GP_PIN(3, 15)

#define PIN_SD2_CLK    RCAR_GP_PIN(4, 0)
#define PIN_SD2_CMD    RCAR_GP_PIN(4,  1)
#define PIN_SD2_DATA0  RCAR_GP_PIN(4,  2)
#define PIN_SD2_DATA1  RCAR_GP_PIN(4,  3)
#define PIN_SD2_DATA2  RCAR_GP_PIN(4,  4)
#define PIN_SD2_DATA3  RCAR_GP_PIN(4,  5)
#define PIN_SD2_DS     RCAR_GP_PIN(4,  6)

#define PIN_SD3_CLK    RCAR_GP_PIN(4,  7)
#define PIN_SD3_CMD    RCAR_GP_PIN(4,  8)
#define PIN_SD3_DATA0  RCAR_GP_PIN(4,  9)
#define PIN_SD3_DATA1  RCAR_GP_PIN(4,  10)
#define PIN_SD3_DATA2  RCAR_GP_PIN(4,  11)
#define PIN_SD3_DATA3  RCAR_GP_PIN(4,  12)
#define PIN_SD3_DATA4  RCAR_GP_PIN(4,  13)
#define PIN_SD3_DATA5  RCAR_GP_PIN(4,  14)
#define PIN_SD3_DATA6  RCAR_GP_PIN(4,  15)
#define PIN_SD3_DATA7  RCAR_GP_PIN(4,  16)
#define PIN_SD3_DS     RCAR_GP_PIN(4,  17)

#define PIN_TX2_A      RCAR_GP_PIN(5, 10)
#define PIN_RX2_A      RCAR_GP_PIN(5, 11)

/* Pinmux function declarations */
#define FUNC_SD0_CLK   IPSR(7, 12, 0)
#define FUNC_SD0_CMD   IPSR(7, 16, 0)
#define FUNC_SD0_DAT0  IPSR(7, 20, 0)
#define FUNC_SD0_DAT1  IPSR(7, 24, 0)
#define FUNC_SD0_DAT2  IPSR(8, 0, 0)
#define FUNC_SD0_DAT3  IPSR(8, 4, 0)
#define FUNC_SD0_CD    IPSR(11, 8, 0)
#define FUNC_SD0_WP    IPSR(11, 12, 0)

#define FUNC_SD1_CLK   IPSR(8, 8, 0)
#define FUNC_SD1_CMD   IPSR(8, 12, 0)
#define FUNC_SD1_DAT0  IPSR(8, 16, 0)
#define FUNC_SD1_DAT1  IPSR(8, 20, 0)
#define FUNC_SD1_DAT2  IPSR(8, 24, 0)
#define FUNC_SD1_DAT3  IPSR(8, 28, 0)
#define FUNC_SD1_CD    IPSR(11, 16, 0)
#define FUNC_SD1_WP    IPSR(11, 20, 0)

#define FUNC_SD2_CLK   IPSR(9, 0, 0)
#define FUNC_SD2_CMD   IPSR(9, 4, 0)
#define FUNC_SD2_DAT0  IPSR(9, 8, 0)
#define FUNC_SD2_DAT1  IPSR(9, 12, 0)
#define FUNC_SD2_DAT2  IPSR(9, 16, 0)
#define FUNC_SD2_DAT3  IPSR(9, 20, 0)
#define FUNC_SD2_DAT4  IPSR(8, 16, 1)
#define FUNC_SD2_DAT5  IPSR(8, 20, 1)
#define FUNC_SD2_DAT6  IPSR(8, 24, 1)
#define FUNC_SD2_DAT7  IPSR(8, 28, 1)
#define FUNC_SD2_CD_A  IPSR(10, 20, 1)
#define FUNC_SD2_WP_A  IPSR(10, 24, 1)
#define FUNC_SD2_CD_B  IPSR(13, 0, 3)
#define FUNC_SD2_WP_B  IPSR(13, 4, 3)
#define FUNC_SD2_DS    IPSR(9, 24, 0)

#define FUNC_SD3_CLK   IPSR(9, 28, 0)
#define FUNC_SD3_CMD   IPSR(10, 0, 0)
#define FUNC_SD3_DAT0  IPSR(10, 4, 0)
#define FUNC_SD3_DAT1  IPSR(10, 8, 0)
#define FUNC_SD3_DAT2  IPSR(10, 12, 0)
#define FUNC_SD3_DAT3  IPSR(10, 16, 0)
#define FUNC_SD3_DAT4  IPSR(10, 20, 0)
#define FUNC_SD3_DAT5  IPSR(10, 24, 0)
#define FUNC_SD3_DAT6  IPSR(10, 28, 0)
#define FUNC_SD3_DAT7  IPSR(11, 0, 0)
#define FUNC_SD3_CD    IPSR(10, 28, 1)
#define FUNC_SD3_WP    IPSR(11, 0, 1)
#define FUNC_SD3_DS    IPSR(11, 4, 0)

#define FUNC_TX2_A     IPSR(13, 0, 0)
#define FUNC_RX2_A     IPSR(13, 4, 0)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_R8A77961_H_ */
