/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/** @brief    Custom definition for boolean type to avoid compiler disrepenrencies
 *	@{
 */

#ifndef _INV_BOOL_H_
#define _INV_BOOL_H_

typedef int inv_bool_t;

#ifndef __cplusplus

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#endif /* __cplusplus */

#endif /* _INV_BOOL_H_ */

/** @} */
