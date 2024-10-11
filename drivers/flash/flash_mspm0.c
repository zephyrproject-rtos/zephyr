#include <zephyr/kernel.h>
#include <zephyr/device.h>
#define DT_DRV_COMPAT ti_mspm0_flash_controller
#include <zephyr/drivers/flash.h>
#include <soc.h>
#include <zephyr/logging/log.h>
#include <driverlib/dl_flashctl.h>
#include "flash_mspm0.h"

struct flash_mspm0_config {
	FLASHCTL_Regs *regs;
};


#define MSPM0_BANK_COUNT		1
#define MSPM0_FLASH_SIZE		(FLASH_SIZE)
#define MSPM0_FLASH_PAGE_SIZE		(FLASH_PAGE_SIZE)
#define MSPM0_PAGES_PER_BANK		\
	((MSPM0_FLASH_SIZE / MSPM0_FLASH_PAGE_SIZE) / MSPM0_BANK_COUNT)

LOG_MODULE_REGISTER(flash_mspm0, CONFIG_FLASH_LOG_LEVEL); 

// #define FLASH_TIMEOUT \
//     (2 * DT_PROP(DT_INST(0,mspm0_nv_flash), max_erase_time))

#define FLASH_TIMEOUT 16000000

// #if defined(CONFIG_MULTITHREADING)

// static inline void _flash_mspm0_sem_take(const struct *dev){
// 	k_sem_take(&FLASH_MSPM0_PRIV(dev->sem, K_FOREVER));
// 	//FIXME: Locking here
// }

// static inline void _flash_mspm0_give(const struct device *dev){
// 	//FIXME: unlocking here
// 	k_sem_give(&FLASH_MSPM0_PRIV(dev)->sem);
// }

// #define flash_mspm0_sem_init(dev) k_sem_init(&FLASH_MSPM0_PRIV(dev)->sem, 1, 1)
// #define flash_mspm0_sem_take(dev) _flash_mspm0_sem_take(dev)
// #define flash_mspm0_sem_give(dev) _flash_mspm0_sem_give(dev)
// #else

// #define flash_mspm0_sem_init(dev)
// #define flash_mspm0_sem_take(dev)
// #define flash_mspm0_sem_give(dev)
// #endif
static const struct flash_parameters flash_mspm0_parameters = {
    .write_block_size = FLASH_MSPM0_WRITE_BLOCK_SIZE,
    .erase_value = 0xff   
};

static int flash_mspm0_init(const struct device *dev){
    #if ((CONFIG_FLASH_LOG_LEVEL >= LOG_LEVEL_DBG) && CONFIG_FLASH_PAGE_LAYOUT)
	const struct flash_pages_layout *layout;
	size_t layout_size;

	flash_mspm0_page_layout(dev, &layout, &layout_size);
	for (size_t i = 0; i < layout_size; i++) {
		LOG_DBG("Block %zu: bs: %zu count: %zu", i,
			layout[i].pages_size, layout[i].pages_count);
	}
#endif
    return 0;
}
static int flash_mspm0_write_protection(const struct device *dev, bool enable);

bool __weak flash_mspm0_valid_range(const struct device *dev, off_t offset, uint32_t len, bool write){
    if(write && !flash_mspm0_valid_write(offset, len)){
        return false;
    }
    //return true;
    return flash_mspm0_range_exists(dev, offset, len);
}



static void flash_mspm0_flush_caches(const struct device *dev, off_t offset, size_t len){
	//NOTE: May need to include conditionals for different types of boards

	/*if data cache is enabled (and exists), disable cache. Reset it and then re-enable*/
    
	/*If instruction cache is enabled, disable cache. reset and then re-enable*/
	
}


static int flash_mspm0_erase(const struct device *dev, off_t offset, size_t len){
    FLASHCTL_Regs *regs = FLASH_MSPM0_REGS(dev);
    // int rc;
    if(!flash_mspm0_valid_range(dev, offset, len, true)){
        LOG_ERR("Erase range invalid. Offset %ld, len: %zu", (long int) offset, len);
        return -EINVAL;
    }

    if(!len){
        return 0;
    }

    //FIXME: flash_mspm0_sem_take(dev);
    bool status = true;
    // LOG_DBG("Erase offset: %ld, len: %zu", (long int) offset, len);
    DL_FlashCTL_unprotectSector(regs, offset, DL_FLASHCTL_REGION_SELECT_MAIN);
    DL_FlashCTL_eraseMemory(regs, offset, DL_FLASHCTL_COMMAND_SIZE_SECTOR);
    if(!status){
        
        return -EINVAL;
    }
    else{
        return 1;
    }
    /*FIXME: only need unprotect, erase, and wait for completion. return 0 on success*/


    // rc = flash_mspm0_write_protection(dev, offset, len);
    // if(rc == 0){
    //     rc = flash_mspm0_block_erase_loop(dev, offset, len);
    // }

    // int rc2 = flash_mspm0_write_protection(dev, true);

    // if(!rc2){
    //     rc = rc2;
    // }

    // //FIXME: flash_mspm0_sem_give(dev);

    // return rc;
}

static int flash_mspm0_write(const struct device *dev, off_t offset, const void *data, size_t len){
    FLASHCTL_Regs *regs = FLASH_MSPM0_REGS(dev);
    //printf("%d\n",*regs);
    //int rc; 

    if(!flash_mspm0_valid_range(dev, offset, len, true)){
        LOG_ERR("Erase range invalid. Offset %ld, len: %zu", (long int) offset, len);
        printf("Error!\n");
        return -EINVAL;
    }

    if(!len){
        return 0;
    }
    
    //flash_mspm0_sem_take(dev);

    DL_FlashCTL_unprotectSector(regs, offset, DL_FLASHCTL_REGION_SELECT_MAIN);
    DL_FlashCTL_programMemory64WithECCGenerated(regs, offset, data);
    //DL_FlashCTL_protectSector(regs, offset, DL_FLASHCTL_REGION_SELECT_MAIN);
    return 1;
    
    // LOG_DBG("Write offset: %ld, len: %zu", (long int) offset, len);

	// rc = flash_stm32_write_protection(dev, false);
	// if (rc == 0) {
	// 	rc = flash_stm32_write_range(dev, offset, data, len);
	// }

	// int rc2 = flash_stm32_write_protection(dev, true);

	// if (!rc) {
	// 	rc = rc2;
	// }

	// flash_stm32_sem_give(dev);

	// return rc;

}

static int flash_mspm0_read(const struct device *dev, off_t offset, 
                void *data, 
                size_t len)
{
    if(!flash_mspm0_valid_range(dev, offset, len, false)){
        LOG_ERR("Read range invalid. Offset %d, len %zu", (long int) offset, len);
        return -EINVAL;
    }
    if(!len){
        return 0;
    }

    LOG_DBG("Read offset: %ld, len %zu", (long int) offset, len);
    memcpy(data, (uint8_t*) FLASH_MSPM0_BASE_ADDRESS + offset, len);
    return 0;
}

static const struct flash_parameters * flash_mspm0_get_parameters(const struct device *dev){
    ARG_UNUSED(dev);
    return &flash_mspm0_parameters;
}

static int flash_mspm0_write_protection(const struct device *dev, bool enable)
{
	FLASHCTL_Regs *regs = FLASH_MSPM0_REGS(dev);

	int rc = 0;

	if (enable) {
		rc = flash_stm32_wait_flash_idle(dev);
		if (rc) {
			flash_stm32_sem_give(dev);
			return rc;
		}
    }
/* FIXME: ST32 functionality below, unsure if there are equivalents for MSP*/
// #if defined(FLASH_SECURITY_NS)
// 	if (enable) {
// 		regs->NSCR |= FLASH_STM32_NSLOCK;
// 	} else {
// 		if (regs->NSCR & FLASH_STM32_NSLOCK) {
// 			regs->NSKEYR = FLASH_KEY1;
// 			regs->NSKEYR = FLASH_KEY2;
// 		}
// 	}
// #elif defined(FLASH_CR_LOCK)
// 	if (enable) {
// 		regs->CR |= FLASH_CR_LOCK;
// 	} else {
// 		if (regs->CR & FLASH_CR_LOCK) {
// 			regs->KEYR = FLASH_KEY1;
// 			regs->KEYR = FLASH_KEY2;
// 		}
// 	}
// #else
// 	if (enable) {
// 		regs->PECR |= FLASH_PECR_PRGLOCK;
// 		regs->PECR |= FLASH_PECR_PELOCK;
// 	} else {
// 		if (regs->PECR & FLASH_PECR_PRGLOCK) {
// 			LOG_DBG("Disabling write protection");
// 			regs->PEKEYR = FLASH_PEKEY1;
// 			regs->PEKEYR = FLASH_PEKEY2;
// 			regs->PRGKEYR = FLASH_PRGKEY1;
// 			regs->PRGKEYR = FLASH_PRGKEY2;
// 		}
// 		if (FLASH->PECR & FLASH_PECR_PRGLOCK) {
// 			LOG_ERR("Unlock failed");
// 			rc = -EIO;
// 		}
// 	}
// #endif /* FLASH_SECURITY_NS */

	if (enable) {
		LOG_DBG("Enable write protection");
	} else {
		LOG_DBG("Disable write protection");
	}

	return rc;
}

int flash_mspm0_wait_flash_idle(const struct device *dev){
    int64_t timeout_time = k_uptime_get() + FLASH_TIMEOUT;
    int rc;
    uint32_t busy_flags;

    rc = flash_mspm0_check_status(dev);
    if(rc < 0){
        return -EIO;
    }

    //busy_flags = /*FIXME: Flash SR busy*/;

    // while((FLASH_MSPM0_REGS(dev)->/*FIXME: Flash SR reg*/) & busy_flags){
    //     if(k_uptime_get() > timeout_time){
    //         LOG_ERR("Timeout! val: %d", FLASH_TIMEOUT);
    //         return -EIO;
    //     }
    // }
    return 0;
}

static int flash_mspm0_check_status(const struct device *dev){
    // if(FLASH_MSPM0_REGS(dev)->/*FIXME: security register*/ & /*FIXME: security errors*/){
    //     //LOG_DBG("Status: 0x%08lx", FLASH_MSPM0_REGS(dev)->/*FIXME: security register*/ & /*FIXME: security errors*/);

    //     /*FIXME: clear errors in SR to unblock usage of the flash*/

    //     return -EIO;
    // }

    return 0;
}

int flash_mspm0_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len){
        
}

void flash_mspm0_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
    static struct flash_pages_layout mspm0_flash_layout = {
        .pages_count = 0,
        .pages_size = 0,
    };

    ARG_UNUSED(dev);

    if(mspm0_flash_layout.pages_count == 0){
        mspm0_flash_layout.pages_count = MSPM0_FLASH_SIZE / MSPM0_FLASH_PAGE_SIZE;
        mspm0_flash_layout.pages_size = MSPM0_FLASH_PAGE_SIZE;
    }

    *layout = &mspm0_flash_layout;
    *layout_size = 1;

}

static const struct flash_mspm0_config flash_mspm0_cfg = {	
    .regs = (FLASHCTL_Regs *)DT_INST_REG_ADDR(0),		
};

static const struct flash_driver_api flash_mspm0_driver_api = {
    .erase = flash_mspm0_erase,
    .write = flash_mspm0_write,
    .read = flash_mspm0_read,
    .get_parameters = flash_mspm0_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_mspm0_page_layout,
#endif
};

DEVICE_DT_INST_DEFINE(0, flash_mspm0_init, NULL,
		    NULL, &flash_mspm0_cfg, POST_KERNEL,
		    CONFIG_FLASH_INIT_PRIORITY, &flash_mspm0_driver_api);

// DEVICE_DT_INST_DEFINE(0, flash_mspm0_init, NULL,
// 		    &flash_mspm0_cfg, NULL, POST_KERNEL,
// 		    CONFIG_FLASH_INIT_PRIORITY, &flash_mspm0_driver_api);



