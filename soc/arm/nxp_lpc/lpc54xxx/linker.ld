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

#ifdef CONFIG_RPMSG_LITE
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
