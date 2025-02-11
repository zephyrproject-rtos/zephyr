/*
 * Copyright (c) 2023 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARC Cluster registers and accessors
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_CLUSTER_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_CLUSTER_H_

#include <zephyr/arch/arc/v2/aux_regs.h>
#include <zephyr/sys/util_macro.h>

/* Cluster AUX */
#define _ARC_REG_CLN_BCR                        0xcf

#define _ARC_CLNR_ADDR                          0x640 /* CLN address for CLNR_DATA */
#define _ARC_CLNR_DATA                          0x641 /* CLN data indicated by CLNR_ADDR */
#define _ARC_CLNR_DATA_NXT                      0x642 /* CLNR_DATA and then CLNR_ADDR++ */
#define _ARC_CLNR_BCR_0                         0xF61
#define _ARC_CLNR_BCR_1                         0xF62
#define _ARC_CLNR_BCR_2                         0xF63
#define _ARC_CLNR_SCM_BCR_0                     0xF64
#define _ARC_CLNR_SCM_BCR_1                     0xF65

#define _ARC_REG_CLN_BCR_VER_MAJOR_ARCV3_MIN    32    /* Minimal version of cluster in ARCv3 */
#define _ARC_CLN_BCR_VER_MAJOR_MASK             0xff
#define _ARC_CLNR_BCR_0_HAS_SCM                 BIT(29)

/* Cluster registers (not in the AUX address space - indirect access via CLNR_ADDR + CLNR_DATA) */
#define ARC_CLN_MST_NOC_0_BCR                   0
#define ARC_CLN_MST_NOC_1_BCR                   1
#define ARC_CLN_MST_NOC_2_BCR                   2
#define ARC_CLN_MST_NOC_3_BCR                   3
#define ARC_CLN_MST_PER_0_BCR                   16
#define ARC_CLN_MST_PER_1_BCR                   17
#define ARC_CLN_PER_0_BASE                      2688
#define ARC_CLN_PER_0_SIZE                      2689
#define ARC_CLN_PER_1_BASE                      2690
#define ARC_CLN_PER_1_SIZE                      2691

#define ARC_CLN_SCM_BCR_0                       100
#define ARC_CLN_SCM_BCR_1                       101

#define ARC_CLN_MST_NOC_0_0_ADDR                292
#define ARC_CLN_MST_NOC_0_0_SIZE                293
#define ARC_CLN_MST_NOC_0_1_ADDR                2560
#define ARC_CLN_MST_NOC_0_1_SIZE                2561
#define ARC_CLN_MST_NOC_0_2_ADDR                2562
#define ARC_CLN_MST_NOC_0_2_SIZE                2563
#define ARC_CLN_MST_NOC_0_3_ADDR                2564
#define ARC_CLN_MST_NOC_0_3_SIZE                2565
#define ARC_CLN_MST_NOC_0_4_ADDR                2566
#define ARC_CLN_MST_NOC_0_4_SIZE                2567

#define ARC_CLN_PER0_BASE                       2688
#define ARC_CLN_PER0_SIZE                       2689

#define ARC_CLN_SHMEM_ADDR                      200
#define ARC_CLN_SHMEM_SIZE                      201
#define ARC_CLN_CACHE_ADDR_LO0                  204
#define ARC_CLN_CACHE_ADDR_LO1                  205
#define ARC_CLN_CACHE_ADDR_HI0                  206
#define ARC_CLN_CACHE_ADDR_HI1                  207
#define ARC_CLN_CACHE_CMD                       207
#define ARC_CLN_CACHE_CMD_OP_NOP                0b0000
#define ARC_CLN_CACHE_CMD_OP_LOOKUP             0b0001
#define ARC_CLN_CACHE_CMD_OP_PROBE              0b0010
#define ARC_CLN_CACHE_CMD_OP_IDX_INV            0b0101
#define ARC_CLN_CACHE_CMD_OP_IDX_CLN            0b0110
#define ARC_CLN_CACHE_CMD_OP_IDX_CLN_INV        0b0111
#define ARC_CLN_CACHE_CMD_OP_REG_INV            0b1001
#define ARC_CLN_CACHE_CMD_OP_REG_CLN            0b1010
#define ARC_CLN_CACHE_CMD_OP_REG_CLN_INV        0b1011
#define ARC_CLN_CACHE_CMD_OP_ADDR_INV           0b1101
#define ARC_CLN_CACHE_CMD_OP_ADDR_CLN           0b1110
#define ARC_CLN_CACHE_CMD_OP_ADDR_CLN_INV       0b1111
#define ARC_CLN_CACHE_CMD_INCR                  BIT(4)

#define ARC_CLN_CACHE_STATUS                    209
#define ARC_CLN_CACHE_STATUS_BUSY               BIT(23)
#define ARC_CLN_CACHE_STATUS_DONE               BIT(24)
#define ARC_CLN_CACHE_STATUS_MASK               BIT(26)
#define ARC_CLN_CACHE_STATUS_EN                 BIT(27)
#define ARC_CLN_CACHE_ERR                       210
#define ARC_CLN_CACHE_ERR_ADDR0                 211
#define ARC_CLN_CACHE_ERR_ADDR1                 212


static inline unsigned int arc_cln_read_reg_nolock(unsigned int reg)
{
	z_arc_v2_aux_reg_write(_ARC_CLNR_ADDR, reg);
	return z_arc_v2_aux_reg_read(_ARC_CLNR_DATA);
}

static inline void arc_cln_write_reg_nolock(unsigned int reg, unsigned int data)
{
	z_arc_v2_aux_reg_write(_ARC_CLNR_ADDR, reg);
	z_arc_v2_aux_reg_write(_ARC_CLNR_DATA, data);
}

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_CLUSTER_H_ */
