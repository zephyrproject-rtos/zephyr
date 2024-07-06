/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INCLUDE_ZEPHYR_DT_BINDINGS_DAI_ESAI_H_
#define _INCLUDE_ZEPHYR_DT_BINDINGS_DAI_ESAI_H_

/* ESAI pin IDs
 * the values of these macros are meant to match
 * the bit position from PCRC/PRRC's PC/PDC associated
 * with each of these pins.
 */
#define ESAI_PIN_SCKR 0
#define ESAI_PIN_FSR 1
#define ESAI_PIN_HCKR 2
#define ESAI_PIN_SCKT 3
#define ESAI_PIN_FST 4
#define ESAI_PIN_HCKT 5
#define ESAI_PIN_SDO5_SDI0 6
#define ESAI_PIN_SDO4_SDI1 7
#define ESAI_PIN_SDO3_SDI2 8
#define ESAI_PIN_SDO2_SDI3 9
#define ESAI_PIN_SDO1 10
#define ESAI_PIN_SDO0 11

/* ESAI pin modes
 * the values of these macros are set according to
 * the following table:
 *
 * PDC = 0, PC = 0 => DISCONNECTED (0)
 * PDC = 0, PC = 1 => GPIO INPUT (1)
 * PDC = 1, PC = 0 => GPIO OUTPUT (2)
 * PDC = 1, PC = 1 => ESAI (3)
 */
#define ESAI_PIN_DISCONNECTED 0
#define ESAI_PIN_GPIO_INPUT 1
#define ESAI_PIN_GPIO_OUTPUT 2
#define ESAI_PIN_ESAI 3

/* ESAI clock IDs */
#define ESAI_CLOCK_HCKT 0
#define ESAI_CLOCK_HCKR 1
#define ESAI_CLOCK_SCKR 2
#define ESAI_CLOCK_SCKT 3
#define ESAI_CLOCK_FSR 4
#define ESAI_CLOCK_FST 5

/* ESAI clock directions */
#define ESAI_CLOCK_INPUT 0
#define ESAI_CLOCK_OUTPUT 1

#endif /* _INCLUDE_ZEPHYR_DT_BINDINGS_DAI_ESAI_H_ */
