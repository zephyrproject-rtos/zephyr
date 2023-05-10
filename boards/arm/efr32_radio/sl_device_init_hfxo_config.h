/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SL_DEVICE_INIT_HFXO_CONFIG_H
#define SL_DEVICE_INIT_HFXO_CONFIG_H

#ifdef CONFIG_BOARD_EFR32_RADIO_BRD4187C

#define SL_DEVICE_INIT_HFXO_MODE           cmuHfxoOscMode_Crystal
#define SL_DEVICE_INIT_HFXO_FREQ           39000000
#define SL_DEVICE_INIT_HFXO_CTUNE          140

#endif /* CONFIG_BOARD_EFR32_RADIO_BRD4187C */

#endif /* SL_DEVICE_INIT_HFXO_CONFIG_H */
