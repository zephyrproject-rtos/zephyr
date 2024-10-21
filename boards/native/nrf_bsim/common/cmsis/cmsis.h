/*
 * Copyright (c) 2020 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This header defines replacements for inline
 * ARM Cortex-M CMSIS intrinsics.
 */

#ifndef BOARDS_POSIX_NRF52_BSIM_CMSIS_H
#define BOARDS_POSIX_NRF52_BSIM_CMSIS_H

#include <stdint.h>
#include "cmsis_instr.h"
#if defined(CONFIG_SOC_COMPATIBLE_NRF52833)
#include "nrf52833.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void __enable_irq(void);
void __disable_irq(void);
uint32_t __get_PRIMASK(void);
void __set_PRIMASK(uint32_t primask);

void NVIC_SetPendingIRQ(IRQn_Type IRQn);
void NVIC_ClearPendingIRQ(IRQn_Type IRQn);
void NVIC_DisableIRQ(IRQn_Type IRQn);
uint32_t NVIC_GetPendingIRQ(IRQn_Type IRQn);
void NVIC_EnableIRQ(IRQn_Type IRQn);
void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority);
uint32_t NVIC_GetPriority(IRQn_Type IRQn);
void NVIC_SystemReset(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NRF52_BSIM_CMSIS_H */
