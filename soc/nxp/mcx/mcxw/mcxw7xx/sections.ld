/*
 * Copyright 2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* NOINIT section for shared memory SMU2 */
.noinit_smu2 (NOLOAD) : ALIGN(4)
{
    . = DT_REG_ADDR(DT_NODELABEL(shmem_fwk)) - DT_REG_ADDR(DT_NODELABEL(smu2));
    PROVIDE(m_lowpower_flag_start = . + 0x4);
    . = DT_REG_ADDR(DT_NODELABEL(shmem_rpmsg)) - DT_REG_ADDR(DT_NODELABEL(smu2));
    PROVIDE(rpmsg_sh_mem_start = .);
    *(.noinit.$rpmsg_sh_mem*)
    . = ALIGN(4) ;
    PROVIDE(rpmsg_sh_mem_end = .);
    . = DT_REG_SIZE(DT_NODELABEL(smu2));
} > SMU2
