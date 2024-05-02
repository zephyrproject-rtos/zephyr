/*
 * Copyright (c) 2024 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <soc.h>
#include "espi_hc_emu.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app);

#include "espi_debug.h"

const char *iobar_names[] = {
	"eSPI Config Port IOB",
	"eSPI Mem Port IOB",
	"Mailbox IOB",
	"KBC IOB",
	"ACPI EC0 IOB",
	"ACPI EC1 IOB",
	"ACPI EC2 IOB",
	"ACPI EC3 IOB",
	"ACPI EC4 IOB",
	"ACPI PM1 IOB",
	"KB Port92h IOB",
	"UART0 IOB",
	"UART1 IOB",
	"EMI0 IOB",
	"EMI1 IOB",
	"EMI2 IOB",
	"BIOS Debug IOB",
	"BIOS Debug Alias IOB",
	"RTC IOB",
	"Reserved19",
	"T32B IOB",
	"UART2 IOB",
	"Glue IOB",
	"UART3 IOB",
	"Reserved24",
	"Reserved25",
	"Reserved26",
	"Reserved27",
	"Reserved28",
	"Reserved29",
	"Reserved30",
	"Reserved31",
};

const char *membar_names[] = {
	"Mailbox MEMB",
	"ACPI EC0 MEMB",
	"ACPI EC1 MEMB",
	"ACPI EC2 MEMB",
	"ACPI EC3 MEMB",
	"ACPI EC4 MEMB",
	"EMI0 MEMB",
	"EMI1 MEMB",
	"EMI2 MEMB",
	"T32B MEMB",
};

static int espi_freq_tbl[] = {20, 25, 33, 50, 66, -1, -1, -1};

static int lookup_espi_freq(uint32_t freq_encoding)
{
	return espi_freq_tbl[freq_encoding & 0x7u];
}

static void pr_io_mode(uint32_t io_mode)
{
	switch (io_mode) {
	case 0:
		LOG_INF("  Single I/O");
		break;
	case 1:
		LOG_INF("  Single and Dual I/O");
		break;
	case 2:
		LOG_INF("  Single and Quad I/O");
		break;
	case 3:
		LOG_INF("  Single, Dual, and Quad I/O");
		break;
	default:
		break;
	}
}

/* eSPI Capabilities 32-bit word 0x4 read by eSPI Host */
static void pr_cap_8(uint32_t cap)
{
	uint32_t temp;

	LOG_INF("eSPI Cap word 0x8: 0x%08x", cap);
	LOG_INF("  b[7:0] supported channels = 0x%02x", cap & 0xffu);
	temp = (cap >> 12) & 0xfu;
	LOG_INF("  b[15:12] max allowed wait states (0=16 byte times) = 0x%02x", temp);
	temp = lookup_espi_freq((cap >> 16));
	LOG_INF("  b[18:16] max supported freq (MHz) = 0x%02x is %u MHz",
		((cap >> 16) & 0x7u), temp);
	LOG_INF("  b[19] = Open drain alert supported = %u", (cap >> 19) & 0x1u);
	temp = lookup_espi_freq((cap >> 20));
	LOG_INF("  b[22:20] selected frequency (MHz) = 0x%02x", temp);
	LOG_INF("  b[19] = Open drain alert selected = %u", (cap >> 23) & 0x1u);
	temp = (cap >> 24) & 0x3u;
	LOG_INF("  b[25:24] = 0x%02x Supported I/O modes", temp);
	pr_io_mode(temp);
	temp = (cap >> 26) & 0x3u;
	LOG_INF("  b[27:26] = 0x%02x Selected I/O mode", temp);
	pr_io_mode(temp);
	temp = (cap >> 28) & 0x1u;
	LOG_INF("  b[28] = %u : eSPI Alert source 0(eSPI IO[1]), 1(separate pin)", temp);
	temp = (cap >> 29) & 0x1u;
	LOG_INF("  b[29] = %u : BMC has integrated RTC 0(no), 1(yes)", temp);
	temp = (cap >> 30) & 0x1u;
	LOG_INF("  b[30] = %u : GET_STATUS response modifier 0(disabled), 1(enabled)", temp);
	temp = (cap >> 31) & 0x1u;
	LOG_INF("  b[31] = %u : CRC packet check 0(disabled), 1(enabled)", temp);
}

static void pr_payload_size(uint32_t sz, uint8_t is_oob)
{
	uint32_t byte_size;

	switch (sz & 0x7u) {
	case 1:
		byte_size = 64u;
		break;
	case 2:
		byte_size = 128u;
		break;
	case 3:
		byte_size = 256u;
		break;
	default:
		byte_size = 0u;
		break;
	}

	if (byte_size) {
		if (is_oob) {
			byte_size += 9u;
		}
		LOG_INF("  %u bytes", byte_size);
	} else {
		LOG_INF("  !!! Reserved value !!!");
	}
}

static void pr_max_read_req_size(uint32_t sz)
{
	uint32_t temp;

	if (sz & 0x7u) {
		temp = 1u << (sz + 5u);
		LOG_INF("  %u bytes", temp);
	} else {
		LOG_INF("  !!! Reserved value !!!");
	}
}

static void pr_cap_10(uint32_t cap)
{
	uint32_t temp;

	LOG_INF("eSPI Cap word 0x10 (Peripheral Channel): 0x%08x", cap);
	LOG_INF("  b[0] = PC Enable = %u", cap & 0x1);
	LOG_INF("  b[1] = PC Ready  = %u", (cap >> 1) & 0x1);
	LOG_INF("  b[2] = BM Enable = %u", (cap >> 2) & 0x1);
	LOG_INF("  b[6:4] = Max supported payload size");
	temp = (cap >> 4) & 0x7u;
	pr_payload_size(temp, 0);
	temp = (cap >> 8) & 0x7u;
	LOG_INF("  b[10:8] = Selected payload size = %u", temp);
	pr_payload_size(temp, 0);
	temp = (cap >> 12) & 0x7u;
	LOG_INF("  b[14:12] = Max read request size = %u", temp);
	pr_max_read_req_size(temp);
}

static void pr_cap_20(uint32_t cap)
{
	uint32_t temp;

	LOG_INF("eSPI Cap work 0x20 (VW Channel): 0x%08x", cap);
	LOG_INF("  b[0] = VW Enable = %u", cap & 0x1u);
	LOG_INF("  b[1] = VW Ready  = %u", (cap >> 1) & 0x1u);
	temp = (cap >> 8) & 0x3fu;
	LOG_INF("  b[13:8] = Max Supported VW groups (0-based) = %u", temp);
	temp = (cap >> 16) & 0x3fu;
	LOG_INF("  b[21:16] = Max Selected VW groups (0-based) = %u", temp);
}

static void pr_cap_30(uint32_t cap)
{
	uint32_t temp;

	LOG_INF("eSPI Cap work 0x30 (OOB Channel): 0x%08x", cap);
	LOG_INF("  b[0] = OOB Enable = %u", cap & 0x1u);
	LOG_INF("  b[1] = OOB Ready  = %u", (cap >> 1) & 0x1u);
	temp = (cap >> 4) & 0x7u;
	LOG_INF("  b[6:4] = Max Supported Payload size = %u", temp);
	pr_payload_size(temp, 1);
	temp = (cap >> 8) & 0x7u;
	LOG_INF("  b[6:4] = Max Selected Payload size = %u", temp);
	pr_payload_size(temp, 1);
}

static void pr_fc_erase_size(uint32_t cap)
{
	uint32_t temp = (cap >> 2) & 0x7u;

	LOG_INF("  b[4:2] = Selected Flash Block Erase Size = 0x%2x", temp);
	switch (temp) {
	case 1:
		LOG_INF("  4KB");
		break;
	case 2:
		LOG_INF("  64KB");
		break;
	case 3:
		LOG_INF("  4KB and 64KB");
		break;
	case 4:
		LOG_INF("  128KB");
		break;
	case 5:
		LOG_INF("  256KB");
		break;
	default:
		LOG_INF("  !!! Reserved value !!!");
		break;
	}
}

static void pr_fc_sharing_support(uint32_t cap)
{
	uint32_t temp = (cap >> 16) & 0x3u;

	switch (temp) {
	case 2:
		LOG_INF("  b[17:16]=10b Supports Target Attached Flash only");
		break;
	case 3:
		LOG_INF("  b[17:16]=11b Supports both Controller and Target Attached Flash");
		break;
	default:
		LOG_INF("  b[17:16]=0x%x Supports Controller Attached Flash only", temp);
		break;
	}
}

static void pr_cap_40(uint32_t cap)
{
	uint32_t temp;

	LOG_INF("eSPI Cap work 0x40 (Flash Channel): 0x%08x", cap);
	LOG_INF("  b[0] = FC Enable = %u", cap & 0x1u);
	LOG_INF("  b[1] = FC Ready  = %u", (cap >> 1) & 0x1u);
	pr_fc_erase_size(cap);
	temp = (cap >> 5) & 0x7u;
	LOG_INF("  b[7:5] = 0x%2x Max Supported Payload size", temp);
	pr_payload_size(temp, 0);
	temp = (cap >> 8) & 0x7u;
	LOG_INF("  b[7:5] = 0x%2x Max Selected Payload size", temp);
	pr_payload_size(temp, 0);
	temp = (cap >> 11) & 0x1u;
	LOG_INF("  b[11] Flash Sharing Mode = %u: 0=Controller Attached, 1=Target Attached", temp);
	temp = (cap >> 12) & 0x7u;
	LOG_INF("  b[14:12] Max Reqd Request Size = 0x%2x", temp);
	pr_max_read_req_size(temp);
	pr_fc_sharing_support(cap);
	temp = (cap >> 20) & 0xfu;
	LOG_INF("  b[23:20] RPMC First Flash Number of Counters = %u", temp);
	temp = (cap >> 24) & 0xffu;
	LOG_INF("  b[31:24] RPMC First Flash OP1 Opcode = 0x%02x", temp);
}

void espi_debug_print_cap_word(uint16_t cap_offset, uint32_t cap)
{
	switch (cap_offset) {
	case 0x4:
		LOG_INF("eSPI Cap word 0x4 b[7:0] version ID = 0x%02x", cap);
		break;
	case 0x8:
		pr_cap_8(cap);
		break;
	case 0x10:
		pr_cap_10(cap);
		break;
	case 0x20:
		pr_cap_20(cap);
		break;
	case 0x30:
		pr_cap_30(cap);
		break;
	case 0x40:
		pr_cap_40(cap);
		break;
	default:
		LOG_ERR("Unknown eSPI cap offset 0x%04x = 0x%08x", cap_offset, cap);
	}
}

void espi_debug_print_hc(struct espi_hc_context *hc, uint8_t decode_caps)
{
	if (!hc) {
		return;
	}

	LOG_INF("eSPI Host Controller context structure");
	LOG_INF("Version ID: 0x%08x ", hc->version_id);
	LOG_INF("Global Cap: 0x%08x", hc->global_cap_cfg);
	LOG_INF("Peripheral Chan Cap/Cfg: 0x%08x", hc->pc_cap_cfg);
	LOG_INF("VW Chan Cap/Cfg: 0x%08x", hc->vw_cap_cfg);
	LOG_INF("OOB Chan Cap/Cfg: 0x%08x", hc->oob_cap_cfg);
	LOG_INF("Flash Chan Cap/Cfg: 0x%08x", hc->fc_cap_cfg);
	LOG_INF("Packet Status: 0x%04x", hc->pkt_status);
	LOG_INF("Target HW maximum VWire groups is 0x%0x", hc->vwg_cnt_max);
	LOG_INF("Target current VW group count is 0x%0x", hc->vwg_cnt);
	if (decode_caps) {
		espi_debug_print_cap_word(0x8, hc->global_cap_cfg);
		espi_debug_print_cap_word(0x10, hc->pc_cap_cfg);
		espi_debug_print_cap_word(0x20, hc->vw_cap_cfg);
		espi_debug_print_cap_word(0x30, hc->oob_cap_cfg);
		espi_debug_print_cap_word(0x40, hc->fc_cap_cfg);
	}
}

struct espi_vw_znames {
	enum espi_vwire_signal signal;
	uint8_t host_index;
	uint8_t source_pos;
	uint8_t dir; /* 0=C2T, 1=T2C */
	const char *name_cstr;
};

const struct espi_vw_znames vw_names[] = {
	{ ESPI_VWIRE_SIGNAL_SLP_S3,		0x02u, 0, 0, "SLP_S3#" },
	{ ESPI_VWIRE_SIGNAL_SLP_S4,		0x02u, 1, 0, "SLP_S4#" },
	{ ESPI_VWIRE_SIGNAL_SLP_S5,		0x02u, 2, 0, "SLP_S5#" },
	{ ESPI_VWIRE_SIGNAL_OOB_RST_WARN,	0x03u, 2, 0, "OOB_RST_WARN" },
	{ ESPI_VWIRE_SIGNAL_PLTRST,		0x03u, 1, 0, "PLTRST#" },
	{ ESPI_VWIRE_SIGNAL_SUS_STAT,		0x03u, 0, 0, "SUS_STAT#" },
	{ ESPI_VWIRE_SIGNAL_NMIOUT,		0x07u, 2, 0, "NMIOUT#" },
	{ ESPI_VWIRE_SIGNAL_SMIOUT,		0x07u, 1, 0, "SMIOUT#" },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_WARN,	0x07u, 0, 0, "HOST_RST_WARN" },
	{ ESPI_VWIRE_SIGNAL_SLP_A,		0x41u, 3, 0, "SLP_A#" },
	{ ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK,	0x41u, 1, 0, "SUS_PWRDN_ACK" },
	{ ESPI_VWIRE_SIGNAL_SUS_WARN,		0x41u, 0, 0, "SUS_WARN#" },
	{ ESPI_VWIRE_SIGNAL_SLP_WLAN,		0x42u, 1, 0, "SLP_WLAN#" },
	{ ESPI_VWIRE_SIGNAL_SLP_LAN,		0x42u, 0, 0, "SLP_LAN#" },
	{ ESPI_VWIRE_SIGNAL_HOST_C10,		0x47u, 0, 0, "HOST_C10" },
	{ ESPI_VWIRE_SIGNAL_DNX_WARN,		0x4au, 1, 0, "DNX_WARN" },
	{ ESPI_VWIRE_SIGNAL_PME,		0x04u, 3, 1, "PME#" },
	{ ESPI_VWIRE_SIGNAL_WAKE,		0x04u, 2, 1, "WAKE#" },
	{ ESPI_VWIRE_SIGNAL_OOB_RST_ACK,	0x04u, 0, 1, "OOB_RST_ACK" },
	{ ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS,	0x05u, 3, 1, "TARGET_BOOT_STS" },
	{ ESPI_VWIRE_SIGNAL_ERR_NON_FATAL,	0x05u, 2, 1, "ERR_NON_FATAL" },
	{ ESPI_VWIRE_SIGNAL_ERR_FATAL,		0x05u, 1, 0, "ERR_FATAL" },
	{ ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE,	0x05u, 0, 1, "TARGET_BOOT_DONE" },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_ACK,	0x06u, 3, 1, "HOST_RST_ACK" },
	{ ESPI_VWIRE_SIGNAL_RST_CPU_INIT,	0x06u, 2, 1, "RST_CPU_INIT" },
	{ ESPI_VWIRE_SIGNAL_SMI,		0x06u, 1, 1, "SMI#" },
	{ ESPI_VWIRE_SIGNAL_SCI,		0x06u, 0, 1, "SCI#" },
	{ ESPI_VWIRE_SIGNAL_DNX_ACK,		0x40u, 1, 1, "DNX_ACK" },
	{ ESPI_VWIRE_SIGNAL_SUS_ACK,		0x40u, 0, 1, "SUS_ACK#" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_0,	0x50u, 0, 1, "TARGET_GPIO_0" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_1,	0x50u, 1, 1, "TARGET_GPIO_1" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_2,	0x50u, 2, 1, "TARGET_GPIO_2" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_3,	0x50u, 3, 0, "TARGET_GPIO_3" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_4,	0x51u, 0, 1, "TARGET_GPIO_4" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_5,	0x51u, 1, 1, "TARGET_GPIO_5" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_6,	0x51u, 2, 1, "TARGET_GPIO_6" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_7,	0x51u, 3, 1, "TARGET_GPIO_7" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_8,	0x52u, 0, 1, "TARGET_GPIO_8" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_9,	0x52u, 1, 1, "TARGET_GPIO_9" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_10,	0x52u, 2, 1, "TARGET_GPIO_10" },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_11,	0x52u, 3, 1, "TARGET_GPIO_11" },
};

const char *unknown_vwire = "Unknown VWire";

const char *get_vw_name(uint32_t vwire_enum_val)
{
	for (size_t n = 0; n < ARRAY_SIZE(vw_names); n++) {
		const struct espi_vw_znames *vwn = &vw_names[n];

		if (vwn->signal == (enum espi_vwire_signal)vwire_enum_val) {
			return vwn->name_cstr;
		}
	}

	return unknown_vwire;
}

void espi_debug_print_vws(struct espi_hc_vw *vws, size_t nvws)
{
	if (!vws || !nvws) {
		return;
	}

	for (size_t n = 0; n < nvws; n++) {
		if (vws->host_index != 0xffu) {
			LOG_INF("VWg[%u]: hostIdx=0x%02x mask=0x%02x resetv=0x%02x "
				"levels=0x%02x valid=0x%02x", n, vws->host_index, vws->mask,
				vws->reset_val, vws->levels, vws->valid);
		}
		vws++;
	}
}

void espi_debug_pr_status(uint16_t espi_status)
{
	LOG_INF("eSPI_STATUS = 0x%04x", espi_status);
	if (espi_status & BIT(ESPI_STATUS_PC_FREE_POS)) {
		LOG_INF("PC Free");
	}
	if (espi_status & BIT(ESPI_STATUS_PC_NP_FREE_POS)) {
		LOG_INF("PC NP Free");
	}
	if (espi_status & BIT(ESPI_STATUS_VW_FREE_POS)) {
		LOG_INF("VW Free");
	}
	if (espi_status & BIT(ESPI_STATUS_OOB_FREE_POS)) {
		LOG_INF("OOB Free");
	}
	if (espi_status & BIT(ESPI_STATUS_PC_AVAIL_POS)) {
		LOG_INF("PC Avail");
	}
	if (espi_status & BIT(ESPI_STATUS_PC_NP_AVAIL_POS)) {
		LOG_INF("PC NP Avail");
	}
	if (espi_status & BIT(ESPI_STATUS_VW_AVAIL_POS)) {
		LOG_INF("VW Avail");
	}
	if (espi_status & BIT(ESPI_STATUS_OOB_AVAIL_POS)) {
		LOG_INF("OOB Avail");
	}
	if (espi_status & BIT(ESPI_STATUS_FC_FREE_POS)) {
		LOG_INF("FC Free");
	}
	if (espi_status & BIT(ESPI_STATUS_FC_NP_FREE_POS)) {
		LOG_INF("FC NP Free");
	}
	if (espi_status & BIT(ESPI_STATUS_FC_AVAIL_POS)) {
		LOG_INF("FC Avail");
	}
	if (espi_status & BIT(ESPI_STATUS_FC_NP_AVAIL_POS)) {
		LOG_INF("FC NP Avail");
	}
}
