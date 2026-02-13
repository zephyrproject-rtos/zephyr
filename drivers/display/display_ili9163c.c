/*
 * Copyright (c) 2025 CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9163c.h"
#include "display_ili9xxx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili9163c, CONFIG_DISPLAY_LOG_LEVEL);

int ili9163c_regs_init(const struct device *dev)
{
	const struct ili9xxx_config *config = dev->config;
	const struct ili9163c_regs *regs = config->regs;
	int r;

	LOG_HEXDUMP_DBG(regs->gamset, ILI9163C_GAMSET_LEN, "GAMSET");
	r = ili9xxx_transmit(dev, ILI9163C_GAMSET, regs->gamset, ILI9163C_GAMSET_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->gamarsel, ILI9163C_GAMARSEL_LEN, "GAMARSEL");
	r = ili9xxx_transmit(dev, ILI9163C_GAMARSEL, regs->gamarsel, ILI9163C_GAMARSEL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pgamctrl, ILI9163C_PGAMCTRL_LEN, "PGAMCTRL");
	r = ili9xxx_transmit(dev, ILI9163C_PGAMCTRL, regs->pgamctrl, ILI9163C_PGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ngamctrl, ILI9163C_NGAMCTRL_LEN, "NGAMCTRL");
	r = ili9xxx_transmit(dev, ILI9163C_NGAMCTRL, regs->ngamctrl, ILI9163C_NGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->frmctr1, ILI9163C_FRMCTR1_LEN, "FRMCTR1");
	r = ili9xxx_transmit(dev, ILI9163C_FRMCTR1, regs->frmctr1, ILI9163C_FRMCTR1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl1, ILI9163C_PWCTRL1_LEN, "PWCTRL1");
	r = ili9xxx_transmit(dev, ILI9163C_PWCTRL1, regs->pwctrl1, ILI9163C_PWCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl2, ILI9163C_PWCTRL2_LEN, "PWCTRL2");
	r = ili9xxx_transmit(dev, ILI9163C_PWCTRL2, regs->pwctrl2, ILI9163C_PWCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl3, ILI9163C_PWCTRL3_LEN, "PWCTRL3");
	r = ili9xxx_transmit(dev, ILI9163C_PWCTRL3, regs->pwctrl3, ILI9163C_PWCTRL3_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl4, ILI9163C_PWCTRL4_LEN, "PWCTRL4");
	r = ili9xxx_transmit(dev, ILI9163C_PWCTRL4, regs->pwctrl4, ILI9163C_PWCTRL4_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl5, ILI9163C_PWCTRL5_LEN, "PWCTRL5");
	r = ili9xxx_transmit(dev, ILI9163C_PWCTRL5, regs->pwctrl5, ILI9163C_PWCTRL5_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl1, ILI9163C_VMCTRL1_LEN, "VMCTRL1");
	r = ili9xxx_transmit(dev, ILI9163C_VMCTRL1, regs->vmctrl1, ILI9163C_VMCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl2, ILI9163C_VMCTRL2_LEN, "VMCTRL2");
	r = ili9xxx_transmit(dev, ILI9163C_VMCTRL2, regs->vmctrl2, ILI9163C_VMCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	return 0;
}
