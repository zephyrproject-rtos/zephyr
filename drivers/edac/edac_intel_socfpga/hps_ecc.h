/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_EDAC_EDAC_INTEL_SOCFPGA_HPS_ECC_H_
#define ZEPHYR_INCLUDE_DRIVERS_EDAC_EDAC_INTEL_SOCFPGA_HPS_ECC_H_

/* ECC module IDs*/
#define ECC_OCRAM                        (1u)
#define ECC_USB0_RAM0                    (2u)
#define ECC_USB1_RAM0                    (3u)
#define ECC_EMAC0_RX                     (4u)
#define ECC_EMAC0_TX                     (5u)
#define ECC_EMAC1_RX                     (6u)
#define ECC_EMAC1_TX                     (7u)
#define ECC_EMAC2_RX                     (8u)
#define ECC_EMAC2_TX                     (9u)
#define ECC_DMA0                         (10u)
#define ECC_USB1_RAM1                    (11u)
#define ECC_USB1_RAM2                    (12u)
#define ECC_NAND                         (13u)
#define ECC_SDMMCA                       (13u)
#define ECC_SDMMCB                       (14u)
#define ECC_DMA1                         (18u)
#define ECC_MODULE_SYSMNGR_MAX_INSTANCES (ECC_DMA1)
#define ECC_QSPI                         (19u)
#define ECC_MODULE_MAX_INSTANCES         (ECC_QSPI)

/**
 * @brief ECC_MODULE_INSTANCES_MSK used for masking those ECC modules connected to system manager
 * while performing register read write operations.
 *
 * bit  [0] : Reserved
 *      [1] : OCRAM
 *      [2] : USB0 RAM 0
 *      [3] : USB1 RAM 0
 *      [4] : EMAC 0 RX
 *      [5] : EMAC 0 TX
 *      [6] : EMAC 1 RX
 *      [7] : EMAC 1 TX
 *      [8] : EMAC 2 RX
 *      [9] : EMAC 2 TX
 *      [10] : DMA 0
 *      [11] : USB1 RAM1
 *      [12] : USB1 RAM2
 *      [13] : NAND
 *      [14] : SDMMC A
 *      [15] : SDMMC B
 *      [16] : DDR 0 (not handled)
 *      [17] : DDR 1 (not handled)
 *      [18] : DMA 1
 *      [31:19] : Reserved
 */
#define SYSMNGR_ECC_MODULE_INSTANCES_MSK (0x4FFFE)
#endif /* ZEPHYR_INCLUDE_DRIVERS_EDAC_EDAC_INTEL_SOCFPGA_HPS_ECC_H_ */
