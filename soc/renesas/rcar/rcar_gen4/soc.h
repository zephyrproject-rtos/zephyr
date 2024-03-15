/*
 * Copyright (c) 2023 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _SOC__H_
#define _SOC__H_

/* Define CMSIS configurations */
#define __CR_REV      1U

/* Do not let CMSIS to handle GIC and Timer */
#define __GIC_PRESENT 0
#define __TIM_PRESENT 0

#endif /* _SOC__H_ */
