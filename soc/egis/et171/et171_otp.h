/*
 * Copyright (c) 2026 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_EGIS_ET171_OTP_H__
#define __SOC_EGIS_ET171_OTP_H__

#include <stdint.h>

/**
 *  \brief       Low level load OTP analog config options to AOSMU registers
 *               It won't read OTP again.
 *  \return      none
 */
void et171_otp_ll_load_analog_config(void);

/**
 *  \brief       Low level get system root clock from OTP
 *  \returns     none
 */
uint32_t et171_otp_ll_get_root_clock(void);

#endif  /* __SOC_EGIS_ET171_OTP_H__ */
