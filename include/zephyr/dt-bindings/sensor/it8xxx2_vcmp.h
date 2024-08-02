/*
 * Copyright (c) 2022 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT8XXX2_VCMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT8XXX2_VCMP_H_

/**
 * @name it8xxx2 voltage comparator channel references
 * @{
 */

#define VCMP_CHANNEL_0				0
#define VCMP_CHANNEL_1				1
#define VCMP_CHANNEL_2				2
#define VCMP_CHANNEL_3				3
#define VCMP_CHANNEL_4				4
#define VCMP_CHANNEL_5				5
#define VCMP_CHANNEL_CNT			6

/** @} */

/**
 * @name it8xxx2 voltage comparator scan period for "all comparator channel"
 * @{
 */

#define IT8XXX2_VCMP_SCAN_PERIOD_100US		0x10
#define IT8XXX2_VCMP_SCAN_PERIOD_200US		0x20
#define IT8XXX2_VCMP_SCAN_PERIOD_400US		0x30
#define IT8XXX2_VCMP_SCAN_PERIOD_600US		0x40
#define IT8XXX2_VCMP_SCAN_PERIOD_800US		0x50
#define IT8XXX2_VCMP_SCAN_PERIOD_1MS		0x60
#define IT8XXX2_VCMP_SCAN_PERIOD_1_5MS		0x70
#define IT8XXX2_VCMP_SCAN_PERIOD_2MS		0x80
#define IT8XXX2_VCMP_SCAN_PERIOD_2_5MS		0x90
#define IT8XXX2_VCMP_SCAN_PERIOD_3MS		0xa0
#define IT8XXX2_VCMP_SCAN_PERIOD_4MS		0xb0
#define IT8XXX2_VCMP_SCAN_PERIOD_5MS		0xc0

/** @} */

/**
 * @name it8xxx2 voltage comparator interrupt trigger mode
 * @{
 */

#define IT8XXX2_VCMP_LESS_OR_EQUAL		0
#define IT8XXX2_VCMP_GREATER			1
#define IT8XXX2_VCMP_UNDEFINED			0xffff

/** @} */
#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT8XXX2_VCMP_H_ */
