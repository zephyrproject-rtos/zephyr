/*
 * Copyright (c) 2017 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __DT_BINDING_SAME70_MEM_H
#define __DT_BINDING_SAME70_MEM_H

#define __SIZE_K(x) (x * 1024)

#if defined CONFIG_SOC_PART_NUMBER_SAME70J19
#define DT_FLASH_SIZE		__SIZE_K(512)
#define DT_SRAM_SIZE		__SIZE_K(256)
#elif defined CONFIG_SOC_PART_NUMBER_SAME70J20
#define DT_FLASH_SIZE		__SIZE_K(1024)
#define DT_SRAM_SIZE		__SIZE_K(384)
#elif defined CONFIG_SOC_PART_NUMBER_SAME70J21
#define DT_FLASH_SIZE		__SIZE_K(2048)
#define DT_SRAM_SIZE		__SIZE_K(384)
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N19
#define DT_FLASH_SIZE		__SIZE_K(512)
#define DT_SRAM_SIZE		__SIZE_K(256)
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N20
#define DT_FLASH_SIZE		__SIZE_K(1024)
#define DT_SRAM_SIZE		__SIZE_K(384)
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N21
#define DT_FLASH_SIZE		__SIZE_K(2048)
#define DT_SRAM_SIZE		__SIZE_K(384)
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q19
#define DT_FLASH_SIZE		__SIZE_K(512)
#define DT_SRAM_SIZE		__SIZE_K(256)
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q20
#define DT_FLASH_SIZE		__SIZE_K(1024)
#define DT_SRAM_SIZE		__SIZE_K(384)
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q21
#define DT_FLASH_SIZE		__SIZE_K(2048)
#define DT_SRAM_SIZE		__SIZE_K(384)
#else
#error "Flash and RAM sizes not defined for this chip"
#endif

#endif /* __DT_BINDING_SAME70_MEM_H */
