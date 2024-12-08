# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 sensry.io

#include <zephyr/toolchain.h>


GTEXT(__initialize)

   # abs 0x0000 - entry point after bootloader
.section .pre_start, "ax"
.global __pre_start
__pre_start:
    jal x0, __prestart_routine


   # abs 0x0080 - will be checked by bootloader
.section .validity_marker, "ax"
  .word 0xAA551234

   # abs 0x0100 - text
.section .text, "ax"
__prestart_routine:
    /* things that will be done prior to actually starting zephyr */
    csrwi mstatus, 0x00

    /* Call into Zephyr initialization. */
    jal x0, __start

GTEXT(__soc_is_irq)
SECTION_FUNC(exception.other, __soc_is_irq)
	csrr a0, mcause
	srli a0, a0, 31
	ret

GTEXT(__soc_handle_irq)
SECTION_FUNC(exception.other, __soc_handle_irq)
	## clear pending interrupt
	ret

# we provide a "backup" isr table, if generation (ex. in tests) is disabled
.global _irq_vector_table
.weak _irq_vector_table
.section .text, "ax"
_irq_vector_table:
.option norvc;

  j __no_irq_handler0
  j __no_irq_handler1
  j __no_irq_handler2
  j __no_irq_handler3
  j __no_irq_handler4
  j __no_irq_handler5
  j __no_irq_handler6
  j __no_irq_handler7
  j __no_irq_handler8
  j __no_irq_handler9
  j __no_irq_handler10
  j __no_irq_handler11
  j __no_irq_handler12
  j __no_irq_handler13
  j __no_irq_handler14
  j __no_irq_handler15
  j __no_irq_handler16
  j __no_irq_handler17
  j __no_irq_handler18
  j __no_irq_handler19
  j __no_irq_handler20
  j __no_irq_handler21
  j __no_irq_handler22
  j __no_irq_handler23
  j __no_irq_handler24
  j __no_irq_handler25
  j __no_irq_handler26
  j __no_irq_handler27
  j __no_irq_handler28
  j __no_irq_handler29
  j __no_irq_handler30
  j __no_irq_handler31


__no_irq_handler0:
  j __no_irq_handler0
__no_irq_handler1:
  j __no_irq_handler1
__no_irq_handler2:
  j __no_irq_handler2
__no_irq_handler3:
  j __no_irq_handler3
__no_irq_handler4:
  j __no_irq_handler4
__no_irq_handler5:
  j __no_irq_handler5
__no_irq_handler6:
  j __no_irq_handler6
__no_irq_handler7:
  j __no_irq_handler7
__no_irq_handler8:
  j __no_irq_handler8
__no_irq_handler9:
  j __no_irq_handler9
__no_irq_handler10:
  j __no_irq_handler10
__no_irq_handler11:
  j __no_irq_handler11
__no_irq_handler12:
  j __no_irq_handler12
__no_irq_handler13:
  j __no_irq_handler13
__no_irq_handler14:
  j __no_irq_handler14
__no_irq_handler15:
  j __no_irq_handler15
__no_irq_handler16:
  j __no_irq_handler16
__no_irq_handler17:
  j __no_irq_handler17
__no_irq_handler18:
  j __no_irq_handler18
__no_irq_handler19:
  j __no_irq_handler19
__no_irq_handler20:
  j __no_irq_handler20
__no_irq_handler21:
  j __no_irq_handler21
__no_irq_handler22:
  j __no_irq_handler22
__no_irq_handler23:
  j __no_irq_handler23
__no_irq_handler24:
  j __no_irq_handler24
__no_irq_handler25:
  j __no_irq_handler25
__no_irq_handler26:
  j __no_irq_handler26
__no_irq_handler27:
  j __no_irq_handler27
__no_irq_handler28:
  j __no_irq_handler28
__no_irq_handler29:
  j __no_irq_handler29
__no_irq_handler30:
  j __no_irq_handler30
__no_irq_handler31:
  j __no_irq_handler31
