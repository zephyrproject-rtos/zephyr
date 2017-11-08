#ifndef __DT_BINDING_ST_MEM_H
#define __DT_BINDING_ST_MEM_H

#define __SIZE_K(x) (x * 1024)

#if defined(CONFIG_SOC_QUARK_SE_C1000)
#define DT_FLASH_SIZE		__SIZE_K(144)
#define DT_SRAM_SIZE		__SIZE_K(52)
#elif defined(CONFIG_SOC_QUARK_D2000)
#define DT_FLASH_SIZE		__SIZE_K(32)
#define DT_SRAM_SIZE		__SIZE_K(8)
#elif defined(CONFIG_SOC_IA32)
#define DT_FLASH_SIZE		__SIZE_K(4092)
#if XIP
	#define DT_SRAM_SIZE		__SIZE_K(4096)
#else
	#define DT_SRAM_SIZE		__SIZE_K(8188)
#endif
#else
#error "Flash and RAM sizes not defined for this chip"
#endif

#endif /* __DT_BINDING_ST_MEM_H */
