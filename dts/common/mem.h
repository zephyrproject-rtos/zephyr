/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DT_MEM_H
#define __DT_MEM_H

#define DT_SIZE_K(x) ((x) * 1024)
#define DT_SIZE_M(x) ((x) * 1024 * 1024)

/* concatenate the values of the arguments into one */
#define _DT_DO_CONCAT(x, y) x ## y

#define DT_ADDR(a) _DT_DO_CONCAT(0x, a)

#endif /* __DT_MEM_H */
