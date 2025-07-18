/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include "vector_table.h"

.global __ReservedTrap0
.global __BusErrorTrap
.global __AddressErrorTrap
.global __IllegalInstructionTrap
.global __MathErrorTrap
.global __StackErrorTrap
.global __GeneralTrap
.global __ReservedTrap7
.global _isr_wrapper


.section .exc_vector_table._vector_table_section,code
.global __vector_table
__vector_table:
    .long __ReservedTrap0
    .long __BusErrorTrap
    .long __IllegalInstructionTrap
    .long __AddressErrorTrap
    .long __StackErrorTrap
    .long __MathErrorTrap
    .long __GeneralTrap
    .long __ReservedTrap7
