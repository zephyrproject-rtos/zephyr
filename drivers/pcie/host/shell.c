/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/pcie/pcie.h>

#ifdef CONFIG_PCIE_MSI
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/drivers/pcie/cap.h>
#endif

static void show_msi(const struct shell *sh, pcie_bdf_t bdf)
{
#ifdef CONFIG_PCIE_MSI
	uint32_t msi;
	uint32_t data;

	msi = pcie_get_cap(bdf, PCI_CAP_ID_MSI);

	if (msi) {
		data = pcie_conf_read(bdf, msi + PCIE_MSI_MCR);
		shell_fprintf(sh, SHELL_NORMAL, "    MSI support%s%s\n",
			      (data & PCIE_MSI_MCR_64) ? ", 64-bit" : "",
			      (data & PCIE_MSI_MCR_EN) ?
			      ", enabled" : ", disabled");
	}

	msi = pcie_get_cap(bdf, PCI_CAP_ID_MSIX);

	if (msi) {
		uint32_t offset, table_size;
		uint8_t bir;

		data = pcie_conf_read(bdf, msi + PCIE_MSIX_MCR);

		table_size = ((data & PCIE_MSIX_MCR_TSIZE) >>
			      PCIE_MSIX_MCR_TSIZE_SHIFT) + 1;

		shell_fprintf(sh, SHELL_NORMAL,
			      "    MSI-X support%s table size %d\n",
			      (data & PCIE_MSIX_MCR_EN) ?
			      ", enabled" : ", disabled",
			      table_size);

		offset = pcie_conf_read(bdf, msi + PCIE_MSIX_TR);
		bir = offset & PCIE_MSIX_TR_BIR;
		offset &= PCIE_MSIX_TR_OFFSET;

		shell_fprintf(sh, SHELL_NORMAL,
			      "\tTable offset 0x%x BAR %d\n",
			      offset, bir);

		offset = pcie_conf_read(bdf, msi + PCIE_MSIX_PBA);
		bir = offset & PCIE_MSIX_PBA_BIR;
		offset &= PCIE_MSIX_PBA_OFFSET;

		shell_fprintf(sh, SHELL_NORMAL,
			      "\tPBA offset 0x%x BAR %d\n",
			      offset, bir);
	}
#endif
}

static void show_bars(const struct shell *sh, pcie_bdf_t bdf)
{
	uint32_t data;
	int bar;

	for (bar = PCIE_CONF_BAR0; bar <= PCIE_CONF_BAR5; ++bar) {
		data = pcie_conf_read(bdf, bar);
		if (data == PCIE_CONF_BAR_NONE) {
			continue;
		}

		shell_fprintf(sh, SHELL_NORMAL, "    bar %d: %s%s",
			      bar - PCIE_CONF_BAR0,
			      PCIE_CONF_BAR_IO(data) ? "I/O" : "MEM",
			      PCIE_CONF_BAR_64(data) ? ", 64-bit" : "");

		shell_fprintf(sh, SHELL_NORMAL, " addr 0x");

		if (PCIE_CONF_BAR_64(data)) {
			++bar;
			shell_fprintf(sh, SHELL_NORMAL, "%08x",
				      pcie_conf_read(bdf, bar));
		}

		shell_fprintf(sh, SHELL_NORMAL, "%08x\n",
			      (uint32_t)PCIE_CONF_BAR_ADDR(data));
	}
}

static void pcie_dump(const struct shell *sh, pcie_bdf_t bdf)
{
	for (int i = 0; i < 16; i++) {
		uint32_t val = pcie_conf_read(bdf, i);

		for (int j = 0; j < 4; j++) {
			shell_fprintf(sh, SHELL_NORMAL, "%02x ",
				      (uint8_t)val);
			val >>= 8;
		}

		if (((i + 1) % 4) == 0) {
			shell_fprintf(sh, SHELL_NORMAL, "\n");
		}
	}
}

static pcie_bdf_t get_bdf(char *str)
{
	int bus, dev, func;
	char *tok, *state;

	tok = strtok_r(str, ":", &state);
	if (tok == NULL) {
		return PCIE_BDF_NONE;
	}

	bus = strtoul(tok, NULL, 16);

	tok = strtok_r(NULL, ".", &state);
	if (tok == NULL) {
		return PCIE_BDF_NONE;
	}

	dev = strtoul(tok, NULL, 16);

	tok = strtok_r(NULL, ".", &state);
	if (tok == NULL) {
		return PCIE_BDF_NONE;
	}

	func = strtoul(tok, NULL, 16);

	return PCIE_BDF(bus, dev, func);
}

static void show(const struct shell *sh, pcie_bdf_t bdf, bool dump)
{
	uint32_t data;
	unsigned int irq;

	data = pcie_conf_read(bdf, PCIE_CONF_ID);

	if (data == PCIE_ID_NONE) {
		return;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%d:%x.%d ID %x:%x ",
		     PCIE_BDF_TO_BUS(bdf),
		     PCIE_BDF_TO_DEV(bdf),
		     PCIE_BDF_TO_FUNC(bdf),
		     PCIE_ID_TO_VEND(data),
		     PCIE_ID_TO_DEV(data));

	data = pcie_conf_read(bdf, PCIE_CONF_CLASSREV);
	shell_fprintf(sh, SHELL_NORMAL,
		     "class %x subclass %x prog i/f %x rev %x",
		     PCIE_CONF_CLASSREV_CLASS(data),
		     PCIE_CONF_CLASSREV_SUBCLASS(data),
		     PCIE_CONF_CLASSREV_PROGIF(data),
		     PCIE_CONF_CLASSREV_REV(data));

	data = pcie_conf_read(bdf, PCIE_CONF_TYPE);

	if (PCIE_CONF_TYPE_BRIDGE(data)) {
		shell_fprintf(sh, SHELL_NORMAL, " [bridge]\n");
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "\n");
		show_bars(sh, bdf);
		show_msi(sh, bdf);
		irq = pcie_get_irq(bdf);
		if (irq != PCIE_CONF_INTR_IRQ_NONE) {
			shell_fprintf(sh, SHELL_NORMAL,
				      "    wired interrupt on IRQ %d\n", irq);
		}
	}

	if (dump) {
		pcie_dump(sh, bdf);
	}
}

static int cmd_pcie_ls(const struct shell *sh, size_t argc, char **argv)
{
	pcie_bdf_t bdf = PCIE_BDF_NONE;
	bool dump = false;
	int bus;
	int dev;
	int func;

	for (int i = 1; i < argc; i++) {
		/* Check dump argument */
		if (strncmp(argv[i], "dump", 4) == 0) {
			dump = true;
			continue;
		}

		/* Check BDF string of PCI device */
		if (bdf == PCIE_BDF_NONE) {
			bdf = get_bdf(argv[i]);
		}

		if (bdf == PCIE_BDF_NONE) {
			shell_error(sh, "Unknown parameter: %s", argv[i]);
			return -EINVAL;
		}
	}

	/* Show only specified device */
	if (bdf != PCIE_BDF_NONE) {
		show(sh, bdf, dump);
		return 0;
	}

	for (bus = 0; bus <= PCIE_MAX_BUS; ++bus) {
		for (dev = 0; dev <= PCIE_MAX_DEV; ++dev) {
			for (func = 0; func <= PCIE_MAX_FUNC; ++func) {
				show(sh, PCIE_BDF(bus, dev, func), dump);
			}
		}
	}

	return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(sub_pcie_cmds,
	SHELL_CMD_ARG(ls, NULL,
		      "List PCIE devices\n"
		      "Usage: ls [bus:device:function] [dump]",
		      cmd_pcie_ls, 1, 2),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(pcie, &sub_pcie_cmds, "PCI(e) device information", cmd_pcie_ls);
