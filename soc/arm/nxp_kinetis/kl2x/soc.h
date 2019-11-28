/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <sys/util.h>

#define UART0_CLK_SRC kCLOCK_CoreSysClk

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

/* Add include for DTS generated information */
#include <generated_dts_board.h>

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
