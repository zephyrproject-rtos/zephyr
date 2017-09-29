/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DT_BINDING_ST_MEM_H
#define __DT_BINDING_ST_MEM_H

#define __SIZE_K(x) (x * 1024)

#if defined(CONFIG_SOC_NRF51822_QFAA)
#define DT_FLASH_SIZE		__SIZE_K(256)
#define DT_SRAM_SIZE		__SIZE_K(16)
#elif defined(CONFIG_SOC_NRF51822_QFAB)
#define DT_FLASH_SIZE		__SIZE_K(128)
#define DT_SRAM_SIZE		__SIZE_K(16)
#elif defined(CONFIG_SOC_NRF51822_QFAC)
#define DT_FLASH_SIZE		__SIZE_K(256)
#define DT_SRAM_SIZE		__SIZE_K(32)
#elif defined(CONFIG_SOC_NRF52832_QFAA)
#define DT_FLASH_SIZE		__SIZE_K(512)
#define DT_SRAM_SIZE		__SIZE_K(64)
#elif defined(CONFIG_SOC_NRF52832_CIAA)
#define DT_FLASH_SIZE		__SIZE_K(512)
#define DT_SRAM_SIZE		__SIZE_K(64)
#elif defined(CONFIG_SOC_NRF52832_QFAB)
#define DT_FLASH_SIZE		__SIZE_K(256)
#define DT_SRAM_SIZE		__SIZE_K(32)
#elif defined(CONFIG_SOC_NRF52840_QIAA)
#define DT_FLASH_SIZE		__SIZE_K(1024)
#define DT_SRAM_SIZE		__SIZE_K(256)
#else
#error "Flash and RAM sizes not defined for this chip"
#endif

#endif /* __DT_BINDING_ST_MEM_H */
