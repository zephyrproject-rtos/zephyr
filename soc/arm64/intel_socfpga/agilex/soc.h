/*
 * Copyright (c) 2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

/* FPGA config helpers */
#define INTEL_SIP_SMC_FPGA_CONFIG_ADDR		0x400000
#define INTEL_SIP_SMC_FPGA_CONFIG_SIZE		0x2000000

/* Register Mapping */
#define SOCFPGA_CCU_NOC_REG_BASE		0xf7000000

#define SOCFPGA_MMC_REG_BASE			0xff808000

#define SOCFPGA_RSTMGR_REG_BASE			0xffd11000
#define SOCFPGA_SYSMGR_REG_BASE			0xffd12000

#define SOCFPGA_PINMUX_PIN0SEL_REG_BASE		0xffd13000

#define SOCFPGA_L4_PER_SCR_REG_BASE             0xffd21000
#define SOCFPGA_L4_SYS_SCR_REG_BASE             0xffd21100
#define SOCFPGA_SOC2FPGA_SCR_REG_BASE           0xffd21200
#define SOCFPGA_LWSOC2FPGA_SCR_REG_BASE         0xffd21300

#endif /* _SOC_H_ */
