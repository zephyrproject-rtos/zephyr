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

#ifndef __SOC_WATCH_H__
#define __SOC_WATCH_H__

/* This file relies on the SOC being defined, which comes from qm_soc_regs.h */
#include "qm_soc_regs.h"

/**
 * SoC Watch (Energy Analyzer).
 *
 * @defgroup group SOC_WATCH
 * @{
 */

#include "qm_common.h"

/*
 * To activate the functionality in this file, compile with
 * SOC_WATCH_ENABLE=1 on the make command line.
 *
 * Accurate timestamping through sleep modes also requires:
 *  + board design: provide an RTC crystal
 *  + application : don't reset or disable the RTC.
 */

/**
 * Power profiling events enumeration.
 *
 * In order to maintain binary compatibility, only SOCW_EVENT_MAX should
 * ever be altered: new events should be inserted before SOCW_EVENT_MAX,
 * and SOCW_EVENT_MAX incremented.  Add events, do not replace them.
 */
typedef enum {
	SOCW_EVENT_HALT = 0,      /**< CPU Halt. */
	SOCW_EVENT_INTERRUPT = 1, /**< CPU interrupt generated. */
	SOCW_EVENT_SLEEP = 2,     /**< Sleep mode entered. */
	SOCW_EVENT_REGISTER = 3,  /**< SOC register altered. */
	SOCW_EVENT_APP = 4,       /**< Application-defined event. */
	SOCW_EVENT_FREQ = 5,      /**< Frequency altered. */
	SOCW_EVENT_MAX = 6	/**< End of events sentinel. */
} soc_watch_event_t;

/*
 * Power profiling events for ARC Sensor states.
 *
 * Internally socwatch process the SS1 and SS2 as Halt
 * Sleep events encoding respectively.
 */
#define SOCW_ARC_EVENT_SS1 SOCW_EVENT_HALT
#define SOCW_ARC_EVENT_SS2 SOCW_EVENT_SLEEP

/**
 * Register ID enumeration.
 *
 * The Register Event stores a register ID enumeration instead of a
 * register address in order to save space.  Registers can be added,
 * but they should not be deleted, in order to preserve compatibility
 * with different versions of the post-processor.
 *
 * Note that most of these names mirror the names used elsewhere in
 * the QMSI code, although these are upper case, while the register
 * pointer names are in lower case.  That's one clue for identifying
 * where logging calls should to be added: wherever you see one of the
 * named registers below being written, you should consider that write
 * may need a corresponding SoC Watch logging call.
 */
#if (QUARK_D2000)
typedef enum {
	/* Clock rate registers */
	SOCW_REG_OSC0_CFG1 = 0,       /**< 0x000 OSC0_CFG1 register. */
	SOCW_REG_CCU_LP_CLK_CTL = 1,  /**< 0x02C Clock Control register.*/
	SOCW_REG_CCU_SYS_CLK_CTL = 2, /**< 0x038 System Clock Control. */
	/* Clock gating registers. */
	SOCW_REG_CCU_PERIPH_CLK_GATE_CTL = 3, /**< 0x018 Perip Clock Gate Ctl.*/
	SOCW_REG_CCU_EXT_CLK_CTL = 4, /**< 0x024 CCU Ext Clock Gate Ctl.*/
	/* Registers affecting power consumption */
	SOCW_REG_CMP_PWR = 5,     /**< 0x30C Comprtr Power Enable. */
	SOCW_REG_PMUX_PULLUP = 6, /**< 0x900 Pin Mux Pullup. */
	SOCW_REG_PMUX_SLEW = 7,   /**< 0x910 Pin Mux Slew. */
	SOCW_REG_PMUX_IN_EN = 8,  /**< 0x920 Pin Mux In Enable. */
	SOCW_REG_MAX,		  /**< Register enum sentinel. */
} soc_watch_reg_t;

#elif(QUARK_SE)
typedef enum {
	/* Clock rate registers */
	SOCW_REG_OSC0_CFG1 = 0,       /**< 0x000 OSC0_CFG1 register. */
	SOCW_REG_CCU_LP_CLK_CTL = 1,  /**< 0x02C Clock Control register. */
	SOCW_REG_CCU_SYS_CLK_CTL = 2, /**< 0x038 System Clock Control. */
	/* Clock gating registers. */
	SOCW_REG_CCU_PERIPH_CLK_GATE_CTL = 3, /**< 0x018 Perip Clock Gate Ctl.*/
	SOCW_REG_CCU_SS_PERIPH_CLK_GATE_CTL = 4, /**< 0x0028 SS PCL Gate Ctl.*/
	SOCW_REG_CCU_EXT_CLK_CTL = 5, /**< 0x024 CCU Ext Clock Gate Ctl.*/
	/* Registers affecting power consumption */
	SOCW_REG_CMP_PWR = 6,       /**< 0x30C Comparator Power Enable. */
	SOCW_REG_SLP_CFG = 7,       /**< 0x550 Sleep Configuration.  */
	SOCW_REG_PMUX_PULLUP0 = 8,  /**< 0x900 Pin Mux Pullup. */
	SOCW_REG_PMUX_PULLUP1 = 9,  /**< 0x904 Pin Mux Pullup. */
	SOCW_REG_PMUX_PULLUP2 = 10, /**< 0x908 Pin Mux Pullup. */
	SOCW_REG_PMUX_PULLUP3 = 11, /**< 0x90c Pin Mux Pullup. */
	SOCW_REG_PMUX_SLEW0 = 12,   /**< 0x910 Pin Mux Slew. */
	SOCW_REG_PMUX_SLEW1 = 13,   /**< 0x914 Pin Mux Slew. */
	SOCW_REG_PMUX_SLEW2 = 14,   /**< 0x918 Pin Mux Slew. */
	SOCW_REG_PMUX_SLEW3 = 15,   /**< 0x91c Pin Mux Slew. */
	SOCW_REG_PMUX_IN_EN0 = 16,  /**< 0x920 Pin Mux In Enable. */
	SOCW_REG_PMUX_IN_EN1 = 17,  /**< 0x924 Pin Mux In Enable. */
	SOCW_REG_PMUX_IN_EN2 = 18,  /**< 0x928 Pin Mux In Enable. */
	SOCW_REG_PMUX_IN_EN3 = 19,  /**< 0x92c Pin Mux In Enable. */
	SOCW_REG_MAX,		    /**< Register enum sentinel. */
} soc_watch_reg_t;
#endif /* QUARK_SE */

/**
 * Log a power profile event.
 *
 * Log an event related to power management.  This should be things like
 * halts, or register reads which cause us to go to low power states, or
 * register reads that affect the clock rate, or other clock gating.
 *
 * @param[in] event_id The Event ID of the profile event.
 * @param[in] ev_data  A parameter to the event ID (if the event needs one).
 */
void soc_watch_log_event(soc_watch_event_t event_id, uintptr_t ev_data);

/**
 * Log an application event via the power profile logger.
 *
 * This allows applications layered on top of QMSI to log their own
 * events.  The subtype identifies the type of data for the user, and
 * 'data' is the actual information being logged.
 *
 * @param[in] event_id    The Event ID of the profile event.
 * @param[in] ev_subtype  A 1-byte user-defined event_id.
 * @param[in] ev_data     A parameter to the event ID (if the event needs one).
 *
 * @returns Nothing.
 */
void soc_watch_log_app_event(soc_watch_event_t event_id, uint8_t ev_subtype,
			     uintptr_t ev_data);

/**
 * Trigger a buffer flush via watchpoint.
 * This allows applications layered on top of QMSI to trigger the transfer of
 * profiler information to the host whenever it requires.
 */
void soc_watch_trigger_flush();

/**
 * @}
 */

#endif /* __SOC_WATCH_H__ */
