/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 * Copyright (c) 2022 Konstantinos Papadopoulos <kostas.papadopulos@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9341.h"
#include "display_ili9xxx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili9341, CONFIG_DISPLAY_LOG_LEVEL);

int ili9341_regs_init(const struct device *dev)
{
	const struct ili9xxx_config *config = dev->config;
	const struct ili9341_regs *regs = config->regs;

	int r;

	LOG_HEXDUMP_DBG(regs->pwseqctrl, ILI9341_PWSEQCTRL_LEN, "PWSEQCTRL");
	r = ili9xxx_transmit(dev, ILI9341_PWSEQCTRL, regs->pwseqctrl, ILI9341_PWSEQCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->timctrla, ILI9341_TIMCTRLA_LEN, "TIMCTRLA");
	r = ili9xxx_transmit(dev, ILI9341_TIMCTRLA, regs->timctrla, ILI9341_TIMCTRLA_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->timctrlb, ILI9341_TIMCTRLB_LEN, "TIMCTRLB");
	r = ili9xxx_transmit(dev, ILI9341_TIMCTRLB, regs->timctrlb, ILI9341_TIMCTRLB_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pumpratioctrl, ILI9341_PUMPRATIOCTRL_LEN, "PUMPRATIOCTRL");
	r = ili9xxx_transmit(dev, ILI9341_PUMPRATIOCTRL, regs->pumpratioctrl,
			     ILI9341_PUMPRATIOCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrla, ILI9341_PWCTRLA_LEN, "PWCTRLA");
	r = ili9xxx_transmit(dev, ILI9341_PWCTRLA, regs->pwctrla, ILI9341_PWCTRLA_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrlb, ILI9341_PWCTRLB_LEN, "PWCTRLB");
	r = ili9xxx_transmit(dev, ILI9341_PWCTRLB, regs->pwctrlb, ILI9341_PWCTRLB_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->gamset, ILI9341_GAMSET_LEN, "GAMSET");
	r = ili9xxx_transmit(dev, ILI9341_GAMSET, regs->gamset, ILI9341_GAMSET_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->frmctr1, ILI9341_FRMCTR1_LEN, "FRMCTR1");
	r = ili9xxx_transmit(dev, ILI9341_FRMCTR1, regs->frmctr1, ILI9341_FRMCTR1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->disctrl, ILI9341_DISCTRL_LEN, "DISCTRL");
	r = ili9xxx_transmit(dev, ILI9341_DISCTRL, regs->disctrl, ILI9341_DISCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl1, ILI9341_PWCTRL1_LEN, "PWCTRL1");
	r = ili9xxx_transmit(dev, ILI9341_PWCTRL1, regs->pwctrl1, ILI9341_PWCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl2, ILI9341_PWCTRL2_LEN, "PWCTRL2");
	r = ili9xxx_transmit(dev, ILI9341_PWCTRL2, regs->pwctrl2, ILI9341_PWCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl1, ILI9341_VMCTRL1_LEN, "VMCTRL1");
	r = ili9xxx_transmit(dev, ILI9341_VMCTRL1, regs->vmctrl1, ILI9341_VMCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl2, ILI9341_VMCTRL2_LEN, "VMCTRL2");
	r = ili9xxx_transmit(dev, ILI9341_VMCTRL2, regs->vmctrl2, ILI9341_VMCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pgamctrl, ILI9341_PGAMCTRL_LEN, "PGAMCTRL");
	r = ili9xxx_transmit(dev, ILI9341_PGAMCTRL, regs->pgamctrl, ILI9341_PGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ngamctrl, ILI9341_NGAMCTRL_LEN, "NGAMCTRL");
	r = ili9xxx_transmit(dev, ILI9341_NGAMCTRL, regs->ngamctrl, ILI9341_NGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->enable3g, ILI9341_ENABLE3G_LEN, "ENABLE3G");
	r = ili9xxx_transmit(dev, ILI9341_ENABLE3G, regs->enable3g, ILI9341_ENABLE3G_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifmode, ILI9341_IFMODE_LEN, "IFMODE");
	r = ili9xxx_transmit(dev, ILI9341_IFMODE, regs->ifmode, ILI9341_IFMODE_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifctl, ILI9341_IFCTL_LEN, "IFCTL");
	r = ili9xxx_transmit(dev, ILI9341_IFCTL, regs->ifctl, ILI9341_IFCTL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->etmod, ILI9341_ETMOD_LEN, "ETMOD");
	r = ili9xxx_transmit(dev, ILI9341_ETMOD, regs->etmod, ILI9341_ETMOD_LEN);
	if (r < 0) {
		return r;
	}

	return 0;
}
