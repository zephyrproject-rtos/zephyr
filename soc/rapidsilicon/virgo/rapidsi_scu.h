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

#define ISOLATION_CTRL_ELEM_WIDTH 1

// Isolation control
enum isolation_ctrl_offsets {
  ISOLATION_CTRL_AXI_0_OFFSET = 0,
  ISOLATION_CTRL_AXI_1_OFFSET = 1,
  ISOLATION_CTRL_AHB_OFFSET = 2,
  ISOLATION_CTRL_FPGA_GPIO_OFFSET = 3,
  ISOLATION_CTRL_FCB_OFFSET = 8,
  ISOLATION_CTRL_IRQ_OFFSET = 16,
  ISOLATION_CTRL_DMA_OFFSET = 24
};

/// Software reset control
enum virgo_scu_sw_rst_ctrl_msk {
  VIRGO_SCU_SW_RST_CTRL_MSK_SYSTEM = (0x1UL << 0),
  VIRGO_SCU_SW_RST_CTRL_MSK_BUS = (0x1UL << 1),
  VIRGO_SCU_SW_RST_CTRL_MSK_PER = (0x1UL << 4),
  VIRGO_SCU_SW_RST_CTRL_MSK_FPGA0 = (0x1UL << 5),
  VIRGO_SCU_SW_RST_CTRL_MSK_FPGA1 = (0x1UL << 6),
  VIRGO_SCU_SW_RST_CTRL_MSK_DMA = (0x1UL << 10),
};

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

/**
 * @brief Map the IRQ to subsystem (BCPU, ACPU or FPGA etc)
 *
 * @param IRQn IRQ Number
 * @param SubSystem It can be BCPU, ACPU or FPGA
 */
void scu_irq_map(enum map_mask_control_irq_id IRQn, uint8_t SubSystem);

/**
 * @brief UnMap the IRQ.
 *
 * @param IRQn IRQ Number
 */
void scu_irq_unmap(enum map_mask_control_irq_id IRQn);

/**
 * @brief Enable the IRQ.
 *
 * @param IRQn IRQ Number
 */
void scu_irq_enable(enum map_mask_control_irq_id IRQn);

/**
 * @brief Disable the IRQ.
 *
 * @param IRQn IRQ Number
 */
void scu_irq_disable(enum map_mask_control_irq_id IRQn);

/**
 * @brief Get the IRQ Register Value.
 *
 * @param IRQn IRQ Number
 *
 * @retval register value.
 */
uint32_t scu_get_irq_reg_val(enum map_mask_control_irq_id IRQn);

/**
 * @brief Assert resets to all subsystems.
 *
 */
void scu_assert_reset(void);

#define SCU_CHIP_ID_OFFSET      16
#define SCU_CHIP_ID_MASK        0x00FF0000

#define SCU_VENDOR_ID_OFFSET    24
#define SCU_VENDOR_ID_MASK      0xFF000000

/**
 * @brief Get the IRQ Register Value.
 *
 * @param chip_id output chip id
 * @param vendor_id output vendor id.
 */
void soc_get_id(uint8_t *chip_id, uint8_t *vendor_id);

/**
 * @brief Sets the Isolation Control bit
 *
 * @param inOffset Offset to subsystem. Can be referenced from @isolation_ctrl_offsets
 * @param inBit Value to set.
 */
void scu_set_isolation_ctrl(enum isolation_ctrl_offsets inOffset, bool inBit);

/**
 * @brief Writes the register at a particular offset
 *
 * @param reg Register to modify
 * @param offset Starting location of register to modify
 * @param width Number of bits to modify
 * @param value Value to write to the specified bits.
 */
void write_reg_val(volatile uint32_t *reg, uint32_t offset, uint32_t width,
                        uint32_t value);

/**
 * @brief Reads the register at a particular offset
 *
 * @param reg Register to read
 * @param offset Starting location of register to read
 * @param width Number of bits to read
 * 
 * @return value of reg at offset with number of bits equal to width
 */
uint32_t read_reg_val(const volatile uint32_t *reg, uint32_t offset,
                      uint32_t width);

/**
 * @brief Reads a particular register bit
 *
 * @param reg Register to read
 * @param bit bit number
 * 
 * @return value of reg bit
 */
uint32_t read_reg_bit(const volatile uint32_t *reg, uint32_t bit);                      
