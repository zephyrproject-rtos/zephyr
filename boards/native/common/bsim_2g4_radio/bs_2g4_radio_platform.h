/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The board owns the connection to the 2G4 Phy, which the model uses through
 * the libPhyCom "_nc" API, and the interrupt controller, so each board
 * provides these functions to the model.
 */

#ifndef BOARDS_NATIVE_COMMON_BSIM_2G4_RADIO_BS_2G4_RADIO_PLATFORM_H
#define BOARDS_NATIVE_COMMON_BSIM_2G4_RADIO_BS_2G4_RADIO_PLATFORM_H

#include "bs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Convert a device time into a Phy time, and back */
bs_time_t bsr_plat_phy_time_from_dev(bs_time_t dev_time);
bs_time_t bsr_plat_dev_time_from_phy(bs_time_t phy_time);

/* Raise an interrupt in the CPU running the embedded software */
void bsr_plat_irq_raise(unsigned int irq);

/* The device has synchronized with the Phy up to the given device time */
void bsr_plat_phy_synced(bs_time_t dev_time);

/* The Phy disconnected this device */
void bsr_plat_phy_disconnected(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_NATIVE_COMMON_BSIM_2G4_RADIO_BS_2G4_RADIO_PLATFORM_H */
