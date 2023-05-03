/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_R8A779F_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_R8A779F_H_

#include "pinctrl-rcar-common.h"

/* Pins declaration */
#define PIN_NONE           -1

/*
 * note: we don't need ipsr configuration for sd/mmc pins,
 *       they don't have alternative functions.
 */
#define PIN_SD_WP          RCAR_GP_PIN(1, 24)
#define PIN_SD_CD          RCAR_GP_PIN(1, 23)
#define PIN_MMC_SD_CMD     RCAR_GP_PIN(1, 22)
#define PIN_MMC_DS         RCAR_GP_PIN(1, 20)
#define PIN_MMC_D7         RCAR_GP_PIN(1, 21)
#define PIN_MMC_D6         RCAR_GP_PIN(1, 19)
#define PIN_MMC_D5         RCAR_GP_PIN(1, 17)
#define PIN_MMC_D4         RCAR_GP_PIN(1, 18)
#define PIN_MMC_SD_D3      RCAR_GP_PIN(1, 16)
#define PIN_MMC_SD_D2      RCAR_GP_PIN(1, 15)
#define PIN_MMC_SD_D1      RCAR_GP_PIN(1, 14)
#define PIN_MMC_SD_D0      RCAR_GP_PIN(1, 13)
#define PIN_MMC_SD_CLK     RCAR_GP_PIN(1, 12)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_R8A779F_H_ */
