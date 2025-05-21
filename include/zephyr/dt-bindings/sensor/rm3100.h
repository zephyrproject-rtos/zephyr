/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PNI_RM3100_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PNI_RM3100_H_

/**
 * @defgroup RM3100 PNI DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup RM3100_ODR Output data rate options
 * @{
 */
#define RM3100_DT_ODR_600	0x92
#define RM3100_DT_ODR_300	0x93
#define RM3100_DT_ODR_150	0x94
#define RM3100_DT_ODR_75	0x95
#define RM3100_DT_ODR_37_5	0x96
#define RM3100_DT_ODR_18	0x97
#define RM3100_DT_ODR_9		0x98
#define RM3100_DT_ODR_4_5	0x99
#define RM3100_DT_ODR_2_3	0x9A
#define RM3100_DT_ODR_1_2	0x9B
#define RM3100_DT_ODR_0_6	0x9C
#define RM3100_DT_ODR_0_3	0x9D
#define RM3100_DT_ODR_0_015	0x9E
#define RM3100_DT_ODR_0_0075	0x9F
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PNI_RM3100_H_ */
