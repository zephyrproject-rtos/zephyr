/*
 * Copyright (c) 2017, Intel Corporation
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

#ifndef __QM_ISR_H__
#define __QM_ISR_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Interrupt Service Routines.
 *
 * @defgroup groupISR ISR
 * @{
 */

#if (QUARK_D2000)
/**
 * ISR for ADC 0 convert and calibration interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_ADC_0_CAL_INT, qm_adc_0_cal_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_adc_0_cal_isr);

/**
 * ISR for ADC 0 change mode interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_ADC_0_PWR_INT, qm_adc_0_pwr_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_adc_0_pwr_isr);
#endif /* QUARK_D2000 */

/**
 * ISR for Always-on Periodic Timer 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_AONPT_0_INT, qm_aonpt_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_aonpt_0_isr);

/**
 * ISR for Analog Comparator 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_COMPARATOR_0_INT, qm_comparator_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_comparator_0_isr);

/**
 * ISR for DMA error interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_DMA_0_ERROR_INT, qm_dma_0_error_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_dma_0_error_isr);

/**
 * ISR for DMA channel 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_DMA_0_INT_0, qm_dma_0_isr_0);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_dma_0_isr_0);

/**
 * ISR for DMA channel 1 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_DMA_0_INT_1, qm_dma_0_isr_1);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_dma_0_isr_1);

#if (QUARK_SE)
/**
 * ISR for DMA channel 2 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_DMA_0_INT_2, qm_dma_0_isr_2);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_dma_0_isr_2);

/**
 * ISR for DMA channel 3 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_DMA_0_INT_3, qm_dma_0_isr_3);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_dma_0_isr_3);

/**
 * ISR for DMA channel 4 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_DMA_0_INT_4, qm_dma_0_isr_4);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_dma_0_isr_4);

/**
 * ISR for DMA channel 5 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_DMA_0_INT_5, qm_dma_0_isr_5);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_dma_0_isr_5);

/**
 * ISR for DMA channel 6 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_DMA_0_INT_6, qm_dma_0_isr_6);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_dma_0_isr_6);

/**
 * ISR for DMA 0 channel 7 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_DMA_0_INT_7, qm_dma_0_isr_7);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_dma_0_isr_7);
#endif /* QUARK_SE */

/**
 * ISR for FPR 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_FLASH_MPR_0_INT, qm_flash_mpr_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_flash_mpr_0_isr);

/**
 * ISR for FPR 1 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_FLASH_MPR_1_INT, qm_flash_mpr_1_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_flash_mpr_1_isr);

/**
 * ISR for GPIO 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_GPIO_0_INT, qm_gpio_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_gpio_0_isr);

#if (HAS_AON_GPIO)
/**
 * ISR for AON GPIO 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_AON_GPIO_0_INT, qm_aon_gpio_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_aon_gpio_0_isr);
#endif /* HAS_AON_GPIO */

/**
 * ISR for I2C 0 irq mode transfer interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_I2C_0_INT, qm_i2c_0_irq_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_i2c_0_irq_isr);

/**
 * ISR for I2C 1 irq mode transfer interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_I2C_1_INT, qm_i2c_1_irq_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_i2c_1_irq_isr);

/**
 * ISR for I2C 0 dma  mode transfer interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_I2C_0_INT, qm_i2c_0_dma_isr);
 * @endcode if DMA based transfers are used.
 */
QM_ISR_DECLARE(qm_i2c_0_dma_isr);

/**
 * ISR for I2C 1 dma mode transfer interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_I2C_1_INT, qm_i2c_1_dma_isr);
 * @endcode if DMA based transfers are used.
 */
QM_ISR_DECLARE(qm_i2c_1_dma_isr);

/**
 * ISR for Mailbox interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_MAILBOX_0_INT, qm_mailbox_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_mailbox_0_isr);

/**
 * ISR for Memory Protection Region interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_SRAM_MPR_0_INT, qm_sram_mpr_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_sram_mpr_0_isr);

/**
 * ISR for PIC Timer interrupt.
 *
 * On Quark Microcontroller D2000 Development Platform,
 * this function needs to be registered with:
 * @code qm_int_vector_request(QM_X86_PIC_TIMER_INT_VECTOR,
 * qm_pic_timer_0_isr);
 * @endcode if IRQ based transfers are used.
 *
 * On Quark SE, this function needs to be registered with:
 * @code QM_IRQ_REQUEST(QM_IRQ_PIC_TIMER, qm_pic_timer_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_pic_timer_0_isr);

/**
 * ISR for PWM 0 Channel 0 interrupt.
 * If there is only one interrupt per controller this ISR handles
 * all channel interrupts.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_PWM_0_INT, qm_pwm_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_pwm_0_isr_0);

#if (NUM_PWM_CONTROLLER_INTERRUPTS > 1)
/**
 * ISR for PWM 0 channel 1 interrupt.
 *
 * This function needs to be registered with
 * @code qm_irq_request(QM_IRQ_PWM_1, qm_pwm_0_isr_1);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_pwm_0_isr_1);

/**
 * ISR for PWM 0 channel 2 interrupt.
 *
 * This function needs to be registered with
 * @code qm_irq_request(QM_IRQ_PWM_2, qm_pwm_0_isr_2);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_pwm_0_isr_2);

/**
 * ISR for PWM 0 channel 3 interrupt.
 *
 * This function needs to be registered with
 * @code qm_irq_request(QM_IRQ_PWM_3, qm_pwm_0_isr_3);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_pwm_0_isr_3);

#endif /* NUM_PWM_CONTROLLER_INTERRUPTS > 1 */

/**
 * ISR for RTC 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_RTC_0_INT, qm_rtc_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_rtc_0_isr);

/**
 * ISR for SPI Master 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_SPI_MASTER_0_INT, qm_spi_master_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_spi_master_0_isr);

#if (QUARK_SE)
/**
 * ISR for SPI Master 1 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_SPI_MASTER_1_INT, qm_spi_master_1_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_spi_master_1_isr);
#endif /* (QUARK_SE) */

/**
 * ISR for SPI Slave 0 interrupt.
 *
 * This function needs to be registered with
 * @code qm_irq_request(QM_IRQ_SPI_SLAVE_0_INT, qm_spi_slave_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_spi_slave_0_isr);

/**
 * ISR for UART 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_UART_0_INT, qm_uart_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_uart_0_isr);

/**
 * ISR for UART 1 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_UART_1_INT, qm_uart_1_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_uart_1_isr);

/**
 * ISR for WDT 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_WDT_0_INT, qm_wdt_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_wdt_0_isr);

#if (NUM_WDT_CONTROLLERS > 1)
/**
 * ISR for WDT 1 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_WDT_1_INT, qm_wdt_1_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_wdt_1_isr);
#endif /* NUM_WDT_CONTROLLERS > 1 */

/**
 * ISR for USB 0 interrupt.
 *
 * This function needs to be registered with
 * @code QM_IRQ_REQUEST(QM_IRQ_USB_0_INT, qm_usb_0_isr);
 * @endcode if IRQ based transfers are used.
 */
QM_ISR_DECLARE(qm_usb_0_isr);

/**
 * @}
 */

#endif /* __QM_ISR_H__ */
