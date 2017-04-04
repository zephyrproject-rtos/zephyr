#ifndef __DT_BINDING_ST_MEM_H
#define __DT_BINDING_ST_MEM_H

#if defined(CONFIG_SOC_STM32F103XB)
#define DT_FLASH_SIZE		0x20000
#define DT_SRAM_SIZE		0x5000
#elif defined(CONFIG_SOC_STM32L476XX)
#define DT_FLASH_SIZE		0x100000
#define DT_SRAM_SIZE		0x18000
#else
#endif

#endif /* __DT_BINDING_ST_MEM_H */
