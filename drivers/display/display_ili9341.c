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
	uint8_t *buf_nocache = config->nocache_buf;
	int r;

	LOG_HEXDUMP_DBG(regs->pwseqctrl, ILI9341_PWSEQCTRL_LEN, "PWSEQCTRL");
	memcpy(buf_nocache, regs->pwseqctrl, ILI9341_PWSEQCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9341_PWSEQCTRL, buf_nocache, ILI9341_PWSEQCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->timctrla, ILI9341_TIMCTRLA_LEN, "TIMCTRLA");
	memcpy(buf_nocache, regs->timctrla, ILI9341_TIMCTRLA_LEN);
	r = ili9xxx_transmit(dev, ILI9341_TIMCTRLA, buf_nocache, ILI9341_TIMCTRLA_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->timctrlb, ILI9341_TIMCTRLB_LEN, "TIMCTRLB");
	memcpy(buf_nocache, regs->timctrlb, ILI9341_TIMCTRLB_LEN);
	r = ili9xxx_transmit(dev, ILI9341_TIMCTRLB, buf_nocache, ILI9341_TIMCTRLB_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pumpratioctrl, ILI9341_PUMPRATIOCTRL_LEN, "PUMPRATIOCTRL");
	memcpy(buf_nocache, regs->pumpratioctrl, ILI9341_PUMPRATIOCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9341_PUMPRATIOCTRL, buf_nocache, ILI9341_PUMPRATIOCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrla, ILI9341_PWCTRLA_LEN, "PWCTRLA");
	memcpy(buf_nocache, regs->pwctrla, ILI9341_PWCTRLA_LEN);
	r = ili9xxx_transmit(dev, ILI9341_PWCTRLA, buf_nocache, ILI9341_PWCTRLA_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrlb, ILI9341_PWCTRLB_LEN, "PWCTRLB");
	memcpy(buf_nocache, regs->pwctrlb, ILI9341_PWCTRLB_LEN);
	r = ili9xxx_transmit(dev, ILI9341_PWCTRLB, buf_nocache, ILI9341_PWCTRLB_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->gamset, ILI9341_GAMSET_LEN, "GAMSET");
	memcpy(buf_nocache, regs->gamset, ILI9341_GAMSET_LEN);
	r = ili9xxx_transmit(dev, ILI9341_GAMSET, buf_nocache, ILI9341_GAMSET_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->frmctr1, ILI9341_FRMCTR1_LEN, "FRMCTR1");
	memcpy(buf_nocache, regs->frmctr1, ILI9341_FRMCTR1_LEN);
	r = ili9xxx_transmit(dev, ILI9341_FRMCTR1, buf_nocache, ILI9341_FRMCTR1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->disctrl, ILI9341_DISCTRL_LEN, "DISCTRL");
	memcpy(buf_nocache, regs->disctrl, ILI9341_DISCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9341_DISCTRL, buf_nocache, ILI9341_DISCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl1, ILI9341_PWCTRL1_LEN, "PWCTRL1");
	memcpy(buf_nocache, regs->pwctrl1, ILI9341_PWCTRL1_LEN);
	r = ili9xxx_transmit(dev, ILI9341_PWCTRL1, buf_nocache, ILI9341_PWCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pwctrl2, ILI9341_PWCTRL2_LEN, "PWCTRL2");
	memcpy(buf_nocache, regs->pwctrl2, ILI9341_PWCTRL2_LEN);
	r = ili9xxx_transmit(dev, ILI9341_PWCTRL2, buf_nocache, ILI9341_PWCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl1, ILI9341_VMCTRL1_LEN, "VMCTRL1");
	memcpy(buf_nocache, regs->vmctrl1, ILI9341_VMCTRL1_LEN);
	r = ili9xxx_transmit(dev, ILI9341_VMCTRL1, buf_nocache, ILI9341_VMCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->vmctrl2, ILI9341_VMCTRL2_LEN, "VMCTRL2");
	memcpy(buf_nocache, regs->vmctrl2, ILI9341_VMCTRL2_LEN);
	r = ili9xxx_transmit(dev, ILI9341_VMCTRL2, buf_nocache, ILI9341_VMCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->pgamctrl, ILI9341_PGAMCTRL_LEN, "PGAMCTRL");
	memcpy(buf_nocache, regs->pgamctrl, ILI9341_PGAMCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9341_PGAMCTRL, buf_nocache, ILI9341_PGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ngamctrl, ILI9341_NGAMCTRL_LEN, "NGAMCTRL");
	memcpy(buf_nocache, regs->ngamctrl, ILI9341_NGAMCTRL_LEN);
	r = ili9xxx_transmit(dev, ILI9341_NGAMCTRL, buf_nocache, ILI9341_NGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->enable3g, ILI9341_ENABLE3G_LEN, "ENABLE3G");
	memcpy(buf_nocache, regs->enable3g, ILI9341_ENABLE3G_LEN);
	r = ili9xxx_transmit(dev, ILI9341_ENABLE3G, buf_nocache, ILI9341_ENABLE3G_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifmode, ILI9341_IFMODE_LEN, "IFMODE");
	memcpy(buf_nocache, regs->ifmode, ILI9341_IFMODE_LEN);
	r = ili9xxx_transmit(dev, ILI9341_IFMODE, buf_nocache, ILI9341_IFMODE_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->ifctl, ILI9341_IFCTL_LEN, "IFCTL");
	memcpy(buf_nocache, regs->ifctl, ILI9341_IFCTL_LEN);
	r = ili9xxx_transmit(dev, ILI9341_IFCTL, buf_nocache, ILI9341_IFCTL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(regs->etmod, ILI9341_ETMOD_LEN, "ETMOD");
	memcpy(buf_nocache, regs->etmod, ILI9341_ETMOD_LEN);
	r = ili9xxx_transmit(dev, ILI9341_ETMOD, buf_nocache, ILI9341_ETMOD_LEN);
	if (r < 0) {
		return r;
	}

	return 0;
}
