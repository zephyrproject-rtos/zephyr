/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TXQ23_SIZE  0x1080
#define SMU1_SIZE   DT_REG_SIZE(DT_NODELABEL(smu1_data))
#define SMU2_SIZE   DT_REG_SIZE(DT_NODELABEL(smu2_data))


.SMU1 (NOLOAD) :
{
    . = ALIGN(4);
    /* CPU3 <-> CPU1 mailbox */
    *(.smu_cpu13_mbox)
    KEEP (*(.smu_cpu13_mbox))
    /* CPU3 -> CPU1 TXQ */
    *(.smu_cpu31_txq)
    KEEP (*(.smu_cpu31_txq))
    /* Left space is reserved for CPU3/CPU1 operations */
    . += SMU1_SIZE - .;
} > SMU1

.SMU2 (NOLOAD) :
{
    . = ALIGN(4);
    /* CPU3 <-> CPU2 mailbox */
    *(.smu_cpu23_mbox)
    KEEP (*(.smu_cpu23_mbox))
    /* Reserve space for CPU1 -> CPU3 TXQ (allocated by the CPU2) */
    . += TXQ23_SIZE;
    /* CPU3 -> CPU2 TXQ */
    *(.smu_cpu32_txq)
    KEEP (*(.smu_cpu32_txq))
    /* Left space is reserved for CPU3/CPU2 operations */
    . += SMU2_SIZE - .;
} > SMU2
