/*
 * Copyright (c) 2024 Rapid Silicon.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * VIRGO SoC specific interrupt map and mask functions
 */

#include <zephyr/kernel.h>

#define SCU_IRQ_MASK_OFFSET   0
#define SCU_IRQ_MAP_OFFSET    16

#define SCU_MASK_IRQ_DISABLE  0x0
#define SCU_MASK_IRQ_ENABLE   0x1

#define SCU_IRQ_MAP_TO_NONE   0x0
#define SCU_IRQ_MAP_TO_BCPU   0x1
#define SCU_IRQ_MAP_TO_RSVD   0x2
#define SCU_IRQ_MAP_TO_FPGA   0x4
#define SCU_IRQ_MAP_MASK      0x7

/*********************************
* The following table is aligned
* with respect to the scu map and
* mask interrupt control register.
* Each element has an offset of 
* -1 w.r.t the CLIC mappings.
*********************************/
enum map_mask_control_irq_id {
  IRQ_ID_BCPU_WDT = 0,
  IRQ_ID_GPT = 1,
  IRQ_ID_RESERVED1 = 2,
  IRQ_ID_RESERVED2 = 3,
  IRQ_ID_UART0 = 4,
  IRQ_ID_RESERVED4 = 5,
  IRQ_ID_SPI = 6,
  IRQ_ID_I2C = 7,
  IRQ_ID_GPIO = 8,
  IRQ_ID_SYSTEM_DMA = 9,
  IRQ_ID_RESERVED9 = 10,
  IRQ_ID_AHB_MATRIX = 11,
  IRQ_ID_RESERVED11 = 12,
  IRQ_ID_RESERVED12 = 13,
  IRQ_ID_RESERVED13 = 14,
  // Encryption engine
  IRQ_ID_PUFCC = 15,
  // FPGA GENERATED
  IRQ_ID_FPGAGEN0 = 16,   
  IRQ_ID_FPGAGEN1 = 17,   
  IRQ_ID_FPGAGEN2 = 18,   
  IRQ_ID_FPGAGEN3 = 19,   
  IRQ_ID_FPGAGEN4 = 20,   
  IRQ_ID_FPGAGEN5 = 21,   
  IRQ_ID_FPGAGEN6 = 22,   
  IRQ_ID_FPGAGEN7 = 23,   
  IRQ_ID_FPGAGEN8 = 24,   
  IRQ_ID_FPGAGEN9 = 25,   
  IRQ_ID_FPGAGEN10 = 26,  
  IRQ_ID_FPGAGEN11 = 27,  
  IRQ_ID_FPGAGEN12 = 28,  
  IRQ_ID_FPGAGEN13 = 29,  
  IRQ_ID_FPGAGEN14 = 30,  
  IRQ_ID_FPGAGEN15 = 31  
};

void scu_irq_map(enum map_mask_control_irq_id IRQn, uint8_t SubSystem);
void scu_irq_unmap(enum map_mask_control_irq_id IRQn);
void scu_irq_enable(enum map_mask_control_irq_id IRQn);
void scu_irq_disable(enum map_mask_control_irq_id IRQn);

#define SCU_CHIP_ID_OFFSET      16
#define SCU_CHIP_ID_MASK        0x00FF0000

#define SCU_VENDOR_ID_OFFSET    24
#define SCU_VENDOR_ID_MASK      0xFF000000

void soc_get_id(uint8_t *chip_id, uint8_t *vendor_id);
