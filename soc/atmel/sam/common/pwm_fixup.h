/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAM_PWM_FIXUP_H_
#define _ATMEL_SAM_PWM_FIXUP_H_

/* The SAMV71 HALs change the name of the field, so we need to
 * define it this way to match how the other SoC variants name it
 */
#if defined(CONFIG_SOC_SERIES_SAMV71)
#define PWM_CH_NUM PwmChNum
#endif

#endif /* _ATMEL_SAM_PWM_FIXUP_H_ */
