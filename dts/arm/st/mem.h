#ifndef __DT_BINDING_ST_MEM_H
#define __DT_BINDING_ST_MEM_H

#if defined(CONFIG_SOC_STM32F103XB)
#define DT_FLASH_SIZE		0x20000
#define DT_SRAM_SIZE		0x5000
#elif defined(CONFIG_SOC_STM32F103XE)
#define DT_FLASH_SIZE		0x80000
#define DT_SRAM_SIZE		0x10000
#elif defined(CONFIG_SOC_STM32F107XC)
#define DT_FLASH_SIZE		0x40000
#define DT_SRAM_SIZE		0x10000
#elif defined(CONFIG_SOC_STM32F334X8)
#define DT_FLASH_SIZE		0x10000
#define DT_SRAM_SIZE		0x3000
#elif defined(CONFIG_SOC_STM32F373XC)
#define DT_FLASH_SIZE		0x40000
#define DT_SRAM_SIZE		0x8000
#elif defined(CONFIG_SOC_STM32F401XE) || defined(CONFIG_SOC_STM32F411XE)
#define DT_FLASH_SIZE		0x80000
#define DT_SRAM_SIZE		0x18000
#elif defined(CONFIG_SOC_STM32L476XX)
#define DT_FLASH_SIZE		0x100000
#define DT_SRAM_SIZE		0x18000
#else
#error "Flash and RAM sizes not defined for this chip"
#endif

#endif /* __DT_BINDING_ST_MEM_H */
