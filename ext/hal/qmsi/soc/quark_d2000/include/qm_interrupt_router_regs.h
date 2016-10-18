/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_INTERRUPT_ROUTER_REGS_H__
#define __QM_INTERRUPT_ROUTER_REGS_H__

/**
 * Quark D2000 SoC Interrupt Router registers.
 *
 * @defgroup groupQUARKD2000INTERRUPTROUTER SoC Interrupt Router (D2000)
 * @{
 */

/**
 * Masks for single source interrupts in the Interrupt Router.
 * To enable:  reg &= ~(MASK)
 * To disable: reg |= MASK;
 */
#define QM_IR_INT_LMT_MASK BIT(0)

/* Masks for single source halts in the Interrupt Router. */
#define QM_IR_INT_LMT_HALT_MASK BIT(16)

/* Interrupt Router Unmask interrupts for a peripheral. */
#define QM_IR_UNMASK_LMT_INTERRUPTS(_peripheral_)                              \
	(_peripheral_ &= ~(QM_IR_INT_LMT_MASK))

/* Mask interrupts for a peripheral. */
#define QM_IR_MASK_LMT_INTERRUPTS(_peripheral_)                                \
	(_peripheral_ |= QM_IR_INT_LMT_MASK)

/* Unmask halt for a peripheral. */
#define QM_IR_UNMASK_LMT_HALTS(_peripheral_)                                   \
	(_peripheral_ &= ~(QM_IR_INT_LMT_HALT_MASK))

/* Mask halt for a peripheral. */
#define QM_IR_MASK_LMT_HALTS(_peripheral_)                                     \
	(_peripheral_ |= QM_IR_INT_LMT_HALT_MASK)

#define QM_IR_GET_LMT_MASK(_peripheral_) (_peripheral_ & QM_IR_INT_LMT_MASK)
#define QM_IR_GET_LMT_HALT_MASK(_peripheral_)                                  \
	(_peripheral_ & QM_IR_INT_LMT_HALT_MASK)

/* Define macros for use by the active core. */
#if (QM_LAKEMONT)
#define QM_IR_UNMASK_INTERRUPTS(_peripheral_)                                  \
	QM_IR_UNMASK_LMT_INTERRUPTS(_peripheral_)
#define QM_IR_MASK_INTERRUPTS(_peripheral_)                                    \
	QM_IR_MASK_LMT_INTERRUPTS(_peripheral_)
#define QM_IR_UNMASK_HALTS(_peripheral_) QM_IR_UNMASK_LMT_HALTS(_peripheral_)
#define QM_IR_MASK_HALTS(_peripheral_) QM_IR_MASK_LMT_HALTS(_peripheral_)

#define QM_IR_INT_MASK QM_IR_INT_LMT_MASK
#define QM_IR_INT_HALT_MASK QM_IR_INT_LMT_HALT_MASK
#define QM_IR_GET_MASK(_peripheral_) QM_IR_GET_LMT_MASK(_peripheral_)
#define QM_IR_GET_HALT_MASK(_peripheral_) QM_IR_GET_LMT_HALT_MASK(_peripheral_)

#else
#error "No active core selected."
#endif

/** Interrupt register map. */
typedef struct {
	QM_RW uint32_t i2c_master_0_int_mask; /**< I2C Master 0, Mask 0. */
	QM_R uint32_t reserved[2];
	QM_RW uint32_t spi_master_0_int_mask; /**< SPI Master 0, Mask 3. */
	QM_R uint32_t reserved1;
	QM_RW uint32_t spi_slave_0_int_mask; /**< SPI Slave 0, Mask 5. */
	QM_RW uint32_t uart_0_int_mask;      /**< UART 0, Mask 6. */
	QM_RW uint32_t uart_1_int_mask;      /**< UART 1, Mask 7. */
	QM_RW uint32_t reserved2;
	QM_RW uint32_t gpio_0_int_mask;  /**< GPIO 0, Mask 9. */
	QM_RW uint32_t timer_0_int_mask; /**< Timer 0, Mask 10. */
	QM_R uint32_t reserved3;
	QM_RW uint32_t rtc_0_int_mask;   /**< RTC 0, Mask 12. */
	QM_RW uint32_t wdt_0_int_mask;   /**< WDT 0, Mask 13. */
	QM_RW uint32_t dma_0_int_0_mask; /**< DMA 0 int 0, Mask 14. */
	QM_RW uint32_t dma_0_int_1_mask; /**< DMA 0 int 1, Mask 15. */
	QM_RW uint32_t reserved4[8];
	/** Comparator 0 Host halt, Mask 24. */
	QM_RW uint32_t comparator_0_host_halt_int_mask;
	QM_R uint32_t reserved5;
	/** Comparator 0 Host, Mask 26. */
	QM_RW uint32_t comparator_0_host_int_mask;
	QM_RW uint32_t host_bus_error_int_mask; /**< Host bus error, Mask 27. */
	QM_RW uint32_t dma_0_error_int_mask;    /**< DMA 0 Error, Mask 28. */
	QM_RW uint32_t sram_mpr_0_int_mask;     /**< SRAM MPR 0, Mask 29. */
	QM_RW uint32_t flash_mpr_0_int_mask;    /**< Flash MPR 0, Mask 30. */
	QM_R uint32_t reserved6;
	QM_RW uint32_t aonpt_0_int_mask;   /**< AONPT 0, Mask 32. */
	QM_RW uint32_t adc_0_pwr_int_mask; /**< ADC 0 PWR, Mask 33. */
	QM_RW uint32_t adc_0_cal_int_mask; /**< ADC 0 CAL, Mask 34. */
	QM_R uint32_t reserved7;
	QM_RW uint32_t lock_int_mask_reg; /**< Interrupt Mask Lock Register. */
} qm_interrupt_router_reg_t;

/* Number of interrupt mask registers (excluding mask lock register). */
#define QM_INTERRUPT_ROUTER_MASK_NUMREG                                        \
	((sizeof(qm_interrupt_router_reg_t) / sizeof(uint32_t)) - 1)

/* Default POR interrupt mask (all interrupts masked). */
#define QM_INTERRUPT_ROUTER_MASK_DEFAULT (0xFFFFFFFF)

#if (UNIT_TEST)
qm_interrupt_router_reg_t test_interrupt_router;
#define QM_INTERRUPT_ROUTER                                                    \
	((qm_interrupt_router_reg_t *)(&test_interrupt_router))

#else
#define QM_INTERRUPT_ROUTER_BASE (0xB0800448)
#define QM_INTERRUPT_ROUTER                                                    \
	((qm_interrupt_router_reg_t *)QM_INTERRUPT_ROUTER_BASE)
#endif

#define QM_IR_DMA_ERROR_HOST_MASK (0x00000003)

/** @} */

#endif /* __QM_INTERRUPT_ROUTER_REGS_H__ */
