/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9340_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9340_H_

#include <sys/util.h>

/* Commands/registers. */
#define ILI9340_SLPOUT 0x11
#define ILI9340_GAMSET 0x26
#define ILI9340_DISPOFF 0x28
#define ILI9340_DISPON 0x29
#define ILI9340_CASET 0x2a
#define ILI9340_PASET 0x2b
#define ILI9340_RAMWR 0x2c
#define ILI9340_MADCTL 0x36
#define ILI9340_PIXSET 0x3A
#define ILI9340_FRMCTR1 0xB1
#define ILI9340_DISCTRL 0xB6
#define ILI9340_PWCTRL1 0xC0
#define ILI9340_PWCTRL2 0xC1
#define ILI9340_VMCTRL1 0xC5
#define ILI9340_VMCTRL2 0xC7
#define ILI9340_PGAMCTRL 0xE0
#define ILI9340_NGAMCTRL 0xE1

/* Commands/registers length. */
#define ILI9340_GAMSET_LEN 1U
#define ILI9340_FRMCTR1_LEN 2U
#define ILI9340_DISCTRL_LEN 3U
#define ILI9340_PWCTRL1_LEN 2U
#define ILI9340_PWCTRL2_LEN 1U
#define ILI9340_VMCTRL1_LEN 2U
#define ILI9340_VMCTRL2_LEN 1U
#define ILI9340_PGAMCTRL_LEN 15U
#define ILI9340_NGAMCTRL_LEN 15U

/* MADCTL register fields. */
#define ILI9340_MADCTL_MY BIT(7U)
#define ILI9340_MADCTL_MX BIT(6U)
#define ILI9340_MADCTL_MV BIT(5U)
#define ILI9340_MADCTL_ML BIT(4U)
#define ILI9340_MADCTL_BGR BIT(3U)
#define ILI9340_MADCTL_MH BIT(2U)

/* PIXSET register fields. */
#define ILI9340_PIXSET_RGB_18_BIT 0x60
#define ILI9340_PIXSET_RGB_16_BIT 0x50
#define ILI9340_PIXSET_MCU_18_BIT 0x06
#define ILI9340_PIXSET_MCU_16_BIT 0x05

/** Command/data GPIO level for commands. */
#define ILI9340_CMD 1U
/** Command/data GPIO level for data. */
#define ILI9340_DATA 0U

/** Sleep out time (ms), ref. 8.2.12 of ILI9340 manual. */
#define ILI9340_SLEEP_OUT_TIME 120

/** Reset pulse time (ms), ref 15.4 of ILI9340 manual. */
#define ILI9340_RESET_PULSE_TIME 1

/** Reset wait time (ms), ref 15.4 of ILI9340 manual. */
#define ILI9340_RESET_WAIT_TIME 5

/** X resolution (pixels). */
#define ILI9340_X_RES 240U

/** Y resolution (pixels). */
#define ILI9340_Y_RES 320U

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9340_H_ */
