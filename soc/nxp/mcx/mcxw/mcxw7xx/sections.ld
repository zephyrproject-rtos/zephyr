/*
 * Copyright 2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(smu2), okay)
#define SHARED_MEM_LABEL smu2
#define SHARED_MEM_REGION SMU2
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(stcm_shared), okay)
#define SHARED_MEM_LABEL stcm_shared
#define SHARED_MEM_REGION STCM_SHARED
#endif

#if defined(SHARED_MEM_LABEL)
/* NOINIT section for shared memory */
.noinit_shared_mem (NOLOAD) : ALIGN(4)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(shmem_fwk), okay)
    . = DT_REG_ADDR(DT_NODELABEL(shmem_fwk)) - DT_REG_ADDR(DT_NODELABEL(SHARED_MEM_LABEL));
    PROVIDE(m_lowpower_flag_start = . + 0x4);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(shmem_rpmsg), okay)
    . = DT_REG_ADDR(DT_NODELABEL(shmem_rpmsg)) - DT_REG_ADDR(DT_NODELABEL(SHARED_MEM_LABEL));
    PROVIDE(rpmsg_sh_mem_start = .);
    *(.noinit.$rpmsg_sh_mem*)
    . = ALIGN(4) ;
    PROVIDE(rpmsg_sh_mem_end = .);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(shmem_data), okay)
    *(.m_shared_data)
    . = ALIGN(4);
#endif
    . = DT_REG_SIZE(DT_NODELABEL(SHARED_MEM_LABEL));
} > SHARED_MEM_REGION
#endif
