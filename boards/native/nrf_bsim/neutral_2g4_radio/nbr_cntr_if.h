/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A free running 1MHz counter of the simulation time of the device, the time
 * base of the packet level radio model, with one compare.
 */

#ifndef BOARDS_NATIVE_NRF_BSIM_NEUTRAL_2G4_RADIO_NBR_CNTR_IF_H
#define BOARDS_NATIVE_NRF_BSIM_NEUTRAL_2G4_RADIO_NBR_CNTR_IF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the counter, giving the interrupt line of its compare */
void nbr_cntr_init(unsigned int irq);

/* Current value of the counter */
uint64_t nbr_cntr_get(void);

/*
 * Raise the counter interrupt when the counter reaches the given value. A
 * value that is not in the future never matches.
 */
void nbr_cntr_cmp_set(uint64_t value);
void nbr_cntr_cmp_disable(void);

/*
 * Return true if the counter compare matched since the last call, clearing
 * the match. The counter interrupt line may be shared with other uses.
 */
bool nbr_cntr_cmp_evt_get_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_NATIVE_NRF_BSIM_NEUTRAL_2G4_RADIO_NBR_CNTR_IF_H */
