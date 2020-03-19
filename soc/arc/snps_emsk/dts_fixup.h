/* SPDX-License-Identifier: Apache-2.0 */

/* SoC level DTS fixup file */

/* CCM configuration */
#define DT_DCCM_BASE_ADDRESS		DT_INST_0_ARC_DCCM_BASE_ADDRESS
#define DT_DCCM_SIZE			(DT_INST_0_ARC_DCCM_SIZE >> 10)

#define DT_ICCM_BASE_ADDRESS		DT_INST_0_ARC_ICCM_BASE_ADDRESS
#define DT_ICCM_SIZE			(DT_INST_0_ARC_ICCM_SIZE >> 10)

#define DT_DDR_BASE_ADDRESS		DT_MMIO_SRAM_10000000_BASE_ADDRESS
#define DT_DDR_SIZE			(DT_MMIO_SRAM_10000000_SIZE >> 10)

/* End of SoC Level DTS fixup file */
