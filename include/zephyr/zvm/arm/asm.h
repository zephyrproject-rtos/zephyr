/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_ASM_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_ASM_H_

#define ARM_VM_EXCEPTION_SYNC			(0x01)
#define ARM_VM_EXCEPTION_IRQ			(0x02)
#define ARM_VM_EXCEPTION_SERROR			(0x03)
#define ARM_VM_EXCEPTION_IRQ_IN_SYNC	(0x04)

#define DFSC_FT_TRANS_L0	(0x04)
#define DFSC_FT_TRANS_L1	(0x05)
#define DFSC_FT_TRANS_L2	(0x06)
#define DFSC_FT_TRANS_L3	(0x07)
#define DFSC_FT_ACCESS_L0	(0x08)
#define DFSC_FT_ACCESS_L1	(0x09)
#define DFSC_FT_ACCESS_L2	(0x0A)
#define DFSC_FT_ACCESS_L3	(0x0B)
#define DFSC_FT_PERM_L0		(0x0C)
#define DFSC_FT_PERM_L1		(0x0D)
#define DFSC_FT_PERM_L2		(0x0E)
#define DFSC_FT_PERM_L3		(0x0F)

#define ARM_VM_SERROR_PENDING(x)	  !!((x) & (1U << 31))

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_ASM_H_ */
