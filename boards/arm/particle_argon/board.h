/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* SKYWORKS SKY13351 antenna selection settings */
#define SKY_UFLn_GPIO_NAME	DT_GPIO_LABEL(DT_INST(0, skyworks_sky13351), vctl1_gpios)
#define SKY_UFLn_GPIO_FLAGS	DT_GPIO_FLAGS(DT_INST(0, skyworks_sky13351), vctl1_gpios)
#define SKY_UFLn_GPIO_PIN	DT_GPIO_PIN(DT_INST(0, skyworks_sky13351), vctl1_gpios)
#define SKY_PCBn_GPIO_NAME	DT_GPIO_LABEL(DT_INST(0, skyworks_sky13351), vctl2_gpios)
#define SKY_PCBn_GPIO_FLAGS	DT_GPIO_FLAGS(DT_INST(0, skyworks_sky13351), vctl2_gpios)
#define SKY_PCBn_GPIO_PIN	DT_GPIO_PIN(DT_INST(0, skyworks_sky13351), vctl2_gpios)

#endif /* __INC_BOARD_H */
