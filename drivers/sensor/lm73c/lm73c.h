/*
 * Copyright (c) 2020 SER Consulting LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LM73C_H
#define _LM73C_H

/* LM73 registers */
#define LM73_REG_TEMP   0x00
#define LM73_REG_CONF   0x01
#define LM73_REG_THIGH  0x02
#define LM73_REG_TLOW   0x03
#define LM73_REG_CTRL   0x04
#define LM73_REG_ID     0x07

#define LM73_ID         0x9001

#if defined CONFIG_LM73C_TEMP_PREC_1_4
#define LM73_TEMP_PREC                (0x00)
#elif defined CONFIG_LM73C_TEMP_PREC_1_8
#define LM73_TEMP_PREC                (0x20)
#elif defined CONFIG_LM73C_TEMP_PREC_1_16
#define LM73_TEMP_PREC                (0x40)
#elif defined CONFIG_LM73C_TEMP_PREC_1_32
#define LM73_TEMP_PREC                (0x60)
#endif

#endif /* _LM73C_H */
