/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2023, Intel Corporation
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_INTEL_SOCFPGA_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_INTEL_SOCFPGA_RESET_H_

/* The Reset line value will be used by the reset controller driver to
 * derive the register offset and the associated device bit to perform
 * device assert and de-assert.
 *
 * The reset lines should be passed as a parameter to the resets property
 * of the driver node in dtsi which will call reset-controller driver to
 * assert/de-assert itself.
 *
 * Example: Deriving Reset Line value
 * per0modrst register offset = 0x24;
 * NAND RSTLINE pin = 5;
 * RSTMGR_NAND_RSTLINE = (0x24 * 8) + 5 = 293
 */

#define RSTMGR_SDMCOLDRST_RSTLINE           0
#define RSTMGR_SDMWARMRST_RSTLINE           1
#define RSTMGR_SDMLASTPORRST_RSTLINE        2
#define RSTMGR_L4WD0RST_RSTLINE             16
#define RSTMGR_L4WD1RST_RSTLINE             17
#define RSTMGR_L4WD2RST_RSTLINE             18
#define RSTMGR_L4WD3RST_RSTLINE             19
#define RSTMGR_L4WD4RST_RSTLINE             20
#define RSTMGR_DEBUGRST_RSTLINE             21
#define RSTMGR_CSDAPRST_RSTLINE             22
#define RSTMGR_EMIFTIMEOUT_RSTLINE          64
#define RSTMGR_FPGAHSTIMEOUT_RSTLINE        66
#define RSTMGR_ETRSTALLTIMEOUT_RSTLINE      67
#define RSTMGR_LWSOC2FPGATIMEOUT_RSTLINE    72
#define RSTMGR_SOC2FPGATIMEOUT_RSTLINE      73
#define RSTMGR_F2SDRAMTIMEOUT_RSTLINE       74
#define RSTMGR_F2STIMEOUT_RSTLINE           75
#define RSTMGR_L3NOCDBGTIMEOUT_RSTLINE      79
#define RSTMGR_DEBUGL3NOCTIMEOUT_RSTLINE    80
#define RSTMGR_EMIF_FLUSH_RSTLINE           128
#define RSTMGR_FPGAHSEN_RSTLINE             130
#define RSTMGR_ETRSTALLEN_RSTLINE           131
#define RSTMGR_LWSOC2FPGA_FLUSH_RSTLINE     137
#define RSTMGR_SOC2FPGA_FLUSH_RSTLINE       138
#define RSTMGR_F2SDRAM_FLUSH_RSTLINE        139
#define RSTMGR_F2SOC_FLUSH_RSTLINE          140
#define RSTMGR_L3NOC_DBG_RSTLINE            144
#define RSTMGR_DEBUG_L3NOC_RSTLINE          145
#define RSTMGR_EMIF_FLUSH_REQ_RSTLINE       160
#define RSTMGR_FPGAHSREQ_RSTLINE            162
#define RSTMGR_ETRSTALLREQ_RSTLINE          163
#define RSTMGR_LWSOC2FPGA_FLUSH_REQ_RSTLINE 169
#define RSTMGR_SOC2FPGA_FLUSH_REQ_RSTLINE   170
#define RSTMGR_F2SDRAM_FLUSH_REQ_RSTLINE    171
#define RSTMGR_F2S_FLUSH_REQ_RSTLINE        172
#define RSTMGR_L3NOC_DBG_REQ_RSTLINE        176
#define RSTMGR_DEBUG_L3NOC_REQ_RSTLINE      177
#define RSTMGR_EMIF_FLUSH_ACK_RSTLINE       192
#define RSTMGR_FPGAHSACK_RSTLINE            194
#define RSTMGR_ETRSTALLACK_RSTLINE          195
#define RSTMGR_LWSOC2FPGA_FLUSH_ACK_RSTLINE 201
#define RSTMGR_SOC2FPGA_FLUSH_ACK_RSTLINE   202
#define RSTMGR_F2SDRAM_FLUSH_ACK_RSTLINE    203
#define RSTMGR_F2S_FLUSH_ACK_RSTLINE        204
#define RSTMGR_L3NOC_DBG_ACK_RSTLINE        208
#define RSTMGR_DEBUG_L3NOC_ACK_RSTLINE      209
#define RSTMGR_ETRSTALLWARMRST_RSTLINE      224
#define RSTMGR_TSN0_RSTLINE                 288
#define RSTMGR_TSN1_RSTLINE                 289
#define RSTMGR_TSN2_RSTLINE                 290
#define RSTMGR_USB0_RSTLINE                 291
#define RSTMGR_USB1_RSTLINE                 292
#define RSTMGR_NAND_RSTLINE                 293
#define RSTMGR_SOFTPHY_RSTLINE              294
#define RSTMGR_SDMMC_RSTLINE                295
#define RSTMGR_TSN0ECC_RSTLINE              296
#define RSTMGR_TSN1ECC_RSTLINE              297
#define RSTMGR_TSN2ECC_RSTLINE              298
#define RSTMGR_USB0ECC_RSTLINE              299
#define RSTMGR_USB1ECC_RSTLINE              300
#define RSTMGR_NANDECC_RSTLINE              301
#define RSTMGR_SDMMCECC_RSTLINE             303
#define RSTMGR_DMA_RSTLINE                  304
#define RSTMGR_SPIM0_RSTLINE                305
#define RSTMGR_SPIM1_RSTLINE                306
#define RSTMGR_SPIS0_RSTLINE                307
#define RSTMGR_SPIS1_RSTLINE                308
#define RSTMGR_DMAECC_RSTLINE               309
#define RSTMGR_EMACPTP_RSTLINE              310
#define RSTMGR_DMAIF0_RSTLINE               312
#define RSTMGR_DMAIF1_RSTLINE               313
#define RSTMGR_DMAIF2_RSTLINE               314
#define RSTMGR_DMAIF3_RSTLINE               315
#define RSTMGR_DMAIF4_RSTLINE               316
#define RSTMGR_DMAIF5_RSTLINE               317
#define RSTMGR_DMAIF6_RSTLINE               318
#define RSTMGR_DMAIF7_RSTLINE               319
#define RSTMGR_WATCHDOG0_RSTLINE            320
#define RSTMGR_WATCHDOG1_RSTLINE            321
#define RSTMGR_WATCHDOG2_RSTLINE            322
#define RSTMGR_WATCHDOG3_RSTLINE            323
#define RSTMGR_L4SYSTIMER0_RSTLINE          324
#define RSTMGR_L4SYSTIMER1_RSTLINE          325
#define RSTMGR_SPTIMER0_RSTLINE             326
#define RSTMGR_SPTIMER1_RSTLINE             327
#define RSTMGR_I2C0_RSTLINE                 328
#define RSTMGR_I2C1_RSTLINE                 329
#define RSTMGR_I2C2_RSTLINE                 330
#define RSTMGR_I2C3_RSTLINE                 331
#define RSTMGR_I2C4_RSTLINE                 332
#define RSTMGR_I3C0_RSTLINE                 333
#define RSTMGR_I3C1_RSTLINE                 334
#define RSTMGR_UART0_RSTLINE                336
#define RSTMGR_UART1_RSTLINE                337
#define RSTMGR_GPIO0_RSTLINE                344
#define RSTMGR_GPIO1_RSTLINE                345
#define RSTMGR_WATCHDOG4_RSTLINE            346
#define RSTMGR_SOC2FPGA_RSTLINE             352
#define RSTMGR_LWSOC2FPGA_RSTLINE           353
#define RSTMGR_FPGA2SOC_RSTLINE             354
#define RSTMGR_FPGA2SDRAM_RSTLINE           355
#define RSTMGR_MPFE_RSTLINE                 358
#define RSTMGR_DBG_RST_RSTLINE              480
#define RSTMGR_SOC2FPGA_WARM_RSTLINE        608
#define RSTMGR_LWSOC2FPGA_WARM_RSTLINE      609
#define RSTMGR_FPGA2SOC_WARM_RSTLINE        610
#define RSTMGR_FPGA2SDRAM_WARM_RSTLINE      611
#define RSTMGR_MPFE_WARM_RSTLINE            614

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_INTEL_SOCFPGA_RESET_H_ */
