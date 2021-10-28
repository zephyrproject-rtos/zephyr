/*
 * Copyright (c) 2021 Sagar Khadgi <sagar.khadgi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include "mpfs_sys.h"
#include "encoding.h"
#include "mss_sysreg.h"
#include "mss_plic.h"
#include "drivers/mss/mss_gpio/mss_gpio.h"

inline void PLIC_init(void);

/* Selects the 16MHz oscilator on the HiFive1 board, which provides a clock
 * that's accurate enough to actually drive serial ports off of.
 */
 uint8_t  icicleIRQHandler(void);
volatile uint32_t * p_gpio = (volatile uint32_t* )(0x20122000);
uint32_t a_gpio[5] = {0x00};
volatile uint32_t * p_subblock = (volatile uint32_t* )(0x20122084);
volatile uint32_t * p_softreset = (volatile uint32_t* )(0x20122088);

static int pre_mpfs_init(const struct device *dev)
{
	ARG_UNUSED(dev);

    SYSREG->SUBBLK_CLOCK_CR  |= (SUBBLK_CLOCK_CR_MMUART0_MASK | SUBBLK_CLOCK_CR_GPIO2_MASK | SUBBLK_CLOCK_CR_TIMER_MASK);
    /* Remove soft reset */
   SYSREG->SOFT_RESET_CR  &= ~(SOFT_RESET_CR_MMUART0_MASK | SOFT_RESET_CR_GPIO2_MASK | SOFT_RESET_CR_TIMER_MASK);
   SYSREG->SUBBLK_CLOCK_CR = 0xffffffff;
}

static int mpfs_init(const struct device *dev)
{
	ARG_UNUSED(dev);
     int int_num, i;
 
      PLIC_init();
#if 0

	    /* Initialization*/
	//    *(p_gpio+0x80) = 0xFFFFFFFFU;
	//    a_gpio[0] = (p_gpio+0x80);
	    *(p_gpio+0x20) = 0xFFFFFFFFU;
	    a_gpio[0] = (p_gpio+0x20);

	    for(volatile uint32_t i = 200; i>0; i--);

	    /* Configuring as O/P */
	//    *(p_gpio+0x40) = 0x00000005;
	//    a_gpio[1] = (p_gpio+0x40);
	    *(p_gpio+0x10) = 0x00000005;
	    a_gpio[1] = (p_gpio+0x10);

	    for(volatile uint32_t i = 200; i>0; i--);

	    /* Setting output to 0*/
	//    *(p_gpio+0x88) = 0x0;
	//    a_gpio[2] = (p_gpio+0x88);
	    *(p_gpio+0x22) = 0x0;
	    a_gpio[2] = (p_gpio+0x22);
	    for(volatile uint32_t i = 200; i>0; i--);

	    /* Setting GPIO_16 to 1*/
	//        *(p_gpio+0xA4) = 0x01 << 16;
	//        a_gpio[3] = (p_gpio+0xA4);
	    *(p_gpio+0x29) = 0x01 << 16;
	    a_gpio[3] = (p_gpio+0x29);
		for(volatile uint32_t i = 200; i>0; i--);
#endif        
    SYSREG->GPIO_INTERRUPT_FAB_CR = 0xFFFFFFFFUL;

    PLIC_SetPriority_Threshold(0);

    for (int_num = 0u; int_num <= GPIO2_NON_DIRECT_PLIC; int_num++)
    {
        PLIC_SetPriority(GPIO0_BIT0_or_GPIO2_BIT0_PLIC_0 + int_num, 2u);
    }

    MSS_GPIO_init(GPIO2_LO);

    for (int cnt = 16u; cnt< 20u; cnt++)
    {
        MSS_GPIO_config(GPIO2_LO,
                        cnt,
                        MSS_GPIO_OUTPUT_MODE);
    }

    MSS_GPIO_config(GPIO2_LO, MSS_GPIO_26, MSS_GPIO_OUTPUT_MODE);
    MSS_GPIO_config(GPIO2_LO, MSS_GPIO_27, MSS_GPIO_OUTPUT_MODE);
    MSS_GPIO_config(GPIO2_LO, MSS_GPIO_28, MSS_GPIO_OUTPUT_MODE);
    MSS_GPIO_set_output(GPIO2_LO, MSS_GPIO_26, 0u);
    MSS_GPIO_set_output(GPIO2_LO, MSS_GPIO_27, 0u);
    MSS_GPIO_set_output(GPIO2_LO, MSS_GPIO_28, 0u);

    MSS_GPIO_config(GPIO2_LO,
                    MSS_GPIO_30,
                    MSS_GPIO_INPUT_MODE | MSS_GPIO_IRQ_LEVEL_HIGH);

    MSS_GPIO_config(GPIO2_LO,
                    MSS_GPIO_31,
                    MSS_GPIO_INPUT_MODE | MSS_GPIO_IRQ_LEVEL_HIGH);

    MSS_GPIO_set_outputs(GPIO2_LO, 0u);
#if 1
    MSS_GPIO_set_output(GPIO2_LO, MSS_GPIO_16, 1u);
    MSS_GPIO_set_output(GPIO2_LO, MSS_GPIO_17, 1u);
    MSS_GPIO_set_output(GPIO2_LO, MSS_GPIO_18, 1u);
#endif

        
      	/* Setup IRQ handler for PLIC driver */
	IRQ_CONNECT(MSS_GPIO_30+13,
		    0,
		    icicleIRQHandler,
		    NULL,
		    0);
		    
      	/* Setup IRQ handler for PLIC driver */
	IRQ_CONNECT(MSS_GPIO_31+13,
		    0,
		    icicleIRQHandler,
		    NULL,
		    0);
		    
       MSS_GPIO_enable_irq(GPIO2_LO, MSS_GPIO_30);
       MSS_GPIO_enable_irq(GPIO2_LO, MSS_GPIO_31);
    
	/* Enable IRQ for PLIC driver */
	//irq_enable(RISCV_MACHINE_EXT_IRQ);

    //Read back registers
#if 0
       volatile uint32_t *p_prio = 0xC000004;
       volatile uint32_t * p_intrenab = 0xC002000;
       volatile uint32_t * p_intrpend = 0xC001000;
       volatile uint32_t prio_arr[54] = {0x00};
       volatile uint32_t intrenab_arr[6] = {0x00};
       volatile uint32_t intrpen_arr[6] = {0x00};

       //read Priority reg
       for(i = 1 ; i<= 53; i++)
       {
              prio_arr[i] = *p_prio;
              p_prio++;
       }

       //read Interrupt enable register
       for(i = 0 ; i < 6; i++)
       {
             intrenab_arr[i] = *p_intrenab;
             p_intrenab++;
       }

       //read Interrupt pending register
       for(i = 0 ; i < 6; i++)
       {
             intrpen_arr[i] = *p_intrpend;
             p_intrpend++;
       }

#endif



	return 0;
}

uint8_t  icicleIRQHandler(void)
{
    MSS_GPIO_set_outputs(GPIO2_LO, 0u);
    return 0;
}

SYS_INIT(pre_mpfs_init, PRE_KERNEL_1, 0);
SYS_INIT(mpfs_init, PRE_KERNEL_1, 2);

