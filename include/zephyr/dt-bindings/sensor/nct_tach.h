/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_NCT_TACH_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_NCT_TACH_H_

/* NCT tachometer pin selection */
#define NCT_TACH_PIN_SELECT_DEFAULT	0xff
#define NCT_TACH_PIN_SELECT_0		0
#define NCT_TACH_PIN_SELECT_1		1
#define NCT_TACH_PIN_SELECT_2		2
#define NCT_TACH_PIN_SELECT_3		3
#define NCT_TACH_PIN_SELECT_4		4
#define NCT_TACH_PIN_SELECT_5		5
#define NCT_TACH_PIN_SELECT_6		6
#define NCT_TACH_PIN_SELECT_7		7
#define NCT_TACH_PIN_SELECT_8		8
#define NCT_TACH_PIN_SELECT_9		9

/* NCT tachometer specific operate frequency */
#define NCT_TACH_FREQ_LFCLK 32768

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_NCT_TACH_H_ */
