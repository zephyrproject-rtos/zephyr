/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for both standard images and XIP images.
 */

#include <autoconf.h>

/* Specify the memory areas */
MEMORY
{
  m_core1_image         (RX)  : ORIGIN = 0x00030000, LENGTH = 0x00010000
}

/* Define output sections */
SECTIONS
{
  /* section for storing the secondary core image */
  .m0code :
  {
     . = ALIGN(4) ;
    KEEP (*(.m0code))
     *(.m0code*)
     . = ALIGN(4) ;
  } > m_core1_image
}

#ifdef CONFIG_IPC_RPMSG_LITE
/* Specify the shared memory area */
MEMORY
{
  rpmsg_sh_mem          (RW)  : ORIGIN = 0x20026800, LENGTH = 0x1800
}

SECTIONS
{
  /* NOINIT section for rpmsg_sh_mem */
  .noinit_rpmsg_sh_mem (NOLOAD) : ALIGN(4)
  {
     *(.noinit.$rpmsg_sh_mem*)
     . = ALIGN(4) ;
  } > rpmsg_sh_mem
}
#endif

#include <arch/arm/cortex_m/scripts/linker.ld>
