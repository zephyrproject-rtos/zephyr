/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl808_psram_uhs_controller

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/cache.h>
LOG_MODULE_REGISTER(memc_bflb_bl808_psram_uhs, CONFIG_MEMC_LOG_LEVEL);

#include <bouffalolab/bl808/bflb_soc.h>
#include <bouffalolab/bl808/glb_reg.h>
#include <bouffalolab/bl808/psram_uhs_reg.h>
#include <bouffalolab/bl808/tzc_sec_reg.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1, "Only one PSRAM instance supported");

#define UHS_BASE DT_INST_REG_ADDR(0)

#define BFLB_PSRAM_MEM_NODE  DT_INST(0, bflb_bl808_psram_uhs)
#define BFLB_PSRAM_MEM_BASE  DT_REG_ADDR(BFLB_PSRAM_MEM_NODE)
#define BFLB_PSRAM_MEM_SIZE  DT_REG_SIZE(BFLB_PSRAM_MEM_NODE)
#define BFLB_PSRAM_PAGE_SIZE DT_PROP(BFLB_PSRAM_MEM_NODE, page_size)
#define BFLB_PSRAM_HIGH_TEMP DT_PROP(BFLB_PSRAM_MEM_NODE, high_temp)
#define BFLB_PSRAM_IS_64MB   (BFLB_PSRAM_MEM_SIZE > MB(32))

/* Supported PCK range (MHz). */
#define BFLB_PSRAM_PCK_MIN_MHZ 400U
#define BFLB_PSRAM_PCK_MAX_MHZ 2300U

BUILD_ASSERT(BFLB_PSRAM_PAGE_SIZE == 2048 || BFLB_PSRAM_PAGE_SIZE == 4096,
	     "page-size must be 2048 or 4096");

/* LINEAR_BND_B page-boundary codes: 2 KB = 0x0B, 4 KB = 0x16 */
#define PAGE_CODE_2KB        0x0BU
#define PAGE_CODE_4KB        0x16U
#define BFLB_PSRAM_PAGE_CODE ((BFLB_PSRAM_PAGE_SIZE == 4096) ? PAGE_CODE_4KB : PAGE_CODE_2KB)

/* PSRAM_TYPE field: x16 = 32MB device, x32 = 64MB device */
#define PSRAM_TYPE_X16  0x1U
#define PSRAM_TYPE_X32  0x2U
#define BFLB_PSRAM_TYPE (BFLB_PSRAM_IS_64MB ? PSRAM_TYPE_X32 : PSRAM_TYPE_X16)

/* PSRAM mode register 0: latency code and drive strength */
#define MR0_LATENCY_CODE 3U
#define MR0_DRIVE_34OHM  2U
#define MR0_READ_MATCH   ((MR0_DRIVE_34OHM << 3) + MR0_LATENCY_CODE)

/* PHY analog power sequencing via DMY1/DMY0 registers */
#define DMY1_HOLD_OUTPUTS 0xFCU
#define DMY0_PAD_ENABLE   0x01U
#define DMY1_LDO_SWITCH   0xCCU
#define DMY1_RELEASE      0xCFU
#define DMY0_NORMAL       0x00U

/* PHY drive-strength delay tap defaults */
#define PHY_DRV_DQ  0x7U
#define PHY_DRV_DQS 0x6U
#define PHY_DRV_CK  0xBU
#define PHY_DRV_CEN 0x8U

/* PHY slew rate, output-enable timing, VREF */
#define PHY_SLEW_RATE       0x2U
#define PHY_OE_TIMER        0x3U
#define PHY_OE_EDGE         0x3U
#define PHY_VREF_INTERNAL   0x1U
#define PHY_DQS_DIFF_DLY_RX 0x3U
#define PHY_WL_CEN_ANA      0x1U

/* Controller timing: { tRC, tCPHR, tCPHW, tRFC } packed per frequency range */
#define TIMING_CTRL_LE_1600MHZ 0x1202000BU
#define TIMING_CTRL_GT_1600MHZ 0x1A03000FU

/* Refresh window cycles (normal=32ms, high-temp=16ms at reference clock) */
#define REFRESH_WIN_NORMAL     0x16E360U
#define REFRESH_WIN_HIGH_TEMP  0x0B71B0U
#define REFRESH_REFI_NORMAL    370U
#define REFRESH_REFI_HIGH_TEMP 190U
#define REFRESH_BUST_CYCLE     5U

/* PHY timing registers: packed {byte3,byte2,byte1,byte0} from SDK freq tables.
 * CFG34 = { dqs_start, dqs_array_stop, array_write, array_read }
 * CFG38 = { auto_refresh, reg_write, reg_read, dqs_stop }
 * CFG3C = { self_refresh_in, self_refresh_exit, global_rst[13:0] }
 * CFG44 = { array_read_busy, array_write_busy, reg_read_busy, reg_write_busy }
 */
#define PHY_TIM_SAFE_64MB_CFG34 0x09020303U
#define PHY_TIM_SAFE_64MB_CFG38 0x040c0313U
#define PHY_TIM_SAFE_64MB_CFG3C 0x07d11515U
#define PHY_TIM_SAFE_64MB_CFG44 0x060f050cU
#define PHY_TIM_SAFE_32MB_CFG34 0x05000501U
#define PHY_TIM_SAFE_32MB_CFG38 0x02080108U
#define PHY_TIM_SAFE_32MB_CFG3C 0x03e90b0bU
#define PHY_TIM_SAFE_32MB_CFG44 0x040b0308U

#define PHY_TIM_2000_CFG34 0x0b030404U
#define PHY_TIM_2000_CFG38 0x050e0418U
#define PHY_TIM_2000_CFG3C 0x0a6a1c1cU
#define PHY_TIM_2000_CFG44 0x07110710U
#define PHY_TIM_2000_CFG50 0x01333333U

/* Calibration constants */
#define CAL_ADDR            BFLB_PSRAM_MEM_BASE
#define CAL_DATA0           0x12345678U
#define CAL_DATA1           0x87654321U
#define CAL_DATA2           0x23456789U
#define CAL_DATA3           0x98765432U
#define CAL_LEN_LARGE       (1024U << 4)
#define CAL_LEN_SMALL       128U
#define CAL_LATENCY_R_INIT  41U
#define CAL_LATENCY_R_MIN   34U
#define CAL_LATENCY_R_ARRAY 35U

/* DQS/DQ delay lines are 4-bit, swept across their full range during cal */
#define DLY_TAP_MAX 15U

/* Safe-rate bring-up frequencies (MHz) before target-rate calibration */
#define SAFE_MHZ_64MB 1400U
#define SAFE_MHZ_32MB 800U

/* Initial write/read latency codes for safe-rate init */
#define LATENCY_W_INIT_64MB 9U
#define LATENCY_W_INIT_32MB 1U
#define LATENCY_R_SAFE_64MB 30U
#define LATENCY_R_SAFE_32MB 17U

/* GLB LDO12UHS VOUT_SEL code for the ~1.2V PSRAM PHY analog rail */
#define LDO12UHS_VOUT_SEL 6U

/* DQ pair config registers, low->high lane index */
static const uint16_t dq_regs[8] = {
	PSRAM_UHS_PHY_CFG_08_OFFSET, PSRAM_UHS_PHY_CFG_0C_OFFSET, PSRAM_UHS_PHY_CFG_10_OFFSET,
	PSRAM_UHS_PHY_CFG_14_OFFSET, PSRAM_UHS_PHY_CFG_18_OFFSET, PSRAM_UHS_PHY_CFG_1C_OFFSET,
	PSRAM_UHS_PHY_CFG_20_OFFSET, PSRAM_UHS_PHY_CFG_24_OFFSET,
};

struct memc_cfg {
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

/* Mutable calibration state (single instance). */
static uint32_t cal_pck_mhz;
static uint32_t latency_w; /* working write latency */
static uint32_t latency_r; /* upper bound for read latency search */

static inline uint32_t uhs_read(uint32_t off)
{
	return sys_read32(UHS_BASE + off);
}

static inline void uhs_write(uint32_t off, uint32_t val)
{
	sys_write32(val, UHS_BASE + off);
}

static void reg_field_set(uint32_t off, uint32_t pos, uint32_t len, uint32_t val)
{
	uint32_t msk = (((1U << len) - 1U) << pos);
	uint32_t tmp = uhs_read(off);

	tmp = (tmp & ~msk) | ((val << pos) & msk);
	uhs_write(off, tmp);
}

static uint32_t reg_field_get(uint32_t off, uint32_t pos, uint32_t len)
{
	return (uhs_read(off) >> pos) & ((1U << len) - 1U);
}

/* PHY delay-tap config */

static void cfg_dq_rx(uint8_t dq)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(dq_regs); i++) {
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ1_DLY_RX_POS, PSRAM_UHS_DQ1_DLY_RX_LEN, dq);
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ0_DLY_RX_POS, PSRAM_UHS_DQ0_DLY_RX_LEN, dq);
	}
	k_busy_wait(10);
}

static void cfg_dqs_rx(uint8_t dqs)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_28_OFFSET, PSRAM_UHS_DQS0_DIFF_DLY_RX_POS,
		      PSRAM_UHS_DQS0_DIFF_DLY_RX_LEN, dqs);
	reg_field_set(PSRAM_UHS_PHY_CFG_2C_OFFSET, PSRAM_UHS_DQS1_DIFF_DLY_RX_POS,
		      PSRAM_UHS_DQS1_DIFF_DLY_RX_LEN, dqs);
	k_busy_wait(10);
}

static void cfg_dq_drv(uint32_t dq)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_04_OFFSET, PSRAM_UHS_DM0_DLY_DRV_POS,
		      PSRAM_UHS_DM0_DLY_DRV_LEN, dq);
	reg_field_set(PSRAM_UHS_PHY_CFG_04_OFFSET, PSRAM_UHS_DM1_DLY_DRV_POS,
		      PSRAM_UHS_DM1_DLY_DRV_LEN, dq);
	for (uint32_t i = 0; i < ARRAY_SIZE(dq_regs); i++) {
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ1_DLY_DRV_POS, PSRAM_UHS_DQ1_DLY_DRV_LEN, dq);
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ0_DLY_DRV_POS, PSRAM_UHS_DQ0_DLY_DRV_LEN, dq);
	}
}

static void cfg_dqs_drv(uint32_t dqs)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_28_OFFSET, PSRAM_UHS_DQS0_DLY_DRV_POS,
		      PSRAM_UHS_DQS0_DLY_DRV_LEN, dqs);
	reg_field_set(PSRAM_UHS_PHY_CFG_2C_OFFSET, PSRAM_UHS_DQS1_DLY_DRV_POS,
		      PSRAM_UHS_DQS1_DLY_DRV_LEN, dqs);
	k_busy_wait(10);
}

static void cfg_ck_cen_drv(uint8_t ck, uint8_t cen)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_00_OFFSET, PSRAM_UHS_CK_DLY_DRV_POS,
		      PSRAM_UHS_CK_DLY_DRV_LEN, ck);
	reg_field_set(PSRAM_UHS_PHY_CFG_00_OFFSET, PSRAM_UHS_CEN_DLY_DRV_POS,
		      PSRAM_UHS_CEN_DLY_DRV_LEN, cen);
	k_busy_wait(50);
}

static void set_ck_dly_drv(uint32_t ck)
{
	uint32_t dq = (ck >= 4U) ? (ck - 4U) : 0U;
	uint32_t cen = dq + 1U;

	k_busy_wait(10);
	dq = MIN(dq, DLY_TAP_MAX);
	cen = MIN(cen, DLY_TAP_MAX);
	cfg_dq_drv(dq);
	cfg_ck_cen_drv(ck, cen);
}

static void set_uhs_latency_r(uint32_t lat)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_PHY_RL_ANA_POS,
		      PSRAM_UHS_PHY_RL_ANA_LEN, lat % 4U);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_PHY_RL_DIG_POS,
		      PSRAM_UHS_PHY_RL_DIG_LEN, lat / 4U);
	k_busy_wait(50);
}

static void set_uhs_latency_w(uint32_t lat)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_PHY_WL_ANA_POS,
		      PSRAM_UHS_PHY_WL_ANA_LEN, lat % 4U);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_PHY_WL_DIG_POS,
		      PSRAM_UHS_PHY_WL_DIG_LEN, lat / 4U);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_PHY_WL_DQ_ANA_POS,
		      PSRAM_UHS_PHY_WL_DQ_ANA_LEN, (lat + 1U) % 4U);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_PHY_WL_DQ_DIG_POS,
		      PSRAM_UHS_PHY_WL_DQ_DIG_LEN, (lat + 1U) / 4U);
	k_busy_wait(50);
}

/* PHY analog bring-up */

static void phy_set_cen_ck_ckn(void)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_50_OFFSET, PSRAM_UHS_DQ_OE_MID_N_REG_POS,
		      PSRAM_UHS_DQ_OE_MID_N_REG_LEN, 0);
	reg_field_set(PSRAM_UHS_PHY_CFG_50_OFFSET, PSRAM_UHS_DQ_OE_MID_P_REG_POS,
		      PSRAM_UHS_DQ_OE_MID_P_REG_LEN, 0);
	k_busy_wait(10);
	reg_field_set(PSRAM_UHS_PHY_CFG_40_OFFSET, PSRAM_UHS_REG_UHS_DMY1_POS,
		      PSRAM_UHS_REG_UHS_DMY1_LEN, DMY1_HOLD_OUTPUTS);
	reg_field_set(PSRAM_UHS_PHY_CFG_40_OFFSET, PSRAM_UHS_REG_UHS_DMY0_POS,
		      PSRAM_UHS_REG_UHS_DMY0_LEN, DMY0_PAD_ENABLE);
	k_busy_wait(10);
}

static void phy_set_or_uhs(void)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_48_OFFSET, PSRAM_UHS_PSRAM_TYPE_POS,
		      PSRAM_UHS_PSRAM_TYPE_LEN, BFLB_PSRAM_TYPE);
	reg_field_set(PSRAM_UHS_PHY_CFG_4C_OFFSET, PSRAM_UHS_ODT_SEL_HW_POS,
		      PSRAM_UHS_ODT_SEL_HW_LEN, 0x0);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_ODT_SEL_POS, PSRAM_UHS_ODT_SEL_LEN,
		      0x0);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_VREF_MODE_POS, PSRAM_UHS_VREF_MODE_LEN,
		      PHY_VREF_INTERNAL);
	reg_field_set(PSRAM_UHS_PHY_CFG_00_OFFSET, PSRAM_UHS_CEN_DLY_DRV_POS,
		      PSRAM_UHS_CEN_DLY_DRV_LEN, PHY_DRV_CEN);
	reg_field_set(PSRAM_UHS_PHY_CFG_00_OFFSET, PSRAM_UHS_CK_DLY_DRV_POS,
		      PSRAM_UHS_CK_DLY_DRV_LEN, PHY_DRV_CK);
	reg_field_set(PSRAM_UHS_PHY_CFG_04_OFFSET, PSRAM_UHS_DM0_DLY_DRV_POS,
		      PSRAM_UHS_DM0_DLY_DRV_LEN, PHY_DRV_DQ);
	reg_field_set(PSRAM_UHS_PHY_CFG_04_OFFSET, PSRAM_UHS_DM1_DLY_DRV_POS,
		      PSRAM_UHS_DM1_DLY_DRV_LEN, PHY_DRV_DQ);
	for (uint32_t i = 0; i < ARRAY_SIZE(dq_regs); i++) {
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ1_DLY_DRV_POS, PSRAM_UHS_DQ1_DLY_DRV_LEN,
			      PHY_DRV_DQ);
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ0_DLY_DRV_POS, PSRAM_UHS_DQ0_DLY_DRV_LEN,
			      PHY_DRV_DQ);
	}
	reg_field_set(PSRAM_UHS_PHY_CFG_28_OFFSET, PSRAM_UHS_DQS0_DLY_DRV_POS,
		      PSRAM_UHS_DQS0_DLY_DRV_LEN, PHY_DRV_DQS);
	reg_field_set(PSRAM_UHS_PHY_CFG_2C_OFFSET, PSRAM_UHS_DQS1_DLY_DRV_POS,
		      PSRAM_UHS_DQS1_DLY_DRV_LEN, PHY_DRV_DQS);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_OE_TIMER_POS, PSRAM_UHS_OE_TIMER_LEN,
		      PHY_OE_TIMER);
	reg_field_set(PSRAM_UHS_PHY_CFG_00_OFFSET, PSRAM_UHS_CEN_SR_POS, PSRAM_UHS_CEN_SR_LEN,
		      PHY_SLEW_RATE);
	reg_field_set(PSRAM_UHS_PHY_CFG_00_OFFSET, PSRAM_UHS_CK_SR_POS, PSRAM_UHS_CK_SR_LEN,
		      PHY_SLEW_RATE);
	reg_field_set(PSRAM_UHS_PHY_CFG_04_OFFSET, PSRAM_UHS_DM1_SR_POS, PSRAM_UHS_DM1_SR_LEN,
		      PHY_SLEW_RATE);
	reg_field_set(PSRAM_UHS_PHY_CFG_04_OFFSET, PSRAM_UHS_DM0_SR_POS, PSRAM_UHS_DM0_SR_LEN,
		      PHY_SLEW_RATE);
	for (uint32_t i = 0; i < ARRAY_SIZE(dq_regs); i++) {
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ1_SR_POS, PSRAM_UHS_DQ1_SR_LEN,
			      PHY_SLEW_RATE);
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ0_SR_POS, PSRAM_UHS_DQ0_SR_LEN,
			      PHY_SLEW_RATE);
	}
	reg_field_set(PSRAM_UHS_PHY_CFG_50_OFFSET,
		      PSRAM_UHS_DQ_OE_DN_P_REG_POS,
		      PSRAM_UHS_DQ_OE_DN_P_REG_LEN, PHY_OE_EDGE);
	reg_field_set(PSRAM_UHS_PHY_CFG_50_OFFSET,
		      PSRAM_UHS_DQ_OE_DN_N_REG_POS,
		      PSRAM_UHS_DQ_OE_DN_N_REG_LEN, PHY_OE_EDGE);
	reg_field_set(PSRAM_UHS_PHY_CFG_50_OFFSET,
		      PSRAM_UHS_DQ_OE_UP_P_REG_POS,
		      PSRAM_UHS_DQ_OE_UP_P_REG_LEN, PHY_OE_EDGE);
	reg_field_set(PSRAM_UHS_PHY_CFG_50_OFFSET,
		      PSRAM_UHS_DQ_OE_UP_N_REG_POS,
		      PSRAM_UHS_DQ_OE_UP_N_REG_LEN, PHY_OE_EDGE);
	for (uint32_t i = 0; i < ARRAY_SIZE(dq_regs); i++) {
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ1_DLY_RX_POS, PSRAM_UHS_DQ1_DLY_RX_LEN, 0x0);
		reg_field_set(dq_regs[i], PSRAM_UHS_DQ0_DLY_RX_POS, PSRAM_UHS_DQ0_DLY_RX_LEN, 0x0);
	}
	reg_field_set(PSRAM_UHS_PHY_CFG_28_OFFSET, PSRAM_UHS_DQS0N_DLY_RX_POS,
		      PSRAM_UHS_DQS0N_DLY_RX_LEN, 0x0);
	reg_field_set(PSRAM_UHS_PHY_CFG_28_OFFSET, PSRAM_UHS_DQS0_DLY_RX_POS,
		      PSRAM_UHS_DQS0_DLY_RX_LEN, 0x0);
	reg_field_set(PSRAM_UHS_PHY_CFG_28_OFFSET, PSRAM_UHS_DQS0_DIFF_DLY_RX_POS,
		      PSRAM_UHS_DQS0_DIFF_DLY_RX_LEN, PHY_DQS_DIFF_DLY_RX);
	reg_field_set(PSRAM_UHS_PHY_CFG_2C_OFFSET, PSRAM_UHS_DQS1N_DLY_RX_POS,
		      PSRAM_UHS_DQS1N_DLY_RX_LEN, 0x0);
	reg_field_set(PSRAM_UHS_PHY_CFG_2C_OFFSET, PSRAM_UHS_DQS1_DLY_RX_POS,
		      PSRAM_UHS_DQS1_DLY_RX_LEN, 0x0);
	reg_field_set(PSRAM_UHS_PHY_CFG_2C_OFFSET, PSRAM_UHS_DQS1_DIFF_DLY_RX_POS,
		      PSRAM_UHS_DQS1_DIFF_DLY_RX_LEN, PHY_DQS_DIFF_DLY_RX);
	k_busy_wait(200);
}

static void phy_switch_to_ldo12uhs(void)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_40_OFFSET, PSRAM_UHS_REG_UHS_DMY1_POS,
		      PSRAM_UHS_REG_UHS_DMY1_LEN, DMY1_LDO_SWITCH);
	k_busy_wait(200);
}

static void phy_release_cen_ck_ckn(void)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_40_OFFSET, PSRAM_UHS_REG_UHS_DMY1_POS,
		      PSRAM_UHS_REG_UHS_DMY1_LEN, DMY1_RELEASE);
	reg_field_set(PSRAM_UHS_PHY_CFG_40_OFFSET, PSRAM_UHS_REG_UHS_DMY0_POS,
		      PSRAM_UHS_REG_UHS_DMY0_LEN, DMY0_NORMAL);
	k_busy_wait(10);
	reg_field_set(PSRAM_UHS_PHY_CFG_50_OFFSET, PSRAM_UHS_DQ_OE_MID_P_REG_POS,
		      PSRAM_UHS_DQ_OE_MID_P_REG_LEN, PHY_OE_EDGE);
	reg_field_set(PSRAM_UHS_PHY_CFG_50_OFFSET, PSRAM_UHS_DQ_OE_MID_N_REG_POS,
		      PSRAM_UHS_DQ_OE_MID_N_REG_LEN, PHY_OE_EDGE);
	k_busy_wait(10);
}

/* Power up the GLB LDO12UHS rail that feeds the PHY 1.2V analog supply. */
static void psram_ldo12uhs_on(void)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_LDO12UHS_OFFSET);
	tmp |= GLB_PU_LDO12UHS_MSK;
	sys_write32(tmp, GLB_BASE + GLB_LDO12UHS_OFFSET);
	k_busy_wait(300);

	tmp = sys_read32(GLB_BASE + GLB_LDO12UHS_OFFSET);
	tmp &= GLB_LDO12UHS_VOUT_SEL_UMSK;
	tmp |= (LDO12UHS_VOUT_SEL << GLB_LDO12UHS_VOUT_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_LDO12UHS_OFFSET);
	k_busy_wait(1);
}

static void psram_analog_init(void)
{
	phy_set_cen_ck_ckn();
	phy_set_or_uhs();
	phy_switch_to_ldo12uhs();
	phy_release_cen_ck_ckn();
}

/* Safe-rate PHY defaults */
static void set_uhs_phy_init(void)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_ODT_SEL_POS, PSRAM_UHS_ODT_SEL_LEN,
		      0x0);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_OE_CTRL_HW_POS,
		      PSRAM_UHS_OE_CTRL_HW_LEN, 1);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_VREF_MODE_POS, PSRAM_UHS_VREF_MODE_LEN,
		      PHY_VREF_INTERNAL);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_OE_TIMER_POS, PSRAM_UHS_OE_TIMER_LEN,
		      PHY_OE_TIMER);
	if (BFLB_PSRAM_IS_64MB) {
		uhs_write(PSRAM_UHS_PHY_CFG_34_OFFSET, PHY_TIM_SAFE_64MB_CFG34);
		uhs_write(PSRAM_UHS_PHY_CFG_38_OFFSET, PHY_TIM_SAFE_64MB_CFG38);
		uhs_write(PSRAM_UHS_PHY_CFG_3C_OFFSET, PHY_TIM_SAFE_64MB_CFG3C);
		uhs_write(PSRAM_UHS_PHY_CFG_44_OFFSET, PHY_TIM_SAFE_64MB_CFG44);
	} else {
		uhs_write(PSRAM_UHS_PHY_CFG_34_OFFSET, PHY_TIM_SAFE_32MB_CFG34);
		uhs_write(PSRAM_UHS_PHY_CFG_38_OFFSET, PHY_TIM_SAFE_32MB_CFG38);
		uhs_write(PSRAM_UHS_PHY_CFG_3C_OFFSET, PHY_TIM_SAFE_32MB_CFG3C);
		uhs_write(PSRAM_UHS_PHY_CFG_44_OFFSET, PHY_TIM_SAFE_32MB_CFG44);
	}
	reg_field_set(PSRAM_UHS_PHY_CFG_50_OFFSET, PSRAM_UHS_PHY_WL_CEN_ANA_POS,
		      PSRAM_UHS_PHY_WL_CEN_ANA_LEN, PHY_WL_CEN_ANA);
	k_busy_wait(100);
}

/* Target-rate PHY timing */
static void set_uhs_phy(void)
{
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_ODT_SEL_POS, PSRAM_UHS_ODT_SEL_LEN,
		      0x0);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_OE_CTRL_HW_POS,
		      PSRAM_UHS_OE_CTRL_HW_LEN, 1);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_VREF_MODE_POS, PSRAM_UHS_VREF_MODE_LEN,
		      PHY_VREF_INTERNAL);
	reg_field_set(PSRAM_UHS_PHY_CFG_30_OFFSET, PSRAM_UHS_OE_TIMER_POS, PSRAM_UHS_OE_TIMER_LEN,
		      PHY_OE_TIMER);
	set_uhs_latency_w(latency_w);
	uhs_write(PSRAM_UHS_PHY_CFG_34_OFFSET, PHY_TIM_2000_CFG34);
	uhs_write(PSRAM_UHS_PHY_CFG_38_OFFSET, PHY_TIM_2000_CFG38);
	uhs_write(PSRAM_UHS_PHY_CFG_3C_OFFSET, PHY_TIM_2000_CFG3C);
	uhs_write(PSRAM_UHS_PHY_CFG_44_OFFSET, PHY_TIM_2000_CFG44);
	uhs_write(PSRAM_UHS_PHY_CFG_50_OFFSET, PHY_TIM_2000_CFG50);
	k_busy_wait(100);
}

/* controller register-channel access (mode-register read/write) */

static void uhs_af_onoff(uint8_t on)
{
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_AF_EN_POS, PSRAM_UHS_REG_AF_EN_LEN,
		      on ? 1 : 0);
	k_busy_wait(50);
}

static int uhs_reg_w(uint32_t latency, uint32_t drive, uint32_t ma, uint32_t bl)
{
	if (ma == 0) {
		reg_field_set(PSRAM_UHS_UHS_PSRAM_CONFIGURE_OFFSET, PSRAM_UHS_REG_UHS_LATENCY_POS,
			      PSRAM_UHS_REG_UHS_LATENCY_LEN, latency);
		reg_field_set(PSRAM_UHS_UHS_PSRAM_CONFIGURE_OFFSET, PSRAM_UHS_REG_UHS_DRIVE_ST_POS,
			      PSRAM_UHS_REG_UHS_DRIVE_ST_LEN, drive);
	} else if (ma == 2) {
		reg_field_set(PSRAM_UHS_UHS_PSRAM_CONFIGURE_OFFSET, PSRAM_UHS_REG_UHS_BL_64_POS,
			      PSRAM_UHS_REG_UHS_BL_64_LEN, (bl == 1) ? 1 : 0);
		reg_field_set(PSRAM_UHS_UHS_PSRAM_CONFIGURE_OFFSET, PSRAM_UHS_REG_UHS_BL_32_POS,
			      PSRAM_UHS_REG_UHS_BL_32_LEN, (bl == 1 || bl == 2) ? 1 : 0);
		reg_field_set(PSRAM_UHS_UHS_PSRAM_CONFIGURE_OFFSET, PSRAM_UHS_REG_UHS_BL_16_POS,
			      PSRAM_UHS_REG_UHS_BL_16_LEN, (bl == 2) ? 1 : 0);
	} else {
		/* other mode-register indices need no extra config here */
	}
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_MODE_REG_POS,
		      PSRAM_UHS_REG_MODE_REG_LEN, ma);
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_CONFIG_REQ_POS,
		      PSRAM_UHS_REG_CONFIG_REQ_LEN, 1);
	if (!WAIT_FOR(reg_field_get(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_CONFIG_GNT_POS,
				    PSRAM_UHS_REG_CONFIG_GNT_LEN),
		      1000, k_busy_wait(10))) {
		return -ETIMEDOUT;
	}
	reg_field_set(PSRAM_UHS_UHS_CMD_OFFSET, PSRAM_UHS_REG_REGW_PULSE_POS,
		      PSRAM_UHS_REG_REGW_PULSE_LEN, 1);
	if (!WAIT_FOR(reg_field_get(PSRAM_UHS_UHS_CMD_OFFSET, PSRAM_UHS_STS_REGW_DONE_POS,
				    PSRAM_UHS_STS_REGW_DONE_LEN),
		      1000, k_busy_wait(10))) {
		reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_CONFIG_REQ_POS,
			      PSRAM_UHS_REG_CONFIG_REQ_LEN, 0);
		return -ETIMEDOUT;
	}
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_CONFIG_REQ_POS,
		      PSRAM_UHS_REG_CONFIG_REQ_LEN, 0);
	k_busy_wait(10);
	return 0;
}

static int uhs_reg_r(uint32_t ma, uint8_t flag)
{
	bool done;

	if (flag == 1) {
		uhs_af_onoff(0);
		reg_field_set(PSRAM_UHS_PHY_CFG_48_OFFSET, PSRAM_UHS_FORCE_FSM_POS,
			      PSRAM_UHS_FORCE_FSM_LEN, 1);
		k_busy_wait(50);
	}
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_MODE_REG_POS,
		      PSRAM_UHS_REG_MODE_REG_LEN, ma);
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_CONFIG_REQ_POS,
		      PSRAM_UHS_REG_CONFIG_REQ_LEN, 1);
	if (!WAIT_FOR(reg_field_get(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_CONFIG_GNT_POS,
				    PSRAM_UHS_REG_CONFIG_GNT_LEN),
		      1000, k_busy_wait(10))) {
		return -ETIMEDOUT;
	}
	reg_field_set(PSRAM_UHS_UHS_CMD_OFFSET, PSRAM_UHS_REG_REGR_PULSE_POS,
		      PSRAM_UHS_REG_REGR_PULSE_LEN, 1);
	/* Read data is only valid once REGR_DONE asserts; a missing done is a
	 * hard fault, so report it rather than matching against stale data.
	 */
	if (flag == 1) {
		done = WAIT_FOR(reg_field_get(PSRAM_UHS_UHS_CMD_OFFSET, PSRAM_UHS_STS_REGR_DONE_POS,
					      PSRAM_UHS_STS_REGR_DONE_LEN),
				2000, k_busy_wait(20));
	} else {
		done = WAIT_FOR(reg_field_get(PSRAM_UHS_UHS_CMD_OFFSET, PSRAM_UHS_STS_REGR_DONE_POS,
					      PSRAM_UHS_STS_REGR_DONE_LEN),
				1000, k_busy_wait(10));
	}
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_CONFIG_REQ_POS,
		      PSRAM_UHS_REG_CONFIG_REQ_LEN, 0);
	if (flag == 1) {
		reg_field_set(PSRAM_UHS_PHY_CFG_48_OFFSET, PSRAM_UHS_FORCE_FSM_POS,
			      PSRAM_UHS_FORCE_FSM_LEN, 0);
		k_busy_wait(50);
		uhs_af_onoff(1);
	}
	k_busy_wait(10);
	return done ? 0 : -ETIMEDOUT;
}

static uint32_t uhs_reg_rdata(void)
{
	return uhs_read(PSRAM_UHS_UHS_CMD_OFFSET) >> 24;
}

static int mr_read_back(void)
{
	for (uint32_t i = 0; i <= 4U; i++) {
		if (i == 3U) {
			continue;
		}
		if (uhs_reg_r(i, 0) != 0) {
			return -ETIMEDOUT;
		}
	}
	return 0;
}

/* controller init */

static uint32_t pck_t_div(uint32_t mhz)
{
	if (mhz >= 2200U) {
		return 5U;
	}
	if (mhz >= 1800U) {
		return 4U;
	}
	if (mhz >= 1500U) {
		return 3U;
	}
	if (mhz >= 1400U) {
		return 2U;
	}
	if (mhz >= 666U) {
		return 1U;
	}
	return 0U;
}

static void controller_init(uint32_t mhz, uint8_t mem_size_code)
{
	uint32_t tmp;

	uhs_write(PSRAM_UHS_UHS_TIMING_CTRL_OFFSET,
		  (mhz > 1600U) ? TIMING_CTRL_GT_1600MHZ : TIMING_CTRL_LE_1600MHZ);
	psram_analog_init();
	k_busy_wait(150);

	tmp = uhs_read(PSRAM_UHS_UHS_MANUAL_OFFSET);
	tmp &= PSRAM_UHS_REG_PCK_T_DIV_UMSK;
	tmp |= (pck_t_div(mhz) << PSRAM_UHS_REG_PCK_T_DIV_POS);
	uhs_write(PSRAM_UHS_UHS_MANUAL_OFFSET, tmp);

	/* Refresh window/refi are fixed; pck_t_div normalises the refresh clock. */
	uhs_write(PSRAM_UHS_UHS_AUTO_FRESH_1_OFFSET,
		  BFLB_PSRAM_HIGH_TEMP ? REFRESH_WIN_HIGH_TEMP : REFRESH_WIN_NORMAL);
	reg_field_set(PSRAM_UHS_UHS_AUTO_FRESH_2_OFFSET, PSRAM_UHS_REG_REFI_CYCLE_POS,
		      PSRAM_UHS_REG_REFI_CYCLE_LEN,
		      BFLB_PSRAM_HIGH_TEMP ? REFRESH_REFI_HIGH_TEMP : REFRESH_REFI_NORMAL);
	reg_field_set(PSRAM_UHS_UHS_AUTO_FRESH_4_OFFSET, PSRAM_UHS_REG_BUST_CYCLE_POS,
		      PSRAM_UHS_REG_BUST_CYCLE_LEN, REFRESH_BUST_CYCLE);

	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_AF_EN_POS, PSRAM_UHS_REG_AF_EN_LEN,
		      1);
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_ADDRMB_MSK_POS,
		      PSRAM_UHS_REG_ADDRMB_MSK_LEN, mem_size_code);
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_LINEAR_BND_B_POS,
		      PSRAM_UHS_REG_LINEAR_BND_B_LEN, BFLB_PSRAM_PAGE_CODE);
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_INIT_EN_POS,
		      PSRAM_UHS_REG_INIT_EN_LEN, 1);

	reg_field_set(PSRAM_UHS_PHY_CFG_48_OFFSET, PSRAM_UHS_PSRAM_TYPE_POS,
		      PSRAM_UHS_PSRAM_TYPE_LEN, BFLB_PSRAM_TYPE);
	phy_set_or_uhs();
	k_busy_wait(250);
}

/* Global reset pulse to the PSRAM device */
static void psram_global_reset(void)
{
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_CONFIG_REQ_POS,
		      PSRAM_UHS_REG_CONFIG_REQ_LEN, 1);
	k_busy_wait(10);
	reg_field_set(PSRAM_UHS_UHS_CMD_OFFSET, PSRAM_UHS_REG_GLBR_PULSE_POS,
		      PSRAM_UHS_REG_GLBR_PULSE_LEN, 1);
	k_busy_wait(100);
	reg_field_set(PSRAM_UHS_UHS_BASIC_OFFSET, PSRAM_UHS_REG_CONFIG_REQ_POS,
		      PSRAM_UHS_REG_CONFIG_REQ_LEN, 0);
	k_busy_wait(100);
}

/* array (memory) test vectors used by calibration */

/*
 * The PSRAM window is cacheable write-back, so the sweeps must bypass the
 * D-cache or a cached read-back would pass on every tap. Flush after writing,
 * invalidate before reading.
 */
static void array_write_fix(uint32_t addr, uint32_t len, uint32_t data0, uint32_t data1)
{
	volatile uint32_t *p = (volatile uint32_t *)addr;

	for (uint32_t i = 0; i < (len / sizeof(uint32_t)); i++) {
		p[i] = ((i % 2U) == 0U) ? (data0 + i) : (data1 + i);
	}
	sys_cache_data_flush_and_invd_range((void *)addr, len);
	k_busy_wait(10);
}

static uint8_t array_read_fix(uint32_t addr, uint32_t len, uint32_t data0, uint32_t data1)
{
	volatile uint32_t *p = (volatile uint32_t *)addr;
	uint32_t expected;

	sys_cache_data_invd_range((void *)addr, len);
	for (uint32_t i = 0; i < (len / sizeof(uint32_t)); i++) {
		expected = ((i % 2U) == 0U) ? (data0 + i) : (data1 + i);
		if (p[i] != expected) {
			return 0;
		}
	}
	k_busy_wait(10);
	return 1;
}

/* calibration: read/write eye-centering sweeps */

static int reg_read_cal(void)
{
	uint32_t reg_dqs;
	uint32_t reg_dq;
	uint8_t found;

	for (uint32_t lat = latency_r; lat > 0U; lat--) {
		reg_dqs = 0;
		reg_dq = 0;
		found = 0;

		if (lat == CAL_LATENCY_R_MIN) {
			return -EIO;
		}
		set_uhs_latency_r(lat);
		cfg_dq_rx(0);
		for (int32_t i = DLY_TAP_MAX; i >= 0; i--) {
			cfg_dqs_rx(i);
			if (uhs_reg_r(0, 1)) {
				return -ETIMEDOUT;
			}
			if (uhs_reg_rdata() == MR0_READ_MATCH) {
				found = 1;
				break;
			}
		}
		cfg_dqs_rx(0);
		for (int32_t i = DLY_TAP_MAX; i >= 0; i--) {
			cfg_dq_rx(i);
			if (uhs_reg_r(0, 1)) {
				return -ETIMEDOUT;
			}
			if (uhs_reg_rdata() == MR0_READ_MATCH) {
				found = 1;
				break;
			}
		}
		if (!found) {
			continue;
		}

		lat -= 2U;
		set_uhs_latency_r(lat);
		latency_r = lat;
		found = 0;

		cfg_dq_rx(0);
		for (int32_t i = DLY_TAP_MAX; i >= 0; i--) {
			cfg_dqs_rx(i);
			if (uhs_reg_r(0, 0)) {
				return -ETIMEDOUT;
			}
			if (uhs_reg_rdata() == MR0_READ_MATCH) {
				reg_dqs += i;
				found = 1;
				break;
			}
		}
		for (int32_t i = 0; i <= (int32_t)DLY_TAP_MAX; i++) {
			cfg_dqs_rx(i);
			if (uhs_reg_r(0, 0)) {
				return -ETIMEDOUT;
			}
			if (uhs_reg_rdata() == MR0_READ_MATCH) {
				reg_dqs += i;
				found = 1;
				break;
			}
		}
		cfg_dqs_rx(0);
		for (int32_t i = DLY_TAP_MAX; i >= 0; i--) {
			cfg_dq_rx(i);
			if (uhs_reg_r(0, 0)) {
				return -ETIMEDOUT;
			}
			if (uhs_reg_rdata() == MR0_READ_MATCH) {
				reg_dq += i;
				found = 1;
				break;
			}
		}
		for (int32_t i = 0; i <= (int32_t)DLY_TAP_MAX; i++) {
			cfg_dq_rx(i);
			if (uhs_reg_r(0, 0)) {
				return -ETIMEDOUT;
			}
			if (uhs_reg_rdata() == MR0_READ_MATCH) {
				reg_dq += i;
				found = 1;
				break;
			}
		}
		if (!found) {
			return -EIO;
		}
		if (reg_dqs >= reg_dq) {
			reg_dqs = (reg_dqs - reg_dq) / 2U;
			reg_dq = 0;
		} else {
			reg_dq = (reg_dq - reg_dqs) / 2U;
			reg_dqs = 0;
		}
		cfg_dqs_rx(reg_dqs);
		cfg_dq_rx(reg_dq);
		if (mr_read_back()) {
			return -ETIMEDOUT;
		}
		LOG_DBG("reg read lat=%u dqs=%u dq=%u", latency_r, reg_dqs, reg_dq);
		return 0;
	}
	return -EIO;
}

/*
 * With the current write settings applied, sweep read latency and RX taps.
 * Returns 1 if a combination reads the array pattern back correctly. Relies on
 * reg_read_cal landing latency_r above 36; below that the range is empty
 * (matches vendor).
 */
static int array_read_sweep(uint32_t len)
{
	for (uint32_t rl = latency_r; rl >= CAL_LATENCY_R_ARRAY; rl--) {
		set_uhs_latency_r(rl);
		cfg_dq_rx(0);
		for (uint32_t rdqs = 0; rdqs <= DLY_TAP_MAX; rdqs++) {
			cfg_dqs_rx(rdqs);
			if (array_read_fix(CAL_ADDR, len, CAL_DATA0, CAL_DATA1)) {
				return 1;
			}
		}
		cfg_dqs_rx(0);
		for (uint32_t rdq = 0; rdq <= DLY_TAP_MAX; rdq++) {
			cfg_dq_rx(rdq);
			if (array_read_fix(CAL_ADDR, len, CAL_DATA0, CAL_DATA1)) {
				return 1;
			}
		}
	}
	return 0;
}

static int init_array_write(void)
{
	static const uint8_t wl_val[6] = {13, 12, 14, 11, 15, 10};
	static const uint8_t wdq_val[3] = {0, 5, 11};
	uint32_t len = CAL_LEN_LARGE;

	for (int wi = 0; wi < 6; wi++) {
		latency_w = wl_val[wi];
		set_uhs_latency_w(wl_val[wi]);
		for (int di = 0; di < 3; di++) {
			cfg_dq_drv(wdq_val[di]);
			cfg_ck_cen_drv(wdq_val[di] + 4U, wdq_val[di] + 1U);
			for (uint32_t wdqs = 0; wdqs <= DLY_TAP_MAX; wdqs++) {
				cfg_dqs_drv(wdqs);
				array_write_fix(CAL_ADDR, len, CAL_DATA0, CAL_DATA1);
				if (array_read_sweep(len)) {
					return 0;
				}
			}
		}
	}
	return -EIO;
}

static int array_read_latency_cal(void)
{
	static const uint8_t ck_val[3] = {4, 9, 15};
	uint32_t len = CAL_LEN_SMALL;
	uint32_t array_dqs;
	uint32_t array_dq;
	uint8_t dqs_ok;
	uint8_t dq_ok;

	for (uint32_t lat = latency_r; lat > 0U; lat--) {
		if (lat == CAL_LATENCY_R_MIN) {
			return -EIO;
		}
		set_uhs_latency_r(lat);
		for (int ci = 0; ci < 3; ci++) {
			array_dqs = 0;
			array_dq = 0;
			dqs_ok = 0;
			dq_ok = 0;

			set_ck_dly_drv(ck_val[ci]);
			cfg_dq_rx(0);
			for (int32_t i = DLY_TAP_MAX; i >= 0; i--) {
				cfg_dqs_rx(i);
				if (array_read_fix(CAL_ADDR, len, CAL_DATA0, CAL_DATA1)) {
					array_dqs += i;
					dqs_ok = 1;
					break;
				}
			}
			for (int32_t i = 0; i <= (int32_t)DLY_TAP_MAX; i++) {
				cfg_dqs_rx(i);
				if (array_read_fix(CAL_ADDR, len, CAL_DATA0, CAL_DATA1)) {
					array_dqs += i;
					dqs_ok = 1;
					break;
				}
			}
			cfg_dqs_rx(0);
			for (int32_t i = DLY_TAP_MAX; i >= 0; i--) {
				cfg_dq_rx(i);
				if (array_read_fix(CAL_ADDR, len, CAL_DATA0, CAL_DATA1)) {
					array_dq += i;
					dq_ok = 1;
					break;
				}
			}
			for (int32_t i = 0; i <= (int32_t)DLY_TAP_MAX; i++) {
				cfg_dq_rx(i);
				if (array_read_fix(CAL_ADDR, len, CAL_DATA0, CAL_DATA1)) {
					array_dq += i;
					dq_ok = 1;
					break;
				}
			}
			if (dqs_ok || dq_ok) {
				if (array_dqs >= array_dq) {
					array_dqs = (array_dqs - array_dq) / 2U;
					array_dq = 0;
				} else {
					array_dq = (array_dq - array_dqs) / 2U;
					array_dqs = 0;
				}
				cfg_dq_rx(array_dq);
				cfg_dqs_rx(array_dqs);
				LOG_DBG("read cal: rl=%u dqs=%u dq=%u ck=%u", lat, array_dqs,
					array_dq, ck_val[ci]);
				return 0;
			}
		}
	}
	return -EIO;
}

static int array_write_ck_cal(void)
{
	uint32_t len = CAL_LEN_LARGE;
	uint32_t ck1 = 15, ck2 = 4, ck;
	uint8_t flag1, flag2, fail1 = 0, fail2 = 0;

	for (ck = 4; ck <= DLY_TAP_MAX; ck++) {
		set_ck_dly_drv(ck);
		if (array_read_fix(CAL_ADDR, len, CAL_DATA0, CAL_DATA1) == 0 && !fail2) {
			fail2 = 1;
			ck2 = ck;
		}
	}
	flag2 = fail2 ? 0 : 1;
	for (ck = DLY_TAP_MAX; ck >= 4U; ck--) {
		set_ck_dly_drv(ck);
		if (array_read_fix(CAL_ADDR, len, CAL_DATA0, CAL_DATA1) == 0 && !fail1) {
			fail1 = 1;
			ck1 = ck;
		}
	}
	flag1 = fail1 ? 0 : 1;

	if (flag1 == 0 && flag2 == 0) {
		if (ck1 == 15U && ck2 == 4U) {
			return -EIO;
		} else if ((15U - ck1) >= (ck2 - 4U)) {
			ck = 15;
		} else {
			ck = 4;
		}
	} else if (flag1 == 0 && flag2 == 1) {
		ck = (ck1 > 9U) ? 4 : 15;
	} else if (flag1 == 1 && flag2 == 0) {
		ck = (ck2 > 9U) ? 4 : 15;
	} else {
		ck = (15 + 4) / 2;
	}
	set_ck_dly_drv(ck);
	LOG_DBG("write ck cal: ck=%u", ck);
	return 0;
}

static int array_write_dqs_dq_cal(void)
{
	uint32_t addr = BFLB_PSRAM_MEM_BASE;
	uint32_t len = CAL_LEN_SMALL;
	uint32_t d0 = CAL_DATA2, d1 = CAL_DATA3;
	uint32_t drv = 0, drv1 = 0, drv2 = 0;
	uint8_t f1 = 0, f2 = 0;

	for (int32_t i = DLY_TAP_MAX; i >= 0; i--) {
		cfg_dqs_drv(i);
		array_write_fix(addr, len, d0, d1);
		if (array_read_fix(addr, len, d0, d1)) {
			drv1 = i;
			f1 = 1;
			break;
		}
	}
	for (int32_t i = 0; i <= (int32_t)DLY_TAP_MAX; i++) {
		cfg_dqs_drv(i);
		array_write_fix(addr, len, d1, d0);
		if (array_read_fix(addr, len, d1, d0)) {
			drv2 = i;
			f2 = 1;
			break;
		}
	}
	if (f1 && f2) {
		drv = (drv1 + drv2) / 2U;
	} else if (f1) {
		drv = drv1;
	} else if (f2) {
		drv = drv2;
	} else {
		return -EIO;
	}
	if (reg_field_get(PSRAM_UHS_PHY_CFG_00_OFFSET, PSRAM_UHS_CK_DLY_DRV_POS,
			  PSRAM_UHS_CK_DLY_DRV_LEN) == 4U) {
		drv = 0;
	}
	cfg_dqs_drv(drv);
	LOG_DBG("write dqs cal: dqs=%u", drv);
	return 0;
}

/*
 * Sweep RX taps via the register read channel. Returns 1 on a data match,
 * 0 if none matched, or -ETIMEDOUT on a controller timeout.
 */
static int reg_read_sweep(void)
{
	cfg_dq_rx(0);
	for (uint32_t rdqs = 0; rdqs <= DLY_TAP_MAX; rdqs++) {
		cfg_dqs_rx(rdqs);
		if (uhs_reg_r(0, 1)) {
			return -ETIMEDOUT;
		}
		if (uhs_reg_rdata() == MR0_READ_MATCH) {
			return 1;
		}
	}
	cfg_dqs_rx(0);
	for (uint32_t rdq = 0; rdq <= DLY_TAP_MAX; rdq++) {
		cfg_dq_rx(rdq);
		if (uhs_reg_r(0, 1)) {
			return -ETIMEDOUT;
		}
		if (uhs_reg_rdata() == MR0_READ_MATCH) {
			return 1;
		}
	}
	return 0;
}

/*
 * Sweep write-DQS drive for the current write-drive setting: program each tap
 * then look for a read match. Returns 1 on match, 0 if none, -ETIMEDOUT on a
 * controller timeout.
 */
static int reg_write_try(void)
{
	int ret;

	for (uint32_t wdqs = 0; wdqs <= DLY_TAP_MAX; wdqs++) {
		cfg_dqs_drv(wdqs);
		set_uhs_phy_init();
		if (uhs_reg_w(MR0_LATENCY_CODE, MR0_DRIVE_34OHM, 0, 0)) {
			return -ETIMEDOUT;
		}
		set_uhs_phy();
		ret = reg_read_sweep();
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

/* Find a working write-latency code at the safe stage */
static int init_reg_write(void)
{
	static const uint8_t wl32[6] = {1, 0, 2, 3, 4, 5};
	static const uint8_t wl64[6] = {9, 8, 10, 7, 11, 6};
	static const uint8_t rl_val[5] = {36, 37, 38, 39, 40};
	static const uint8_t wdq_val[3] = {0, 5, 11};
	const uint8_t *wl_val = BFLB_PSRAM_IS_64MB ? wl64 : wl32;
	int ret;

	for (int ri = 0; ri < 5; ri++) {
		set_uhs_latency_r(rl_val[ri]);
		for (int wi = 0; wi < 6; wi++) {
			latency_w = wl_val[wi];
			set_uhs_latency_w(wl_val[wi]);
			for (int di = 0; di < 3; di++) {
				cfg_dq_drv(wdq_val[di]);
				cfg_ck_cen_drv(wdq_val[di] + 4U, wdq_val[di] + 1U);
				ret = reg_write_try();
				if (ret < 0) {
					return ret;
				}
				if (ret == 1) {
					return 0;
				}
			}
		}
	}
	return -EIO;
}

static void tzc_psrama_release(void)
{
	uint32_t tmp = sys_read32(TZC_SEC_BASE + TZC_SEC_TZC_PSRAMA_TZSRG_CTRL_OFFSET);

	tmp &= ~BIT(16);
	sys_write32(tmp, TZC_SEC_BASE + TZC_SEC_TZC_PSRAMA_TZSRG_CTRL_OFFSET);
}

/* Full read/write eye-centering at the target rate */
static int self_cal(const struct memc_cfg *config, uint8_t mem_size_code)
{
	int err;

	err = init_reg_write();
	if (err != 0) {
		LOG_ERR("init_reg_write failed (%d)", err);
		return err;
	}

	err = clock_control_set_rate(
		config->clock_dev, config->clock_subsys,
		(clock_control_subsys_rate_t)(uintptr_t)(cal_pck_mhz * MHZ(1)));
	if (err != 0) {
		return err;
	}
	controller_init(cal_pck_mhz, mem_size_code);
	set_uhs_phy();

	err = reg_read_cal();
	if (err != 0) {
		LOG_ERR("reg_read_cal failed (%d)", err);
		return err;
	}
	err = init_array_write();
	if (err != 0) {
		LOG_ERR("init_array_write failed (%d)", err);
		return err;
	}
	err = array_read_latency_cal();
	if (err != 0) {
		LOG_ERR("array_read_latency_cal failed (%d)", err);
		return err;
	}
	err = array_write_ck_cal();
	if (err != 0) {
		LOG_ERR("array_write_ck_cal failed (%d)", err);
		return err;
	}
	err = array_write_dqs_dq_cal();
	if (err != 0) {
		LOG_ERR("array_write_dqs_dq_cal failed (%d)", err);
		return err;
	}
	return 0;
}

static int psram_memory_test(void)
{
	volatile uint32_t *mem = (volatile uint32_t *)BFLB_PSRAM_MEM_BASE;
	const size_t last = (BFLB_PSRAM_MEM_SIZE / sizeof(uint32_t)) - 1U;
	const size_t mid = last / 2U;

	mem[0] = 0xCAFEBABEU;
	mem[mid] = 0xDEADBEEFU;
	mem[last] = 0x5AA5F00DU;
	/* Bypass the cache so the read-back round-trips through PSRAM; distinct
	 * corner values also catch an aliasing address bus (wrong size code).
	 */
	sys_cache_data_flush_and_invd_range((void *)&mem[0], sizeof(uint32_t));
	sys_cache_data_flush_and_invd_range((void *)&mem[mid], sizeof(uint32_t));
	sys_cache_data_flush_and_invd_range((void *)&mem[last], sizeof(uint32_t));
	if (mem[0] != 0xCAFEBABEU || mem[mid] != 0xDEADBEEFU || mem[last] != 0x5AA5F00DU) {
		LOG_ERR("PSRAM data-path/aliasing test failed");
		return -EIO;
	}
	return 0;
}

static int memc_bflb_bl808_psram_uhs_init(const struct device *dev)
{
	const struct memc_cfg *config = dev->config;
	uint8_t mem_size_code = (BFLB_PSRAM_MEM_SIZE / MB(1)) - 1;
	uint32_t safe_mhz = BFLB_PSRAM_IS_64MB ? SAFE_MHZ_64MB : SAFE_MHZ_32MB;
	uint32_t pck_freq;
	int64_t t0;
	int err;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	/* UHS PLL (clock controller) */
	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err != 0) {
		return err;
	}
	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &pck_freq);
	if (err != 0) {
		return err;
	}
	cal_pck_mhz = pck_freq / MHZ(1);
	if (cal_pck_mhz < BFLB_PSRAM_PCK_MIN_MHZ || cal_pck_mhz > BFLB_PSRAM_PCK_MAX_MHZ) {
		LOG_ERR("UHS PLL rate %u MHz outside supported %u-%u", cal_pck_mhz,
			BFLB_PSRAM_PCK_MIN_MHZ, BFLB_PSRAM_PCK_MAX_MHZ);
		return -ENOTSUP;
	}
	/* PHY 1.2V analog rail + analog bring-up */
	psram_ldo12uhs_on();
	psram_analog_init();

	/* Safe-rate bring-up before calibrating at the target rate. */
	latency_r = CAL_LATENCY_R_INIT;
	latency_w = BFLB_PSRAM_IS_64MB ? LATENCY_W_INIT_64MB : LATENCY_W_INIT_32MB;
	err = clock_control_set_rate(config->clock_dev, config->clock_subsys,
				     (clock_control_subsys_rate_t)(uintptr_t)(safe_mhz * MHZ(1)));
	if (err != 0) {
		return err;
	}
	controller_init(safe_mhz, mem_size_code);
	set_uhs_phy_init();
	set_uhs_latency_w(latency_w);
	set_uhs_latency_r(BFLB_PSRAM_IS_64MB ? LATENCY_R_SAFE_64MB : LATENCY_R_SAFE_32MB);
	psram_global_reset();

	/* Release TZC before calibration: the array sweeps read/write 0x50000000. */
	tzc_psrama_release();

	/* Calibrate at the target rate. */
	t0 = k_uptime_get();
	err = self_cal(config, mem_size_code);
	if (err != 0) {
		return err;
	}

	err = mr_read_back();
	if (err != 0) {
		LOG_ERR("PSRAM mode register readback failed");
		return err;
	}
	err = psram_memory_test();
	if (err != 0) {
		return err;
	}

	LOG_INF("UHS PSRAM: %u MB at 0x%08x, pck %u MHz, calibrated in %lld ms",
		(uint32_t)(BFLB_PSRAM_MEM_SIZE / MB(1)), BFLB_PSRAM_MEM_BASE, cal_pck_mhz,
		k_uptime_get() - t0);
	return 0;
}

static const struct memc_cfg config = {
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, id),
};

/* The PSRAM region links regardless of init success; consumers placing data
 * here must depend on this device being ready.
 */
DEVICE_DT_INST_DEFINE(0, memc_bflb_bl808_psram_uhs_init, NULL, NULL, &config, POST_KERNEL,
		      CONFIG_MEMC_INIT_PRIORITY, NULL);
