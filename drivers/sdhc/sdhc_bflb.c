/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_sdhc

#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control.h>

#include <bflb_soc.h>
#include <glb_reg.h>
#include <sdh_reg.h>

#if defined(CONFIG_SOC_SERIES_BL61X)
#include <zephyr/dt-bindings/clock/bflb_bl61x_clock.h>
#define SDH_SRC_CLK_0_ID	BL61X_CLKID_CLK_WIFIPLL
#define SDH_SRC_CLK_1_ID	BL61X_CLKID_CLK_AUPLL
#elif defined(CONFIG_SOC_SERIES_BL808)
#include <zephyr/dt-bindings/clock/bflb_bl808_clock.h>
#define SDH_SRC_CLK_0_ID	BL808_CLKID_CLK_WIFIPLL
#define SDH_SRC_CLK_1_ID	BL808_CLKID_CLK_CPUPLL
#else
#error Unsupported platform
#endif

LOG_MODULE_REGISTER(sdhc_bflb, CONFIG_SDHC_LOG_LEVEL);

/* SDH source clock: WIFIPLL 96MHz.
 * Frequency is controlled via GLB divider only and SDHCI divider is always bypassed:
 * GLB divider is 3-bit (reg 0-7), so actual divisor ranges from 1 to 8.
 * The minimum clock = 96/8 = 12MHz and is the default.
 */
#define SDH_SRC_CLK_HZ		MHZ(96U)
#define SDH_CLK_SEL		0U
#define SDH_GLB_DIV_INIT	8U
#define SDH_GLB_DIV_MAX		8U

#define SDH_MAX_TIMEOUT		0xf
#define SDH_ACTIVATE_CLK_CNT	80

/* Polling timeouts (milliseconds) */
#define RESET_TIMEOUT_MS	100
#define CLK_STABLE_TIMEOUT_MS	10
#define CMD_TIMEOUT_MS		1000

/* SD bus voltage values for host ctrl register */
#define SD_BUS_VLT_33		0x7 /* 3.3V */

/* ADMA2 descriptor attributes */
#define ADMA2_ATTR_VALID	BIT(0)
#define ADMA2_ATTR_END		BIT(1)
#define ADMA2_ATTR_TRAN		(2 << 4)

/* ADMA2 descriptor: 8 bytes — { attr(2) | len(2) | addr(4) } */
struct adma2_desc {
	uint16_t attr;
	uint16_t len; /* 0 = 65536 bytes */
	uint32_t addr;
} __packed;

static inline uint32_t glb_read32(uint32_t offset)
{
	return sys_read32(GLB_BASE + offset);
}

static inline void glb_write32(uint32_t offset, uint32_t val)
{
	sys_write32(val, GLB_BASE + offset);
}

struct sdhc_bflb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pcfg;
	uint8_t bus_width;
	unsigned int power_delay_ms;
	void (*irq_config_func)(void);
};

struct sdhc_bflb_data {
	struct sdhc_host_props props;
	struct sdhc_io host_io;
	struct k_mutex lock;
	struct k_sem irq_sem;
	volatile uint16_t irq_status;
	volatile uint16_t irq_err;
};

static void sdhc_bflb_isr(const struct device *dev)
{
	const struct sdhc_bflb_config *cfg = dev->config;
	struct sdhc_bflb_data *data = dev->data;
	uintptr_t base = cfg->base;
	uint16_t status, err;

	status = sys_read16(base + SDH_SD_NORMAL_INT_STATUS_OFFSET);
	err = sys_read16(base + SDH_SD_ERROR_INT_STATUS_OFFSET);

	/* Clear handled bits (W1C) */
	if (status & (BIT(SDH_CMD_COMPLETE_POS) | BIT(SDH_XFER_COMPLETE_POS))) {
		sys_write16(status & (BIT(SDH_CMD_COMPLETE_POS) | BIT(SDH_XFER_COMPLETE_POS)),
			    base + SDH_SD_NORMAL_INT_STATUS_OFFSET);
	}
	if (err) {
		sys_write16(err, base + SDH_SD_ERROR_INT_STATUS_OFFSET);
	}

	data->irq_status |= status;
	data->irq_err |= err;

	if (status & (BIT(SDH_CMD_COMPLETE_POS) | BIT(SDH_XFER_COMPLETE_POS) |
		      BIT(SDH_ERR_INT_POS))) {
		k_sem_give(&data->irq_sem);
	}
}

static void sdhc_bflb_bus_reset(void)
{
	uint32_t val;

	/* GLB AHB software reset for SDH: SWRST_CFG0 bit 22.
	 * SDK pattern: clear bit, set bit, clear bit (pulse).
	 */
	val = glb_read32(GLB_SWRST_CFG0_OFFSET);
	val &= ~GLB_SWRST_S1_EXT_SDH_MSK;
	glb_write32(GLB_SWRST_CFG0_OFFSET, val);
	k_busy_wait(10);
	val |= GLB_SWRST_S1_EXT_SDH_MSK;
	glb_write32(GLB_SWRST_CFG0_OFFSET, val);
	k_busy_wait(10);
	val &= ~GLB_SWRST_S1_EXT_SDH_MSK;
	glb_write32(GLB_SWRST_CFG0_OFFSET, val);
}

static void sdhc_bflb_set_glb_clk_div(uint32_t divider)
{
	uint32_t val;

	val = glb_read32(GLB_SDH_CFG0_OFFSET);
	val &= ~(GLB_REG_SDH_CLK_DIV_MSK | GLB_REG_SDH_CLK_SEL_MSK | GLB_REG_SDH_CLK_EN_MSK);
	val |= ((divider - 1U) << GLB_REG_SDH_CLK_DIV_POS) & GLB_REG_SDH_CLK_DIV_MSK;
	val |= (SDH_CLK_SEL << GLB_REG_SDH_CLK_SEL_POS) & GLB_REG_SDH_CLK_SEL_MSK;
	val |= GLB_REG_SDH_CLK_EN_MSK;
	glb_write32(GLB_SDH_CFG0_OFFSET, val);
}

static int sdhc_bflb_sw_reset(uintptr_t base, uint16_t reset_bits)
{
	uint16_t val;
	k_timepoint_t end_timeout = sys_timepoint_calc(K_MSEC(RESET_TIMEOUT_MS));

	val = sys_read16(base + SDH_SD_TIMEOUT_CTRL_SW_RESET_OFFSET);
	val |= reset_bits;
	sys_write16(val, base + SDH_SD_TIMEOUT_CTRL_SW_RESET_OFFSET);

	while ((sys_read16(base + SDH_SD_TIMEOUT_CTRL_SW_RESET_OFFSET) & reset_bits) != 0
			&& !sys_timepoint_expired(end_timeout)) {
		k_busy_wait(10);
	}

	if (sys_timepoint_expired(end_timeout)) {
		LOG_ERR("SW reset timeout (bits=0x%04x)", reset_bits);
		return -ETIMEDOUT;
	}

	return 0;
}

static int sdhc_bflb_enable_internal_clock(uintptr_t base)
{
	uint16_t val;
	k_timepoint_t end_timeout = sys_timepoint_calc(K_MSEC(CLK_STABLE_TIMEOUT_MS));

	val = sys_read16(base + SDH_SD_CLOCK_CTRL_OFFSET);
	val |= BIT(SDH_INT_CLK_EN_POS);
	sys_write16(val, base + SDH_SD_CLOCK_CTRL_OFFSET);

	while ((sys_read16(base + SDH_SD_CLOCK_CTRL_OFFSET) & BIT(SDH_INT_CLK_STABLE_POS)) == 0
			&& !sys_timepoint_expired(end_timeout)) {
		k_busy_wait(10);
	}

	if (sys_timepoint_expired(end_timeout)) {
		LOG_ERR("Internal clock not stable");
		return -ETIMEDOUT;
	}

	return 0;
}

static void sdhc_bflb_set_timeout_max(uintptr_t base)
{
	uint16_t val;

	val = sys_read16(base + SDH_SD_TIMEOUT_CTRL_SW_RESET_OFFSET);
	val &= ~SDH_TIMEOUT_VALUE_MSK;
	val |= (SDH_MAX_TIMEOUT << SDH_TIMEOUT_VALUE_POS);
	sys_write16(val, base + SDH_SD_TIMEOUT_CTRL_SW_RESET_OFFSET);
}

static void sdhc_bflb_enable_status_bits(uintptr_t base)
{
	/* Enable all normal + error status bits via single 32-bit write,
	 * matching lhal bflb_sdh_init() pattern. Normal status enable at
	 * 0x34 (lower 16), error status enable at 0x36 (upper 16).
	 * Bit 15 (error summary) is cleared per lhal convention.
	 */
	uint32_t tmp = 0x7fff; /* normal: all except bit 15 (error summary) */

	tmp |= (uint32_t)0x03ff << 16; /* error: all 10 error bits */
	sys_write32(tmp, base + SDH_SD_NORMAL_INT_STATUS_EN_OFFSET);

	/* Enable interrupt signals for CMD_COMPLETE, XFER_COMPLETE,
	 * and all error bits so the ISR fires on completion.
	 */
	tmp = BIT(SDH_CMD_COMPLETE_POS) | BIT(SDH_XFER_COMPLETE_POS);

	tmp |= (uint32_t)0x03FF << 16; /* error interrupt signals */
	sys_write32(tmp, base + SDH_SD_NORMAL_INT_STATUS_INT_EN_OFFSET);
}

static void sdhc_bflb_configure_vendor_regs(uintptr_t base)
{
	uint16_t val16;
	uint32_t val32;

	/* CLK_BURST_SETUP (0x10A): burst size = 64 bytes (1), FIFO threshold = 128 (1).
	 * Matches SDK: dma_burst=SDH_DMA_BURST_64(1), dma_fifo_th=SDH_DMA_FIFO_THRESHOLD_128(1).
	 */
	val16 = sys_read16(base + SDH_SD_CLOCK_AND_BURST_SIZE_SETUP_OFFSET);
	val16 &= ~(SDH_BRST_SIZE_MSK | SDH_DMA_SIZE_MSK);
	val16 |= (1U << SDH_BRST_SIZE_POS); /* 64-byte burst */
	val16 |= (1U << SDH_DMA_SIZE_POS);  /* 128-byte FIFO threshold */
	sys_write16(val16, base + SDH_SD_CLOCK_AND_BURST_SIZE_SETUP_OFFSET);

	/* TX_CFG_REG (0x118): enable internal TX clock for command/data output.
	 * lhal has a bug here (16-bit write to 32-bit reg, bit 30 unreachable).
	 * We use correct 32-bit access.
	 */
	val32 = sys_read32(base + SDH_TX_CFG_REG_OFFSET);
	val32 |= BIT(SDH_TX_INT_CLK_SEL_POS);
	sys_write32(val32, base + SDH_TX_CFG_REG_OFFSET);
}

/* Send active clock pulses on SD_CLK via vendor pad clock mechanism.
 * Matches SDK SDH_CMD_ACTIVE_CLK_OUT (V3-only): generates 'count' clock
 * cycles on the SD bus before CMD0, as required by SD spec for card init.
 * SDK calls this with count=80 right before CMD0 (sdh_sd_go_idle).
 */
static void sdhc_bflb_send_active_clk(uintptr_t base, uint8_t count)
{
	uint16_t val16;
	uint32_t val32;

	/* Enable and clear misc completed status (CE_ATA_2 register, 0x10E) */
	val16 = sys_read16(base + SDH_SD_CE_ATA_2_OFFSET);
	val16 |= SDH_MISC_INT_MSK;    /* W1C: clear pending */
	val16 |= SDH_MISC_INT_EN_MSK; /* enable misc interrupt status */
	sys_write16(val16, base + SDH_SD_CE_ATA_2_OFFSET);

	/* Set pad clock count and start (CFG_FIFO_PARAM register, 0x100) */
	val32 = sys_read32(base + SDH_SD_CFG_FIFO_PARAM_OFFSET);
	val32 &= ~SDH_GEN_PAD_CLK_CNT_MSK;
	val32 |= ((uint32_t)count << SDH_GEN_PAD_CLK_CNT_POS) & SDH_GEN_PAD_CLK_CNT_MSK;
	sys_write32(val32, base + SDH_SD_CFG_FIFO_PARAM_OFFSET);
	val32 |= SDH_GEN_PAD_CLK_ON_MSK;
	sys_write32(val32, base + SDH_SD_CFG_FIFO_PARAM_OFFSET);

	/* Wait for misc completed */
	do {
		val16 = sys_read16(base + SDH_SD_CE_ATA_2_OFFSET);
	} while (!(val16 & SDH_MISC_INT_MSK));

	/* Clear misc completed status (W1C) */
	val16 |= SDH_MISC_INT_MSK;
	sys_write16(val16, base + SDH_SD_CE_ATA_2_OFFSET);
}

static void sdhc_bflb_read_caps(uintptr_t base, struct sdhc_host_props *props)
{
	uint16_t cap1, cap2, cap3, cap4, cur1, cur2;

	cap1 = sys_read16(base + SDH_SD_CAPABILITIES_1_OFFSET);
	cap2 = sys_read16(base + SDH_SD_CAPABILITIES_2_OFFSET);
	cap3 = sys_read16(base + SDH_SD_CAPABILITIES_3_OFFSET);
	cap4 = sys_read16(base + SDH_SD_CAPABILITIES_4_OFFSET);
	cur1 = sys_read16(base + SDH_SD_MAX_CURRENT_1_OFFSET);
	cur2 = sys_read16(base + SDH_SD_MAX_CURRENT_2_OFFSET);

	memset(&props->host_caps, 0, sizeof(props->host_caps));

	props->host_caps.timeout_clk_freq = (cap1 >> SDH_TIMEOUT_FREQ_POS) & GENMASK(5, 0);
	props->host_caps.timeout_clk_unit = (cap1 >> SDH_TIMEOUT_UNIT_POS) & 0x1;
	props->host_caps.sd_base_clk = (cap1 >> SDH_BASE_FREQ_POS) & GENMASK(7, 0);
	props->host_caps.max_blk_len = (cap2 >> SDH_MAX_BLK_LEN_POS) & GENMASK(1, 0);
	props->host_caps.bus_8_bit_support = (cap2 >> SDH_EX_DATA_WIDTH_SUPPORT_POS) & 0x1;
	props->host_caps.adma_2_support = (cap2 >> SDH_ADMA2_SUPPORT_POS) & 0x1;
	props->host_caps.high_spd_support = (cap2 >> SDH_HI_SPEED_SUPPORT_POS) & 0x1;
	props->host_caps.sdma_support = (cap2 >> SDH_SDMA_SUPPORT_POS) & 0x1;
	props->host_caps.suspend_res_support = (cap2 >> SDH_SUS_RES_SUPPORT_POS) & 0x1;
	props->host_caps.vol_330_support = (cap2 >> SDH_VLG_33_SUPPORT_POS) & 0x1;
	props->host_caps.vol_300_support = (cap2 >> SDH_VLG_30_SUPPORT_POS) & 0x1;
	props->host_caps.vol_180_support = (cap2 >> SDH_VLG_18_SUPPORT_POS) & 0x1;

	props->host_caps.sdr50_support = (cap3 >> SDH_SDR50_SUPPORT_POS) & 0x1;
	props->host_caps.sdr104_support = (cap3 >> SDH_SDR104_SUPPORT_POS) & 0x1;
	props->host_caps.ddr50_support = (cap3 >> SDH_DDR50_SUPPORT_POS) & 0x1;
	props->host_caps.drv_type_a_support = (cap3 >> SDH_DRV_TYPE_A_POS) & 0x1;
	props->host_caps.drv_type_c_support = (cap3 >> SDH_DRV_TYPE_C_POS) & 0x1;
	props->host_caps.drv_type_d_support = (cap3 >> SDH_DRV_TYPE_D_POS) & 0x1;
	props->host_caps.retune_timer_count = (cap3 >> SDH_TMR_RETUNE_POS) & GENMASK(3, 0);
	props->host_caps.sdr50_needs_tuning = (cap3 >> SDH_SDR50_TUNE_POS) & 0x1;
	props->host_caps.retuning_mode = (cap3 >> SDH_RETUNE_MODES_POS) & GENMASK(1, 0);

	props->host_caps.clk_multiplier = (cap4 >> SDH_CLK_MULTIPLIER_POS) & GENMASK(7, 0);

	/* Max current capabilities (in mA, 4mA steps per SDHCI spec) */
	props->max_current_330 = ((cur1 >> SDH_MAX_CUR_33_POS) & GENMASK(7, 0)) * 4;
	props->max_current_300 = ((cur1 >> SDH_MAX_CUR_30_POS) & GENMASK(7, 0)) * 4;
	props->max_current_180 = ((cur2 >> SDH_MAX_CUR_18_POS) & GENMASK(7, 0)) * 4;

	/* V3 clock path: frequency controlled by GLB divider (1-8), SDHCI bypass.
	 * f_max = 96MHz / 1 = 96MHz, f_min = 96MHz / 8 = 12MHz.
	 * The caps register base_clk value is not meaningful for V3 since
	 * the GLB divider dynamically changes the effective base clock.
	 */
	props->f_max = SDH_SRC_CLK_HZ;
	props->f_min = SDH_SRC_CLK_HZ / SDH_GLB_DIV_MAX;

	props->bus_4_bit_support = true;
	props->is_spi = false;
}

static int sdhc_bflb_set_clock(uintptr_t base, uint32_t freq_hz)
{
	uint16_t val;
	uint32_t divider;
	uint32_t actual_hz;

	/* Step 1: Disable SD output clock (SDK SDH_CMD_SET_BUS_CLK_EN false) */
	val = sys_read16(base + SDH_SD_CLOCK_CTRL_OFFSET);
	val &= ~BIT(SDH_SD_CLK_EN_POS);
	sys_write16(val, base + SDH_SD_CLOCK_CTRL_OFFSET);

	if (freq_hz == 0) {
		return 0;
	}

	if (freq_hz > SDH_SRC_CLK_HZ) {
		return -EINVAL;
	}

	/* Step 2: V3 clock path — frequency via GLB divider, SDHCI bypass. */
	divider = (SDH_SRC_CLK_HZ + freq_hz - 1U) / freq_hz;
	if (divider > SDH_GLB_DIV_MAX) {
		divider = SDH_GLB_DIV_MAX;
	} else if (divider == 0) {
		divider = 1U;
	}
	actual_hz = SDH_SRC_CLK_HZ / divider;

	/* Update GLB SDH clock divider */
	sdhc_bflb_set_glb_clk_div(divider);

	/* Step 3: SDHCI divider bypass — set freq_sel = 0.
	 * Matches SDK SDH_CMD_SET_BUS_CLK_DIV(0): arg=(0+1)/2=0 → bypass.
	 */
	val = sys_read16(base + SDH_SD_CLOCK_CTRL_OFFSET);
	val &= ~(SDH_SD_FREQ_SEL_LO_MSK | SDH_SD_FREQ_SEL_HI_MSK);
	sys_write16(val, base + SDH_SD_CLOCK_CTRL_OFFSET);

	/* Step 4: HS mode based on actual frequency (matches SDK) */
	val = sys_read16(base + SDH_SD_HOST_CTRL_OFFSET);
	if (actual_hz > MHZ(25U)) {
		val |= BIT(SDH_HI_SPEED_EN_POS);
	} else {
		val &= ~BIT(SDH_HI_SPEED_EN_POS);
	}
	sys_write16(val, base + SDH_SD_HOST_CTRL_OFFSET);

	/* Step 5: Enable SD output clock (SDK SDH_CMD_SET_BUS_CLK_EN true).
	 * SDK does not re-enable internal clock or wait for stable here —
	 * internal clock was enabled once during init and stays running.
	 */
	val = sys_read16(base + SDH_SD_CLOCK_CTRL_OFFSET);
	val |= BIT(SDH_SD_CLK_EN_POS);
	sys_write16(val, base + SDH_SD_CLOCK_CTRL_OFFSET);

	LOG_DBG("Set clock: requested %u Hz, actual %u Hz (GLB div=%u)",
		freq_hz, actual_hz, divider);

	return 0;
}

/* Build CMD register lower byte (0x0E) — response type, CRC/index check, cmd type.
 * Matches lhal bflb_sdh_cmd_cfg(): 8-bit read-modify-write of CMD lower byte.
 * CMD_INDEX (bits 13:8) is written separately to CMD+1 (0x0F) to trigger.
 */
static uint8_t sdhc_bflb_encode_cmd_lo(struct sdhc_command *cmd)
{
	uint8_t val = 0;

	switch (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		/* resp_type=0, no checks */
		break;
	case SD_RSP_TYPE_R2:
		val |= (1U << SDH_RESP_TYPE_POS);
		val |= SDH_CMD_CRC_CHK_EN_MSK;
		break;
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		val |= (2U << SDH_RESP_TYPE_POS);
		break;
	case SD_RSP_TYPE_R1b:
	case SD_RSP_TYPE_R5b:
		val |= (3U << SDH_RESP_TYPE_POS);
		val |= SDH_CMD_CRC_CHK_EN_MSK;
		val |= SDH_CMD_INDEX_CHK_EN_MSK;
		break;
	default:
		/* R1, R5, R6, R7: 48-bit with CRC+index check */
		val |= (2U << SDH_RESP_TYPE_POS);
		val |= SDH_CMD_CRC_CHK_EN_MSK;
		val |= SDH_CMD_INDEX_CHK_EN_MSK;
		break;
	}

	return val;
}

static int sdhc_bflb_wait_irq(struct sdhc_bflb_data *data, uint16_t mask, int timeout_ms)
{
	while (!(data->irq_status & mask)) {
		if (data->irq_err) {
			return -EIO;
		}
		if (k_sem_take(&data->irq_sem, K_MSEC(timeout_ms)) != 0) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

#if !defined(CONFIG_SDHC_BFLB_DMA)

static int sdhc_bflb_wait_for(uintptr_t base, uint16_t status_offset, uint16_t mask, int timeout_ms)
{
	k_timepoint_t end_timeout = sys_timepoint_calc(K_MSEC(timeout_ms));
	uint16_t err;

	do {
		if ((sys_read16(base + status_offset) & mask) != 0) {
			return 0;
		}
		/* Check error status */
		err = sys_read16(base + SDH_SD_ERROR_INT_STATUS_OFFSET);
		if (err) {
			return -EIO;
		}
		k_busy_wait(10);
	} while (!sys_timepoint_expired(end_timeout));

	return -ETIMEDOUT;
}

#endif

static void sdhc_bflb_read_response(uintptr_t base, struct sdhc_command *cmd)
{
	uint8_t resp_type = cmd->response_type & SDHC_NATIVE_RESPONSE_MASK;

	if (resp_type == SD_RSP_TYPE_NONE) {
		return;
	}

	if (resp_type == SD_RSP_TYPE_R2) {
		/* 136-bit response: SDHCI stores bits [127:8] of the CSD/CID
		 * (120 bits, CRC stripped) right-shifted in 128-bit register space.
		 * Zephyr SD subsystem expects the full 128-bit CSD/CID layout
		 * with response[3] bits [31:30] = CSD_STRUCTURE. Apply 8-bit
		 * left shift to realign, matching Cadence/SAM/Renesas/Xilinx.
		 */
		uint16_t r0 = sys_read16(base + SDH_SD_RESP_0_OFFSET);
		uint16_t r1 = sys_read16(base + SDH_SD_RESP_1_OFFSET);
		uint16_t r2 = sys_read16(base + SDH_SD_RESP_2_OFFSET);
		uint16_t r3 = sys_read16(base + SDH_SD_RESP_3_OFFSET);
		uint16_t r4 = sys_read16(base + SDH_SD_RESP_4_OFFSET);
		uint16_t r5 = sys_read16(base + SDH_SD_RESP_5_OFFSET);
		uint16_t r6 = sys_read16(base + SDH_SD_RESP_6_OFFSET);
		uint16_t r7 = sys_read16(base + SDH_SD_RESP_7_OFFSET);

		uint32_t w0 = ((uint32_t)r1 << 16) | r0;
		uint32_t w1 = ((uint32_t)r3 << 16) | r2;
		uint32_t w2 = ((uint32_t)r5 << 16) | r4;
		uint32_t w3 = ((uint32_t)r7 << 16) | r6;

		/* 8-bit left shift: move bits [119:0] → [127:8] */
		cmd->response[3] = (w3 << 8) | (w2 >> 24);
		cmd->response[2] = (w2 << 8) | (w1 >> 24);
		cmd->response[1] = (w1 << 8) | (w0 >> 24);
		cmd->response[0] = (w0 << 8);
	} else {
		/* 48-bit response: 32 bits in RESP[1:0] */
		uint16_t r0 = sys_read16(base + SDH_SD_RESP_0_OFFSET);
		uint16_t r1 = sys_read16(base + SDH_SD_RESP_1_OFFSET);

		cmd->response[0] = ((uint32_t)r1 << 16) | r0;
	}
}

#ifdef CONFIG_SDHC_BFLB_DMA

static void sdhc_bflb_setup_adma2(uintptr_t base, struct sdhc_data *data, bool is_read)
{
	static struct adma2_desc desc __aligned(4);
	uint32_t total_len = data->block_size * data->blocks;

	desc.attr = ADMA2_ATTR_VALID | ADMA2_ATTR_END | ADMA2_ATTR_TRAN;
	/* Code will throw an ADMA_ERR if more than this amount is read at once */
	desc.len = (total_len >= 65536) ? 0 : (uint16_t)total_len;
	desc.addr = (uint32_t)(uintptr_t)data->data;

	sys_cache_data_flush_range(&desc, sizeof(desc));
	if (!is_read) {
		sys_cache_data_flush_range(data->data, total_len);
	}

	/* Write 32-bit descriptor table address to ADMA system address regs */
	sys_write16((uint16_t)((uint32_t)(uintptr_t)&desc & 0xFFFF),
		    base +  SDH_SD_ADMA_SYS_ADDR_1_OFFSET);

	sys_write16((uint16_t)((uint32_t)(uintptr_t)&desc >> 16),
		    base + SDH_SD_ADMA_SYS_ADDR_2_OFFSET);
}

#else

static int sdhc_bflb_xfer_data_pio(uintptr_t base, struct sdhc_data *data, bool is_read)
{
	uint32_t total_words;
	uint32_t *buf = (uint32_t *)data->data;
	int ret;

	total_words = (data->block_size * data->blocks) / 4;
	data->bytes_xfered = 0;

	for (uint32_t blk = 0; blk < data->blocks; blk++) {
		uint32_t blk_words = data->block_size / 4;
		uint16_t status_bit =
			is_read ? BIT(SDH_BUFFER_RD_EN_POS) : BIT(SDH_BUFFER_WR_EN_POS);

		/* Wait for buffer ready */
		ret = sdhc_bflb_wait_for(base, SDH_SD_PRESENT_STATE_1_OFFSET, status_bit,
					 data->timeout_ms);
		if (ret != 0) {
			LOG_ERR("Buffer not ready (blk %u, %s)", blk,
				is_read ? "rd" : "wr");
			return ret;
		}

		for (uint32_t i = 0; i < blk_words; i++) {
			if (is_read) {
				uint16_t lo = sys_read16(base + SDH_SD_BUFFER_DATA_PORT_0_OFFSET);
				uint16_t hi = sys_read16(base + SDH_SD_BUFFER_DATA_PORT_1_OFFSET);
				*buf++ = ((uint32_t)hi << 16) | lo;
			} else {
				uint32_t w = *buf++;

				sys_write16((uint16_t)(w & 0xffff),
					    base + SDH_SD_BUFFER_DATA_PORT_0_OFFSET);
				sys_write16((uint16_t)(w >> 16),
					    base + SDH_SD_BUFFER_DATA_PORT_1_OFFSET);
			}
		}
		data->bytes_xfered += data->block_size;
	}

	ARG_UNUSED(total_words);
	return 0;
}

#endif

/* --- Driver API --- */

static int sdhc_bflb_reset(const struct device *dev)
{
	const struct sdhc_bflb_config *cfg = dev->config;
	uintptr_t base = cfg->base;
	uint16_t val;
	int ret;

	/* lhal's SDH_CMD_SOFT_RESET_ALL handler is empty — it doesn't do
	 * SW_RST_ALL. Re-initialize key registers matching bflb_sdh_init().
	 */
	ret = sdhc_bflb_enable_internal_clock(base);
	if (ret) {
		return ret;
	}

	/* Re-apply HOST_CTRL settings */
	val = sys_read16(base + SDH_SD_HOST_CTRL_OFFSET);
	val &= ~SDH_DMA_SEL_MSK;
	val |= (2U << SDH_DMA_SEL_POS); /* ADMA2 */
	val &= ~SDH_SD_BUS_VLT_MSK;
	val |= (SD_BUS_VLT_33 << SDH_SD_BUS_VLT_POS);
	sys_write16(val, base + SDH_SD_HOST_CTRL_OFFSET);

	sdhc_bflb_configure_vendor_regs(base);
	sdhc_bflb_enable_status_bits(base);

	return 0;
}

static int sdhc_bflb_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct sdhc_bflb_config *cfg = dev->config;
	struct sdhc_bflb_data *data = dev->data;
	uintptr_t base = cfg->base;
	uint16_t val;
	int ret;

	/* Power mode */
	if (ios->power_mode != data->host_io.power_mode) {
		val = sys_read16(base + SDH_SD_HOST_CTRL_OFFSET);
		if (ios->power_mode == SDHC_POWER_ON) {
			val |= BIT(SDH_SD_BUS_POWER_POS);
			val &= ~SDH_SD_BUS_VLT_MSK;
			val |= (SD_BUS_VLT_33 << SDH_SD_BUS_VLT_POS);
		} else {
			val &= ~BIT(SDH_SD_BUS_POWER_POS);
		}
		sys_write16(val, base + SDH_SD_HOST_CTRL_OFFSET);
		data->host_io.power_mode = ios->power_mode;
	}

	/* Clock */
	if (ios->clock != data->host_io.clock) {
		ret = sdhc_bflb_set_clock(base, ios->clock);
		if (ret != 0) {
			return ret;
		}
		data->host_io.clock = ios->clock;
	}

	/* Bus width */
	if (ios->bus_width != data->host_io.bus_width) {
		val = sys_read16(base + SDH_SD_HOST_CTRL_OFFSET);
		val &= ~(BIT(SDH_DATA_WIDTH_POS) | BIT(SDH_EX_DATA_WIDTH_POS));
		if (ios->bus_width == SDHC_BUS_WIDTH4BIT) {
			val |= BIT(SDH_DATA_WIDTH_POS);
		} else if (ios->bus_width == SDHC_BUS_WIDTH8BIT) {
			val |= BIT(SDH_EX_DATA_WIDTH_POS);
		}
		sys_write16(val, base + SDH_SD_HOST_CTRL_OFFSET);
		data->host_io.bus_width = ios->bus_width;
	}

	/* Timing / high-speed */
	if (ios->timing != data->host_io.timing) {
		val = sys_read16(base + SDH_SD_HOST_CTRL_OFFSET);
		if (ios->timing == SDHC_TIMING_HS || ios->timing == SDHC_TIMING_SDR25) {
			val |= BIT(SDH_HI_SPEED_EN_POS);
		} else {
			val &= ~BIT(SDH_HI_SPEED_EN_POS);
		}
		sys_write16(val, base + SDH_SD_HOST_CTRL_OFFSET);
		data->host_io.timing = ios->timing;
	}

	return 0;
}

static int sdhc_bflb_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *sdhc_data_i)
{
	const struct sdhc_bflb_config *cfg = dev->config;
	struct sdhc_bflb_data *data = dev->data;
	k_timepoint_t end_timeout;
	uintptr_t base = cfg->base;
	uint8_t cmd_lo;
	uint16_t xfer_mode;
	int ret;
	uint16_t err;
	bool has_data = (sdhc_data_i != NULL);
	bool is_read = true;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Send 80 active clock pulses before CMD0 (matches SDK sdh_sd_go_idle) */
	if (cmd->opcode == 0) {
		sdhc_bflb_send_active_clk(base, SDH_ACTIVATE_CLK_CNT);
	}

	/* Wait for CMD line free (CMD_INHIBIT_CMD clear) */
	end_timeout = sys_timepoint_calc(K_MSEC(cmd->timeout_ms));

	while ((sys_read16(base + SDH_SD_PRESENT_STATE_1_OFFSET) & BIT(SDH_CMD_INHIBIT_CMD_POS))
	       != 0 && !sys_timepoint_expired(end_timeout)) {
		k_busy_wait(10);
	}

	if (sys_timepoint_expired(end_timeout)) {
		LOG_ERR("CMD inhibit timeout");
		ret = -ETIMEDOUT;
		goto exit_unlock;
	}

	/* If busy-response or data, also wait for DAT line free */
	if (has_data || (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) == SD_RSP_TYPE_R1b ||
	    (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) == SD_RSP_TYPE_R5b) {

		while ((sys_read16(base + SDH_SD_PRESENT_STATE_1_OFFSET) &
				BIT(SDH_CMD_INHIBIT_DAT_POS)) != 0
				&& !sys_timepoint_expired(end_timeout)) {
			k_busy_wait(10);
		}

		if (sys_timepoint_expired(end_timeout)) {
			LOG_ERR("DAT inhibit timeout");
			ret = -ETIMEDOUT;
			goto exit_unlock;
		}
	}

	/* Clear pending status and reset IRQ state */
	sys_write32((0xffffU << 16) | BIT(SDH_CMD_COMPLETE_POS) | BIT(SDH_XFER_COMPLETE_POS),
		    base + SDH_SD_NORMAL_INT_STATUS_OFFSET);
	data->irq_status = 0;
	data->irq_err = 0;
	k_sem_reset(&data->irq_sem);

	/* ---- Phase 1: cmd_cfg (lhal bflb_sdh_cmd_cfg) ----
	 * Write argument, then configure CMD register lower byte (0x0E)
	 * via 8-bit write. This does NOT trigger the command — the trigger
	 * happens when the upper byte (0x0F) is written in phase 3.
	 */
	sys_write32(cmd->arg, base + SDH_SD_ARG_LOW_OFFSET);

	cmd_lo = sdhc_bflb_encode_cmd_lo(cmd);
	sys_write8(cmd_lo, base + SDH_SD_CMD_OFFSET);

	/* ---- Phase 2: data_cfg (lhal bflb_sdh_data_cfg) ----
	 * Configure transfer mode, data present, block size/count.
	 */
	if (has_data) {
		/* Set DATA_PRESENT in CMD lower byte */
		cmd_lo |= BIT(SDH_DATA_PRESENT_POS);
		sys_write8(cmd_lo, base + SDH_SD_CMD_OFFSET);

		/* Check if write command */
		if (cmd->opcode == 24 || cmd->opcode == 25) {
			is_read = false;
		}

		/* Transfer mode */
		xfer_mode = sys_read16(base + SDH_SD_TRANSFER_MODE_OFFSET);
		xfer_mode &= ~(SDH_TO_HOST_DIR_MSK | SDH_AUTO_CMD_EN_MSK | SDH_MULTI_BLK_SEL_MSK |
			       SDH_BLK_CNT_EN_MSK | SDH_DMA_EN_MSK);
		if (is_read) {
			xfer_mode |= SDH_TO_HOST_DIR_MSK;
		}
		if (sdhc_data_i->blocks > 1) {
			xfer_mode |= SDH_MULTI_BLK_SEL_MSK;
			/* AUTO_CMD12: Zephyr SD subsystem does not send CMD12
			 * explicitly — SDHC driver must handle it.
			 */
			xfer_mode |= (1U << SDH_AUTO_CMD_EN_POS);
		}
		if (sdhc_data_i->blocks > 0) {
			xfer_mode |= SDH_BLK_CNT_EN_MSK;
		}

#ifdef CONFIG_SDHC_BFLB_DMA
		xfer_mode |= SDH_DMA_EN_MSK;
#endif
		sys_write16(xfer_mode, base + SDH_SD_TRANSFER_MODE_OFFSET);

		/* Block count and size */
		sys_write16(sdhc_data_i->blocks, base + SDH_SD_BLOCK_COUNT_OFFSET);
		sys_write16(sdhc_data_i->block_size & SDH_BLOCK_SIZE_MSK,
			    base + SDH_SD_BLOCK_SIZE_OFFSET);
	} else {
		/* No data: clear DATA_PRESENT in CMD lower byte */
		cmd_lo &= ~BIT(SDH_DATA_PRESENT_POS);
		sys_write8(cmd_lo, base + SDH_SD_CMD_OFFSET);

		/* Clear transfer mode: disable auto cmd, multi-block, DMA */
		xfer_mode = sys_read16(base + SDH_SD_TRANSFER_MODE_OFFSET);
		xfer_mode &= ~(SDH_AUTO_CMD_EN_MSK | SDH_MULTI_BLK_SEL_MSK | SDH_DMA_EN_MSK);
		sys_write16(xfer_mode, base + SDH_SD_TRANSFER_MODE_OFFSET);
	}

#ifdef CONFIG_SDHC_BFLB_DMA
	/* Set up ADMA2 descriptor before triggering the command */
	if (has_data) {
		sdhc_bflb_setup_adma2(base, sdhc_data_i, is_read);
	}
#endif

	/* ---- Phase 3: Trigger command (lhal bflb_sdh_transfer_start) ----
	 * Write CMD_INDEX to CMD register upper byte (0x0F) via 8-bit write.
	 * Per SDHCI spec, writing the upper byte of the Command Register
	 * triggers command generation on the SD bus.
	 */
	sys_write8(cmd->opcode, base + SDH_SD_CMD_OFFSET + 1U);

	/* Wait for command complete */
	ret = sdhc_bflb_wait_irq(data, BIT(SDH_CMD_COMPLETE_POS), CMD_TIMEOUT_MS);
	if (ret != 0) {
		err = data->irq_err;
		if (err & BIT(SDH_CMD_TIMEOUT_ERR_POS)) {
			LOG_DBG("CMD%d timeout", cmd->opcode);
			ret = -ETIMEDOUT;
		} else {
			LOG_ERR("CMD%d error: 0x%04x", cmd->opcode, err);
			ret = -EIO;
		}
		sdhc_bflb_sw_reset(base, BIT(SDH_SW_RST_CMD_POS));
		if (has_data) {
			sdhc_bflb_sw_reset(base, BIT(SDH_SW_RST_DAT_POS));
		}
		goto exit_unlock;
	}

	/* Read response */
	sdhc_bflb_read_response(base, cmd);

	/* Data transfer phase */
	if (has_data) {
#ifdef CONFIG_SDHC_BFLB_DMA
		/* Wait for transfer complete (ADMA2 runs autonomously) */
		ret = sdhc_bflb_wait_irq(data, BIT(SDH_XFER_COMPLETE_POS), sdhc_data_i->timeout_ms);
		if (ret) {
			LOG_ERR("Xfer complete timeout/error: 0x%04x", data->irq_err);
			goto exit_sw_reset;
		}

		if (is_read) {
			sys_cache_data_invd_range(sdhc_data_i->data,
						  sdhc_data_i->block_size * sdhc_data_i->blocks);
		}
		sdhc_data_i->bytes_xfered = sdhc_data_i->block_size * sdhc_data_i->blocks;
#else
		ret = sdhc_bflb_xfer_data_pio(base, sdhc_data_i, is_read);
		if (ret != 0) {
			goto exit_sw_reset;
		}

		/* Wait for transfer complete */
		ret = sdhc_bflb_wait_irq(data, BIT(SDH_XFER_COMPLETE_POS), sdhc_data_i->timeout_ms);
		if (ret != 0) {
			LOG_ERR("Xfer complete timeout/error: 0x%04x", data->irq_err);
			goto exit_sw_reset;
		}
#endif
	}

	ret = 0;

exit_sw_reset:
	sdhc_bflb_sw_reset(base, BIT(SDH_SW_RST_DAT_POS));
exit_unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int sdhc_bflb_get_card_present(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* SDK never checks CARD_INSERTED/CARD_STABLE bits and many
	 * boards (e.g. M1S Dock) lack a card-detect pin, so these bits
	 * stay at their reset default of 0.  Always report present.
	 */

	return 1;
}

static int sdhc_bflb_card_busy(const struct device *dev)
{
	const struct sdhc_bflb_config *cfg = dev->config;
	uint16_t state;

	state = sys_read16(cfg->base + SDH_SD_PRESENT_STATE_2_OFFSET);

	/* DAT[0] is bit 4 of DAT_LEVEL field. Low = busy. */
	if ((state & BIT(SDH_DAT_LEVEL_POS)) == 0) {
		return 1; /* busy */
	}

	return 0; /* not busy */
}

static int sdhc_bflb_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct sdhc_bflb_data *data = dev->data;

	*props = data->props;
	return 0;
}

static DEVICE_API(sdhc, sdhc_bflb_api) = {
	.reset = sdhc_bflb_reset,
	.request = sdhc_bflb_request,
	.set_io = sdhc_bflb_set_io,
	.get_card_present = sdhc_bflb_get_card_present,
	.card_busy = sdhc_bflb_card_busy,
	.get_host_props = sdhc_bflb_get_host_props,
};

static int sdhc_bflb_init(const struct device *dev)
{
	const struct sdhc_bflb_config *cfg = dev->config;
	const struct device *clock_ctrl = DEVICE_DT_GET_ANY(bflb_clock_controller);
	struct sdhc_bflb_data *data = dev->data;
	uintptr_t base = cfg->base;
	uint16_t val;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->irq_sem, 0, 1);

	/* Match SDK sdh_host_init() + bflb_sdh_init() order exactly:
	 * Board-level: GLB_Set_SDH_CLK → PERIPHERAL_CLOCK_SDH_ENABLE →
	 *              GLB_AHB_MCU_Software_Reset → pinctrl
	 * bflb_sdh_init: INT_CLK → HOST_CTRL (DMA_SEL+voltage) →
	 *                CLK_BURST_SETUP → TX_CFG → status enable
	 * Then: sdh_set_bus_clock(400k) → bus power on
	 */

	/* WIFIPLL must be on */
	if (clock_control_get_status(clock_ctrl, (void *)SDH_SRC_CLK_0_ID)
	    != CLOCK_CONTROL_STATUS_ON) {
		LOG_ERR("Source clock is unavailable");
		return -ENOTSUP;
	}

	/* Configure SDH source clock */
	sdhc_bflb_set_glb_clk_div(SDH_GLB_DIV_INIT);

	/* GLB bus-level reset for SDH (SDK: GLB_AHB_MCU_Software_Reset) */
	sdhc_bflb_bus_reset();

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl: %d", ret);
		return ret;
	}

	/* Enable internal clock, wait stable (lhal bflb_sdh_init) */
	ret = sdhc_bflb_enable_internal_clock(base);
	if (ret != 0) {
		return ret;
	}

	/* HOST_CTRL — set DMA_SEL=ADMA2 + power voltage (no bus power bit).
	 * Matches lhal: DMA select for ADMA2, voltage for 3.3V.
	 * Bus power bit is set later via set_io().
	 */
	val = sys_read16(base + SDH_SD_HOST_CTRL_OFFSET);
	val &= ~SDH_DMA_SEL_MSK;
	val |= (2U << SDH_DMA_SEL_POS); /* ADMA2 */
	val &= ~SDH_SD_BUS_VLT_MSK;
	val |= (SD_BUS_VLT_33 << SDH_SD_BUS_VLT_POS);
	sys_write16(val, base + SDH_SD_HOST_CTRL_OFFSET);

	/* Configure vendor-specific registers (CLK_BURST_SETUP + TX_CFG) */
	sdhc_bflb_configure_vendor_regs(base);

	/*  Enable status bits and interrupt signals */
	sdhc_bflb_enable_status_bits(base);

	/* Connect and enable IRQ */
	cfg->irq_config_func();

	/* Set data timeout to maximum */
	sdhc_bflb_set_timeout_max(base);

	/* Read capabilities into host_props */
	data->props.power_delay = cfg->power_delay_ms;
	data->props.hs200_support = false;
	data->props.hs400_support = false;
	sdhc_bflb_read_caps(base, &data->props);

	memset(&data->host_io, 0, sizeof(data->host_io));

	return 0;
}

#define SDHC_BFLB_IRQ_CONFIG(n)                                                                    \
	static void sdhc_bflb_irq_config_##n(void)                                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                             \
			    sdhc_bflb_isr, DEVICE_DT_INST_GET(n), 0);                              \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define SDHC_BFLB_INIT(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	SDHC_BFLB_IRQ_CONFIG(n)                                                                    \
	static const struct sdhc_bflb_config sdhc_bflb_config_##n = {                              \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.bus_width = DT_INST_PROP_OR(n, bus_width, 4),                                     \
		.power_delay_ms = DT_INST_PROP_OR(n, power_delay_ms, 500),                         \
		.irq_config_func = sdhc_bflb_irq_config_##n,                                       \
	};                                                                                         \
	static struct sdhc_bflb_data sdhc_bflb_data_##n = {0};                                     \
	DEVICE_DT_INST_DEFINE(n, sdhc_bflb_init, NULL, &sdhc_bflb_data_##n, &sdhc_bflb_config_##n, \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &sdhc_bflb_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_BFLB_INIT)
