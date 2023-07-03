/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 * Copyright (c) 2022 Konstantinos Papadopoulos <kostas.papadopulos@gmail.com>
 * Copyright (c) 2022 Mohamed ElShahawi <ExtremeGTX@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9342c.h"
#include "display_ili9xxx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili9342c, CONFIG_DISPLAY_LOG_LEVEL);

int ili9342c_regs_init(const struct device *dev)
{
	const struct ili9xxx_config *config = dev->config;
	const struct ili9342c_regs *regs = config->regs;
	int r;

	/*  some commands require that SETEXTC be set first before it becomes enabled. */
	LOG_HEXDUMP_DBG(regs->setextc, ILI9342C_SETEXTC_LEN, "SETEXTC");
	r = ili9xxx_transmit(dev, ILI9342C_SETEXTC, regs->setextc,
			     ILI9342C_SETEXTC_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->gamset, ILI9342C_GAMSET_LEN, "GAMSET");
	r = ili9xxx_transmit(dev, ILI9342C_GAMSET, regs->gamset,
			     ILI9342C_GAMSET_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifmode, ILI9342C_IFMODE_LEN, "IFMODE");
	r = ili9xxx_transmit(dev, ILI9342C_IFMODE, regs->ifmode,
			     ILI9342C_IFMODE_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->frmctr1, ILI9342C_FRMCTR1_LEN, "FRMCTR1");
	r = ili9xxx_transmit(dev, ILI9342C_FRMCTR1, regs->frmctr1,
			     ILI9342C_FRMCTR1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->invtr, ILI9342C_INVTR_LEN, "INVTR");
	r = ili9xxx_transmit(dev, ILI9342C_INVTR, regs->invtr,
			     ILI9342C_INVTR_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->disctrl, ILI9342C_DISCTRL_LEN, "DISCTRL");
	r = ili9xxx_transmit(dev, ILI9342C_DISCTRL, regs->disctrl,
			     ILI9342C_DISCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->etmod, ILI9342C_ETMOD_LEN, "ETMOD");
	r = ili9xxx_transmit(dev, ILI9342C_ETMOD, regs->etmod,
			     ILI9342C_ETMOD_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl1, ILI9342C_PWCTRL1_LEN, "PWCTRL1");
	r = ili9xxx_transmit(dev, ILI9342C_PWCTRL1, regs->pwctrl1,
			     ILI9342C_PWCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl2, ILI9342C_PWCTRL2_LEN, "PWCTRL2");
	r = ili9xxx_transmit(dev, ILI9342C_PWCTRL2, regs->pwctrl2,
			     ILI9342C_PWCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl3, ILI9342C_PWCTRL3_LEN, "PWCTRL3");
	r = ili9xxx_transmit(dev, ILI9342C_PWCTRL3, regs->pwctrl3,
			     ILI9342C_PWCTRL3_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl1, ILI9342C_VMCTRL1_LEN, "VMCTRL1");
	r = ili9xxx_transmit(dev, ILI9342C_VMCTRL1, regs->vmctrl1,
			     ILI9342C_VMCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pgamctrl, ILI9342C_PGAMCTRL_LEN, "PGAMCTRL");
	r = ili9xxx_transmit(dev, ILI9342C_PGAMCTRL, regs->pgamctrl,
			     ILI9342C_PGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ngamctrl, ILI9342C_NGAMCTRL_LEN, "NGAMCTRL");
	r = ili9xxx_transmit(dev, ILI9342C_NGAMCTRL, regs->ngamctrl,
			     ILI9342C_NGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifctl, ILI9342C_IFCTL_LEN, "IFCTL");
	r = ili9xxx_transmit(dev, ILI9342C_IFCTL, regs->ifctl,
			     ILI9342C_IFCTL_LEN);
	if (r < 0) {
		return r;
	}

	return 0;
}
