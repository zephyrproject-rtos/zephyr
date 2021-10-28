/*
 * Copyright (c) 2021 Sagar Khadgi <sagar.khadgi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include "mpfs_sys.h"
#include "encoding.h"
#include "mss_plic.h"
#include "mss_hart_ints.h"
#include "mss_nwc_init.h"
#include "mpfs_hal_config/mss_sw_config.h"

/* Selects the 16MHz oscilator on the HiFive1 board, which provides a clock
 * that's accurate enough to actually drive serial ports off of.
 */
#define MPFS_HAL_HW_CONFIG

#ifdef  MPFS_HAL_HW_CONFIG

#define VIRTUAL_BOOTROM_BASE_ADDR   0x20003120UL
#define NB_BOOT_ROM_WORDS           8U

const uint32_t rom1[NB_BOOT_ROM_WORDS] =
{
	0x00000513U,    /* li a0, 0 */
	0x34451073U,    /* csrw mip, a0 */
	0x10500073U,    /* wfi */
	0xFF5FF06FU,    /* j 0x20003120 */
	0xFF1FF06FU,    /* j 0x20003120 */
	0xFEDFF06FU,    /* j 0x20003120 */
	0xFE9FF06FU,    /* j 0x20003120 */
	0xFE5FF06FU     /* j 0x20003120 */
};

void load_virtual_rom(void)
{
    uint8_t inc;
    volatile uint32_t * p_virtual_bootrom = (uint32_t *)VIRTUAL_BOOTROM_BASE_ADDR;



    for(inc = 0; inc < NB_BOOT_ROM_WORDS; ++inc)
    {
        p_virtual_bootrom[inc] = rom1[inc];
    }
}
#endif

void __disable_irq(void)
{
    csr_clear(mstatus, MSTATUS_MIE);
    csr_clear(mstatus, MSTATUS_MPIE);
}

void __disable_all_irqs(void)
{
    __disable_irq();
    csr_write(mie, 0x00U);
    csr_write(mip, 0x00);
}

uint8_t init_bus_error_unit(void)
{
    int hard_idx;

    /* Init BEU in all harts - enable local interrupt */
    for(hard_idx = MPFS_HAL_FIRST_HART; hard_idx <= MPFS_HAL_LAST_HART; hard_idx++)
    {
        BEU->regs[hard_idx].ENABLE      = BEU_ENABLE;
        BEU->regs[hard_idx].PLIC_INT    = BEU_PLIC_INT;
        BEU->regs[hard_idx].LOCAL_INT   = BEU_LOCAL_INT;
    }

    return (0U);
}

static int icicle_before_init(const struct device *dev)
{

#ifdef  MPFS_HAL_HW_CONFIG
        load_virtual_rom();
//        config_l2_cache();
#endif  /* MPFS_HAL_HW_CONFIG */

        //init_memory
        __disable_all_irqs();      /* disables local and global interrupt enable */

#ifdef  MPFS_HAL_HW_CONFIG
       (void)init_bus_error_unit(); //not addded
//       (void)init_mem_protection_unit();
//       (void)init_pmp((uint8_t)MPFS_HAl_FIRST_HART);
#endif
 
       PLIC_ClearPendingIRQ();
//        mss_nwc_init();
//        mss_pll_config();
        

        //PLIC_init_on_reset();
        

        
 return 0;
}


SYS_INIT(icicle_before_init, PRE_KERNEL_1, 1);

