/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2020 Oticon A/S
 * Copyright (c) 2009-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This header defines replacements for inline
 * ARM Cortex-M CMSIS intrinsics.
 */

#ifndef BOARDS_POSIX_NRF52_BSIM_CMSIS_INSTR_H
#define BOARDS_POSIX_NRF52_BSIM_CMSIS_INSTR_H

/* Implement the following ARM intrinsics as no-op:
 * - ARM Data Synchronization Barrier
 * - ARM Data Memory Synchronization Barrier
 * - ARM Instruction Synchronization Barrier
 * - ARM No Operation
 */
#ifndef __DMB
#define __DMB()
#endif

#ifndef __DSB
#define __DSB()
#endif

#ifndef __ISB
#define __ISB()
#endif

#ifndef __NOP
#define __NOP()
#endif

void __WFE(void);
void __WFI(void);
void __SEV(void);

/*
 *  Implement the following ARM intrinsics as non-exclusive accesses
 *
 *  - STR Exclusive(8,16 & 32bit) (__STREX{B,H,W})
 *  - LDR Exclusive(8,16 & 32bit) (__LDREX{B,H,W})
 *  - CLREX : Exclusive lock removal (__CLREX) - no-op
 *
 *  Description:
 *    These accesses always succeed, and do NOT set any kind of internal
 *    exclusive access flag;
 *    There is no local/global memory monitors, MPU control of what are
 *    shareable regions, exclusive reservations granules, automatic clearing
 *    on context switch, or so.
 *
 *    This should be enough for the expected uses of LDR/STREXB
 *    (locking mutexes or guarding other atomic operations, inside a few lines
 *    of code in the same function): As the POSIX arch will not make an embedded
 *    thread lose context while just executing its own code, and it does not
 *    allow parallel embedded SW threads to execute at the same exact time,
 *    there is no actual need to protect atomicity.
 *
 *    But as this ARM exclusive access monitor mechanism can in principle be
 *    used for other, unexpected, purposes, this simple replacement may not be
 *    enough.
 */

/**
 * \brief   Pretend to execute a STR Exclusive (8 bit)
 * \details Executes a ~exclusive~ STR instruction for 8 bit values.
 * \param [in]  value  Value to store
 * \param [in]    ptr  Pointer to location
 * \return          0  Function succeeded (always)
 */
static inline uint32_t __STREXB(uint8_t value, volatile uint8_t *ptr)
{
	*ptr = value;
	return 0;
}

/**
 * \brief   Pretend to execute a STR Exclusive (16 bit)
 * \details Executes a ~exclusive~ STR instruction for 16 bit values.
 * \param [in]  value  Value to store
 * \param [in]    ptr  Pointer to location
 * \return          0  Function succeeded (always)
 */
static inline uint32_t __STREXH(uint16_t value, volatile uint16_t *ptr)
{
	*ptr = value;
	return 0;
}

/**
 * \brief   Pretend to execute a STR Exclusive (32 bit)
 * \details Executes a ~exclusive~ STR instruction for 32 bit values.
 * \param [in]  value  Value to store
 * \param [in]    ptr  Pointer to location
 * \return          0  Function succeeded (always)
 */
static inline uint32_t __STREXW(uint32_t value, volatile uint32_t *ptr)
{
	*ptr = value;
	return 0;
}

/**
 * \brief   Pretend to execute a LDR Exclusive (8 bit)
 * \details Executes an ~exclusive~ LDR instruction for 8 bit value.
 *          Meaning, it does not set a exclusive lock,
 *          instead just loads the stored value
 * \param [in]    ptr  Pointer to data
 * \return             value of type uint8_t at (*ptr)
 */
static inline uint8_t __LDREXB(volatile uint8_t *ptr)
{
	return *ptr;
}

/**
 * \brief   Pretend to execute a LDR Exclusive (16 bit)
 * \details Executes an ~exclusive~ LDR instruction for 16 bit value.
 *          Meaning, it does not set a exclusive lock,
 *          instead just loads the stored value
 * \param [in]    ptr  Pointer to data
 * \return             value of type uint8_t at (*ptr)
 */
static inline uint16_t __LDREXH(volatile uint16_t *ptr)
{
	return *ptr;
}

/**
 * \brief   Pretend to execute a LDR Exclusive (32 bit)
 * \details Executes an ~exclusive~ LDR instruction for 32 bit value.
 *          Meaning, it does not set a exclusive lock,
 *          instead just loads the stored value
 * \param [in]    ptr  Pointer to data
 * \return             value of type uint8_t at (*ptr)
 */
static inline uint32_t __LDREXW(volatile uint32_t *ptr)
{
	return *ptr;
}

/**
 * \brief   Pretend to remove the exclusive lock
 * \details The real function would removes the exclusive lock which is created
 *          by LDREX, this one does nothing
 */
static inline void __CLREX(void) { /* Nothing to be done */ }

/**
 * \brief Model of an ARM CLZ instruction
 */
static inline unsigned char __CLZ(uint32_t value)
{
	if (value == 0) {
		return 32;
	} else {
		return __builtin_clz(value);
	}
}

#endif /* BOARDS_POSIX_NRF52_BSIM_CMSIS_INSTR_H */
