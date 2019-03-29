/*
 * Copyright (c) 2019 Vaishali Pathak vaishali@electronut.in
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _LTR329ALS01_H
#define _LTR329ALS01_H

#ifdef __cplusplus
extern "C" {
#endif


/* Light sensor address */
#define ALS_ADDR 0x29

/* Light sensor registers */
#define ALS_CONTR_REG 0x80
#define ALS_MEAS_RATE_REG 0x85
#define PART_ID_REG 0x86
#define MANUFAC_ID_REG 0x87
#define ALS_DATA_CH1_0_REG 0x88
#define ALS_DATA_CH1_1_REG 0x89
#define ALS_DATA_CH0_0_REG 0x8A
#define ALS_DATA_CH0_1_REG 0x8B
#define ALS_STATUS_REG 0x8C

#define RESERVED -1

#ifdef __cplusplus
}
#endif

#endif /* _MODULE_H */
