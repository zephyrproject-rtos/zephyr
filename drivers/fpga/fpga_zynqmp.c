/*
 * Copyright (c) 2021-2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_fpga

#include <zephyr/device.h>
#include <zephyr/drivers/fpga.h>
#include "fpga_zynqmp.h"
#include <errno.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <stdlib.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fpga_zynqmp);

static void power_up_fpga(void)
{
	PMU_GLOBAL_PWRUP_EN = PWR_PL_MASK;
	PMU_REQ_PWRUP_TRIG = PWR_PL_MASK;

	while (PWR_STATUS & PWR_PL_MASK) {
	};
}

struct zynqmp_fpga_data {
	char FPGA_info[16];
};

static void update_part_name(const struct device *dev)
{
	struct zynqmp_fpga_data *data = dev->data;

	int zu_number = 0;

	switch (IDCODE & IDCODE_MASK) {
	case ZU2_IDCODE:
		zu_number = 2;
		break;
	case ZU3_IDCODE:
		zu_number = 3;
		break;
	case ZU4_IDCODE:
		zu_number = 4;
		break;
	case ZU5_IDCODE:
		zu_number = 5;
		break;
	case ZU6_IDCODE:
		zu_number = 6;
		break;
	case ZU7_IDCODE:
		zu_number = 7;
		break;
	case ZU9_IDCODE:
		zu_number = 9;
		break;
	case ZU11_IDCODE:
		zu_number = 11;
		break;
	case ZU15_IDCODE:
		zu_number = 15;
		break;
	case ZU17_IDCODE:
		zu_number = 17;
		break;
	case ZU19_IDCODE:
		zu_number = 19;
		break;
	case ZU21_IDCODE:
		zu_number = 21;
		break;
	case ZU25_IDCODE:
		zu_number = 25;
		break;
	case ZU27_IDCODE:
		zu_number = 27;
		break;
	case ZU28_IDCODE:
		zu_number = 28;
		break;
	case ZU29_IDCODE:
		zu_number = 29;
		break;
	case ZU39_IDCODE:
		zu_number = 39;
		break;
	case ZU43_IDCODE:
		zu_number = 43;
		break;
	case ZU46_IDCODE:
		zu_number = 46;
		break;
	case ZU47_IDCODE:
		zu_number = 47;
		break;
	case ZU48_IDCODE:
		zu_number = 48;
		break;
	case ZU49_IDCODE:
		zu_number = 49;
		break;
	}

	if (zu_number == 0) {
		snprintf(data->FPGA_info, sizeof(data->FPGA_info), "unknown");
	} else {
		snprintf(data->FPGA_info, sizeof(data->FPGA_info), "Part name: ZU%d", zu_number);
	}

}

/*
 * This function is responsible for shifting the bitstream
 * by its header and extracting information from this header.
 * The bitstream header has 5 sections starting with the letters a,b,c...
 * Each section has the following structure:
 * [key][length of data][data]
 */
static uint32_t *parse_header(const struct device *dev, uint32_t *image_ptr,
		       uint32_t *img_size)
{
	unsigned char *header = (unsigned char *)image_ptr;

	uint32_t length = XLNX_BITSTREAM_SECTION_LENGTH(header);

	/* shift to the next section*/
	header += 0x4U + length;

	if (*header++ != 'a') {
		LOG_ERR("Incorrect bitstream format");
		return NULL;
	}

	length = XLNX_BITSTREAM_SECTION_LENGTH(header);
	/* shift to the data section*/
	header += 0x2U;

	LOG_DBG("Design name = %s", header);

	header += length;

	if (*header++ != 'b') {
		LOG_ERR("Incorrect bitstream format");
		return NULL;
	}

	length = XLNX_BITSTREAM_SECTION_LENGTH(header);
	/* shift to the data section*/
	header += 0x2U;
	LOG_DBG("Part name = %s", header);

	header += length;

	if (*header++ != 'c') {
		LOG_ERR("Incorrect bitstream format");
		return NULL;
	}

	length = XLNX_BITSTREAM_SECTION_LENGTH(header);
	/* shift to the data section*/
	header += 0x2U;

	LOG_DBG("Date =  %s", header);

	header += length;

	if (*header++ != 'd') {
		LOG_ERR("Incorrect bitstream format");
		return NULL;
	}

	length = XLNX_BITSTREAM_SECTION_LENGTH(header);
	/* shift to the data section*/
	header += 0x2U;

	LOG_DBG("Time =  %s", header);

	header += length;

	if (*header++ != 'e') {
		LOG_ERR("Incorrect bitstream format");
		return NULL;
	}

	/*
	 * The last section is the raw bitstream.
	 * It is preceded by its size, which is needed for DMA transfer.
	 */
	*img_size =
		((uint32_t)*header << 24) | ((uint32_t) *(header + 1) << 16) |
		((uint32_t) *(header + 2) << 8) | ((uint32_t) *(header + 3));

	return (uint32_t *)header;
}

static int csudma_transfer(uint32_t size)
{
	/* setup the source DMA channel */
	CSUDMA_SRC_ADDR = (uint32_t)BITSTREAM & CSUDMA_SRC_ADDR_MASK;
	CSUDMA_SRC_ADDR_MSB = 0;
	CSUDMA_SRC_SIZE = size << CSUDMA_SRC_SIZE_SHIFT;

	/* wait for the SRC_DMA to complete */
	while ((CSUDMA_SRC_I_STS & CSUDMA_I_STS_DONE_MASK) != CSUDMA_I_STS_DONE_MASK) {
	};

	/* acknowledge the transfer has completed */
	CSUDMA_SRC_I_STS = CSUDMA_I_STS_DONE_MASK;

	return 0;
}

static int wait_for_done(void)
{
	/* wait for PCAP PL_DONE */
	while ((PCAP_STATUS & PCAP_PL_DONE_MASK) != PCAP_PL_DONE_MASK) {
	};

	PCAP_RESET = PCAP_RESET_MASK;
	power_up_fpga();

	return 0;
}

static enum FPGA_status zynqmp_fpga_get_status(const struct device *dev)
{
	ARG_UNUSED(dev);

	if ((PCAP_STATUS & PCAP_PL_INIT_MASK) && (PCAP_STATUS & PCAP_PL_DONE_MASK)) {
		return FPGA_STATUS_ACTIVE;
	} else {
		return FPGA_STATUS_INACTIVE;
	}
}

static const char *zynqmp_fpga_get_info(const struct device *dev)
{
	struct zynqmp_fpga_data *data = dev->data;

	return data->FPGA_info;
}

static int zynqmp_fpga_reset(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Reset PL */
	PCAP_PROG = PCAP_PROG_RESET_MASK;
	PCAP_PROG = ~PCAP_PROG_RESET_MASK;

	while ((PCAP_STATUS & PCAP_CFG_RESET) != PCAP_CFG_RESET) {
	};

	return 0;
}

static int init_pcap(const struct device *dev)
{
	/* take PCAP out of Reset */
	PCAP_RESET = ~PCAP_RESET_MASK;

	/* select PCAP mode and change PCAP to write mode */
	PCAP_CTRL = PCAP_PR_MASK;
	PCAP_RDWR = PCAP_WRITE_MASK;

	power_up_fpga();

	/* setup the SSS */
	CSU_SSS_CFG = PCAP_PCAP_SSS_MASK;

	zynqmp_fpga_reset(dev);

	/* wait for pl init */
	while ((PCAP_STATUS & PCAP_PL_INIT_MASK) != PCAP_PL_INIT_MASK) {
	};

	return 0;
}

static int zynqmp_fpga_load(const struct device *dev, uint32_t *image_ptr,
			    uint32_t img_size)
{
	uint32_t *addr = parse_header(dev, image_ptr, &img_size);

	if (addr == NULL) {
		LOG_ERR("Failed to read bitstream");
		return -EINVAL;
	}

	for (int i = 0; i < (img_size / 4); i++) {
		*(BITSTREAM + i) = __bswap_32(*(addr + i));
	}

	init_pcap(dev);
	csudma_transfer(img_size);
	wait_for_done();

	return 0;
}

static int zynqmp_fpga_init(const struct device *dev)
{
	/* turn on PCAP CLK */
	PCAP_CLK_CTRL = PCAP_CLK_CTRL | PCAP_CLKACT_MASK;

	update_part_name(dev);

	return 0;
}

static struct zynqmp_fpga_data fpga_data;

static const struct fpga_driver_api zynqmp_api = {
	.reset = zynqmp_fpga_reset,
	.load = zynqmp_fpga_load,
	.get_status = zynqmp_fpga_get_status,
	.get_info = zynqmp_fpga_get_info
};

DEVICE_DT_INST_DEFINE(0, &zynqmp_fpga_init, NULL, &fpga_data, NULL,
	      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &zynqmp_api);
