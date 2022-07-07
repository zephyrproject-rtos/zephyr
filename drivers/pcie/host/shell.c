/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/pcie/pcie.h>

#ifdef CONFIG_PCIE_MSI
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/drivers/pcie/cap.h>
#endif

static void show_msi(const struct shell *shell, pcie_bdf_t bdf)
{
#ifdef CONFIG_PCIE_MSI
	uint32_t msi;
	uint32_t data;

	msi = pcie_get_cap(bdf, PCI_CAP_ID_MSI);

	if (msi) {
		data = pcie_conf_read(bdf, msi + PCIE_MSI_MCR);
		shell_fprintf(shell, SHELL_NORMAL, "    MSI support%s%s\n",
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

		shell_fprintf(shell, SHELL_NORMAL,
			      "    MSI-X support%s table size %d\n",
			      (data & PCIE_MSIX_MCR_EN) ?
			      ", enabled" : ", disabled",
			      table_size);

		offset = pcie_conf_read(bdf, msi + PCIE_MSIX_TR);
		bir = offset & PCIE_MSIX_TR_BIR;
		offset &= PCIE_MSIX_TR_OFFSET;

		shell_fprintf(shell, SHELL_NORMAL,
			      "\tTable offset 0x%x BAR %d\n",
			      offset, bir);

		offset = pcie_conf_read(bdf, msi + PCIE_MSIX_PBA);
		bir = offset & PCIE_MSIX_PBA_BIR;
		offset &= PCIE_MSIX_PBA_OFFSET;

		shell_fprintf(shell, SHELL_NORMAL,
			      "\tPBA offset 0x%x BAR %d\n",
			      offset, bir);
	}
#endif
}

static void show_bars(const struct shell *shell, pcie_bdf_t bdf)
{
	uint32_t data;
	int bar;

	for (bar = PCIE_CONF_BAR0; bar <= PCIE_CONF_BAR5; ++bar) {
		data = pcie_conf_read(bdf, bar);
		if (data == PCIE_CONF_BAR_NONE) {
			continue;
		}

		shell_fprintf(shell, SHELL_NORMAL, "    bar %d: %s%s",
			      bar - PCIE_CONF_BAR0,
			      PCIE_CONF_BAR_IO(data) ? "I/O" : "MEM",
			      PCIE_CONF_BAR_64(data) ? ", 64-bit" : "");

		shell_fprintf(shell, SHELL_NORMAL, " addr 0x");

		if (PCIE_CONF_BAR_64(data)) {
			++bar;
			shell_fprintf(shell, SHELL_NORMAL, "%08x",
				      pcie_conf_read(bdf, bar));
		}

		shell_fprintf(shell, SHELL_NORMAL, "%08x\n",
			      (uint32_t)PCIE_CONF_BAR_ADDR(data));
	}
}

static void show(const struct shell *shell, pcie_bdf_t bdf)
{
	uint32_t data;
	unsigned int irq;

	data = pcie_conf_read(bdf, PCIE_CONF_ID);

	if (data == PCIE_ID_NONE) {
		return;
	}

	shell_fprintf(shell, SHELL_NORMAL, "%d:%x.%d ID %x:%x ",
		     PCIE_BDF_TO_BUS(bdf),
		     PCIE_BDF_TO_DEV(bdf),
		     PCIE_BDF_TO_FUNC(bdf),
		     PCIE_ID_TO_VEND(data),
		     PCIE_ID_TO_DEV(data));

	data = pcie_conf_read(bdf, PCIE_CONF_CLASSREV);
	shell_fprintf(shell, SHELL_NORMAL,
		     "class %x subclass %x prog i/f %x rev %x",
		     PCIE_CONF_CLASSREV_CLASS(data),
		     PCIE_CONF_CLASSREV_SUBCLASS(data),
		     PCIE_CONF_CLASSREV_PROGIF(data),
		     PCIE_CONF_CLASSREV_REV(data));

	data = pcie_conf_read(bdf, PCIE_CONF_TYPE);

	if (PCIE_CONF_TYPE_BRIDGE(data)) {
		shell_fprintf(shell, SHELL_NORMAL, " [bridge]\n");
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "\n");
		show_bars(shell, bdf);
		show_msi(shell, bdf);
		irq = pcie_get_irq(bdf);
		if (irq != PCIE_CONF_INTR_IRQ_NONE) {
			shell_fprintf(shell, SHELL_NORMAL,
				      "    wired interrupt on IRQ %d\n", irq);
		}
	}
}

static int cmd_pcie_ls(const struct shell *shell, size_t argc, char **argv)
{
	int bus;
	int dev;
	int func;

	for (bus = 0; bus <= PCIE_MAX_BUS; ++bus) {
		for (dev = 0; dev <= PCIE_MAX_DEV; ++dev) {
			for (func = 0; func <= PCIE_MAX_FUNC; ++func) {
				show(shell, PCIE_BDF(bus, dev, func));
			}
		}
	}

	return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(sub_pcie_cmds,
			       SHELL_CMD(ls, NULL,
					 "List PCIE devices", cmd_pcie_ls),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
		);


SHELL_CMD_REGISTER(pcie, &sub_pcie_cmds, "PCI(e) device information", cmd_pcie_ls);
