/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * eSPI Target Attached Flash Sharing (TAFS)
 * Version 1.5 hardware
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ESPI_MEC_ESPI_TAF_REGS_H_
#define ZEPHYR_INCLUDE_DRIVERS_ESPI_MEC_ESPI_TAF_REGS_H_

#include <zephyr/sys/util.h> /* GENMASK, BIT, FIELD_PREP, FIELD_GET */

/* Common helpers */
#define XEC_TAFS_REG(base, off) ((base) + (off))

/*
 * Register spaces:
 * - TAF Communication Space (EC-only): base provided by SoC integration
 * - TAF Bridge Space (EC-only): base provided by SoC integration
 *
 * This header provides offsets relative to each base.
 */

/* -----------------------------------------------------------------------------
 * TAF Communication Space (EC-only)
 * -----------------------------------------------------------------------------
 */

/* TAF Communication Mode Register (taf_mode) */
#define XEC_TAFS_COMM_MODE_OFS         0x2B8u
#define XEC_TAFS_COMM_MODE_PREFETCH_EN BIT(0)
/* Bits[31:1] are TEST/diagnostic; preserve with RMW */

/* -----------------------------------------------------------------------------
 * TAF Bridge Space (EC-only) - Flash device configuration
 * -----------------------------------------------------------------------------
 */

#define XEC_TAFS_MAX_CHIP_SELECTS  2U
#define XEC_TAFS_MAX_FLASH_DEVICES 2U

/* taf_config_size */
#define XEC_TAFS_CFG_SIZE_LIM_OFS 0x030U
#define XEC_TAFS_CFG_SIZE_LIM_MSK GENMASK(31, 0)

/* taf_config_threshold */
#define XEC_TAFS_CFG_THRLD_OFS 0x034U
#define XEC_TAFS_CFG_THRLD_MSK GENMASK(31, 0)

/* taf_config_misc */
#define XEC_TAFS_CFG_MISC_OFS 0x038U

#define XEC_TAFS_CFG_MISC_CS1_RPMC_SUSP_MODE_POS 26
#define XEC_TAFS_CFG_MISC_CS0_RPMC_SUSP_MODE_POS 25
#define XEC_TAFS_CFG_MISC_FORCE_RPMC_SUCCESS_POS 24

#define XEC_TAFS_CFG_MISC_CNT_RLD_EC0_EN_POS  21
#define XEC_TAFS_CFG_MISC_CNT_RLD_ESPI_EN_POS 20

#define XEC_TAFS_CFG_MISC_LOW_PWR_SAC_EN_POS  17
#define XEC_TAFS_CFG_MISC_LOW_PWR_HSLP_EN_POS 16
#define XEC_TAFS_CFG_MISC_LOW_PWR_LSLP_EN_POS 15

#define XEC_TAFS_CFG_MISC_TAF_MODE_LOCK_POS 13
#define XEC_TAFS_CFG_MISC_TAF_MODE_EN_POS   12

#define XEC_TAFS_CFG_MISC_CS1_CONT_PREFIX_EN_POS 7
#define XEC_TAFS_CFG_MISC_CS0_CONT_PREFIX_EN_POS 6

#define XEC_TAFS_CFG_MISC_CS1_4B_ADDR_MODE_POS 5
#define XEC_TAFS_CFG_MISC_CS0_4B_ADDR_MODE_POS 4

/* Bits[3:2] reserved: write 0 */
#define XEC_TAFS_CFG_MISC_PREFETCH_OPT_EN_MSK   GENMASK(1, 0)
#define XEC_TAFS_CFG_MISC_PREFETCH_OPT_CANON    0U
#define XEC_TAFS_CFG_MISC_PREFETCH_OPT_EXPEDITE 3U
#define XEC_TAFS_CFG_MISC_PREFETCH_OPT_SET(p) FIELD_PREP(XEC_TAFS_CFG_MISC_PREFETCH_OPT_EN_MSK, (p))
#define XEC_TAFS_CFG_MISC_PREFETCH_OPT_GET(r) FIELD_GET(XEC_TAFS_CFG_MISC_PREFETCH_OPT_EN_MSK, (r))

/* TAF Chip select 0 and 1 Opcode registers */
#define XEC_TAFS_CFG_CS0_OPCODE_A_OFS 0x04CU
#define XEC_TAFS_CFG_CS1_OPCODE_A_OFS 0x05CU

#define XEC_TAFS_CFG_OPCODE_A_OP_POLL1_MSK GENMASK(31, 24)
#define XEC_TAFS_CFG_OPCODE_A_OP_RSM_MSK   GENMASK(23, 16)
#define XEC_TAFS_CFG_OPCODE_A_OP_SUS_MSK   GENMASK(15, 8)
#define XEC_TAFS_CFG_OPCODE_A_OP_WE_MSK    GENMASK(7, 0)

/* taf_config_cs0_opcode_b / cs1 */
#define XEC_TAFS_CFG_CS0_OPCODE_B_OFS 0x050U
#define XEC_TAFS_CFG_CS1_OPCODE_B_OFS 0x060U

#define XEC_TAFS_CFG_OPCODE_B_OP_PROG_MSK   GENMASK(31, 24)
#define XEC_TAFS_CFG_OPCODE_B_OP_ERASE2_MSK GENMASK(23, 16)
#define XEC_TAFS_CFG_OPCODE_B_OP_ERASE1_MSK GENMASK(15, 8)
#define XEC_TAFS_CFG_OPCODE_B_OP_ERASE0_MSK GENMASK(7, 0)

/* taf_config_cs0_opcode_c / cs1 */
#define XEC_TAFS_CFG_CS0_OPCODE_C_OFS 0x054U
#define XEC_TAFS_CFG_CS1_OPCODE_C_OFS 0x064U

#define XEC_TAFS_CFG_OPCODE_C_OP_POLL2_MSK  GENMASK(31, 24)
#define XEC_TAFS_CFG_OPCODE_C_MODE_CONT_MSK GENMASK(23, 16)
#define XEC_TAFS_CFG_OPCODE_C_MODE_NONC_MSK GENMASK(15, 8)
#define XEC_TAFS_CFG_OPCODE_C_OP_READ_MSK   GENMASK(7, 0)

/* taf_config_cs0_opcode_d / cs1 */
#define XEC_TAFS_CFG_CS0_OPCODE_D_OFS 0x1C4U
#define XEC_TAFS_CFG_CS1_OPCODE_D_OFS 0x1C8U

#define XEC_TAFS_CFG_OPCODE_D_OP_RPMC_OP2_MSK GENMASK(23, 16)
#define XEC_TAFS_CFG_OPCODE_D_OP_EXIT_PD_MSK  GENMASK(15, 8)
#define XEC_TAFS_CFG_OPCODE_D_OP_ENTER_PD_MSK GENMASK(7, 0)

/* taf_config_cs0_descriptors / cs1 */
#define XEC_TAFS_CFG_CS0_DESCR_OFS 0x058U
#define XEC_TAFS_CFG_CS1_DESCR_OFS 0x068U

#define XEC_TAFS_CFG_PER_DESCR_SIZE_CONT_MSK  GENMASK(15, 12)
#define XEC_TAFS_CFG_PER_DESCR_READ_CONT_MSK  GENMASK(11, 8)
#define XEC_TAFS_CFG_PER_DESCR_ENTER_CONT_MSK GENMASK(3, 0)

/* TAF config general descriptors */
#define XEC_TAFS_CFG_GD_OFS          0x06CU
#define XEC_TAFS_CFG_GD_EXCM_POS     0
#define XEC_TAFS_CFG_GD_EXCM_MSK     GENMASK(3, 0)
#define XEC_TAFS_CFG_GD_EXCM_SET(d)  FIELD_PREP(XEC_TAFS_CFG_GD_EXCM_MSK, (d))
#define XEC_TAFS_CFG_GD_EXCM_GET(r)  FIELD_GET(XEC_TAFS_CFG_GD_EXCM_MSK, (r))
#define XEC_TAFS_CFG_GD_EXCM_STD_VAL 12U

#define XEC_TAFS_CFG_GD_POLL1_POS     8
#define XEC_TAFS_CFG_GD_POLL1_MSK     GENMASK(11, 8)
#define XEC_TAFS_CFG_GD_POLL1_SET(d)  FIELD_PREP(XEC_TAFS_CFG_GD_POLL1_MSK, (d))
#define XEC_TAFS_CFG_GD_POLL1_GET(r)  FIELD_GET(XEC_TAFS_CFG_GD_POLL1_MSK, (r))
#define XEC_TAFG_CFG_GD_POLL1_STD_VAL 14U

#define XEC_TAFS_CFG_GD_POLL2_POS     12
#define XEC_TAFS_CFG_GD_POLL2_MSK     GENMASK(15, 12)
#define XEC_TAFS_CFG_GD_POLL2_SET(d)  FIELD_PREP(XEC_TAFS_CFG_GD_POLL2_MSK, (d))
#define XEC_TAFS_CFG_GD_POLL2_GET(r)  FIELD_GET(XEC_TAFS_CFG_GD_POLL2_MSK, (r))
#define XEC_TAFS_CFG_GD_POLL2_STD_VAL 14U

/* taf_config_cs_poll2_mask */
#define XEC_TAFS_CFG_CS_POLL2_MASK_OFS     0x1A4U
#define XEC_TAFS_CFG_CS_POLL2_MASK_CS1_MSK GENMASK(31, 16)
#define XEC_TAFS_CFG_CS_POLL2_MASK_CS0_MSK GENMASK(15, 0)

/* taf_test_reg_control */
#define XEC_TAFS_TEST_REG_CR_OFS                0x1A8U
#define XEC_TAFS_TEST_REG_CR_DISABLE_SUS_WR_CS1 BIT(3)
#define XEC_TAFS_TEST_REG_CR_DISABLE_SUS_WR_CS0 BIT(2)
#define XEC_TAFS_TEST_REG_CR_DISABLE_SUS_ER_CS1 BIT(1)
#define XEC_TAFS_TEST_REG_CR_DISABLE_SUS_ER_CS0 BIT(0)

/* taf_continuous_prefix_config */
#define XEC_TAFS_CONT_PREFIX_CFG_OFS     0x1B0U
#define XEC_TAFS_CONT_PREFIX_CS1_DAT_MSK GENMASK(31, 24)
#define XEC_TAFS_CONT_PREFIX_CS1_OP_MSK  GENMASK(23, 16)
#define XEC_TAFS_CONT_PREFIX_CS0_DAT_MSK GENMASK(15, 8)
#define XEC_TAFS_CONT_PREFIX_CS0_OP_MSK  GENMASK(7, 0)

/* Activity counter reload */
#define XEC_TAFS_ACTV_CNTR_RELOAD_OFS 0x1B8U
#define XEC_TAFS_ACTV_CNTR_RELOAD_TMO GENMASK(15, 0)

/* Flash power-down control/status */
#define XEC_TAFS_PD_CR_OFS                      0x1BCU
#define XEC_TAFS_PD_CR_CS1_WK_ON_EC_ACTV_EN_POS 3
#define XEC_TAFS_PD_CR_CS0_WK_ON_EC_ACTV_EN_POS 2
#define XEC_TAFS_PD_CR_CS1_PD_EN_POS            1
#define XEC_TAFS_DP_CR_CS0_PD_EN_POS            0

#define XEC_TAFS_PD_SR_OFS                0x1C0U
#define XEC_TAFS_PD_SR_CS1_POWER_DOWN_STS BIT(1)
#define XEC_TAFS_PD_SR_CS0_POWER_DOWN_STS BIT(0)

/* Power down/up minimum interval */
#define XEC_TAFS_PD_TMO_POWER_DOWN_UP_OFS     0x1CCU
#define XEC_TAFS_PD_TMO_POWER_DOWN_UP_MIN_MSK GENMASK(15, 0)

/* Per-flash clock divider */
#define XEC_TAFS_QSPI_CLK_DIV_CS0_OFS  0x200U
#define XEC_TAFS_QSPI_CLK_DIV_CS1_OFS  0x204U
#define XEC_TAFS_QSPI_CLK_DIV_REST_MSK GENMASK(31, 16)
#define XEC_TAFS_QSPI_CLK_DIV_READ_MSK GENMASK(15, 0)

/* -----------------------------------------------------------------------------
 * TAF Bridge Space (EC-only) - Timing registers
 * -----------------------------------------------------------------------------
 */

#define XEC_TAFS_TMO_TOTAL_POLL_OFS     0x194U
#define XEC_TAFS_TMO_TOTAL_POLL_VAL_MSK GENMASK(17, 0)

#define XEC_TAFS_TMO_POLL_HOLDOFF_OFS     0x198U
#define XEC_TAFS_TMO_POLL_HOLDOFF_VAL_MSK GENMASK(15, 0)

#define XEC_TAFS_TMO_RESUSPEND_OFS     0x19CU
#define XEC_TAFS_TMO_RESUSPEND_VAL_MSK GENMASK(15, 0)

#define XEC_TAFS_TMO_READ_WHILE_SUSP_OFS     0x1A0U
#define XEC_TAFS_TMO_READ_WHILE_SUSP_VAL_MSK GENMASK(19, 0)

#define XEC_TAFS_TMO_DELAY_SUSP_CHECK_OFS     0x1ACU
#define XEC_TAFS_TMO_DELAY_SUSP_CHECK_VAL_MSK GENMASK(21, 0)

/* -----------------------------------------------------------------------------
 * TAF Bridge Space (EC-only) - EC portal
 * -----------------------------------------------------------------------------
 */

/* TAF EC Portal command register */
#define XEC_TAFS_ECP_CMD_OFS          0x018U
#define XEC_TAFS_ECP_CMD_PUT_POS      0
#define XEC_TAFS_ECP_CMD_PUT_MSK      GENMASK(7, 0)
#define XEC_TAFS_ECP_CMD_PUT_FLASH_NP 0x0A
#define XEC_TAFS_ECP_CMD_PUT_SET(c)   FIELD_PREP(XEC_TAFS_BR_EC_CMD_PUT_CMD_MSK, (c))
#define XEC_TAFS_ECP_CMD_PUT_GET(r)   FIELD_GET(XEC_TAFS_BR_EC_CMD_PUT_CMD_MSK, (r))

/* The flash command type to perform */
#define XEC_TAFS_ECP_CMD_CTYPE_POS    8
#define XEC_TAFS_ECP_CMD_CTYPE_MSK    GENMASK(15, 8)
#define XEC_TAFS_ECP_CMD_CTYPE_SET(c) FIELD_PREP(XEC_TAFS_ECP_CTYPE_CMD_MSK, (c))
#define XEC_TAFS_ECP_CMD_CTYPE_GET(r) FIELD_GET(XEC_TAFS_ECP_CTYPE_CMD_MSK, (r))

/* CTYPE command values */
#define XEC_TAFS_ECP_CTYPE_READ         0x00U
#define XEC_TAFS_ECP_CTYPE_WRITE        0x01U
#define XEC_TAFS_ECP_CTYPE_ERASE        0x02U
#define XEC_TAFS_ECP_CTYPE_RPMC_OP1_CS0 0x03U
#define XEC_TAFS_ECP_CTYPE_RPMC_OP2_CS0 0x04U
#define XEC_TAFS_ECP_CTYPE_RPMC_OP1_CS1 0x23U
#define XEC_TAFS_ECP_CTYPE_RPMC_OP2_CS1 0x24U

#define XEC_TAFS_ECP_CMD_LEN_POS        24
#define XEC_TAFS_ECP_CMD_LEN_MSK        GENMASK(31, 24)
#define XEC_TAFS_ECP_CMD_LEN_ERASE_4KB  0
#define XEC_TAFS_ECP_CMD_LEN_ERASE_32KB 1U
#define XEC_TAFS_ECP_CMD_LEN_ERASE_64KB 2U

/* TAF EC Portal flash address register */
#define XEC_TAFS_ECP_FLASH_ADDR_OFS 0x01CU
#define XEC_TAFS_ECP_FLASH_ADDR_MSK GENMASK(31, 0)

/* TAF EC Portal Start register */
#define XEC_TAFS_ECP_START_OFS    0x020U
#define XEC_TAFS_ECP_START_EN_POS 0

/* TAF EC Portal memory buffer address */
#define XEC_TAFS_ECP_MBUF_ADDR_OFS     0x024U
#define XEC_TAFS_ECP_MBUF_ADDR_BUF_MSK GENMASK(31, 2)
/* address must be aligned on a 4-byte or greater boundary */

/* TAF EC Portal Status register implemented bits are read-write-1-to-clear */
#define XEC_TAFS_ECP_SR_OFS            0x028U
#define XEC_TAFS_ECP_SR_BAD_REQ_POS    8
#define XEC_TAFS_ECP_SR_START_OVFL_POS 7
#define XEC_TAFS_ECP_SR_ERSZ_POS       6
#define XEC_TAFS_ECP_SR_4KBND_POS      5
#define XEC_TAFS_ECP_SR_AV_POS         4
#define XEC_TAFS_ECP_SR_OOR_POS        3
#define XEC_TAFS_ECP_SR_TMO_POS        2
#define XEC_TAFS_ECP_SR_DONE_TST_POS   1
#define XEC_TAFS_ECP_SR_DONE_POS       0
#define XEC_TAFS_ECP_SR_MSK            GENMASK(8, 0)
#define XEC_TAFS_ECP_SR_ERR_MSK        GENMASK(8, 2)

/* TAF EC Portal Interrupt Enable register */
#define XEC_TAFS_ECP_IER_OFS         0x02CU
#define XEC_TAFS_ECP_IER_DONE_EN_POS 0

/* TAF EC Portal Busy register */
#define XEC_TAFS_ECP_BUSY_OFS     0x044U
#define XEC_TAFS_ECP_BUSY_EC0_POS 0

/* -----------------------------------------------------------------------------
 * TAF Bridge Space (EC-only) - Protection
 * -----------------------------------------------------------------------------
 */

/* Tag maps */
#define XEC_TAFS_TAG_MAP0_OFS 0x078U
#define XEC_TAFS_TAG_MAP1_OFS 0x07CU
#define XEC_TAFS_TAG_MAP2_OFS 0x080U

/* Each tag field is 3 bits with 1-bit reserved spacing as shown */
#define XEC_TAFS_TAGMAP_TM0_MSK GENMASK(2, 0)
#define XEC_TAFS_TAGMAP_TM1_MSK GENMASK(6, 4)
#define XEC_TAFS_TAGMAP_TM2_MSK GENMASK(10, 8)
#define XEC_TAFS_TAGMAP_TM3_MSK GENMASK(14, 12)
#define XEC_TAFS_TAGMAP_TM4_MSK GENMASK(18, 16)
#define XEC_TAFS_TAGMAP_TM5_MSK GENMASK(22, 20)
#define XEC_TAFS_TAGMAP_TM6_MSK GENMASK(26, 24)
#define XEC_TAFS_TAGMAP_TM7_MSK GENMASK(30, 28)

#define XEC_TAFS_TAGMAP_TM8_MSK GENMASK(2, 0)
#define XEC_TAFS_TAGMAP_TM9_MSK GENMASK(6, 4)
#define XEC_TAFS_TAGMAP_TMA_MSK GENMASK(10, 8)
#define XEC_TAFS_TAGMAP_TMB_MSK GENMASK(14, 12)
#define XEC_TAFS_TAGMAP_TMC_MSK GENMASK(18, 16)
#define XEC_TAFS_TAGMAP_TMD_MSK GENMASK(22, 20)
#define XEC_TAFS_TAGMAP_TME_MSK GENMASK(26, 24)
#define XEC_TAFS_TAGMAP_TMF_MSK GENMASK(30, 28)

#define XEC_TAFS_TAG_MAP2_TM_LK     BIT(31)
#define XEC_TAFS_TAG_MAP2_TM_EC_MSK GENMASK(2, 0)

/* Protection lock/dirty */
#define XEC_TAFS_PROT_LOCK_OFS  0x070U
#define XEC_TAFS_PROT_DIRTY_OFS 0x074U

#define XEC_TAFS_PROT_LOCK_RR(rr)  BIT(rr)
/* Dirty bits exit only for regions 0..11 */
#define XEC_TAFS_PROT_DIRTY_RR(rr) BIT(rr)

/* DnX protection bypass */
#define XEC_TAFS_DNX_PROT_BYP_OFS      0x1B4U
#define XEC_TAFS_DNX_PROT_BYP_DNX_LK   BIT(28)
#define XEC_TAFS_DNX_PROT_BYP_DNX_DM   BIT(24)
#define XEC_TAFS_DNX_PROT_BYP_DNX_DS   BIT(20)
#define XEC_TAFS_DNX_PROT_BYP_TAG(tag) BIT(tag) /* tag 0..15 */

/* TAF external Controller IDs */
#define XEC_TAF_CID_CS_INIT_POS 0
#define XEC_TAF_CID_CPU_POS     1U
#define XEC_TAF_CID_CS_ME_POS   2U
#define XEC_TAF_CID_CS_LAN_POS  3U
#define XEC_TAF_CID_UNUSED4_POS 4U
#define XEC_TAF_CID_EC_FW_POS   5U
#define XEC_TAF_CID_CS_IE_POS   6U
#define XEC_TAF_CID_UNUSED7_POS 7U
#define XEC_TAF_CID_MAX_POS     8U
#define XEC_TAF_CID_ALL_MSK     0xffU

/* Protection region register set addressing (RR=0..16) */
#define XEC_TAFS_PROT_RG_MAX_REGIONS   17U
#define XEC_TAFS_PROT_RG_START_OFS(rr) (0x084U + ((uint32_t)(rr) * 0x10U))
#define XEC_TAFS_PROT_RG_LIMIT_OFS(rr) (0x088U + ((uint32_t)(rr) * 0x10U))
#define XEC_TAFS_PROT_RG_WR_OFS(rr)    (0x08CU + ((uint32_t)(rr) * 0x10U))
#define XEC_TAFS_PROT_RG_RD_OFS(rr)    (0x090U + ((uint32_t)(rr) * 0x10U))

#define XEC_TAFS_PROT_RG_START_MSK GENMASK(19, 0)
#define XEC_TAFS_PROT_RG_LIMIT_MSK GENMASK(19, 0)

#define XEC_TAF_PROT_RG_START_DFLT 0x07ffffU
#define XEC_TAF_PROT_RG_LIMIT_DFLT 0

/* WR/RD bitmaps: bits [7:1] R/W; bit[0] is read-only 1 (Master0) */
#define XEC_TAFS_PROT_BITMAP_M0   BIT(0)
#define XEC_TAFS_PROT_BITMAP_M(m) BIT(m) /* m=1..7 */

/* -----------------------------------------------------------------------------
 * TAF Bridge Space (EC-only) - eSPI bus error monitoring
 * -----------------------------------------------------------------------------
 */

#define XEC_TAFS_ESPI_BMON_SR_OFS  0x03CU
#define XEC_TAFS_ESPI_BMON_IER_OFS 0x040U

#define XEC_TAFS_ESPI_ERR_TMO_POS   0
#define XEC_TAFS_ESPI_ERR_OOR_POS   1
#define XEC_TAFS_ESPI_ERR_AV_POS    2
#define XEC_TAFS_ESPI_ERR_4KBND_POS 3
#define XEC_TAFS_ESPI_ERR_ERSZ_POS  4
#define XEC_TAFS_ESPI_ERR_MSK       GENMASK(4, 0)

/* -----------------------------------------------------------------------------
 * TAF Bridge Space (EC-only) - RPMC result buffer addresses
 * -----------------------------------------------------------------------------
 */

#define XEC_TAFS_RPMC_OP2_ESPI_RESULT_ADDR_OFS 0x208U
#define XEC_TAFS_RPMC_OP2_EC0_RESULT_ADDR_OFS  0x20CU
#define XEC_TAFS_RPMC_RESULT_ADDR              GENMASK(31, 0)

/* -----------------------------------------------------------------------------
 * Convenience inline helpers (optional usage)
 * -----------------------------------------------------------------------------
 */

static inline uint32_t xec_tafs_field_prep(uint32_t mask, uint32_t val)
{
	return FIELD_PREP(mask, val);
}

static inline uint32_t xec_tafs_field_get(uint32_t mask, uint32_t reg)
{
	return FIELD_GET(mask, reg);
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_ESPI_MEC_ESPI_TAF_REGS_H_ */
