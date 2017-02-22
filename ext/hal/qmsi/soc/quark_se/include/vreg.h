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

#ifndef __VREG_H__
#define __VREG_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

typedef enum {
	VREG_MODE_SWITCHING = 0,
	VREG_MODE_LINEAR,
	VREG_MODE_SHUTDOWN,
	VREG_MODE_NUM,
} vreg_mode_t;

/**
 * Voltage Regulators Control.
 *
 * @defgroup groupVREG Quark SE Voltage Regulators
 * @{
 */

/**
 * Set AON Voltage Regulator mode.
 *
 * The AON Voltage Regulator is not a
 * switching regulator and only acts as
 * a linear regulator.
 * VREG_SWITCHING_MODE is not a value mode
 * for the AON Voltage Regulator.
 *
 * @param[in] mode Voltage Regulator mode.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int vreg_aon_set_mode(const vreg_mode_t mode);

/**
 * Set Platform 3P3 Voltage Regulator mode.
 *
 * @param[in] mode Voltage Regulator mode.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int vreg_plat3p3_set_mode(const vreg_mode_t mode);

/**
 * Set Platform 1P8 Voltage Regulator mode.
 *
 * @param[in] mode Voltage Regulator mode.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int vreg_plat1p8_set_mode(const vreg_mode_t mode);

/**
 * Set Host Voltage Regulator mode.
 *
 * @param[in] mode Voltage Regulator mode.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int vreg_host_set_mode(const vreg_mode_t mode);

/**
 * @}
 */

#endif /* __VREG_H__ */
