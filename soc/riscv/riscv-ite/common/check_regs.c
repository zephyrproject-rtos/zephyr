/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <soc.h>

/* GCTRL register structure check */
IT8XXX2_REG_SIZE_CHECK(gctrl_it8xxx2_regs, 0x88);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_RSTS, 0x06);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_SPCTRL4, 0x1c);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_RSTC5, 0x21);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_MCCR2, 0x44);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_ECHIPID2, 0x86);
