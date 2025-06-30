/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

volatile uint32_t reason, address;

#define EXCEPTION_HANDLER __attribute__((interrupt, no_auto_psv, keep))

void EXCEPTION_HANDLER _ReservedTrap7(void);
void __attribute__((weak)) TRAPS_halt_on_error(void);
void EXCEPTION_HANDLER _BusErrorTrap(void);
void EXCEPTION_HANDLER _AddressErrorTrap(void);
void EXCEPTION_HANDLER _IllegalInstructionTrap(void);
void EXCEPTION_HANDLER _MathErrorTrap(void);
void EXCEPTION_HANDLER _StackErrorTrap(void);
void EXCEPTION_HANDLER _GeneralTrap(void);
void EXCEPTION_HANDLER _ReservedTrap0(void);
void EXCEPTION_HANDLER _ReservedTrap7(void);

void __attribute__((weak)) TRAPS_halt_on_error(void)
{
}

/** Bus error.**/
void EXCEPTION_HANDLER _BusErrorTrap(void)
{
	__asm__("nop");
	__asm__("retfie");
}

/** Address error.**/
void EXCEPTION_HANDLER _AddressErrorTrap(void)
{
	__asm__("nop");
	__asm__("retfie");
}

/** Illegal instruction.**/
void EXCEPTION_HANDLER _IllegalInstructionTrap(void)
{
	__asm__("nop");
	__asm__("retfie");
}

/** Math error.**/
void EXCEPTION_HANDLER _MathErrorTrap(void)
{
	__asm__("nop");
	__asm__("retfie");
}

/** Stack error.**/
void EXCEPTION_HANDLER _StackErrorTrap(void)
{
	__asm__("nop");
	__asm__("retfie");
}

/** Generic error.**/
void EXCEPTION_HANDLER _GeneralTrap(void)
{
	__asm__("nop");
	__asm__("retfie");
}

/** Reserved Trap0.**/
void EXCEPTION_HANDLER _ReservedTrap0(void)
{
	__asm__("nop");
	__asm__("retfie");
}

/** Reserved Trap7.**/
void EXCEPTION_HANDLER _ReservedTrap7(void)
{
	__asm__("nop");
	__asm__("retfie");
}
