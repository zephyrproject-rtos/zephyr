/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rapidsi_scu.h"

typedef struct {
    volatile uint32_t idrev;             /* 0x00 */
    volatile uint32_t sw_rst_control;    /* 0x04 */
    volatile uint32_t reserved[9];       /* 0x08 - 0x28 */
    volatile uint32_t irq_map_mask[32];  /* 0x2C - 0xA8 */   
    volatile uint32_t isolation_control; /* 0xAC */
} scu_registers_t;

#if DT_HAS_COMPAT_STATUS_OKAY(rapidsi_scu)
    static volatile scu_registers_t *s_scu_regs = (scu_registers_t*)DT_REG_ADDR(DT_NODELABEL(scu));
#else
    #warning "Turn the rapidsi,scu status to okay in DTS else *s_scu_regs = NULL;"
    static volatile scu_registers_t *s_scu_regs = NULL;
#endif

void write_reg_val(volatile uint32_t *reg, uint32_t offset, uint32_t width,
                        uint32_t value) {
  uint32_t mask;

  mask = (width == 32) ? 0xffffffff : (1U << width) - 1U;

  *reg = (*reg & ~(mask << offset)) | ((value & mask) << offset);
}

uint32_t read_reg_val(const volatile uint32_t *reg, uint32_t offset,
                      uint32_t width) {
  uint32_t mask;

  mask = (width == 32) ? 0xffffffff : (1U << width) - 1U;

  return (*reg >> offset) & (mask);
}

uint32_t read_reg_bit(const volatile uint32_t *reg, uint32_t bit)
{
    return read_reg_val(reg, bit, 1U);
}

void scu_assert_reset(void)
{
  uint32_t reset_flags =
      (VIRGO_SCU_SW_RST_CTRL_MSK_BUS | VIRGO_SCU_SW_RST_CTRL_MSK_PER |
       VIRGO_SCU_SW_RST_CTRL_MSK_FPGA0 | VIRGO_SCU_SW_RST_CTRL_MSK_FPGA1 |
       VIRGO_SCU_SW_RST_CTRL_MSK_DMA);
  s_scu_regs->sw_rst_control |= reset_flags;
}

void soc_get_id(uint8_t *chip_id, uint8_t *vendor_id)
{
    *chip_id = ((s_scu_regs->idrev & SCU_CHIP_ID_MASK) >> SCU_CHIP_ID_OFFSET);
    *vendor_id = ((s_scu_regs->idrev & SCU_VENDOR_ID_MASK) >> SCU_VENDOR_ID_OFFSET);
}

void scu_irq_map(enum map_mask_control_irq_id IRQn, uint8_t SubSystem)
{
    // Clearing Existing Value of the Mask
    s_scu_regs->irq_map_mask[IRQn] &= ~(SCU_IRQ_MAP_MASK << SCU_IRQ_MAP_OFFSET);
    // Mapping to SubSystem
    s_scu_regs->irq_map_mask[IRQn] |= (SubSystem << SCU_IRQ_MAP_OFFSET);
}

void scu_irq_unmap(enum map_mask_control_irq_id IRQn)
{
    s_scu_regs->irq_map_mask[IRQn] &= ~(SCU_IRQ_MAP_MASK << SCU_IRQ_MAP_OFFSET);
}

void scu_irq_enable(enum map_mask_control_irq_id IRQn)
{
    s_scu_regs->irq_map_mask[IRQn] |= SCU_MASK_IRQ_ENABLE;
}

void scu_irq_disable(enum map_mask_control_irq_id IRQn)
{
    s_scu_regs->irq_map_mask[IRQn] &= ~(SCU_MASK_IRQ_ENABLE << SCU_IRQ_MASK_OFFSET);
}

uint32_t scu_get_irq_reg_val(enum map_mask_control_irq_id IRQn)
{
    return s_scu_regs->irq_map_mask[IRQn];
}

void scu_set_isolation_ctrl(enum isolation_ctrl_offsets inOffset, bool inBit)
{
    write_reg_val(&s_scu_regs->isolation_control, inOffset,
                ISOLATION_CTRL_ELEM_WIDTH, inBit);
}