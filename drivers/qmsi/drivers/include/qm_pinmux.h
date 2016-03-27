/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef __QM_PINMUX_H__
#define __QM_PINMUX_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Pin muxing configuration for Quark Microcontrollers.
 *
 * @defgroup groupPinMux Pin Muxing setup
 * @{
 */

/**
 * Pin function type.
 */
typedef enum {
	QM_PMUX_FN_0,
	QM_PMUX_FN_1,
	QM_PMUX_FN_2,
	QM_PMUX_FN_3,
} qm_pmux_fn_t;

/**
 * Pin slew rate setting.
 */
typedef enum {
#if (QUARK_SE)
	QM_PMUX_SLEW_2MA,
	QM_PMUX_SLEW_4MA,
#else
	QM_PMUX_SLEW_12MA,
	QM_PMUX_SLEW_16MA,
#endif
	QM_PMUX_SLEW_NUM
} qm_pmux_slew_t;

/**
 * External Pad pin identifiers
 **/
typedef enum {
	QM_PIN_ID_0,
	QM_PIN_ID_1,
	QM_PIN_ID_2,
	QM_PIN_ID_3,
	QM_PIN_ID_4,
	QM_PIN_ID_5,
	QM_PIN_ID_6,
	QM_PIN_ID_7,
	QM_PIN_ID_8,
	QM_PIN_ID_9,
	QM_PIN_ID_10,
	QM_PIN_ID_11,
	QM_PIN_ID_12,
	QM_PIN_ID_13,
	QM_PIN_ID_14,
	QM_PIN_ID_15,
	QM_PIN_ID_16,
	QM_PIN_ID_17,
	QM_PIN_ID_18,
	QM_PIN_ID_19,
	QM_PIN_ID_20,
	QM_PIN_ID_21,
	QM_PIN_ID_22,
	QM_PIN_ID_23,
	QM_PIN_ID_24,
#if (QUARK_SE)
	QM_PIN_ID_25,
	QM_PIN_ID_26,
	QM_PIN_ID_27,
	QM_PIN_ID_28,
	QM_PIN_ID_29,
	QM_PIN_ID_30,
	QM_PIN_ID_31,
	QM_PIN_ID_32,
	QM_PIN_ID_33,
	QM_PIN_ID_34,
	QM_PIN_ID_35,
	QM_PIN_ID_36,
	QM_PIN_ID_37,
	QM_PIN_ID_38,
	QM_PIN_ID_39,
	QM_PIN_ID_40,
	QM_PIN_ID_41,
	QM_PIN_ID_42,
	QM_PIN_ID_43,
	QM_PIN_ID_44,
	QM_PIN_ID_45,
	QM_PIN_ID_46,
	QM_PIN_ID_47,
	QM_PIN_ID_48,
	QM_PIN_ID_49,
	QM_PIN_ID_50,
	QM_PIN_ID_51,
	QM_PIN_ID_52,
	QM_PIN_ID_53,
	QM_PIN_ID_54,
	QM_PIN_ID_55,
	QM_PIN_ID_56,
	QM_PIN_ID_57,
	QM_PIN_ID_58,
	QM_PIN_ID_59,
	QM_PIN_ID_60,
	QM_PIN_ID_61,
	QM_PIN_ID_62,
	QM_PIN_ID_63,
	QM_PIN_ID_64,
	QM_PIN_ID_65,
	QM_PIN_ID_66,
	QM_PIN_ID_67,
	QM_PIN_ID_68,
#endif
	QM_PIN_ID_NUM
} qm_pin_id_t;

/**
 * Set up pin muxing for a SoC pin.  Select one of the pin functions.
 *
 * @param [in] pin which pin to configure.
 * @param [in] fn the function to assign to the pin.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_pmux_select(qm_pin_id_t pin, qm_pmux_fn_t fn);

/**
 * Set up pin's slew rate in the pin mux controller.
 *
 * @param [in] pin which pin to configure.
 * @param [in] slew the slew rate to assign to the pin.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_pmux_set_slew(qm_pin_id_t pin, qm_pmux_slew_t slew);

/**
 * Enable input for a pin in the pin mux controller.
 *
 * @param [in] pin which pin to configure.
 * @param [in] enable set to true to enable input.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_pmux_input_en(qm_pin_id_t pin, bool enable);

/**
 * Enable pullup for a pin in the pin mux controller.
 *
 * @param [in] pin which pin to configure.
 * @param [in] enable set to true to enable pullup.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_pmux_pullup_en(qm_pin_id_t pin, bool enable);

/**
 * @}
 */

#endif /* __QM_PINMUX_H__ */
