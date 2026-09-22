/*
 * Copyright (c) 2021-2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FPGA_ZYNQMP_H
#define ZEPHYR_DRIVERS_FPGA_ZYNQMP_H

#define PCAP_STATUS (*(volatile uint32_t *)             (0xFFCA3010))
#define PCAP_RESET (*(volatile uint32_t *)              (0xFFCA300C))
#define PCAP_CTRL (*(volatile uint32_t *)               (0xFFCA3008))
#define PCAP_RDWR (*(volatile uint32_t *)               (0xFFCA3004))
#define PMU_REQ_PWRUP_TRIG (*(volatile uint32_t *)      (0xFFD80120))
#define PCAP_PROG (*(volatile uint32_t *)               (0xFFCA3000))
#define CSU_SSS_CFG (*(volatile uint32_t *)             (0xFFCA0008))
#define CSUDMA_SRC_ADDR (*(volatile uint32_t *)         (0xFFC80000))
#define CSUDMA_SRC_SIZE (*(volatile uint32_t *)         (0xFFC80004))
#define CSUDMA_SRC_I_STS (*(volatile uint32_t *)        (0xFFC80014))
#define CSUDMA_SRC_ADDR_MSB (*(volatile uint32_t *)     (0xFFC80028))
#define PWR_STATUS (*(volatile uint32_t *)              (0xFFD80110))
#define PMU_GLOBAL_ISO_STATUS (*(volatile uint32_t *)   (0xFFD80310))
#define PMU_GLOBAL_PWRUP_EN (*(volatile uint32_t *)     (0xFFD80118))
#define PCAP_CLK_CTRL (*(volatile uint32_t *)           (0xFF5E00A4))
#define PMU_GLOBAL_ISO_INT_EN (*(volatile uint32_t *)   (0xFFD80318))
#define PMU_GLOBAL_ISO_TRIG (*(volatile uint32_t *)     (0xFFD80320))
#define IDCODE (*(volatile uint32_t *)                  (0xFFCA0040))
#define BITSTREAM ((volatile uint32_t *)                (0x01000000))

#define PWR_PL_MASK                 0x800000U
#define ISO_MASK                    0x4U
#define PCAP_RESET_MASK             0x1U
#define PCAP_PROG_RESET_MASK        0x0U
#define PCAP_PR_MASK                0x1U
#define PCAP_WRITE_MASK             0x0U
#define PCAP_PL_INIT_MASK           0x4U
#define PCAP_CLKACT_MASK            0x1000000U
#define PCAP_PCAP_SSS_MASK          0x5U
#define PCAP_PL_DONE_MASK           0x8U
#define PCAP_CFG_RESET              0x40U
#define CSUDMA_I_STS_DONE_MASK      0x2U
#define CSUDMA_SRC_ADDR_MASK        0xFFFFFFFCU
#define CSUDMA_SRC_SIZE_SHIFT       0x2U

#define IDCODE_MASK       0xFFFFFFF
#define ZU2_IDCODE        0x4711093
#define ZU3_IDCODE        0x4710093
#define ZU4_IDCODE        0x4721093
#define ZU5_IDCODE        0x4720093
#define ZU6_IDCODE        0x4739093
#define ZU7_IDCODE        0x4730093
#define ZU9_IDCODE        0x4738093
#define ZU11_IDCODE       0x4740093
#define ZU15_IDCODE       0x4750093
#define ZU17_IDCODE       0x4759093
#define ZU19_IDCODE       0x4758093
#define ZU21_IDCODE       0x47E1093
#define ZU25_IDCODE       0x47E5093
#define ZU27_IDCODE       0x47E4093
#define ZU28_IDCODE       0x47E0093
#define ZU29_IDCODE       0x47E2093
#define ZU39_IDCODE       0x47E6093
#define ZU43_IDCODE       0x47FD093
#define ZU46_IDCODE       0x47F8093
#define ZU47_IDCODE       0x47FF093
#define ZU48_IDCODE       0x47FB093
#define ZU49_IDCODE       0x47FE093

#define XLNX_BITSTREAM_SECTION_LENGTH(data) (*(data + 1) | *data << 0x8U);

#endif /* ZEPHYR_DRIVERS_FPGA_ZYNQMP_H */
