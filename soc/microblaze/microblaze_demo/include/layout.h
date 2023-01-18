/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_SOC_MICROBLAZE_SOC_INCLUDE_LAYOUT_H_
#define ZEPHYR_SOC_MICROBLAZE_SOC_INCLUDE_LAYOUT_H_

#include <zephyr/devicetree.h>

#define _DDR_NODE          DT_CHOSEN(zephyr_sram)
#define _LAYOUT_DDR_LOC	   DT_REG_ADDR(_DDR_NODE)
#define _LAYOUT_DDR_SIZE   DT_REG_SIZE(_DDR_NODE)

#define _RESET_VECTOR (_LAYOUT_DDR_LOC)
#define _USER_VECTOR  (_RESET_VECTOR + 0x8)
#define _INTR_VECTOR  (_RESET_VECTOR + 0x10)
#define _EXC_VECTOR   (_RESET_VECTOR + 0x20)

#endif /* ZEPHYR_SOC_MICROBLAZE_SOC_INCLUDE_LAYOUT_H_ */
