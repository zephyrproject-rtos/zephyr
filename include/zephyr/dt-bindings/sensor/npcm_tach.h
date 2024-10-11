/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_NPCM_TACH_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_NPCM_TACH_H_

/* NPCM tachometer pin selection */
#define NPCM_TACH_PIN_SELECT_DEFAULT	0xff
#define NPCM_TACH_PIN_SELECT_0		0
#define NPCM_TACH_PIN_SELECT_1		1
#define NPCM_TACH_PIN_SELECT_2		2
#define NPCM_TACH_PIN_SELECT_3		3
#define NPCM_TACH_PIN_SELECT_4		4
#define NPCM_TACH_PIN_SELECT_5		5
#define NPCM_TACH_PIN_SELECT_6		6
#define NPCM_TACH_PIN_SELECT_7		7
#define NPCM_TACH_PIN_SELECT_8		8
#define NPCM_TACH_PIN_SELECT_9		9

/* NPCM tachometer specific operate frequency */
#define NPCM_TACH_FREQ_LFCLK 32768

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_NPCM_TACH_H_ */
