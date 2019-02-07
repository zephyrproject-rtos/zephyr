/*

Copyright (c) 2010 - 2018, Nordic Semiconductor ASA All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef _NRF9160_PERIPHERALS_H
#define _NRF9160_PERIPHERALS_H

/* UICR */
#define UICR_KEYSLOT_COUNT 128

/* Clock Peripheral */
#define CLOCK_PRESENT
#define CLOCK_COUNT 1

/* Power Peripheral */
#define POWER_PRESENT
#define POWER_COUNT 1

/* GPIO */
#define GPIO_PRESENT
#define GPIO_COUNT 1

#define P0_PIN_NUM 32

/* Distributed  Peripheral to Peripheral Interconnect */
#define DPPI_PRESENT
#define DPPI_COUNT 1

#define DPPI_CH_NUM 16
#define DPPI_GROUP_NUM 6

/* Event Generator Unit */
#define EGU_PRESENT
#define EGU_COUNT 6

#define EGU0_CH_NUM 16
#define EGU1_CH_NUM 16
#define EGU2_CH_NUM 16
#define EGU3_CH_NUM 16
#define EGU4_CH_NUM 16
#define EGU5_CH_NUM 16

/* Timer/Counter */
#define TIMER_PRESENT
#define TIMER_COUNT 3

#define TIMER0_MAX_SIZE 32
#define TIMER1_MAX_SIZE 32
#define TIMER2_MAX_SIZE 32


#define TIMER0_CC_NUM 6
#define TIMER1_CC_NUM 6
#define TIMER2_CC_NUM 6

/* Real Time Counter */
#define RTC_PRESENT
#define RTC_COUNT 2

#define RTC0_CC_NUM 4
#define RTC1_CC_NUM 4

/* Watchdog Timer */
#define WDT_PRESENT
#define WDT_COUNT 1

/* Serial Peripheral Interface Master with DMA */
#define SPIM_PRESENT
#define SPIM_COUNT 4

#define SPIM0_MAX_DATARATE  8
#define SPIM1_MAX_DATARATE  8
#define SPIM2_MAX_DATARATE  8
#define SPIM3_MAX_DATARATE  8

#define SPIM0_EASYDMA_MAXCNT_SIZE 16
#define SPIM1_EASYDMA_MAXCNT_SIZE 16
#define SPIM2_EASYDMA_MAXCNT_SIZE 16
#define SPIM3_EASYDMA_MAXCNT_SIZE 16

/* Serial Peripheral Interface Slave with DMA*/
#define SPIS_PRESENT
#define SPIS_COUNT 4

#define SPIS0_EASYDMA_MAXCNT_SIZE 16
#define SPIS1_EASYDMA_MAXCNT_SIZE 16
#define SPIS2_EASYDMA_MAXCNT_SIZE 16
#define SPIS3_EASYDMA_MAXCNT_SIZE 16

/* Two Wire Interface Master with DMA */
#define TWIM_PRESENT
#define TWIM_COUNT 4

#define TWIM0_EASYDMA_MAXCNT_SIZE 16
#define TWIM1_EASYDMA_MAXCNT_SIZE 16
#define TWIM2_EASYDMA_MAXCNT_SIZE 16
#define TWIM3_EASYDMA_MAXCNT_SIZE 16

/* Two Wire Interface Slave with DMA */
#define TWIS_PRESENT
#define TWIS_COUNT 4

#define TWIS0_EASYDMA_MAXCNT_SIZE 16
#define TWIS1_EASYDMA_MAXCNT_SIZE 16
#define TWIS2_EASYDMA_MAXCNT_SIZE 16
#define TWIS3_EASYDMA_MAXCNT_SIZE 16

/* Universal Asynchronous Receiver-Transmitter with DMA */
#define UARTE_PRESENT
#define UARTE_COUNT 4

#define UARTE0_EASYDMA_MAXCNT_SIZE 16
#define UARTE1_EASYDMA_MAXCNT_SIZE 16
#define UARTE2_EASYDMA_MAXCNT_SIZE 16
#define UARTE3_EASYDMA_MAXCNT_SIZE 16


/* Successive Approximation Analog to Digital Converter */
#define SAADC_PRESENT
#define SAADC_COUNT 1

#define SAADC_EASYDMA_MAXCNT_SIZE 15

/* GPIO Tasks and Events */
#define GPIOTE_PRESENT
#define GPIOTE_COUNT 2

#define GPIOTE_CH_NUM 8

#define GPIOTE_FEATURE_SET_PRESENT
#define GPIOTE_FEATURE_CLR_PRESENT

/* Pulse Width Modulator */
#define PWM_PRESENT
#define PWM_COUNT 4

#define PWM_CH_NUM 4

#define PWM_EASYDMA_MAXCNT_SIZE 15

/* Pulse Density Modulator */
#define PDM_PRESENT
#define PDM_COUNT 1

#define PDM_EASYDMA_MAXCNT_SIZE 15

/* Inter-IC Sound Interface */
#define I2S_PRESENT
#define I2S_COUNT 1

#define I2S_EASYDMA_MAXCNT_SIZE 14

/* Inter Processor Communication */
#define IPC_PRESENT
#define IPC_COUNT 1

#define IPC_CH_NUM 8
#define IPC_GPMEM_NUM 4

/* FPU */
#define FPU_PRESENT
#define FPU_COUNT 1

/* SPU */
#define SPU_PRESENT
#define SPU_COUNT 1

/* CRYPTOCELL */
#define CRYPTOCELL_PRESENT
#define CRYPTOCELL_COUNT 1

/* KMU */
#define KMU_PRESENT
#define KMU_COUNT 1

#define KMU_KEYSLOT_PRESENT

/* MAGPIO */
#define MAGPIO_PRESENT
#define MAGPIO_COUNT 1
#define MAGPIO_PIN_NUM 3

/* REGULATORS */
#define REGULATORS_PRESENT
#define REGULATORS_COUNT 1


#endif  // _NRF9160_PERIPHERALS_H
