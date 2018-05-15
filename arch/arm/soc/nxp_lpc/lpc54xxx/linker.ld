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

#include <arch/arm/cortex_m/scripts/linker.ld>
