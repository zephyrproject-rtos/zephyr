/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_CONFIG_NRF51_H__
#define NRFX_CONFIG_NRF51_H__

// <<< Use Configuration Wizard in Context Menu >>>\n

// <h> nRF_Drivers

// <e> NRFX_ADC_ENABLED - nrfx_adc - ADC peripheral driver
//==========================================================
#ifdef CONFIG_NRFX_ADC
#define NRFX_ADC_ENABLED 1
#endif
// <o> NRFX_ADC_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_ADC_CONFIG_IRQ_PRIORITY
#define NRFX_ADC_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_ADC_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_ADC_CONFIG_LOG_ENABLED
#define NRFX_ADC_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_ADC_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_ADC_CONFIG_LOG_LEVEL
#define NRFX_ADC_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_ADC_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_ADC_CONFIG_INFO_COLOR
#define NRFX_ADC_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_ADC_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_ADC_CONFIG_DEBUG_COLOR
#define NRFX_ADC_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_CLOCK_ENABLED - nrfx_clock - CLOCK peripheral driver
//==========================================================
#ifndef NRFX_CLOCK_ENABLED
#define NRFX_CLOCK_ENABLED 0
#endif
// <o> NRFX_CLOCK_CONFIG_LF_SRC  - LF Clock Source

// <0=> RC
// <1=> XTAL
// <2=> Synth

#ifndef NRFX_CLOCK_CONFIG_LF_SRC
#define NRFX_CLOCK_CONFIG_LF_SRC 1
#endif

// <o> NRFX_CLOCK_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_CLOCK_CONFIG_IRQ_PRIORITY
#define NRFX_CLOCK_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_CLOCK_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_CLOCK_CONFIG_LOG_ENABLED
#define NRFX_CLOCK_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_CLOCK_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_CLOCK_CONFIG_LOG_LEVEL
#define NRFX_CLOCK_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_CLOCK_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_CLOCK_CONFIG_INFO_COLOR
#define NRFX_CLOCK_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_CLOCK_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_CLOCK_CONFIG_DEBUG_COLOR
#define NRFX_CLOCK_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_GPIOTE_ENABLED - nrfx_gpiote - GPIOTE peripheral driver
//==========================================================
#ifndef NRFX_GPIOTE_ENABLED
#define NRFX_GPIOTE_ENABLED 0
#endif
// <o> NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS - Number of lower power input pins
#ifndef NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS
#define NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 1
#endif

// <o> NRFX_GPIOTE_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_GPIOTE_CONFIG_IRQ_PRIORITY
#define NRFX_GPIOTE_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_GPIOTE_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_GPIOTE_CONFIG_LOG_ENABLED
#define NRFX_GPIOTE_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_GPIOTE_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_GPIOTE_CONFIG_LOG_LEVEL
#define NRFX_GPIOTE_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_GPIOTE_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_GPIOTE_CONFIG_INFO_COLOR
#define NRFX_GPIOTE_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_GPIOTE_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_GPIOTE_CONFIG_DEBUG_COLOR
#define NRFX_GPIOTE_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_LPCOMP_ENABLED - nrfx_lpcomp - LPCOMP peripheral driver
//==========================================================
#ifndef NRFX_LPCOMP_ENABLED
#define NRFX_LPCOMP_ENABLED 0
#endif
// <o> NRFX_LPCOMP_CONFIG_REFERENCE  - Reference voltage

// <0=> Supply 1/8
// <1=> Supply 2/8
// <2=> Supply 3/8
// <3=> Supply 4/8
// <4=> Supply 5/8
// <5=> Supply 6/8
// <6=> Supply 7/8
// <8=> Supply 1/16 (nRF52)
// <9=> Supply 3/16 (nRF52)
// <10=> Supply 5/16 (nRF52)
// <11=> Supply 7/16 (nRF52)
// <12=> Supply 9/16 (nRF52)
// <13=> Supply 11/16 (nRF52)
// <14=> Supply 13/16 (nRF52)
// <15=> Supply 15/16 (nRF52)
// <7=> External Ref 0
// <65543=> External Ref 1

#ifndef NRFX_LPCOMP_CONFIG_REFERENCE
#define NRFX_LPCOMP_CONFIG_REFERENCE 3
#endif

// <o> NRFX_LPCOMP_CONFIG_DETECTION  - Detection

// <0=> Crossing
// <1=> Up
// <2=> Down

#ifndef NRFX_LPCOMP_CONFIG_DETECTION
#define NRFX_LPCOMP_CONFIG_DETECTION 2
#endif

// <o> NRFX_LPCOMP_CONFIG_INPUT  - Analog input

// <0=> 0
// <1=> 1
// <2=> 2
// <3=> 3
// <4=> 4
// <5=> 5
// <6=> 6
// <7=> 7

#ifndef NRFX_LPCOMP_CONFIG_INPUT
#define NRFX_LPCOMP_CONFIG_INPUT 0
#endif

// <q> NRFX_LPCOMP_CONFIG_HYST  - Hysteresis


#ifndef NRFX_LPCOMP_CONFIG_HYST
#define NRFX_LPCOMP_CONFIG_HYST 0
#endif

// <o> NRFX_LPCOMP_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_LPCOMP_CONFIG_IRQ_PRIORITY
#define NRFX_LPCOMP_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_LPCOMP_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_LPCOMP_CONFIG_LOG_ENABLED
#define NRFX_LPCOMP_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_LPCOMP_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_LPCOMP_CONFIG_LOG_LEVEL
#define NRFX_LPCOMP_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_LPCOMP_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_LPCOMP_CONFIG_INFO_COLOR
#define NRFX_LPCOMP_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_LPCOMP_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_LPCOMP_CONFIG_DEBUG_COLOR
#define NRFX_LPCOMP_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_POWER_ENABLED - nrfx_power - POWER peripheral driver
//==========================================================
#ifndef NRFX_POWER_ENABLED
#define NRFX_POWER_ENABLED 0
#endif
// <o> NRFX_POWER_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_POWER_CONFIG_IRQ_PRIORITY
#define NRFX_POWER_CONFIG_IRQ_PRIORITY 3
#endif

// <q> NRFX_POWER_CONFIG_DEFAULT_DCDCEN  - The default configuration of main DCDC regulator


// <i> This settings means only that components for DCDC regulator are installed and it can be enabled.

#ifndef NRFX_POWER_CONFIG_DEFAULT_DCDCEN
#define NRFX_POWER_CONFIG_DEFAULT_DCDCEN 0
#endif

// <q> NRFX_POWER_CONFIG_DEFAULT_DCDCENHV  - The default configuration of High Voltage DCDC regulator


// <i> This settings means only that components for DCDC regulator are installed and it can be enabled.

#ifndef NRFX_POWER_CONFIG_DEFAULT_DCDCENHV
#define NRFX_POWER_CONFIG_DEFAULT_DCDCENHV 0
#endif

// </e>

// <e> NRFX_PPI_ENABLED - nrfx_ppi - PPI peripheral allocator
//==========================================================
#ifndef NRFX_PPI_ENABLED
#define NRFX_PPI_ENABLED 0
#endif
// <e> NRFX_PPI_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_PPI_CONFIG_LOG_ENABLED
#define NRFX_PPI_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_PPI_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_PPI_CONFIG_LOG_LEVEL
#define NRFX_PPI_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_PPI_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_PPI_CONFIG_INFO_COLOR
#define NRFX_PPI_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_PPI_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_PPI_CONFIG_DEBUG_COLOR
#define NRFX_PPI_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_PRS_ENABLED - nrfx_prs - Peripheral Resource Sharing module
//==========================================================
#ifndef NRFX_PRS_ENABLED
#define NRFX_PRS_ENABLED 0
#endif
// <q> NRFX_PRS_BOX_0_ENABLED  - Enables box 0 in the module.


#ifndef NRFX_PRS_BOX_0_ENABLED
#define NRFX_PRS_BOX_0_ENABLED 0
#endif

// <q> NRFX_PRS_BOX_1_ENABLED  - Enables box 1 in the module.


#ifndef NRFX_PRS_BOX_1_ENABLED
#define NRFX_PRS_BOX_1_ENABLED 0
#endif

// <e> NRFX_PRS_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_PRS_CONFIG_LOG_ENABLED
#define NRFX_PRS_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_PRS_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_PRS_CONFIG_LOG_LEVEL
#define NRFX_PRS_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_PRS_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_PRS_CONFIG_INFO_COLOR
#define NRFX_PRS_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_PRS_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_PRS_CONFIG_DEBUG_COLOR
#define NRFX_PRS_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_QDEC_ENABLED - nrfx_qdec - QDEC peripheral driver
//==========================================================
#ifndef NRFX_QDEC_ENABLED
#define NRFX_QDEC_ENABLED 0
#endif
// <o> NRFX_QDEC_CONFIG_REPORTPER  - Report period

// <0=> 10 Samples
// <1=> 40 Samples
// <2=> 80 Samples
// <3=> 120 Samples
// <4=> 160 Samples
// <5=> 200 Samples
// <6=> 240 Samples
// <7=> 280 Samples

#ifndef NRFX_QDEC_CONFIG_REPORTPER
#define NRFX_QDEC_CONFIG_REPORTPER 0
#endif

// <o> NRFX_QDEC_CONFIG_SAMPLEPER  - Sample period

// <0=> 128 us
// <1=> 256 us
// <2=> 512 us
// <3=> 1024 us
// <4=> 2048 us
// <5=> 4096 us
// <6=> 8192 us
// <7=> 16384 us

#ifndef NRFX_QDEC_CONFIG_SAMPLEPER
#define NRFX_QDEC_CONFIG_SAMPLEPER 7
#endif

// <o> NRFX_QDEC_CONFIG_PIO_A - A pin  <0-31>


#ifndef NRFX_QDEC_CONFIG_PIO_A
#define NRFX_QDEC_CONFIG_PIO_A 31
#endif

// <o> NRFX_QDEC_CONFIG_PIO_B - B pin  <0-31>


#ifndef NRFX_QDEC_CONFIG_PIO_B
#define NRFX_QDEC_CONFIG_PIO_B 31
#endif

// <o> NRFX_QDEC_CONFIG_PIO_LED - LED pin  <0-31>


#ifndef NRFX_QDEC_CONFIG_PIO_LED
#define NRFX_QDEC_CONFIG_PIO_LED 31
#endif

// <o> NRFX_QDEC_CONFIG_LEDPRE - LED pre
#ifndef NRFX_QDEC_CONFIG_LEDPRE
#define NRFX_QDEC_CONFIG_LEDPRE 511
#endif

// <o> NRFX_QDEC_CONFIG_LEDPOL  - LED polarity

// <0=> Active low
// <1=> Active high

#ifndef NRFX_QDEC_CONFIG_LEDPOL
#define NRFX_QDEC_CONFIG_LEDPOL 1
#endif

// <q> NRFX_QDEC_CONFIG_DBFEN  - Debouncing enable


#ifndef NRFX_QDEC_CONFIG_DBFEN
#define NRFX_QDEC_CONFIG_DBFEN 0
#endif

// <q> NRFX_QDEC_CONFIG_SAMPLE_INTEN  - Sample ready interrupt enable


#ifndef NRFX_QDEC_CONFIG_SAMPLE_INTEN
#define NRFX_QDEC_CONFIG_SAMPLE_INTEN 0
#endif

// <o> NRFX_QDEC_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_QDEC_CONFIG_IRQ_PRIORITY
#define NRFX_QDEC_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_QDEC_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_QDEC_CONFIG_LOG_ENABLED
#define NRFX_QDEC_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_QDEC_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_QDEC_CONFIG_LOG_LEVEL
#define NRFX_QDEC_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_QDEC_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_QDEC_CONFIG_INFO_COLOR
#define NRFX_QDEC_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_QDEC_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_QDEC_CONFIG_DEBUG_COLOR
#define NRFX_QDEC_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_RNG_ENABLED - nrfx_rng - RNG peripheral driver
//==========================================================
#ifndef NRFX_RNG_ENABLED
#define NRFX_RNG_ENABLED 0
#endif
// <q> NRFX_RNG_CONFIG_ERROR_CORRECTION  - Error correction


#ifndef NRFX_RNG_CONFIG_ERROR_CORRECTION
#define NRFX_RNG_CONFIG_ERROR_CORRECTION 0
#endif

// <o> NRFX_RNG_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_RNG_CONFIG_IRQ_PRIORITY
#define NRFX_RNG_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_RNG_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_RNG_CONFIG_LOG_ENABLED
#define NRFX_RNG_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_RNG_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_RNG_CONFIG_LOG_LEVEL
#define NRFX_RNG_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_RNG_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_RNG_CONFIG_INFO_COLOR
#define NRFX_RNG_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_RNG_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_RNG_CONFIG_DEBUG_COLOR
#define NRFX_RNG_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_RTC_ENABLED - nrfx_rtc - RTC peripheral driver
//==========================================================
#ifndef NRFX_RTC_ENABLED
#define NRFX_RTC_ENABLED 0
#endif
// <q> NRFX_RTC0_ENABLED  - Enable RTC0 instance


#ifndef NRFX_RTC0_ENABLED
#define NRFX_RTC0_ENABLED 0
#endif

// <q> NRFX_RTC1_ENABLED  - Enable RTC1 instance


#ifndef NRFX_RTC1_ENABLED
#define NRFX_RTC1_ENABLED 0
#endif

// <o> NRFX_RTC_MAXIMUM_LATENCY_US - Maximum possible time[us] in highest priority interrupt
#ifndef NRFX_RTC_MAXIMUM_LATENCY_US
#define NRFX_RTC_MAXIMUM_LATENCY_US 2000
#endif

// <o> NRFX_RTC_DEFAULT_CONFIG_FREQUENCY - Frequency  <16-32768>


#ifndef NRFX_RTC_DEFAULT_CONFIG_FREQUENCY
#define NRFX_RTC_DEFAULT_CONFIG_FREQUENCY 32768
#endif

// <q> NRFX_RTC_DEFAULT_CONFIG_RELIABLE  - Ensures safe compare event triggering


#ifndef NRFX_RTC_DEFAULT_CONFIG_RELIABLE
#define NRFX_RTC_DEFAULT_CONFIG_RELIABLE 0
#endif

// <o> NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_RTC_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_RTC_CONFIG_LOG_ENABLED
#define NRFX_RTC_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_RTC_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_RTC_CONFIG_LOG_LEVEL
#define NRFX_RTC_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_RTC_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_RTC_CONFIG_INFO_COLOR
#define NRFX_RTC_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_RTC_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_RTC_CONFIG_DEBUG_COLOR
#define NRFX_RTC_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_SPIS_ENABLED - nrfx_spis - SPIS peripheral driver
//==========================================================
#ifdef CONFIG_NRFX_SPIS
#define NRFX_SPIS_ENABLED 1
#endif
// <q> NRFX_SPIS1_ENABLED  - Enable SPIS1 instance


#ifdef CONFIG_SPI_1_NRF_SPIS
#define NRFX_SPIS1_ENABLED 1
#endif

// <o> NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY 3
#endif

// <o> NRFX_SPIS_DEFAULT_DEF - SPIS default DEF character  <0-255>


#ifndef NRFX_SPIS_DEFAULT_DEF
#define NRFX_SPIS_DEFAULT_DEF 255
#endif

// <o> NRFX_SPIS_DEFAULT_ORC - SPIS default ORC character  <0-255>


#ifndef NRFX_SPIS_DEFAULT_ORC
#define NRFX_SPIS_DEFAULT_ORC 255
#endif

// <e> NRFX_SPIS_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_SPIS_CONFIG_LOG_ENABLED
#define NRFX_SPIS_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_SPIS_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_SPIS_CONFIG_LOG_LEVEL
#define NRFX_SPIS_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_SPIS_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_SPIS_CONFIG_INFO_COLOR
#define NRFX_SPIS_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_SPIS_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_SPIS_CONFIG_DEBUG_COLOR
#define NRFX_SPIS_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_SPI_ENABLED - nrfx_spi - SPI peripheral driver
//==========================================================
#ifdef CONFIG_NRFX_SPI
#define NRFX_SPI_ENABLED 1
#endif
// <q> NRFX_SPI0_ENABLED  - Enable SPI0 instance


#ifdef CONFIG_SPI_0_NRF_SPI
#define NRFX_SPI0_ENABLED 1
#endif

// <q> NRFX_SPI1_ENABLED  - Enable SPI1 instance


#ifdef CONFIG_SPI_1_NRF_SPI
#define NRFX_SPI1_ENABLED 1
#endif

// <o> NRFX_SPI_MISO_PULL_CFG  - MISO pin pull configuration.

// <0=> NRF_GPIO_PIN_NOPULL
// <1=> NRF_GPIO_PIN_PULLDOWN
// <3=> NRF_GPIO_PIN_PULLUP

#ifndef NRFX_SPI_MISO_PULL_CFG
#define NRFX_SPI_MISO_PULL_CFG 1
#endif

// <o> NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_SPI_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_SPI_CONFIG_LOG_ENABLED
#define NRFX_SPI_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_SPI_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_SPI_CONFIG_LOG_LEVEL
#define NRFX_SPI_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_SPI_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_SPI_CONFIG_INFO_COLOR
#define NRFX_SPI_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_SPI_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_SPI_CONFIG_DEBUG_COLOR
#define NRFX_SPI_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_SWI_ENABLED - nrfx_swi - SWI/EGU peripheral allocator
//==========================================================
#ifndef NRFX_SWI_ENABLED
#define NRFX_SWI_ENABLED 0
#endif
// <q> NRFX_SWI0_DISABLED  - Exclude SWI0 from being utilized by the driver


#ifndef NRFX_SWI0_DISABLED
#define NRFX_SWI0_DISABLED 0
#endif

// <q> NRFX_SWI1_DISABLED  - Exclude SWI1 from being utilized by the driver


#ifndef NRFX_SWI1_DISABLED
#define NRFX_SWI1_DISABLED 0
#endif

// <q> NRFX_SWI2_DISABLED  - Exclude SWI2 from being utilized by the driver


#ifndef NRFX_SWI2_DISABLED
#define NRFX_SWI2_DISABLED 0
#endif

// <q> NRFX_SWI3_DISABLED  - Exclude SWI3 from being utilized by the driver


#ifndef NRFX_SWI3_DISABLED
#define NRFX_SWI3_DISABLED 0
#endif

// <q> NRFX_SWI4_DISABLED  - Exclude SWI4 from being utilized by the driver


#ifndef NRFX_SWI4_DISABLED
#define NRFX_SWI4_DISABLED 0
#endif

// <q> NRFX_SWI5_DISABLED  - Exclude SWI5 from being utilized by the driver


#ifndef NRFX_SWI5_DISABLED
#define NRFX_SWI5_DISABLED 0
#endif

// <e> NRFX_SWI_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_SWI_CONFIG_LOG_ENABLED
#define NRFX_SWI_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_SWI_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_SWI_CONFIG_LOG_LEVEL
#define NRFX_SWI_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_SWI_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_SWI_CONFIG_INFO_COLOR
#define NRFX_SWI_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_SWI_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_SWI_CONFIG_DEBUG_COLOR
#define NRFX_SWI_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_TIMER_ENABLED - nrfx_timer - TIMER periperal driver
//==========================================================
#ifndef NRFX_TIMER_ENABLED
#define NRFX_TIMER_ENABLED 0
#endif
// <q> NRFX_TIMER0_ENABLED  - Enable TIMER0 instance


#ifndef NRFX_TIMER0_ENABLED
#define NRFX_TIMER0_ENABLED 0
#endif

// <q> NRFX_TIMER1_ENABLED  - Enable TIMER1 instance


#ifndef NRFX_TIMER1_ENABLED
#define NRFX_TIMER1_ENABLED 0
#endif

// <q> NRFX_TIMER2_ENABLED  - Enable TIMER2 instance


#ifndef NRFX_TIMER2_ENABLED
#define NRFX_TIMER2_ENABLED 0
#endif

// <o> NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY  - Timer frequency if in Timer mode

// <0=> 16 MHz
// <1=> 8 MHz
// <2=> 4 MHz
// <3=> 2 MHz
// <4=> 1 MHz
// <5=> 500 kHz
// <6=> 250 kHz
// <7=> 125 kHz
// <8=> 62.5 kHz
// <9=> 31.25 kHz

#ifndef NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY
#define NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY 0
#endif

// <o> NRFX_TIMER_DEFAULT_CONFIG_MODE  - Timer mode or operation

// <0=> Timer
// <1=> Counter

#ifndef NRFX_TIMER_DEFAULT_CONFIG_MODE
#define NRFX_TIMER_DEFAULT_CONFIG_MODE 0
#endif

// <o> NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH  - Timer counter bit width

// <0=> 16 bit
// <1=> 8 bit
// <2=> 24 bit
// <3=> 32 bit

#ifndef NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH
#define NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH 0
#endif

// <o> NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_TIMER_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_TIMER_CONFIG_LOG_ENABLED
#define NRFX_TIMER_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_TIMER_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_TIMER_CONFIG_LOG_LEVEL
#define NRFX_TIMER_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_TIMER_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_TIMER_CONFIG_INFO_COLOR
#define NRFX_TIMER_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_TIMER_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_TIMER_CONFIG_DEBUG_COLOR
#define NRFX_TIMER_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_TWI_ENABLED - nrfx_twi - TWI peripheral driver
//==========================================================
#ifdef CONFIG_NRFX_TWI
#define NRFX_TWI_ENABLED 1
#endif
// <q> NRFX_TWI0_ENABLED  - Enable TWI0 instance


#ifdef CONFIG_I2C_0_NRF_TWI
#define NRFX_TWI0_ENABLED 1
#endif

// <q> NRFX_TWI1_ENABLED  - Enable TWI1 instance


#ifdef CONFIG_I2C_1_NRF_TWI
#define NRFX_TWI1_ENABLED 1
#endif

// <o> NRFX_TWI_DEFAULT_CONFIG_FREQUENCY  - Frequency

// <26738688=> 100k
// <67108864=> 250k
// <104857600=> 400k

#ifndef NRFX_TWI_DEFAULT_CONFIG_FREQUENCY
#define NRFX_TWI_DEFAULT_CONFIG_FREQUENCY 26738688
#endif

// <q> NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT  - Enables bus holding after uninit


#ifndef NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT
#define NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT 0
#endif

// <o> NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_TWI_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_TWI_CONFIG_LOG_ENABLED
#define NRFX_TWI_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_TWI_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_TWI_CONFIG_LOG_LEVEL
#define NRFX_TWI_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_TWI_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_TWI_CONFIG_INFO_COLOR
#define NRFX_TWI_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_TWI_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_TWI_CONFIG_DEBUG_COLOR
#define NRFX_TWI_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_UART_ENABLED - nrfx_uart - UART peripheral driver
//==========================================================
#ifndef NRFX_UART_ENABLED
#define NRFX_UART_ENABLED 0
#endif
// <o> NRFX_UART0_ENABLED - Enable UART0 instance
#ifndef NRFX_UART0_ENABLED
#define NRFX_UART0_ENABLED 0
#endif

// <o> NRFX_UART_DEFAULT_CONFIG_HWFC  - Hardware Flow Control

// <0=> Disabled
// <1=> Enabled

#ifndef NRFX_UART_DEFAULT_CONFIG_HWFC
#define NRFX_UART_DEFAULT_CONFIG_HWFC 0
#endif

// <o> NRFX_UART_DEFAULT_CONFIG_PARITY  - Parity

// <0=> Excluded
// <14=> Included

#ifndef NRFX_UART_DEFAULT_CONFIG_PARITY
#define NRFX_UART_DEFAULT_CONFIG_PARITY 0
#endif

// <o> NRFX_UART_DEFAULT_CONFIG_BAUDRATE  - Default Baudrate

// <323584=> 1200 baud
// <643072=> 2400 baud
// <1290240=> 4800 baud
// <2576384=> 9600 baud
// <3866624=> 14400 baud
// <5152768=> 19200 baud
// <7729152=> 28800 baud
// <8388608=> 31250 baud
// <10309632=> 38400 baud
// <15007744=> 56000 baud
// <15462400=> 57600 baud
// <20615168=> 76800 baud
// <30924800=> 115200 baud
// <61845504=> 230400 baud
// <67108864=> 250000 baud
// <123695104=> 460800 baud
// <247386112=> 921600 baud
// <268435456=> 1000000 baud

#ifndef NRFX_UART_DEFAULT_CONFIG_BAUDRATE
#define NRFX_UART_DEFAULT_CONFIG_BAUDRATE 30924800
#endif

// <o> NRFX_UART_DEFAULT_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_UART_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_UART_DEFAULT_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_UART_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_UART_CONFIG_LOG_ENABLED
#define NRFX_UART_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_UART_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_UART_CONFIG_LOG_LEVEL
#define NRFX_UART_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_UART_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_UART_CONFIG_INFO_COLOR
#define NRFX_UART_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_UART_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_UART_CONFIG_DEBUG_COLOR
#define NRFX_UART_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// <e> NRFX_WDT_ENABLED - nrfx_wdt - WDT peripheral driver
//==========================================================
#ifdef CONFIG_NRFX_WDT
#define NRFX_WDT_ENABLED 1
#endif
// <o> NRFX_WDT_CONFIG_BEHAVIOUR  - WDT behavior in CPU SLEEP or HALT mode

// <1=> Run in SLEEP, Pause in HALT
// <8=> Pause in SLEEP, Run in HALT
// <9=> Run in SLEEP and HALT
// <0=> Pause in SLEEP and HALT

#ifndef NRFX_WDT_CONFIG_BEHAVIOUR
#define NRFX_WDT_CONFIG_BEHAVIOUR 1
#endif

// <o> NRFX_WDT_CONFIG_RELOAD_VALUE - Reload value  <15-4294967295>


#ifndef NRFX_WDT_CONFIG_RELOAD_VALUE
#define NRFX_WDT_CONFIG_RELOAD_VALUE 2000
#endif

// <o> NRFX_WDT_CONFIG_IRQ_PRIORITY  - Interrupt priority

// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3

#ifndef NRFX_WDT_CONFIG_IRQ_PRIORITY
#define NRFX_WDT_CONFIG_IRQ_PRIORITY 3
#endif

// <e> NRFX_WDT_CONFIG_LOG_ENABLED - Enables logging in the module.
//==========================================================
#ifndef NRFX_WDT_CONFIG_LOG_ENABLED
#define NRFX_WDT_CONFIG_LOG_ENABLED 0
#endif
// <o> NRFX_WDT_CONFIG_LOG_LEVEL  - Default Severity level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef NRFX_WDT_CONFIG_LOG_LEVEL
#define NRFX_WDT_CONFIG_LOG_LEVEL 3
#endif

// <o> NRFX_WDT_CONFIG_INFO_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_WDT_CONFIG_INFO_COLOR
#define NRFX_WDT_CONFIG_INFO_COLOR 0
#endif

// <o> NRFX_WDT_CONFIG_DEBUG_COLOR  - ANSI escape code prefix.

// <0=> Default
// <1=> Black
// <2=> Red
// <3=> Green
// <4=> Yellow
// <5=> Blue
// <6=> Magenta
// <7=> Cyan
// <8=> White

#ifndef NRFX_WDT_CONFIG_DEBUG_COLOR
#define NRFX_WDT_CONFIG_DEBUG_COLOR 0
#endif

// </e>

// </e>

// </h>

#endif // NRFX_CONFIG_NRF51_H__
