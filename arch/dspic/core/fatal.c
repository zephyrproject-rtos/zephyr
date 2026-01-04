/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifndef _ASMLANGUAGE
#include <xc.h>
#ifdef __cplusplus
extern "C" {
#endif

LOG_MODULE_REGISTER(dspic, 4);

volatile uint32_t reason, address;

#define EXCEPTION_HANDLER __attribute__((interrupt, no_auto_psv, weak, naked))
#define BUS_ERROR_MASK    0xF
#define MATH_ERROR_MASK   0x1F
#define STACK_ERROR_MASK   0x10
#define GENERAL_TRAP_MASK 0x8000000Fu

void __attribute__((weak)) TRAPS_halt_on_error(void);
void EXCEPTION_HANDLER _BusErrorTrap(void);
void EXCEPTION_HANDLER _AddressErrorTrap(void);
void EXCEPTION_HANDLER _IllegalInstructionTrap(void);
void EXCEPTION_HANDLER _MathErrorTrap(void);
void EXCEPTION_HANDLER _StackErrorTrap(void);
void EXCEPTION_HANDLER _GeneralTrap(void);
void EXCEPTION_HANDLER _ReservedTrap0(void);
void EXCEPTION_HANDLER _ReservedTrap7(void);

void EXCEPTION_HANDLER _ReservedTrap0(void)
{
}
void EXCEPTION_HANDLER _ReservedTrap7(void)
{
}

void __attribute__((weak)) TRAPS_halt_on_error(void)
{
	/* stay here forever */
	while (1) {
	}
}

/** Bus error.**/
void EXCEPTION_HANDLER _BusErrorTrap(void)
{
	/* Identify bus error via INTCON3, fetch trap address from
	 * PCTRAP, and reset error flags
	 */
	reason = INTCON3 & BUS_ERROR_MASK;
	address = PCTRAP;
	LOG_ERR("ERROR !!! Exception reason = %d, address = 0x%x\n", reason, address);
	INTCON3 &= ~(BUS_ERROR_MASK);
	PCTRAP = 0;
	TRAPS_halt_on_error();
}

/** Address error.**/
void EXCEPTION_HANDLER _AddressErrorTrap(void)
{
	/* fetch trap address from PCTRAP
	 * and reset error flags
	 */
	address = PCTRAP;
	LOG_ERR("ERROR !!! Exception reason = %s, address = 0x%x\n", "Address Error", address);
	INTCON1bits.ADDRERR = 0;
	PCTRAP = 0;
	TRAPS_halt_on_error();
}

/** Illegal instruction.**/
void EXCEPTION_HANDLER _IllegalInstructionTrap(void)
{
	address = PCTRAP;
	LOG_ERR("ERROR !!! Exception reason = %s, address = 0x%x\n", "Illegal Instruction",
		address);
	INTCON1bits.BADOPERR = 0;
	PCTRAP = 0;
	TRAPS_halt_on_error();
}

/** Math error.**/
void EXCEPTION_HANDLER _MathErrorTrap(void)
{
	/* Identify math error via INTCON4, fetch trap address from
	 * PCTRAP, and reset error flags
	 */
	reason = INTCON4 & MATH_ERROR_MASK;
	address = PCTRAP;
	LOG_ERR("ERROR !!! Exception reason = %d, address = 0x%x\n", reason, address);
	INTCON4 &= ~(MATH_ERROR_MASK);
	PCTRAP = 0;
	TRAPS_halt_on_error();
}

/** Stack error.**/
void EXCEPTION_HANDLER _StackErrorTrap(void)
{
	const char *name = k_thread_name_get(_current);

	reason = INTCON1 & STACK_ERROR_MASK;
	address = PCTRAP;
	if (name == NULL) {
		name = "Unnamed";
	}
	LOG_ERR("ERROR !!! Exception reason = %d, address = 0x%x", reason, address);
	LOG_ERR("Thread : %p (%s)\n", _current, name);
	INTCON1bits.STKERR = 0;
	PCTRAP = 0;
	TRAPS_halt_on_error();
}

/** Generic error.**/
void EXCEPTION_HANDLER _GeneralTrap(void)
{
	reason = INTCON5 & GENERAL_TRAP_MASK;
	address = PCTRAP;
	LOG_ERR("ERROR !!! Exception reason = %d, address = 0x%x\n", reason, address);
	INTCON5 &= ~(GENERAL_TRAP_MASK);
	PCTRAP = 0;
	TRAPS_halt_on_error();
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
