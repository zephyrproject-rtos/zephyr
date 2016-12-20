/* Copyright (c) 2016, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   * Neither the name of Nordic Semiconductor ASA nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifndef _NRF51422_PERIPHERALS_H
#define _NRF51422_PERIPHERALS_H


/* Software Interrupts */
#define SWI_PRESENT
#define SWI_COUNT 6

/* GPIO */
#define GPIO_PRESENT
#define GPIO_COUNT 1

#define P0_PIN_NUM 32

/* MPU and BPROT */
#define BPROT_PRESENT

#define BPROT_REGIONS_SIZE 4096
#define BPROT_REGIONS_NUM 64

/* Radio */
#define RADIO_PRESENT
#define RADIO_COUNT 1

/* Accelerated Address Resolver */
#define AAR_PRESENT
#define AAR_COUNT 1

#define AAR_MAX_IRK_NUM 8

/* AES Electronic CodeBook mode encryption */
#define ECB_PRESENT
#define ECB_COUNT 1

/* AES CCM mode encryption */
#define CCM_PRESENT
#define CCM_COUNT 1

/* Peripheral to Peripheral Interconnect */
#define PPI_PRESENT
#define PPI_COUNT 1

#define PPI_CH_NUM 16
#define PPI_GROUP_NUM 4

/* Timer/Counter */
#define TIMER_PRESENT
#define TIMER_COUNT 3

#define TIMER0_MAX_SIZE 32
#define TIMER1_MAX_SIZE 16
#define TIMER2_MAX_SIZE 16

#define TIMER0_CC_NUM 4
#define TIMER1_CC_NUM 4
#define TIMER2_CC_NUM 4

/* Real Time Counter */
#define RTC_PRESENT
#define RTC_COUNT 2

#define RTC0_CC_NUM 3
#define RTC1_CC_NUM 4

/* RNG */
#define RNG_PRESENT
#define RNG_COUNT 1

/* Watchdog Timer */
#define WDT_PRESENT
#define WDT_COUNT 1

/* Temperature Sensor */
#define TEMP_PRESENT
#define TEMP_COUNT 1

/* Serial Peripheral Interface Master */
#define SPI_PRESENT
#define SPI_COUNT 2

/* Serial Peripheral Interface Slave with DMA */
#define SPIS_PRESENT
#define SPIS_COUNT 1

/* Two Wire Interface Master */
#define TWI_PRESENT
#define TWI_COUNT 2

/* Universal Asynchronous Receiver-Transmitter */
#define UART_PRESENT
#define UART_COUNT 1

/* Quadrature Decoder */
#define QDEC_PRESENT
#define QDEC_COUNT 1

/* Analog to Digital Converter */
#define ADC_PRESENT
#define ADC_COUNT 1

/* GPIO Tasks and Events */
#define GPIOTE_PRESENT
#define GPIOTE_COUNT 1

#define GPIOTE_CH_NUM 4

/* Low Power Comparator */
#define LPCOMP_PRESENT
#define LPCOMP_COUNT 1

#define LPCOMP_REFSEL_RESOLUTION 8


#endif      // _NRF51422_PERIPHERALS_H
