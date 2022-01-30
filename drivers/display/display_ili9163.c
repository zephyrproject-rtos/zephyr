/*
 * Copyright (c) 2022 Tavish Naruka <tavishnaruka@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9xxx.h"
#include "display_ili9163.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(display_ili9163, CONFIG_DISPLAY_LOG_LEVEL);

int ili9163_regs_init(const struct device *dev)
{
	const struct ili9xxx_config *config = dev->config;
	const struct ili9163_regs *regs = config->regs;

	int r;


	LOG_HEXDUMP_DBG(regs->gamset, ILI9163_GAMSET_LEN, "GAMSET");
	r = ili9xxx_transmit(dev, ILI9163_GAMSET, regs->gamset,
			     ILI9163_GAMSET_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->gamadj, ILI9163_GAMADJ_LEN, "GAMADJ");
	r = ili9xxx_transmit(dev, ILI9163_GAMADJ, regs->gamadj,
			     ILI9163_GAMADJ_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pgamctrl, ILI9163_PGAMCTRL_LEN, "PGAMCTRL");
	r = ili9xxx_transmit(dev, ILI9163_PGAMCTRL, regs->pgamctrl,
			     ILI9163_PGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ngamctrl, ILI9163_NGAMCTRL_LEN, "NGAMCTRL");
	r = ili9xxx_transmit(dev, ILI9163_NGAMCTRL, regs->ngamctrl,
			     ILI9163_NGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->frmctr1, ILI9163_FRMCTR1_LEN, "FRMCTR1");
	r = ili9xxx_transmit(dev, ILI9163_FRMCTR1, regs->frmctr1,
			     ILI9163_FRMCTR1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->dispinv, ILI9163_DISPINV_LEN, "DISP_INV");
	r = ili9xxx_transmit(dev, ILI9163_DISPINV, regs->dispinv,
			     ILI9163_DISPINV_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl1, ILI9163_PWCTRL1_LEN, "PWCTRL1");
	r = ili9xxx_transmit(dev, ILI9163_PWCTRL1, regs->pwctrl1,
			     ILI9163_PWCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl2, ILI9163_PWCTRL2_LEN, "PWCTRL2");
	r = ili9xxx_transmit(dev, ILI9163_PWCTRL2, regs->pwctrl2,
			     ILI9163_PWCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl1, ILI9163_VMCTRL1_LEN, "VMCTRL1");
	r = ili9xxx_transmit(dev, ILI9163_VMCTRL1, regs->vmctrl1,
			     ILI9163_VMCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl2, ILI9163_VMCTRL2_LEN, "VMCTRL2");
	r = ili9xxx_transmit(dev, ILI9163_VMCTRL2, regs->vmctrl2,
			     ILI9163_VMCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	return 0;
}
