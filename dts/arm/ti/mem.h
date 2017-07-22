/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DT_BINDING_TI_MEM_H
#define __DT_BINDING_TI_MEM_H

#define __SIZE_K(x) (x * 1024)

#if defined(CONFIG_SOC_CC3220SF)
#define DT_SFLASH_SIZE		__SIZE_K(1024)
#define DT_FLASH_SIZE		__SIZE_K(1024)
#define DT_SRAM_SIZE		__SIZE_K(256)
#define DT_SRAM_START		0x20000000
#elif defined(CONFIG_SOC_MSP432P401R)
#define DT_FLASH_SIZE		__SIZE_K(256)
#define DT_SRAM_SIZE		__SIZE_K(64)
#define DT_SRAM_START		0x20000000
#else
#error "Flash and RAM sizes not defined for this chip"
#endif

#endif /* __DT_BINDING_TI_MEM_H */
