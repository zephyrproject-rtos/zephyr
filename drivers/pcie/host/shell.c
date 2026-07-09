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

/* PCI Express Extended Capability Constants */
#define PCIE_EXT_CAP_AER_ID 0x0001U

/* AER Register Offsets (Relative to the AER Capability Base Offset) */
#define PCIE_AER_UNCORR_STATUS_REG (0x04U / 4U)
#define PCIE_AER_UNCORR_MASK_REG   (0x08U / 4U)
#define PCIE_AER_CORR_STATUS_REG   (0x10U / 4U)
#define PCIE_AER_CORR_MASK_REG     (0x14U / 4U)
#define PCIE_AER_HEADER_LOG_REG    (0x1CU / 4U)

static const struct pcie_cap_id_to_str pcie_cap_list[] = {
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

static const struct pcie_cap_id_to_str pcie_ext_cap_list[] = {
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

/* Interactive PCIe Configuration Space Write Utility */
static int cmd_pcie_write(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 4) {
		shell_error(sh, "Usage: pcie write <bus:dev.func> "
				"<register_word_offset> <32bit_hex_data>");
		return -EINVAL;
	}

	pcie_bdf_t bdf = get_bdf(argv[1]);

	if (bdf == PCIE_BDF_NONE) {
		shell_error(sh, "Invalid BDF layout format.");
		return -EINVAL;
	}

	int err = 0;
	unsigned int reg = (unsigned int)shell_strtoul(argv[2], 0, &err);

	if (err != 0) {
		shell_error(sh, "Invalid argument: %s", argv[2]);
		return err;
	}

	if (reg >= (4096U / sizeof(uint32_t))) {
		shell_error(sh, "Register word offset out of range: %u", reg);
		return -EINVAL;
	}

	uint32_t data = (uint32_t)shell_strtoul(argv[3], 0, &err);

	if (err != 0) {
		shell_error(sh, "Invalid argument: %s", argv[3]);
		return err;
	}

	uint32_t id = pcie_conf_read(bdf, PCIE_CONF_ID);

	if (id == 0xFFFFFFFFU || !PCIE_ID_IS_VALID(id)) {
		shell_error(sh, "No responsive endpoint found at target BDF.");
		return -ENODEV;
	}

	uint32_t current_val = pcie_conf_read(bdf, reg);

	shell_print(sh, "Writing 0x%08x to %u:%x.%u reg %u (prev: 0x%08x)...", data,
		    PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf), reg,
		    current_val);

	pcie_conf_write(bdf, reg, data);

	uint32_t read_back = pcie_conf_read(bdf, reg);

	shell_print(sh, "Read-back Verification Value: 0x%08X", read_back);

	return 0;
}

#define MAX_MASKED_DEVICES 8U
static pcie_bdf_t masked_bdfs[MAX_MASKED_DEVICES];
static uint32_t masked_count;

static bool is_bdf_masked(pcie_bdf_t bdf)
{
	for (uint32_t i = 0; i < masked_count; i++) {
		if (masked_bdfs[i] == bdf) {
			return true;
		}
	}
	return false;
}

/* Subcommand: pcie mask ignore <bus:dev.func> */
static int cmd_pcie_mask_ignore(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Usage: pcie mask ignore <bus:dev.func>");
		return -EINVAL;
	}

	if (masked_count >= MAX_MASKED_DEVICES) {
		shell_error(sh, "Mask table allocation full (Max %u).", MAX_MASKED_DEVICES);
		return -ENOMEM;
	}

	pcie_bdf_t bdf = get_bdf(argv[1]);

	if (bdf == PCIE_BDF_NONE) {
		shell_error(sh, "Invalid BDF layout format specification.");
		return -EINVAL;
	}

	bdf = PCIE_BDF(PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), 0);

	if (is_bdf_masked(bdf)) {
		shell_warn(sh, "Target BDF is already registered in the mask array.");
		return 0;
	}

	masked_bdfs[masked_count++] = bdf;
	pcie_scan_override_hook = is_bdf_masked;

	shell_print(sh, "Successfully masked BDF %u:%x.%u. Bus scans will ignore this slot.",
		    PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf));

	return 0;
}

/* Subcommand: pcie mask clear */
static int cmd_pcie_mask_clear(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	masked_count = 0;
	pcie_scan_override_hook = NULL;
	shell_print(sh, "Software PCIe mask tracking array successfully cleared.");
	return 0;
}

/* Subcommand: pcie mask show */
static int cmd_pcie_mask_show(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "--- Active PCIe Software Mask Registry Matrix ---");
	if (masked_count == 0) {
		shell_print(sh, " No devices currently masked.");
		return 0;
	}

	for (uint32_t i = 0; i < masked_count; i++) {
		pcie_bdf_t bdf = masked_bdfs[i];

		shell_print(sh, " [%u] Ignored BDF Slot -> %u:%x.%u", i, PCIE_BDF_TO_BUS(bdf),
			    PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf));
	}
	return 0;
}

static uint64_t probe_bar_size_mask(pcie_bdf_t bdf, unsigned int reg, unsigned int bar_idx,
				    bool is_64bit, uint32_t raw_bar)
{
	uint32_t cmd_backup = pcie_conf_read(bdf, PCIE_CONF_CMDSTAT);
	uint32_t size_mask_lo;
	uint64_t mask;

	pcie_conf_write(bdf, PCIE_CONF_CMDSTAT,
			cmd_backup & (~(PCIE_CONF_CMDSTAT_IO | PCIE_CONF_CMDSTAT_MEM)));

	pcie_conf_write(bdf, reg, 0xFFFFFFFFU);
	size_mask_lo = pcie_conf_read(bdf, reg);
	pcie_conf_write(bdf, reg, raw_bar);

	if (is_64bit && bar_idx < 5) {
		uint32_t raw_bar_hi = pcie_conf_read(bdf, reg + 1);
		uint32_t size_mask_hi;

		pcie_conf_write(bdf, reg + 1, 0xFFFFFFFFU);
		size_mask_hi = pcie_conf_read(bdf, reg + 1);
		pcie_conf_write(bdf, reg + 1, raw_bar_hi);

		mask = ((uint64_t)size_mask_hi << 32) | (uint64_t)size_mask_lo;
	} else {
		mask = (uint64_t)(uint32_t)size_mask_lo;
	}

	pcie_conf_write(bdf, PCIE_CONF_CMDSTAT, cmd_backup);
	return mask;
}

static void show_mem_bar_info(const struct shell *sh, pcie_bdf_t bdf, unsigned int reg,
			      unsigned int *bar_idx, uint64_t size_mask64, uint32_t raw_bar,
			      bool is_64bit)
{
	uint64_t mem_size;
	uint64_t base_addr = (uint64_t)(raw_bar & 0xFFFFFFF0U);
	bool prefetch = ((raw_bar & 0x8U) != 0);

	if (is_64bit && *bar_idx < 5) {
		mem_size = ~(size_mask64 & 0xFFFFFFFFFFFFFFF0U) + 1ULL;
	} else {
		mem_size = (~(size_mask64 & 0xFFFFFFF0U) + 1ULL) & 0xFFFFFFFFULL;
	}

	shell_print(sh, "   -> Type       : MEMORY Space (%s)",
		    is_64bit ? "64-bit Aperture" : "32-bit Aperture");
	shell_print(sh, "   -> Attributes : Prefetchable: %s", prefetch ? "TRUE" : "FALSE");
	shell_print(sh, "   -> Sizing     : Requested Size: %llu KB (Mask: 0x%016llX)",
		    (unsigned long long)(mem_size / 1024U), (unsigned long long)size_mask64);

	if (is_64bit && *bar_idx < 5) {
		uint32_t raw_bar_hi = pcie_conf_read(bdf, reg + 1);
		uint64_t base_addr64 = ((uint64_t)raw_bar_hi << 32) | base_addr;
		uint64_t end_addr64 = base_addr64 + (mem_size - 1ULL);

		shell_print(sh, "   -> Map Window : 0x%016llX - 0x%016llX",
			    (unsigned long long)base_addr64, (unsigned long long)end_addr64);
		*bar_idx += 2;
	} else {
		uint64_t end_addr32 = (base_addr + (mem_size - 1ULL)) & 0xFFFFFFFFULL;

		shell_print(sh, "   -> Map Window : 0x%016llX - 0x%016llX",
			    (unsigned long long)base_addr, (unsigned long long)end_addr32);
		*bar_idx += 1;
	}
}

static void show_io_bar_info(const struct shell *sh, uint64_t size_mask64, uint32_t raw_bar,
			     unsigned int *bar_idx)
{
	uint32_t io_size = ~((uint32_t)size_mask64 & 0xFFFFFFFCU) + 1;
	uint32_t base_io = raw_bar & 0xFFFFFFFCU;

	shell_print(sh, "   -> Type       : I/O Space");
	shell_print(sh, "   -> Sizing     : Requested Size: %u Bytes (Mask: 0x%016llX)", io_size,
		    (unsigned long long)size_mask64);
	shell_print(sh, "   -> Map Window : Port Range: 0x%08X - 0x%08X", base_io,
		    base_io + (io_size - 1U));
	*bar_idx += 1;
}

static int cmd_pcie_resource_show(const struct shell *sh, size_t argc, char **argv)
{
	pcie_bdf_t bdf;
	uint32_t id;
	unsigned int bar_idx;

	if (argc < 2) {
		shell_error(sh, "Usage: pcie resource show <bus:dev.func>");
		return -EINVAL;
	}

	bdf = get_bdf(argv[1]);
	if (bdf == PCIE_BDF_NONE) {
		shell_error(sh, "Invalid BDF layout format specification.");
		return -EINVAL;
	}

	id = pcie_conf_read(bdf, PCIE_CONF_ID);
	if (id == 0xFFFFFFFFU || !PCIE_ID_IS_VALID(id)) {
		shell_error(sh, "No responsive endpoint found at target BDF.");
		return -ENODEV;
	}

	shell_print(sh, "====================================================================");
	shell_print(sh, "   PCIe BAR RESOURCE DECODER DIAGNOSTIC GRID: %u:%x.%u",
		    PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf));
	shell_print(sh, "====================================================================");

	bar_idx = 0;
	while (bar_idx < 6) {
		unsigned int reg = PCIE_CONF_BAR0 + bar_idx;
		uint32_t raw_bar = pcie_conf_read(bdf, reg);
		uint64_t size_mask64;
		bool is_mem;
		bool is_64bit;

		if (PCIE_CONF_BAR_INVAL_FLAGS(raw_bar)) {
			bar_idx++;
			continue;
		}

		is_mem = ((raw_bar & 0x1U) == 0);
		is_64bit = (is_mem && ((raw_bar & 0x6U) == 0x4U));

		size_mask64 = probe_bar_size_mask(bdf, reg, bar_idx, is_64bit, raw_bar);

		if (size_mask64 == 0 || size_mask64 == 0xFFFFFFFFFFFFFFFFU ||
		    (is_64bit && size_mask64 == 0xFFFFFFFF00000000U)) {
			bar_idx += (is_64bit && bar_idx < 5) ? 2 : 1;
			continue;
		}

		shell_print(sh, "BAR %u [Reg Offset: 0x%02X]:", bar_idx, reg * 4);

		if (is_mem) {
			show_mem_bar_info(sh, bdf, reg, &bar_idx, size_mask64, raw_bar, is_64bit);
		} else {
			show_io_bar_info(sh, size_mask64, raw_bar, &bar_idx);
		}
		shell_print(sh, "----------------------------------------------------");
	}

	return 0;
}

/* Subcommand: pcie link set_speed <bus:dev.func> <gen1|gen2|gen3|gen4> */
static int cmd_pcie_link_set_speed(const struct shell *sh, size_t argc, char **argv)
{
	pcie_bdf_t bdf;
	uint32_t id;
	uint32_t pcie_cap_offset;
	uint32_t target_speed;
	uint32_t pcie_cap;
	uint32_t pcie_cap_ver;
	uint32_t lnkcap2_reg;
	uint32_t lnkcap2;
	uint32_t supported_speeds;
	uint32_t link_ctrl2_reg;
	uint32_t link_ctrl2;
	uint32_t link_ctrl_reg;
	uint32_t link_ctrl;
	uint32_t link_status;
	uint32_t negotiated_speed;

	if (argc < 3) {
		shell_error(sh, "Usage: pcie link set_speed <bus:dev.func> "
				"<gen1|gen2|gen3|gen4>");
		return -EINVAL;
	}

	bdf = get_bdf(argv[1]);
	if (bdf == PCIE_BDF_NONE) {
		shell_error(sh, "Invalid BDF layout format specification.");
		return -EINVAL;
	}

	id = pcie_conf_read(bdf, PCIE_CONF_ID);
	if (id == 0xFFFFFFFFU || !PCIE_ID_IS_VALID(id)) {
		shell_error(sh, "No responsive endpoint found at target BDF.");
		return -ENODEV;
	}

	pcie_cap_offset = pcie_get_cap(bdf, PCI_CAP_ID_EXP);
	if (pcie_cap_offset == 0) {
		shell_error(sh, "Target device does not support native "
				"PCIe Capability structures.");
		return -EOPNOTSUPP;
	}

	if (strcmp(argv[2], "gen1") == 0) {
		target_speed = 0x1;
	} else if (strcmp(argv[2], "gen2") == 0) {
		target_speed = 0x2;
	} else if (strcmp(argv[2], "gen3") == 0) {
		target_speed = 0x3;
	} else if (strcmp(argv[2], "gen4") == 0) {
		target_speed = 0x4;
	} else {
		shell_error(sh, "Invalid speed argument. Supported values: "
				"gen1, gen2, gen3, gen4");
		return -EINVAL;
	}

	pcie_cap = pcie_conf_read(bdf, pcie_cap_offset);
	pcie_cap_ver = (pcie_cap >> 16) & 0xFU;

	if (pcie_cap_ver < 2U) {
		shell_error(sh, "PCIe cap v%u does not support Link Control 2.", pcie_cap_ver);
		return -EOPNOTSUPP;
	}

	lnkcap2_reg = pcie_cap_offset + (0x2CU / 4U);
	lnkcap2 = pcie_conf_read(bdf, lnkcap2_reg);
	supported_speeds = lnkcap2 & 0x0FU;

	if ((supported_speeds & (1U << (target_speed - 1U))) == 0U) {
		shell_error(sh, "Target speed %s is not supported by this link.", argv[2]);
		return -EINVAL;
	}

	link_ctrl2_reg = pcie_cap_offset + (0x30U / 4U);
	link_ctrl2 = pcie_conf_read(bdf, link_ctrl2_reg);

	link_ctrl2 &= ~0x0FU; /* Clear Target Link Speed bitfield (Bits 3:0) */
	link_ctrl2 |= target_speed;
	pcie_conf_write(bdf, link_ctrl2_reg, link_ctrl2);

	shell_print(sh,
		    "Target speed configured to %s (Value: 0x%X) inside "
		    "Link Control 2.",
		    argv[2], target_speed);

	link_ctrl_reg = pcie_cap_offset + (0x10U / 4U);
	link_ctrl = pcie_conf_read(bdf, link_ctrl_reg);

	shell_print(sh, "Issuing Retrain Link command (Setting Bit 5)...");
	link_ctrl |= (1U << 5);
	pcie_conf_write(bdf, link_ctrl_reg, link_ctrl);

	/* Poll Link Status until retraining completes (Link Training bit clears) */
	link_status = 0U;
	for (int i = 0; i < 1000; i++) {
		link_status = pcie_conf_read(bdf, link_ctrl_reg);
		if (((link_status >> 27) & 0x1U) == 0U) {
			break;
		}
	}

	negotiated_speed = (link_status >> 16) & 0x0FU;

	shell_print(sh, "Active Negotiated Link Speed reported by hardware: Gen%u",
		    negotiated_speed);

	return 0;
}

/* Subcommand: pcie irq status <bus:dev.func> */
static int cmd_pcie_irq_status(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t intr;
	uint8_t int_pin;
	uint8_t int_line;
	uint32_t msi_offset;
	uint32_t msix_offset;
	pcie_bdf_t bdf;
	uint32_t id;

	if (argc < 2) {
		shell_error(sh, "Usage: pcie irq status <bus:dev.func>");
		return -EINVAL;
	}

	bdf = get_bdf(argv[1]);
	if (bdf == PCIE_BDF_NONE) {
		shell_error(sh, "Invalid BDF layout format specification.");
		return -EINVAL;
	}

	id = pcie_conf_read(bdf, PCIE_CONF_ID);
	if (!PCIE_ID_IS_VALID(id)) {
		shell_error(sh, "No responsive endpoint found at target BDF.");
		return -ENODEV;
	}

	shell_print(sh, "====================================================================");
	shell_print(sh, "   PCIe INTERRUPT LINE DIAGNOSTIC MATRIX: %u:%x.%u", PCIE_BDF_TO_BUS(bdf),
		    PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf));
	shell_print(sh, "====================================================================");

	intr = pcie_conf_read(bdf, PCIE_CONF_INTR);
	int_pin = (intr >> 8) & 0xFFU;
	int_line = intr & 0xFFU;

	shell_print(sh, "Legacy INTx Parameters:");
	shell_print(sh, "   -> Interrupt Pin  : INT%c", int_pin ? ('A' + int_pin - 1) : '-');

	if (int_line == PCIE_CONF_INTR_IRQ_NONE) {
		shell_print(sh, "   -> Interrupt Line : NOT ROUTED");
	} else {
		shell_print(sh, "   -> Interrupt Line : IRQ %u", (unsigned int)int_line);
	}

	shell_print(sh, "----------------------------------------------------");

	msi_offset = pcie_get_cap(bdf, PCI_CAP_ID_MSI);
	if (msi_offset != 0) {
		uint32_t msg_ctrl = pcie_conf_read(bdf, msi_offset);
		bool msi_en = (msg_ctrl & (1U << 16)) != 0;
		uint32_t multi_msg = (msg_ctrl >> 17) & 0x07U;
		uint32_t multi_en = (msg_ctrl >> 20) & 0x07U;
		bool is_64bit = (msg_ctrl & (1U << 23)) != 0;
		uint32_t addr_lo = pcie_conf_read(bdf, msi_offset + 1);

		shell_print(sh, "Message Signaled Interrupts (MSI) Capability [Offset 0x%02X]:",
			    msi_offset * 4);
		shell_print(sh, "   -> Active Status  : %s", msi_en ? "ENABLED" : "DISABLED");
		shell_print(sh, "   -> Capabilities   : Multiple Message Capable: %u vectors",
			    1U << multi_msg);
		shell_print(sh, "   -> Allocated      : Multiple Message Enabled: %u vectors",
			    1U << multi_en);

		if (is_64bit) {
			uint32_t addr_hi = pcie_conf_read(bdf, msi_offset + 2);
			uint32_t msg_data = pcie_conf_read(bdf, msi_offset + 3) & 0xFFFFU;

			shell_print(sh, "   -> Target Address : 0x%08X%08X", addr_hi, addr_lo);
			shell_print(sh, "   -> Message Data   : 0x%04X", msg_data);
		} else {
			uint32_t msg_data = pcie_conf_read(bdf, msi_offset + 2) & 0xFFFFU;

			shell_print(sh, "   -> Target Address : 0x%08X", addr_lo);
			shell_print(sh, "   -> Message Data   : 0x%04X", msg_data);
		}
	} else {
		shell_print(sh, "MSI Capability [ID 0x05]     : NOT SUPPORTED");
	}
	shell_print(sh, "----------------------------------------------------");

	msix_offset = pcie_get_cap(bdf, PCI_CAP_ID_MSIX);
	if (msix_offset != 0) {
		uint32_t msg_ctrl = pcie_conf_read(bdf, msix_offset);
		bool msix_en = (msg_ctrl & (1U << 31)) != 0;
		bool msix_mask = (msg_ctrl & (1U << 30)) != 0;
		uint32_t table_size = ((msg_ctrl >> 16) & 0x07FFU) + 1;
		uint32_t table_bir = pcie_conf_read(bdf, msix_offset + 1);
		uint8_t bir = table_bir & 0x07U;
		uint32_t offset = table_bir & ~0x07U;

		shell_print(sh,
			    "Extended MSI (MSI-X) Capability [Offset 0x%02X]:", msix_offset * 4);
		shell_print(sh, "   -> Active Status  : %s", msix_en ? "ENABLED" : "DISABLED");
		shell_print(sh, "   -> Global Mask    : %s",
			    msix_mask ? "TRUE (All Vectors Blocked)" : "FALSE");
		shell_print(sh, "   -> Table Size     : %u distinct vectors", table_size);
		shell_print(sh, "   -> Vector Table   : Located in BAR %u at base offset + 0x%08X",
			    bir, offset);
	} else {
		shell_print(sh, "MSI-X Capability [ID 0x11]   : NOT SUPPORTED");
	}
	shell_print(sh, "====================================================================");

	return 0;
}

static inline void log_pcie_aer_uncorr(const struct shell *sh, uint32_t status)
{
	if (status == 0U) {
		shell_print(sh, "   -> No uncorrectable fatal hardware fractures logged.");
		return;
	}

	if ((status & (1U << 0)) != 0U) {
		shell_print(sh, "   [CRITICAL] -> Link Training Error (LTSSM Failure)");
	}
	if ((status & (1U << 4)) != 0U) {
		shell_print(sh, "   [CRITICAL] -> Data Link Protocol Error Detected");
	}
	if ((status & (1U << 12)) != 0U) {
		shell_print(sh, "   [CRITICAL] -> Poisoned TLP Execution Blocked");
	}
	if ((status & (1U << 13)) != 0U) {
		shell_print(sh, "   [CRITICAL] -> Flow Control Protocol Error (FCPE)");
	}
	if ((status & (1U << 16)) != 0U) {
		shell_print(sh, "   [CRITICAL] -> Unexpected Completion Trap");
	}
	if ((status & (1U << 20)) != 0U) {
		shell_print(sh, "   [CRITICAL] -> Unsupported Request (UR) Abort");
	}
}

static inline void log_pcie_aer_corr(const struct shell *sh, uint32_t status)
{
	if (status == 0U) {
		shell_print(sh, "   -> No physical signal degradation deviations reported.");
		return;
	}

	if ((status & (1U << 0)) != 0U) {
		shell_print(sh, "   [WARNING]  -> Receiver Physical Layer Error");
	}
	if ((status & (1U << 6)) != 0U) {
		shell_print(sh, "   [WARNING]  -> Bad TLP Frame CRC Detected");
	}
	if ((status & (1U << 7)) != 0U) {
		shell_print(sh, "   [WARNING]  -> Bad DLLP Frame Checksum Detected");
	}
	if ((status & (1U << 8)) != 0U) {
		shell_print(sh, "   [WARNING]  -> REPLAY_NUM Rollover/Timer Timeout");
	}
}

/* Subcommand: pcie aer status <bus:dev.func> */
static int cmd_pcie_aer_status(const struct shell *sh, size_t argc, char **argv)
{
	pcie_bdf_t bdf;
	uint32_t id;
	uint32_t aer_offset;
	uint32_t uncorr_status;
	uint32_t uncorr_mask;
	uint32_t corr_status;
	uint32_t corr_mask;
	uint32_t header_log[4];

	if (argc < 2) {
		shell_error(sh, "Usage: pcie aer status <bus:dev.func>");
		return -EINVAL;
	}

	bdf = get_bdf(argv[1]);
	if (bdf == PCIE_BDF_NONE) {
		shell_error(sh, "Invalid BDF layout format specification.");
		return -EINVAL;
	}

	id = pcie_conf_read(bdf, PCIE_CONF_ID);
	if (id == 0xFFFFFFFFU || !PCIE_ID_IS_VALID(id)) {
		shell_error(sh, "No responsive endpoint found at target BDF.");
		return -ENODEV;
	}

	aer_offset = pcie_get_ext_cap(bdf, PCIE_EXT_CAP_AER_ID);
	if (aer_offset == 0) {
		shell_error(sh, "Target device does not support native "
				"Advanced Error Reporting (AER).");
		return -EOPNOTSUPP;
	}

	uncorr_status = pcie_conf_read(bdf, aer_offset + PCIE_AER_UNCORR_STATUS_REG);
	uncorr_mask = pcie_conf_read(bdf, aer_offset + PCIE_AER_UNCORR_MASK_REG);
	corr_status = pcie_conf_read(bdf, aer_offset + PCIE_AER_CORR_STATUS_REG);
	corr_mask = pcie_conf_read(bdf, aer_offset + PCIE_AER_CORR_MASK_REG);

	shell_print(sh, "====================================================================");
	shell_print(sh, "   PCIe ADVANCED ERROR REPORTING (AER) STATUS MATRIX: %u:%x.%u",
		    PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf));
	shell_print(sh, "====================================================================");

	shell_print(sh, "Uncorrectable Errors Status: 0x%08X [Mask: 0x%08X]", uncorr_status,
		    uncorr_mask);
	log_pcie_aer_uncorr(sh, uncorr_status);

	shell_print(sh, "----------------------------------------------------");

	shell_print(sh, "Correctable Errors Status:   0x%08X [Mask: 0x%08X]", corr_status,
		    corr_mask);
	log_pcie_aer_corr(sh, corr_status);

	shell_print(sh, "----------------------------------------------------");

	shell_print(sh, "Transaction Layer Packet (TLP) Header Log Matrix:");
	for (uint32_t i = 0U; i < 4U; i++) {
		header_log[i] = pcie_conf_read(bdf, aer_offset + PCIE_AER_HEADER_LOG_REG + i);
	}
	shell_print(sh, "   [DW0-DW1] : 0x%08X 0x%08X", header_log[0], header_log[1]);
	shell_print(sh, "   [DW2-DW3] : 0x%08X 0x%08X", header_log[2], header_log[3]);
	shell_print(sh, "====================================================================");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_pcie_mask_cmds,
	SHELL_CMD_ARG(ignore, NULL, "Register a coordinate slot to be ignored by bus scans",
		      cmd_pcie_mask_ignore, 2, 0),
	SHELL_CMD_ARG(show, NULL, "Display all blacklisted BDF slots currently active",
		      cmd_pcie_mask_show, 1, 0),
	SHELL_CMD_ARG(clear, NULL, "Flush the mask configuration tracking registers",
		      cmd_pcie_mask_clear, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_pcie_resource_cmds,
	SHELL_CMD_ARG(show, NULL, "Decode and map all active Base Address Registers (BARs)",
		      cmd_pcie_resource_show, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pcie_link_cmds,
			       SHELL_CMD_ARG(set_speed, NULL,
					     "Scale the target PCIe bus lane speed dynamically",
					     cmd_pcie_link_set_speed, 3, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_pcie_irq_cmds,
	SHELL_CMD_ARG(status, NULL, "Trace legacy and MSI/MSI-X vector configuration statuses",
		      cmd_pcie_irq_status, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_pcie_aer_cmds,
	SHELL_CMD_ARG(status, NULL, "Trace correctable and uncorrectable hardware error states",
		      cmd_pcie_aer_status, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_pcie_cmds,
	SHELL_CMD_ARG(ls, NULL,
		      "List PCIE devices\n"
		      "Usage: ls [bus:device:function] [dump]",
		      cmd_pcie_ls, 1, 2),
	SHELL_CMD_ARG(write, NULL,
		      "Write a 32-bit word directly to a device register.\n"
		      "Usage: write <bus:device.function> <reg_offset> <32bit_hex>",
		      cmd_pcie_write, 4, 0),
	SHELL_CMD(mask, &sub_pcie_mask_cmds, "Manage PCIe device scanning parameters", NULL),
	SHELL_CMD(resource, &sub_pcie_resource_cmds, "PCI(e) memory mapping and resource metrics",
		  NULL),
	SHELL_CMD(link, &sub_pcie_link_cmds,
		  "Manage PCIe hardware link performance and power metrics", NULL),
	SHELL_CMD(irq, &sub_pcie_irq_cmds, "Trace legacy and vector interrupt message allocations",
		  NULL),
	SHELL_CMD(aer, &sub_pcie_aer_cmds, "Interact with PCIe Advanced Error Reporting metrics",
		  NULL),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pcie, &sub_pcie_cmds, "PCI(e) device information", cmd_pcie_ls);
