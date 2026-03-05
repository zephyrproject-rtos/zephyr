/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9488.h"
#include "display_ili9xxx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili9488, CONFIG_DISPLAY_LOG_LEVEL);

int ili9488_regs_init(const struct device *dev)
{
	const struct ili9xxx_config *config = dev->config;
	const struct ili9488_regs *regs = config->regs;
	uint8_t *buf_nocache = config->nocache_buf;
	int r;

	LOG_HEXDUMP_DBG(regs->frmctr1, ILI9488_FRMCTR1_LEN, "FRMCTR1");
	memcpy(buf_nocache, regs->frmctr1, ILI9488_FRMCTR1_LEN);
	r = ili9xxx_transmit(dev, ILI9488_FRMCTR1, buf_nocache, ILI9488_FRMCTR1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->disctrl, ILI9488_DISCTRL_LEN, "DISCTRL");
	memcpy(buf_nocache, regs->disctrl, ILI9488_DISCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9488_DISCTRL, buf_nocache, ILI9488_DISCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl1, ILI9488_PWCTRL1_LEN, "PWCTRL1");
	memcpy(buf_nocache, regs->pwctrl1, ILI9488_PWCTRL1_LEN);
	r = ili9xxx_transmit(dev, ILI9488_PWCTRL1, buf_nocache, ILI9488_PWCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl2, ILI9488_PWCTRL2_LEN, "PWCTRL2");
	memcpy(buf_nocache, regs->pwctrl2, ILI9488_PWCTRL2_LEN);
	r = ili9xxx_transmit(dev, ILI9488_PWCTRL2, buf_nocache, ILI9488_PWCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl, ILI9488_VMCTRL_LEN, "VMCTRL");
	memcpy(buf_nocache, regs->vmctrl, ILI9488_VMCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9488_VMCTRL, buf_nocache, ILI9488_VMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pgamctrl, ILI9488_PGAMCTRL_LEN, "PGAMCTRL");
	memcpy(buf_nocache, regs->pgamctrl, ILI9488_PGAMCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9488_PGAMCTRL, buf_nocache, ILI9488_PGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ngamctrl, ILI9488_NGAMCTRL_LEN, "NGAMCTRL");
	memcpy(buf_nocache, regs->ngamctrl, ILI9488_NGAMCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9488_NGAMCTRL, buf_nocache, ILI9488_NGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	return 0;
}
