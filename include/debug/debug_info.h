/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Information necessary for debugging
 *
 * @internal No routine is provided for getting the current exception stack
 * frame as the exception handler already has knowledge of the ESF.
 */

#ifndef __DEBUG_INFO_H
#define __DEBUG_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel.h>

#ifndef _ASMLANGUAGE

/**
 * @brief Get the current interrupt stack frame
 *
 * @details This routine (only called from an ISR) returns a
 * pointer to the current interrupt stack frame.
 *
 * @return pointer the current interrupt stack frame
 */
extern NANO_ISF *sys_debug_current_isf_get(void);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_INFO_H */
