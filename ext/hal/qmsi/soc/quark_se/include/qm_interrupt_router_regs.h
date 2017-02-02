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
 * Quark SE SoC Interrupt Router registers.
 *
 * @defgroup groupQUARKSEINTERRUPTROUTER SoC Interrupt Router (SE)
 * @{
 */

/**
 * Masks for single source interrupts in the Interrupt Router.
 * To enable:  reg &= ~(MASK)
 * To disable: reg |= MASK;
 */
#define QM_IR_INT_LMT_MASK BIT(0)
#define QM_IR_INT_SS_MASK BIT(8)

/* Masks for single source halts in the Interrupt Router. */
#define QM_IR_INT_LMT_HALT_MASK BIT(16)
#define QM_IR_INT_SS_HALT_MASK BIT(24)

/**
 * Interrupt Router macros to determine if the specified peripheral interrupt
 * mask has been locked.
 */
#define QM_IR_SS_INT_LOCK_HALT_MASK(_peripheral_)                              \
	(QM_INTERRUPT_ROUTER->lock_int_mask_reg & BIT(3))
#define QM_IR_LMT_INT_LOCK_HALT_MASK(_peripheral_)                             \
	(QM_INTERRUPT_ROUTER->lock_int_mask_reg & BIT(2))
#define QM_IR_SS_INT_LOCK_MASK(_peripheral_)                                   \
	(QM_INTERRUPT_ROUTER->lock_int_mask_reg & BIT(1))
#define QM_IR_LMT_INT_LOCK_MASK(_peripheral_)                                  \
	(QM_INTERRUPT_ROUTER->lock_int_mask_reg & BIT(0))

/* Interrupt Router Unmask interrupts for a peripheral. */
#define QM_IR_UNMASK_LMT_INTERRUPTS(_peripheral_)                              \
	(_peripheral_ &= ~(QM_IR_INT_LMT_MASK))
#define QM_IR_UNMASK_SS_INTERRUPTS(_peripheral_)                               \
	(_peripheral_ &= ~(QM_IR_INT_SS_MASK))

/* Mask interrupts for a peripheral. */
#define QM_IR_MASK_LMT_INTERRUPTS(_peripheral_)                                \
	(_peripheral_ |= QM_IR_INT_LMT_MASK)
#define QM_IR_MASK_SS_INTERRUPTS(_peripheral_)                                 \
	(_peripheral_ |= QM_IR_INT_SS_MASK)

/* Unmask halt for a peripheral. */
#define QM_IR_UNMASK_LMT_HALTS(_peripheral_)                                   \
	(_peripheral_ &= ~(QM_IR_INT_LMT_HALT_MASK))
#define QM_IR_UNMASK_SS_HALTS(_peripheral_)                                    \
	(_peripheral_ &= ~(QM_IR_INT_SS_HALT_MASK))

/* Mask halt for a peripheral. */
#define QM_IR_MASK_LMT_HALTS(_peripheral_)                                     \
	(_peripheral_ |= QM_IR_INT_LMT_HALT_MASK)
#define QM_IR_MASK_SS_HALTS(_peripheral_)                                      \
	(_peripheral_ |= QM_IR_INT_SS_HALT_MASK)

#define QM_IR_GET_LMT_MASK(_peripheral_) (_peripheral_ & QM_IR_INT_LMT_MASK)
#define QM_IR_GET_LMT_HALT_MASK(_peripheral_)                                  \
	(_peripheral_ & QM_IR_INT_LMT_HALT_MASK)

#define QM_IR_GET_SS_MASK(_peripheral_) (_peripheral_ & QM_IR_INT_SS_MASK)
#define QM_IR_GET_SS_HALT_MASK(_peripheral_)                                   \
	(_peripheral_ & QM_IR_INT_SS_HALT_MASK)

/**
 * Mailbox Interrupt Mask enable/disable definitions
 *
 * \#defines use the channel number to determine the register and bit shift to
 * use.
 * The interrupt destination adds an offset to the bit shift.
 */
#define QM_IR_MBOX_ENABLE_LMT_INT_MASK(N)                                      \
	QM_INTERRUPT_ROUTER->mailbox_0_int_mask &=                             \
	    ~(BIT(N + QM_MBOX_HOST_MASK_OFFSET))
#define QM_IR_MBOX_DISABLE_LMT_INT_MASK(N)                                     \
	QM_INTERRUPT_ROUTER->mailbox_0_int_mask |=                             \
	    (BIT(N + QM_MBOX_HOST_MASK_OFFSET))
#define QM_IR_MBOX_ENABLE_SS_INT_MASK(N)                                       \
	QM_INTERRUPT_ROUTER->mailbox_0_int_mask &=                             \
	    ~(BIT(N + QM_MBOX_SS_MASK_OFFSET))
#define QM_IR_MBOX_DISABLE_SS_INT_MASK(N)                                      \
	QM_INTERRUPT_ROUTER->mailbox_0_int_mask |=                             \
	    (BIT(N + QM_MBOX_SS_MASK_OFFSET))

/**
 * Mailbox Interrupt Halt Mask enable/disable definitions
 *
 * \#defines use the channel number to determine the register and bit shift to
 * use.
 * The interrupt destination adds an offset to the bit shift,
 * see above for the bit position layout
 */
#define QM_IR_MBOX_ENABLE_LMT_INT_HALT_MASK(N)                                 \
	QM_INTERRUPT_ROUTER->mailbox_0_int_mask &=                             \
	    ~(BIT(N + QM_MBOX_HOST_HALT_MASK_OFFSET))
#define QM_IR_MBOX_DISABLE_LMT_INT_HALT_MASK(N)                                \
	QM_INTERRUPT_ROUTER->mailbox_0_int_mask |=                             \
	    (BIT(N + QM_MBOX_HOST_HALT_MASK_OFFSET))
#define QM_IR_MBOX_ENABLE_SS_INT_HALT_MASK(N)                                  \
	QM_INTERRUPT_ROUTER->mailbox_0_int_mask &=                             \
	    ~(BIT(N + QM_MBOX_SS_HALT_MASK_OFFSET))
#define QM_IR_MBOX_DISABLE_SS_INT_HALT_MASK(N)                                 \
	QM_INTERRUPT_ROUTER->mailbox_0_int_mask |=                             \
	    (BIT(N + QM_MBOX_SS_HALT_MASK_OFFSET))

/**
 * Mailbox interrupt mask definitions to return the current mask values
 */
#define QM_IR_MBOX_SS_INT_HALT_MASK                                            \
	((QM_MBOX_SS_HALT_MASK_MASK &                                          \
	  QM_INTERRUPT_ROUTER->mailbox_0_int_mask) >>                          \
	 QM_MBOX_SS_HALT_MASK_OFFSET)
#define QM_IR_MBOX_LMT_INT_HALT_MASK                                           \
	((QM_MBOX_HOST_HALT_MASK_MASK &                                        \
	  QM_INTERRUPT_ROUTER->mailbox_0_int_mask) >>                          \
	 QM_MBOX_SS_HALT_MASK_OFFSET)
#define QM_IR_MBOX_SS_ALL_INT_MASK                                             \
	((QM_MBOX_SS_MASK_MASK & QM_INTERRUPT_ROUTER->mailbox_0_int_mask) >>   \
	 QM_MBOX_SS_MASK_OFFSET)
#define QM_IR_MBOX_LMT_ALL_INT_MASK                                            \
	(QM_MBOX_HOST_MASK_MASK & QM_INTERRUPT_ROUTER->mailbox_0_int_mask)

/**
 * Mailbox interrupt macros to determine if the specified mailbox interrupt mask
 * has been locked.
 */
#define QM_IR_MBOX_SS_INT_LOCK_HALT_MASK(N)                                    \
	(QM_INTERRUPT_ROUTER->lock_int_mask_reg & BIT(3))
#define QM_IR_MBOX_LMT_INT_LOCK_HALT_MASK(N)                                   \
	(QM_INTERRUPT_ROUTER->lock_int_mask_reg & BIT(2))
#define QM_IR_MBOX_SS_INT_LOCK_MASK(N)                                         \
	(QM_INTERRUPT_ROUTER->lock_int_mask_reg & BIT(1))
#define QM_IR_MBOX_LMT_INT_LOCK_MASK(N)                                        \
	(QM_INTERRUPT_ROUTER->lock_int_mask_reg & BIT(0))

/**
 * Mailbox macros to check if a particular mailbox has been routed to a core.
 */
#define QM_IR_MBOX_IS_LMT_INT_MASK_EN(N)                                       \
	~(QM_IR_MBOX_LMT_ALL_INT_MASK & ((1 << (N))))
#define QM_IR_MBOX_IS_SS_INT_MASK_EN(N)                                        \
	~(QM_IR_MBOX_SS_ALL_INT_MASK & ((1 << (QM_MBOX_SS_MASK_OFFSET + (N)))))

#define QM_IR_UNMASK_COMPARATOR_LMT_INTERRUPTS(n)                              \
	(QM_INTERRUPT_ROUTER->comparator_0_host_int_mask &= ~(BIT(n)))
#define QM_IR_MASK_COMPARATOR_LMT_INTERRUPTS(n)                                \
	(QM_INTERRUPT_ROUTER->comparator_0_host_int_mask |= BIT(n))
#define QM_IR_UNMASK_COMPARATOR_LMT_HALTS(n)                                   \
	(QM_INTERRUPT_ROUTER->comparator_0_host_halt_int_mask &= ~(BIT(n)))
#define QM_IR_MASK_COMPARATOR_LMT_HALTS(n)                                     \
	(QM_INTERRUPT_ROUTER->comparator_0_host_halt_int_mask |= BIT(n))

#define QM_IR_UNMASK_COMPARATOR_SS_INTERRUPTS(n)                               \
	(QM_INTERRUPT_ROUTER->comparator_0_ss_int_mask &= ~(BIT(n)))
#define QM_IR_MASK_COMPARATOR_SS_INTERRUPTS(n)                                 \
	(QM_INTERRUPT_ROUTER->comparator_0_ss_int_mask |= BIT(n))
#define QM_IR_UNMASK_COMPARATOR_SS_HALTS(n)                                    \
	(QM_INTERRUPT_ROUTER->comparator_0_ss_halt_int_mask &= ~(BIT(n)))
#define QM_IR_MASK_COMPARATOR_SS_HALTS(n)                                      \
	(QM_INTERRUPT_ROUTER->comparator_0_ss_halt_int_mask |= BIT(n))

/* Define macros for use by the active core. */
#if (QM_LAKEMONT)
#define QM_IR_UNMASK_INTERRUPTS(_peripheral_)                                  \
	QM_IR_UNMASK_LMT_INTERRUPTS(_peripheral_)
#define QM_IR_MASK_INTERRUPTS(_peripheral_)                                    \
	QM_IR_MASK_LMT_INTERRUPTS(_peripheral_)
#define QM_IR_UNMASK_HALTS(_peripheral_) QM_IR_UNMASK_LMT_HALTS(_peripheral_)
#define QM_IR_MASK_HALTS(_peripheral_) QM_IR_MASK_LMT_HALTS(_peripheral_)

#define QM_IR_INT_LOCK_MASK(_peripheral_) QM_IR_LMT_INT_LOCK_MASK(_peripheral_)
#define QM_IR_INT_LOCK_HALT_MASK(_peripheral_)                                 \
	QM_IR_LMT_INT_LOCK_MASK(_peripheral_)

#define QM_IR_INT_MASK QM_IR_INT_LMT_MASK
#define QM_IR_INT_HALT_MASK QM_IR_INT_LMT_HALT_MASK
#define QM_IR_GET_MASK(_peripheral_) QM_IR_GET_LMT_MASK(_peripheral_)
#define QM_IR_GET_HALT_MASK(_peripheral_) QM_IR_GET_LMT_HALT_MASK(_peripheral_)

#define QM_IR_UNMASK_COMPARATOR_INTERRUPTS(n)                                  \
	QM_IR_UNMASK_COMPARATOR_LMT_INTERRUPTS(n)
#define QM_IR_MASK_COMPARATOR_INTERRUPTS(n)                                    \
	QM_IR_MASK_COMPARATOR_LMT_INTERRUPTS(n)
#define QM_IR_UNMASK_COMPARATOR_HALTS(n) QM_IR_UNMASK_COMPARATOR_LMT_HALTS(n)
#define QM_IR_MASK_COMPARATOR_HALTS(n) QM_IR_MASK_COMPARATOR_LMT_HALTS(n)

#elif(QM_SENSOR)
#define QM_IR_UNMASK_INTERRUPTS(_peripheral_)                                  \
	QM_IR_UNMASK_SS_INTERRUPTS(_peripheral_)
#define QM_IR_MASK_INTERRUPTS(_peripheral_)                                    \
	QM_IR_MASK_SS_INTERRUPTS(_peripheral_)
#define QM_IR_UNMASK_HALTS(_peripheral_) QM_IR_UNMASK_SS_HALTS(_peripheral_)
#define QM_IR_MASK_HALTS(_peripheral_) QM_IR_MASK_SS_HALTS(_peripheral_)

#define QM_IR_INT_LOCK_MASK(_peripheral_) QM_IR_SS_INT_LOCK_MASK(_peripheral_)
#define QM_IR_INT_LOCK_HALT_MASK(_peripheral_)                                 \
	QM_IR_SS_INT_LOCK_MASK(_peripheral_)

#define QM_IR_INT_MASK QM_IR_INT_SS_MASK
#define QM_IR_INT_HALT_MASK QM_IR_INT_SS_HALT_MASK
#define QM_IR_GET_MASK(_peripheral_) QM_IR_GET_SS_MASK(_peripheral_)
#define QM_IR_GET_HALT_MASK(_peripheral_) QM_IR_GET_SS_HALT_MASK(_peripheral_)

#define QM_IR_UNMASK_COMPARATOR_INTERRUPTS(n)                                  \
	QM_IR_UNMASK_COMPARATOR_SS_INTERRUPTS(n)
#define QM_IR_MASK_COMPARATOR_INTERRUPTS(n)                                    \
	QM_IR_MASK_COMPARATOR_SS_INTERRUPTS(n)
#define QM_IR_UNMASK_COMPARATOR_HALTS(n) QM_IR_UNMASK_COMPARATOR_SS_HALTS(n)
#define QM_IR_MASK_COMPARATOR_HALTS(n) QM_IR_MASK_COMPARATOR_SS_HALTS(n)

#else
#error "No active core selected."
#endif

/** SS I2C Interrupt register map. */
typedef struct {
	QM_RW uint32_t err_mask;
	QM_RW uint32_t rx_avail_mask;
	QM_RW uint32_t tx_req_mask;
	QM_RW uint32_t stop_det_mask;
} int_ss_i2c_reg_t;

/** SS SPI Interrupt register map. */
typedef struct {
	QM_RW uint32_t err_int_mask;
	QM_RW uint32_t rx_avail_mask;
	QM_RW uint32_t tx_req_mask;
} int_ss_spi_reg_t;

/** Interrupt register map. */
typedef struct {
	QM_RW uint32_t ss_adc_0_error_int_mask; /**< Sensor ADC 0 Error. */
	QM_RW uint32_t ss_adc_0_int_mask;       /**< Sensor ADC 0. */
	QM_RW uint32_t ss_gpio_0_int_mask;      /**< Sensor GPIO 0. */
	QM_RW uint32_t ss_gpio_1_int_mask;      /**< Sensor GPIO 1. */
	int_ss_i2c_reg_t ss_i2c_0_int;		/**< Sensor I2C 0 Masks. */
	int_ss_i2c_reg_t ss_i2c_1_int;		/**< Sensor I2C 1 Masks. */
	int_ss_spi_reg_t ss_spi_0_int;		/**< Sensor SPI 0 Masks. */
	int_ss_spi_reg_t ss_spi_1_int;		/**< Sensor SPI 1 Masks. */
	QM_RW uint32_t i2c_master_0_int_mask;   /**< I2C Master 0. */
	QM_RW uint32_t i2c_master_1_int_mask;   /**< I2C Master 1. */
	QM_R uint32_t reserved;
	QM_RW uint32_t spi_master_0_int_mask; /**< SPI Master 0. */
	QM_RW uint32_t spi_master_1_int_mask; /**< SPI Master 1. */
	QM_RW uint32_t spi_slave_0_int_mask;  /**< SPI Slave 0. */
	QM_RW uint32_t uart_0_int_mask;       /**< UART 0. */
	QM_RW uint32_t uart_1_int_mask;       /**< UART 1. */
	QM_RW uint32_t i2s_0_int_mask;	/**< I2S 0. */
	QM_RW uint32_t gpio_0_int_mask;       /**< GPIO 0. */
	QM_RW uint32_t pwm_0_int_mask;	/**< PWM 0. */
	QM_RW uint32_t usb_0_int_mask;	/**< USB 0. */
	QM_RW uint32_t rtc_0_int_mask;	/**< RTC 0. */
	QM_RW uint32_t wdt_0_int_mask;	/**< WDT 0. */
	QM_RW uint32_t dma_0_int_0_mask;      /**< DMA 0 Ch 0. */
	QM_RW uint32_t dma_0_int_1_mask;      /**< DMA 0 Ch 1. */
	QM_RW uint32_t dma_0_int_2_mask;      /**< DMA 0 Ch 2. */
	QM_RW uint32_t dma_0_int_3_mask;      /**< DMA 0 Ch 3. */
	QM_RW uint32_t dma_0_int_4_mask;      /**< DMA 0 Ch 4. */
	QM_RW uint32_t dma_0_int_5_mask;      /**< DMA 0 Ch 5. */
	QM_RW uint32_t dma_0_int_6_mask;      /**< DMA 0 Ch 6. */
	QM_RW uint32_t dma_0_int_7_mask;      /**< DMA 0 Ch 7. */
	/** Mailbox 0 Combined 8 Channel Host and Sensor Masks. */
	QM_RW uint32_t mailbox_0_int_mask;
	/** Comparator Sensor Halt Mask. */
	QM_RW uint32_t comparator_0_ss_halt_int_mask;
	/** Comparator Host Halt Mask. */
	QM_RW uint32_t comparator_0_host_halt_int_mask;
	/** Comparator Sensor Mask. */
	QM_RW uint32_t comparator_0_ss_int_mask;
	/** Comparator Host Mask. */
	QM_RW uint32_t comparator_0_host_int_mask;
	QM_RW uint32_t host_bus_error_int_mask; /**< Host bus error. */
	QM_RW uint32_t dma_0_error_int_mask;    /**< DMA 0 Error. */
	QM_RW uint32_t sram_mpr_0_int_mask;     /**< SRAM MPR 0. */
	QM_RW uint32_t flash_mpr_0_int_mask;    /**< Flash MPR 0. */
	QM_RW uint32_t flash_mpr_1_int_mask;    /**< Flash MPR 1. */
	QM_RW uint32_t aonpt_0_int_mask;	/**< AONPT 0. */
	QM_RW uint32_t adc_0_pwr_int_mask;      /**< ADC 0 PWR. */
	QM_RW uint32_t adc_0_cal_int_mask;      /**< ADC 0 CAL. */
	QM_RW uint32_t aon_gpio_0_int_mask;     /**< AON GPIO 0. */
	QM_RW uint32_t lock_int_mask_reg; /**< Interrupt Mask Lock Register. */
} qm_interrupt_router_reg_t;

/* Number of SCSS interrupt mask registers (excluding mask lock register). */
#define QM_INTERRUPT_ROUTER_MASK_NUMREG                                        \
	((sizeof(qm_interrupt_router_reg_t) / sizeof(uint32_t)) - 1)

/* Default POR SCSS interrupt mask (all interrupts masked). */
#define QM_INTERRUPT_ROUTER_MASK_DEFAULT (0xFFFFFFFF)

#if (UNIT_TEST)
qm_interrupt_router_reg_t test_interrupt_router;
#define QM_INTERRUPT_ROUTER                                                    \
	((qm_interrupt_router_reg_t *)(&test_interrupt_router))

#else
/* System control subsystem interrupt masking register block. */
#define QM_INTERRUPT_ROUTER_BASE (0xB0800400)
#define QM_INTERRUPT_ROUTER                                                    \
	((qm_interrupt_router_reg_t *)QM_INTERRUPT_ROUTER_BASE)
#endif

#define QM_IR_DMA_ERROR_HOST_MASK (0x000000FF)
#define QM_IR_DMA_ERROR_SS_MASK (0x0000FF00)

#if (QM_LAKEMONT)
#define QM_IR_DMA_ERROR_MASK QM_IR_DMA_ERROR_HOST_MASK
#elif(QM_SENSOR)
#define QM_IR_DMA_ERROR_MASK QM_IR_DMA_ERROR_SS_MASK
#endif

/** @} */

#endif /* __QM_INTERRUPT_ROUTER_REGS_H__ */
