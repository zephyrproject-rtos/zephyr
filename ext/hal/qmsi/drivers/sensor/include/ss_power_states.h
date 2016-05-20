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
