/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 * Copyright (c) 2020 Thorvald Natvig
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* Ethernet specific pins */
#define ETH_REF_CLK_GPIO_NAME           "GPIO_A"
#define ETH_REF_CLK_GPIO_PIN            DT_PROP_BY_IDX(DT_INST(0, silabs_gecko_ethernet), location_rmii_refclk, 2)
/* The driver ties CMU_CLK2 to the refclk, and pin A3 is CMU_CLK2 #1 */
#define ETH_REF_CLK_LOCATION            1

#endif  /* __INC_BOARD_H */
