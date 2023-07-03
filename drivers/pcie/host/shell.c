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
#endif

#include <zephyr/drivers/pcie/cap.h>

#include <zephyr/drivers/pcie/vc.h>
#include "vc.h"

struct pcie_cap_id_to_str {
	uint32_t id;
	char *str;
};

static struct pcie_cap_id_to_str pcie_cap_list[] = {
	{ PCI_CAP_ID_PM,     "Power Management" },
	{ PCI_CAP_ID_AGP,    "Accelerated Graphics Port" },
	{ PCI_CAP_ID_VPD,    "Vital Product Data" },
	{ PCI_CAP_ID_SLOTID, "Slot Identification" },
	{ PCI_CAP_ID_MSI,    "Message Signalled Interrupts" },
	{ PCI_CAP_ID_CHSWP,  "CompactPCI HotSwap" },
	{ PCI_CAP_ID_PCIX,   "PCI-X" },
	{ PCI_CAP_ID_HT,     "HyperTransport" },
	{ PCI_CAP_ID_VNDR,   "Vendor-Specific" },
	{ PCI_CAP_ID_DBG,    "Debug port" },
	{ PCI_CAP_ID_CCRC,   "CompactPCI Central Resource Control" },
	{ PCI_CAP_ID_SHPC,   "PCI Standard Hot-Plug Controller" },
	{ PCI_CAP_ID_SSVID,  "Bridge subsystem vendor/device ID" },
	{ PCI_CAP_ID_AGP3,   "AGP 8x" },
	{ PCI_CAP_ID_SECDEV, "Secure Device" },
	{ PCI_CAP_ID_EXP,    "PCI Express" },
	{ PCI_CAP_ID_MSIX,   "MSI-X" },
	{ PCI_CAP_ID_SATA,   "Serial ATA Data/Index Configuration" },
	{ PCI_CAP_ID_AF,     "PCI Advanced Features" },
	{ PCI_CAP_ID_EA,     "PCI Enhanced Allocation" },
	{ PCI_CAP_ID_FPB,    "Flattening Portal Bridge" },
	{ PCI_CAP_ID_NULL,   NULL },
};

static struct pcie_cap_id_to_str pcie_ext_cap_list[] = {
	{ PCIE_EXT_CAP_ID_ERR,     "Advanced Error Reporting" },
	{ PCIE_EXT_CAP_ID_VC,      "Virtual Channel when no MFVC" },
	{ PCIE_EXT_CAP_ID_DSN,     "Device Serial Number" },
	{ PCIE_EXT_CAP_ID_PWR,     "Power Budgeting" },
	{ PCIE_EXT_CAP_ID_RCLD,    "Root Complex Link Declaration" },
	{ PCIE_EXT_CAP_ID_RCILC,   "Root Complex Internal Link Control" },
	{ PCIE_EXT_CAP_ID_RCEC,    "Root Complex Event Collector Endpoint Association" },
	{ PCIE_EXT_CAP_ID_MFVC,    "Multi-Function VC Capability" },
	{ PCIE_EXT_CAP_ID_MFVC_VC, "Virtual Channel used with MFVC" },
	{ PCIE_EXT_CAP_ID_RCRB,    "Root Complex Register Block" },
	{ PCIE_EXT_CAP_ID_VNDR,    "Vendor-Specific Extended Capability" },
	{ PCIE_EXT_CAP_ID_CAC,     "Config Access Correlation - obsolete" },
	{ PCIE_EXT_CAP_ID_ACS,     "Access Control Services" },
	{ PCIE_EXT_CAP_ID_ARI,     "Alternate Routing-ID Interpretation" },
	{ PCIE_EXT_CAP_ID_ATS,     "Address Translation Services" },
	{ PCIE_EXT_CAP_ID_SRIOV,   "Single Root I/O Virtualization" },
	{ PCIE_EXT_CAP_ID_MRIOV,   "Multi Root I/O Virtualization" },
	{ PCIE_EXT_CAP_ID_MCAST,   "Multicast" },
	{ PCIE_EXT_CAP_ID_PRI,     "Page Request Interface" },
	{ PCIE_EXT_CAP_ID_AMD_XXX, "Reserved for AMD" },
	{ PCIE_EXT_CAP_ID_REBAR,   "Resizable BAR" },
	{ PCIE_EXT_CAP_ID_DPA,     "Dynamic Power Allocation" },
	{ PCIE_EXT_CAP_ID_TPH,     "TPH Requester" },
	{ PCIE_EXT_CAP_ID_LTR,     "Latency Tolerance Reporting" },
	{ PCIE_EXT_CAP_ID_SECPCI,  "Secondary PCIe Capability" },
	{ PCIE_EXT_CAP_ID_PMUX,    "Protocol Multiplexing" },
	{ PCIE_EXT_CAP_ID_PASID,   "Process Address Space ID" },
	{ PCIE_EXT_CAP_ID_DPC,     "Downstream Port Containment" },
	{ PCIE_EXT_CAP_ID_L1SS,    "L1 PM Substates" },
	{ PCIE_EXT_CAP_ID_PTM,     "Precision Time Measurement" },
	{ PCIE_EXT_CAP_ID_DVSEC,   "Designated Vendor-Specific Extended Capability" },
	{ PCIE_EXT_CAP_ID_DLF,     "Data Link Feature" },
	{ PCIE_EXT_CAP_ID_PL_16GT, "Physical Layer 16.0 GT/s" },
	{ PCIE_EXT_CAP_ID_LMR,     "Lane Margining at the Receiver" },
	{ PCIE_EXT_CAP_ID_HID,     "Hierarchy ID" },
	{ PCIE_EXT_CAP_ID_NPEM,    "Native PCIe Enclosure Management" },
	{ PCIE_EXT_CAP_ID_PL_32GT, "Physical Layer 32.0 GT/s" },
	{ PCIE_EXT_CAP_ID_AP,      "Alternate Protocol" },
	{ PCIE_EXT_CAP_ID_SFI,     "System Firmware Intermediary" },
	{ PCIE_EXT_CAP_ID_NULL,    NULL },
};

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

static void show_capabilities(const struct shell *sh, pcie_bdf_t bdf)
{
	struct pcie_cap_id_to_str *cap_id2str;
	uint32_t base;

	shell_fprintf(sh, SHELL_NORMAL, "    PCI capabilities:\n");

	cap_id2str = pcie_cap_list;
	while (cap_id2str->str != NULL) {
		base = pcie_get_cap(bdf, cap_id2str->id);
		if (base != 0) {
			shell_fprintf(sh, SHELL_NORMAL,
				      "        %s\n", cap_id2str->str);
		}

		cap_id2str++;
	}

	shell_fprintf(sh, SHELL_NORMAL, "    PCIe capabilities:\n");

	cap_id2str = pcie_ext_cap_list;
	while (cap_id2str->str != NULL) {
		base = pcie_get_ext_cap(bdf, cap_id2str->id);
		if (base != 0) {
			shell_fprintf(sh, SHELL_NORMAL,
				      "        %s\n", cap_id2str->str);
		}

		cap_id2str++;
	}
}

static void show_vc(const struct shell *sh, pcie_bdf_t bdf)
{
	uint32_t base;
	struct pcie_vc_regs regs;
	struct pcie_vc_resource_regs res_regs[PCIE_VC_MAX_COUNT];
	int idx;

	base = pcie_vc_cap_lookup(bdf, &regs);
	if (base == 0) {
		return;
	}

	shell_fprintf(sh, SHELL_NORMAL,
		      "    VC exposed : VC/LPVC count: %u/%u, "
		      "PAT entry size 0x%x, VCA cap 0x%x, "
		      "VCA table Offset 0x%x\n",
		      regs.cap_reg_1.vc_count + 1,
		      regs.cap_reg_1.lpvc_count,
		      regs.cap_reg_1.pat_entry_size,
		      regs.cap_reg_2.vca_cap,
		      regs.cap_reg_2.vca_table_offset);

	pcie_vc_load_resources_regs(bdf, base, res_regs,
				    regs.cap_reg_1.vc_count + 1);

	for (idx = 0; idx < regs.cap_reg_1.vc_count + 1; idx++) {
		shell_fprintf(sh, SHELL_NORMAL,
			      "        VC %d - PA Cap 0x%x, RST %u,"
			      "Max TS %u PAT offset 0x%x\n",
			      idx, res_regs[idx].cap_reg.pa_cap,
			      res_regs[idx].cap_reg.rst,
			      res_regs[idx].cap_reg.max_time_slots,
			      res_regs[idx].cap_reg.pa_table_offset);
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

static void show(const struct shell *sh, pcie_bdf_t bdf, bool details, bool dump)
{
	uint32_t data;
	unsigned int irq;

	data = pcie_conf_read(bdf, PCIE_CONF_ID);

	if (!PCIE_ID_IS_VALID(data)) {
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

	if (details) {
		show_capabilities(sh, bdf);
		show_vc(sh, bdf);
	}

	if (dump) {
		pcie_dump(sh, bdf);
	}
}

struct scan_cb_data {
	const struct shell *sh;
	bool dump;
};

static bool scan_cb(pcie_bdf_t bdf, pcie_id_t id, void *cb_data)
{
	struct scan_cb_data *data = cb_data;

	show(data->sh, bdf, false, data->dump);

	return true;
}

static int cmd_pcie_ls(const struct shell *sh, size_t argc, char **argv)
{
	pcie_bdf_t bdf = PCIE_BDF_NONE;
	struct scan_cb_data data = {
		.sh = sh,
		.dump = false,
	};
	struct pcie_scan_opt scan_opt = {
		.cb = scan_cb,
		.cb_data = &data,
		.flags = (PCIE_SCAN_RECURSIVE | PCIE_SCAN_CB_ALL),
	};

	for (int i = 1; i < argc; i++) {
		/* Check dump argument */
		if (strncmp(argv[i], "dump", 4) == 0) {
			data.dump = true;
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
		show(sh, bdf, true, data.dump);
		return 0;
	}

	pcie_scan(&scan_opt);

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
