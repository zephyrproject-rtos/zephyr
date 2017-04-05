#ifndef __DT_BINDING_ST_MEM_H
#define __DT_BINDING_ST_MEM_H

#define __SIZE_K(x) (x * 1024)

#if defined(CONFIG_SOC_PART_NUMBER_EFM32WG990F256)
#define DT_FLASH_SIZE		__SIZE_K(256)
#define DT_SRAM_SIZE		__SIZE_K(32)
#else
#error "Flash and RAM sizes not defined for this chip"
#endif

#endif /* __DT_BINDING_ST_MEM_H */
