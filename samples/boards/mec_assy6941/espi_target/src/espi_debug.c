/*
 * Copyright (c) 2023 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
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

void espi_debug_print_config(void)
{
	mem_addr_t maddr;
	uint32_t data;

	maddr = (mem_addr_t)&ESPI_IO->CAPID;
	data = sys_read32(maddr);
	LOG_HEXDUMP_INF(&data, 4, "eSPI CapID:Cap0:Cap1:PC_Cap");

	maddr = (mem_addr_t)&ESPI_IO->CAPVW;
	data = sys_read32(maddr);
	LOG_HEXDUMP_INF(&data, 4, "eSPI VW_Cap:OOB_Cap:FC_Cap:PCRDY");

	maddr = (mem_addr_t)&ESPI_IO->OOBRDY;
	data = sys_read32(maddr);
	LOG_HEXDUMP_INF(&data, 4, "eSPI OOBRDY:FCRDY:ERIS:ERIE");

	maddr = (mem_addr_t)&ESPI_IO->PLTRST_SRC;
	data = sys_read32(maddr);
	LOG_HEXDUMP_INF(&data, 4, "eSPI PLTRST_SRC:VWRDY:SAFEBS:RSVD");

	LOG_INF("eSPI Activate = %u", ESPI_IO->ACTV);

	LOG_INF("eSPI PC.Status = 0x%08x", ESPI_IO->PCSTS);
	LOG_INF("eSPI VW.Status = 0x%02x", ESPI_IO->VWSTS);
}

void espi_debug_print_io_bars(void)
{
	uint32_t n, iobar;

	for (n = 0; n < 32u; n++) {
		if (BIT(n) & MEC5_ESPI_IOBAR_MSK_LO) {
			iobar = ESPI_IO->HOST_BAR[n];
			LOG_INF("IOBAR[%u] (%s) = 0x%08x", n, iobar_names[n], iobar);
		}
	}
}

void espi_debug_print_mem_bars(void)
{
	uint32_t n, membar;

	for (n = 0; n < 32u; n++) {
		if (BIT(n) & MEC5_ESPI_MEMBAR_MSK_LO) {
			volatile struct espi_mem_host_mem_bar_regs *m =
				&ESPI_MEM->HOST_MEM_BAR[n];

			membar = m->HOST_MEM_ADDR_B15_0 +
				((uint32_t)m->HOST_MEM_ADDR_B31_16 << 16);
			LOG_INF("MEMBAR[%u] (%s) = 0x%08x valid = %u",
				n, membar_names[n], membar, m->VALID);
		}
	}
}

void espi_debug_print_ctvw(void)
{
	uint32_t r[3];

	for (size_t n = 0; n < MEC5_ESPI_NUM_CTVW; n++) {
		r[0] = ESPI_VW->CTVW[n].HIRSS;
		r[1] = ESPI_VW->CTVW[n].SRC_ISELS;
		r[2] = ESPI_VW->CTVW[n].STATES;
		LOG_INF("CTVW[%u] = 0x%08x:%08x:%08x", n, r[2], r[1], r[0]);
	}
}

void espi_debug_print_tcvw(void)
{
	uint32_t r[2];

	for (size_t n = 0; n < MEC5_ESPI_NUM_TCVW; n++) {
		r[0] = ESPI_VW->TCVW[n].HIRCS;
		r[1] = ESPI_VW->TCVW[n].STATES;
		LOG_INF("TCVW[%u] = 0x%08x:%08x", n, r[1], r[0]);
	}
}

void espi_debug_print_pc(void)
{
	LOG_INF("PC.STATUS = 0x%x", ESPI_IO->PCSTS);
	LOG_INF("PC.IEN    = 0x%x", ESPI_IO->PCIEN);
	LOG_INF("PC.BAR_INHIBIT = 0x%08x:%08x",
		ESPI_IO->PCBINH[1], ESPI_IO->PCBINH[0]);
}

void espi_debug_print_oob(void)
{
	LOG_INF("OOB RX Buf Addr = 0x%x", ESPI_IO->OOBRXA);
	LOG_INF("OOB RX Xfr Len = 0x%x", ESPI_IO->OOBRXL);
	LOG_INF("OOB RX Control = 0x%x", ESPI_IO->OOBRXC);
	LOG_INF("OOB RX IEN = 0x%x", ESPI_IO->OOBRXIEN);
	LOG_INF("OOB RX Status = 0x%x", ESPI_IO->OOBRXSTS);
	LOG_INF("OOB TX Buf Addr = 0x%x", ESPI_IO->OOBTXA);
	LOG_INF("OOB TX Xfr Len = 0x%x", ESPI_IO->OOBTXL);
	LOG_INF("OOB TX Control = 0x%x", ESPI_IO->OOBTXC);
	LOG_INF("OOB TX IEN = 0x%x", ESPI_IO->OOBTXIEN);
	LOG_INF("OOB TX Status = 0x%x", ESPI_IO->OOBTXSTS);
}

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
