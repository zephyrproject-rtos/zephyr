/*
 * Copyright (c) 2023 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PINCTRL_PFC_RCAR_H_
#define ZEPHYR_DRIVERS_PINCTRL_PFC_RCAR_H_

#include <stdint.h>
#include <pinctrl_soc.h>

const struct pfc_bias_reg *pfc_rcar_get_bias_regs(void);
const struct pfc_drive_reg *pfc_rcar_get_drive_regs(void);

/**
 * @brief set the register index for a given pin
 *
 * @param the pin
 * @param pointer for the resulting register index
 * @return 0 if the register index is found, negative
 * errno otherwise.
 */
int pfc_rcar_get_reg_index(uint8_t pin, uint8_t *reg_index);

#endif /* ZEPHYR_DRIVERS_PINCTRL_PFC_RCAR_H_ */
