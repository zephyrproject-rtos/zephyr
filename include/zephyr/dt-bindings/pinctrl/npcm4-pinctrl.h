/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NPCM4_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NPCM4_PINCTRL_H_

/*
 * NPCM4 System Configuration (SCFG) register offsets used by its pin function table.
 * A pin function names the bits it writes as <register bit value> cells.
 */

/* Device control */
#define NPCM_SCFG_DEVCNT   0x00
#define NPCM_SCFG_DEV_CTL2 0x03
#define NPCM_SCFG_DEV_CTL3 0x04
#define NPCM_SCFG_DEV_CTL4 0x06
#define NPCM_SCFG_EMC_CTL  0x4d

/* Alternate function selection */
#define NPCM_SCFG_DEVALT10 0x0b
#define NPCM_SCFG_DEVALT11 0x0c
#define NPCM_SCFG_DEVALT0  0x10
#define NPCM_SCFG_DEVALT1  0x11
#define NPCM_SCFG_DEVALT2  0x12
#define NPCM_SCFG_DEVALT3  0x13
#define NPCM_SCFG_DEVALT4  0x14
#define NPCM_SCFG_DEVALT5  0x15
#define NPCM_SCFG_DEVALT6  0x16
#define NPCM_SCFG_DEVALT7  0x17
#define NPCM_SCFG_DEVALT8  0x18
#define NPCM_SCFG_DEVALT9  0x19
#define NPCM_SCFG_DEVALTA  0x1a
#define NPCM_SCFG_DEVALTB  0x1b
#define NPCM_SCFG_DEVALTC  0x1c
#define NPCM_SCFG_DEVALTD  0x1d
#define NPCM_SCFG_DEVALTE  0x1e
#define NPCM_SCFG_DEVALTF  0x1f
#define NPCM_SCFG_DEVALTCX 0x24
#define NPCM_SCFG_DEVALT2E 0x2e
#define NPCM_SCFG_DEVALT30 0x30
#define NPCM_SCFG_DEVALT31 0x31
#define NPCM_SCFG_DEVALT32 0x32
#define NPCM_SCFG_DEVALT34 0x34
#define NPCM_SCFG_DEVALT50 0x50
#define NPCM_SCFG_DEVALT51 0x51
#define NPCM_SCFG_DEVALT52 0x52
#define NPCM_SCFG_DEVALT53 0x53
#define NPCM_SCFG_DEVALT54 0x54
#define NPCM_SCFG_DEVALT55 0x55
#define NPCM_SCFG_DEVALT59 0x59
#define NPCM_SCFG_DEVALT5A 0x5a
#define NPCM_SCFG_DEVALT5B 0x5b
#define NPCM_SCFG_DEVALT5C 0x5c
#define NPCM_SCFG_DEVALT5D 0x5d
#define NPCM_SCFG_DEVALT5E 0x5e
#define NPCM_SCFG_DEVALT5F 0x5f
#define NPCM_SCFG_DEVALT62 0x62
#define NPCM_SCFG_DEVALT66 0x66
#define NPCM_SCFG_DEVALT67 0x67
#define NPCM_SCFG_DEVALT69 0x69
#define NPCM_SCFG_DEVALT6A 0x6a
#define NPCM_SCFG_DEVALT6B 0x6b
#define NPCM_SCFG_DEVALT6C 0x6c
#define NPCM_SCFG_DEVALT6D 0x6d

/* Pull-up/down enable */
#define NPCM_SCFG_DEVPU0 0x28
#define NPCM_SCFG_DEVPD1 0x29
#define NPCM_SCFG_DEVPU2 0x73
#define NPCM_SCFG_DEVPD3 0x7b

/* Low-voltage select */
#define NPCM_SCFG_LV_CTL0 0x2a
#define NPCM_SCFG_LV_CTL1 0x2b
#define NPCM_SCFG_LV_CTL3 0x2d
#define NPCM_SCFG_LV_CTL4 0x6e

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NPCM4_PINCTRL_H_ */
