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

#ifndef __QM_SS_POWER_STATES_H__
#define __QM_SS_POWER_STATES_H__

#include "qm_common.h"
#include "qm_sensor_regs.h"

/**
 * SS Power mode control for Quark SE Microcontrollers.
 *
 * @defgroup groupSSPower SS Power states
 * @{
 */

/**
 * Sensor Subsystem SS1 Timers mode type.
 */
typedef enum {
	SS_POWER_CPU_SS1_TIMER_OFF = 0, /**< Disable SS Timers in SS1. */
	SS_POWER_CPU_SS1_TIMER_ON       /**< Keep SS Timers enabled in SS1. */
} ss_power_cpu_ss1_mode_t;

/**
 * Enable LPSS state entry.
 *
 * Put the SoC into LPSS on next C2/C2LP and SS2 state combination.<BR>
 * This function needs to be called on the Sensor Core to
 * Clock Gate ADC, I2C0, I2C1, SPI0 and SPI1 sensor peripherals.<BR>
 * Clock Gating sensor peripherals is a requirement to enter LPSS state.<BR>
 * After LPSS, ss_power_soc_lpss_disable needs to be called to
 * restore clock gating.<BR>
 *
 * This needs to be called before any transition to C2/C2LP and SS2
 * in order to enter LPSS.<BR>
 * SoC Hybrid Clock is gated in this state.<BR>
 * Core Well Clocks are gated.<BR>
 * RTC is the only clock running.
 *
 * Possible SoC wake events are:
 * 	- Low Power Comparator Interrupt
 * 	- AON GPIO Interrupt
 * 	- AON Timer Interrupt
 * 	- RTC Interrupt
 */
void ss_power_soc_lpss_enable(void);

/**
 * Disable LPSS state entry.
 *
 * Clear LPSS enable flag.<BR>
 * Disable Clock Gating of ADC, I2C0, I2C1, SPI0 and SPI1 sensor
 * peripherals.<BR>
 * This will prevent entry in LPSS when cores are in C2/C2LP and SS2 states.
 */
void ss_power_soc_lpss_disable(void);

/**
 * Enter Sensor SS1 state.
 *
 * Put the Sensor Subsystem into SS1.<BR>
 * Processor Clock is gated in this state.
 *
 * A wake event causes the Sensor Subsystem to transition to SS0.<BR>
 * A wake event is a sensor subsystem interrupt.
 *
 * According to the mode selected, Sensor Subsystem Timers can be disabled.
 *
 * @param[in] mode Mode selection for SS1 state.
 */
void ss_power_cpu_ss1(const ss_power_cpu_ss1_mode_t mode);

/**
 * Enter Sensor SS2 state or SoC LPSS state.
 *
 * Put the Sensor Subsystem into SS2.<BR>
 * Sensor Complex Clock is gated in this state.<BR>
 * Sensor Peripherals are gated in this state.<BR>
 *
 * This enables entry in LPSS if:
 *  - Sensor Subsystem is in SS2
 *  - Lakemont is in C2 or C2LP
 *  - LPSS entry is enabled
 *
 * A wake event causes the Sensor Subsystem to transition to SS0.<BR>
 * There are two kinds of wake event depending on the Sensor Subsystem
 * and SoC state:
 *  - SS2: a wake event is a Sensor Subsystem interrupt
 *  - LPSS: a wake event is a Sensor Subsystem interrupt or a Lakemont interrupt
 *
 * LPSS wake events apply if LPSS is entered.
 * If Host wakes the SoC from LPSS,
 * Sensor also transitions back to SS0.
 */
void ss_power_cpu_ss2(void);

/**
 * @}
 */

#endif /* __QM_SS_POWER_STATES_H__ */
