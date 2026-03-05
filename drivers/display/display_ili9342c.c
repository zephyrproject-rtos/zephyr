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
	uint8_t *buf_nocache = config->nocache_buf;
	int r;

	/*  some commands require that SETEXTC be set first before it becomes enabled. */
	LOG_HEXDUMP_DBG(regs->setextc, ILI9342C_SETEXTC_LEN, "SETEXTC");
	memcpy(buf_nocache, regs->setextc, ILI9342C_SETEXTC_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_SETEXTC, buf_nocache, ILI9342C_SETEXTC_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->gamset, ILI9342C_GAMSET_LEN, "GAMSET");
	memcpy(buf_nocache, regs->gamset, ILI9342C_GAMSET_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_GAMSET, buf_nocache, ILI9342C_GAMSET_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifmode, ILI9342C_IFMODE_LEN, "IFMODE");
	memcpy(buf_nocache, regs->ifmode, ILI9342C_IFMODE_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_IFMODE, buf_nocache, ILI9342C_IFMODE_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->frmctr1, ILI9342C_FRMCTR1_LEN, "FRMCTR1");
	memcpy(buf_nocache, regs->frmctr1, ILI9342C_FRMCTR1_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_FRMCTR1, buf_nocache, ILI9342C_FRMCTR1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->invtr, ILI9342C_INVTR_LEN, "INVTR");
	memcpy(buf_nocache, regs->invtr, ILI9342C_INVTR_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_INVTR, buf_nocache, ILI9342C_INVTR_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->disctrl, ILI9342C_DISCTRL_LEN, "DISCTRL");
	memcpy(buf_nocache, regs->disctrl, ILI9342C_DISCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_DISCTRL, buf_nocache, ILI9342C_DISCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->etmod, ILI9342C_ETMOD_LEN, "ETMOD");
	memcpy(buf_nocache, regs->etmod, ILI9342C_ETMOD_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_ETMOD, buf_nocache, ILI9342C_ETMOD_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl1, ILI9342C_PWCTRL1_LEN, "PWCTRL1");
	memcpy(buf_nocache, regs->pwctrl1, ILI9342C_PWCTRL1_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_PWCTRL1, buf_nocache, ILI9342C_PWCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl2, ILI9342C_PWCTRL2_LEN, "PWCTRL2");
	memcpy(buf_nocache, regs->pwctrl2, ILI9342C_PWCTRL2_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_PWCTRL2, buf_nocache, ILI9342C_PWCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl3, ILI9342C_PWCTRL3_LEN, "PWCTRL3");
	memcpy(buf_nocache, regs->pwctrl3, ILI9342C_PWCTRL3_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_PWCTRL3, buf_nocache, ILI9342C_PWCTRL3_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl1, ILI9342C_VMCTRL1_LEN, "VMCTRL1");
	memcpy(buf_nocache, regs->vmctrl1, ILI9342C_VMCTRL1_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_VMCTRL1, buf_nocache, ILI9342C_VMCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pgamctrl, ILI9342C_PGAMCTRL_LEN, "PGAMCTRL");
	memcpy(buf_nocache, regs->pgamctrl, ILI9342C_PGAMCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_PGAMCTRL, buf_nocache, ILI9342C_PGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ngamctrl, ILI9342C_NGAMCTRL_LEN, "NGAMCTRL");
	memcpy(buf_nocache, regs->ngamctrl, ILI9342C_NGAMCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_NGAMCTRL, buf_nocache, ILI9342C_NGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifctl, ILI9342C_IFCTL_LEN, "IFCTL");
	memcpy(buf_nocache, regs->ifctl, ILI9342C_IFCTL_LEN);
	r = ili9xxx_transmit(dev, ILI9342C_IFCTL, buf_nocache, ILI9342C_IFCTL_LEN);
	if (r < 0) {
		return r;
	}

	return 0;
}
