/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NPCX_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NPCX_PINCTRL_H_

/**
 * @brief NPCX specific PIN configuration flag
 *
 * Pin configuration is coded with the following fields
 *    Power Switch Logic (PSL) [ 0 : 3 ]
 *    Reserved                 [ 4 : 31]
 *
 * Applicable to NPCX7 series.
 */

/*
 * Power Switch Logic (PSL) input wake-up mode is sensitive to edge signals.
 *
 * This is a component flag that should be combined with other
 * `NPCX_PSL_ACTIVE_*` flags to produce a meaningful configuration.
 */
#define NPCX_PSL_MODE_EDGE                   (1 << 0)

/*
 * Power Switch Logic (PSL) input wake-up mode is sensitive to logical levels.
 *
 * This is a component flag that should be combined with other
 * `NPCX_PSL_ACTIVE_*` flags to produce a meaningful configuration.
 */
#define NPCX_PSL_MODE_LEVEL                  (1 << 1)

/*
 * The active polarity of Power Switch Logic (PSL) input is high level or
 * low-to-high transition.
 *
 * This is a component flag that should be combined with other
 * `NPCX_PSL_MODE_*` flags to produce a meaningful configuration.
 */
#define NPCX_PSL_ACTIVE_HIGH                 (1 << 2)

/*
 * The active polarity of Power Switch Logic (PSL) input is low level or
 * high-to-low transition.
 *
 * This is a component flag that should be combined with other
 * `NPCX_PSL_MODE_*` flags to produce a meaningful configuration.
 */
#define NPCX_PSL_ACTIVE_LOW                  (1 << 3)

/*
 * Configures Power Switch Logic (PSL) input in detecting rising edge.
 *
 * This is used for describing the 'flag' property from PSL input device with
 * 'nuvoton,npcx-pslctrl-conf' compatible.
 */
#define NPCX_PSL_RISING_EDGE (NPCX_PSL_MODE_EDGE | NPCX_PSL_ACTIVE_HIGH)

/*
 * Configures Power Switch Logic (PSL) input in detecting falling edge.
 *
 * This is used for describing the 'flag' property from PSL input device with
 * 'nuvoton,npcx-pslctrl-conf' compatible.
 */
#define NPCX_PSL_FALLING_EDGE (NPCX_PSL_MODE_EDGE | NPCX_PSL_ACTIVE_LOW)

/*
 * Configures Power Switch Logic (PSL) input in detecting level high state (has
 * logical value '1').
 *
 * This is used for describing the 'flag' property from PSL input device with
 * 'nuvoton,npcx-pslctrl-conf' compatible.
 */
#define NPCX_PSL_LEVEL_HIGH (NPCX_PSL_MODE_LEVEL | NPCX_PSL_ACTIVE_HIGH)

/*
 * Configures Power Switch Logic (PSL) input in detecting level low state (has
 * logical value '0').
 *
 * This is used for describing the 'flag' property from PSL input device with
 * 'nuvoton,npcx-pslctrl-conf' compatible.
 */
#define NPCX_PSL_LEVEL_LOW (NPCX_PSL_MODE_LEVEL | NPCX_PSL_ACTIVE_LOW)

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NPCX_PINCTRL_H_ */
