/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MFD parent for the HPPASS analog subsystem.
 *
 * Owns block-wide HPPASS state: subsystem init, the Autonomous Controller
 * (AC) lifecycle and STT, trigger I/O routing, and the combined HPPASS
 * interrupt (AC events + error conditions).  SAR and CSG drivers handle
 * their own peripheral IRQs.
 *
 * HPPASS is shared across multiple Infineon device families. The TRM
 * citations throughout this file refer to the PSOC Control C3 Mainline
 * documentation:
 *   - <em>PSOC Control C3 Mainline Architecture TRM</em>, 002-39348
 *     (§27.2.1 INFRA, §27.2.4 AC, §27.3 interrupts)
 *   - <em>PSOC Control C3 Mainline Registers TRM</em>, 002-39445
 *     (HPPASS_ACTRLR_*)
 */

#define DT_DRV_COMPAT infineon_hppass_analog

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <infineon_hppass.h>
#include <infineon_kconfig.h>

#include "cy_hppass.h"
#include "mfd_infineon_hppass_regs.h"

LOG_MODULE_REGISTER(mfd_infineon_hppass, CONFIG_MFD_LOG_LEVEL);

/* Driver data and config structures */

struct ifx_hppass_mfd_data {
	sys_slist_t callbacks;
	struct k_mutex start_lock;
};

/*
 * Register-layout bitfield overlays.  AAPCS packs unsigned bitfields
 * LSB-first and PSC3 runs little-endian (CFGEND=0), so the layouts below
 * match silicon when read as a uint32_t.  Per-register field widths and
 * offsets per the Registers TRM 002-39445 sections cited on each union.
 */

/* TT_CFG0: SAR/CSG unlock + CSG DOUT lanes (TRM 002-39445 §20.1.71). */
union ifx_hppass_stt_tt_cfg0 {
	struct {
		uint32_t _rsv0:4;       /* [3:0]   rsvd */
		uint32_t sar_unlock:1;  /* [4] */
		uint32_t _rsv1:3;       /* [7:5]   rsvd */
		uint32_t csg_unlock:5;  /* [12:8]  per-CSG unlock (CSG_NUM=5) */
		uint32_t _rsv2:3;       /* [15:13] rsvd */
		uint32_t dout_unlock:1; /* [16] */
		uint32_t _rsv3:3;       /* [19:17] rsvd */
		uint32_t dout:5;        /* [24:20] one bit per CSG channel (DOUT field is 5 wide) */
		uint32_t _rsv4:7;       /* [31:25] rsvd */
	};
	uint32_t raw;
};

/* TT_CFG1: branch addr, condition, action, IRQ, count (TRM 002-39445 §20.1.72). */
union ifx_hppass_stt_tt_cfg1 {
	struct {
		uint32_t br_addr:4;     /* [3:0] */
		uint32_t _rsv0:4;       /* [7:4]   rsvd */
		uint32_t cond:6;        /* [13:8] */
		uint32_t _rsv1:2;       /* [15:14] rsvd */
		uint32_t action:3;      /* [18:16] */
		uint32_t intr_set:1;    /* [19] */
		uint32_t cnt:12;        /* [31:20] */
	};
	uint32_t raw;
};

/* TT_CFG2: per-CSG enable + DAC trigger lanes (TRM 002-39445 §20.1.73). */
union ifx_hppass_stt_tt_cfg2 {
	struct {
		uint32_t csg_enable:5;  /* [4:0]   per-CSG enable (CSG_NUM=5) */
		uint32_t _rsv0:3;       /* [7:5]   rsvd */
		uint32_t csg_dac_tr:5;  /* [12:8]  per-CSG DAC trigger (CSG_NUM=5) */
		uint32_t _rsv1:19;      /* [31:13] rsvd */
	};
	uint32_t raw;
};

/*
 * TT_CFG3: SAR trigger group mask, SAR enable, and analog-route mux control
 * for up to 4 muxes (one unlock bit + a 2-bit channel-index select per mux).
 * Per TRM 002-39445 §20.1.74.  SAR_AROUTE_SEL is 2 bits wide per the
 * Registers TRM; declared as explicit 2-bit fields so a channel index of
 * 2 or 3 is not silently truncated.
 */
union ifx_hppass_stt_tt_cfg3 {
	struct {
		uint32_t sar_tr:8;          /* [7:0] */
		uint32_t sar_en:1;          /* [8] */
		uint32_t _rsv0:3;           /* [11:9]  rsvd */
		uint32_t sar_aroute_tr0:1;  /* [12]    per-mux unlock */
		uint32_t sar_aroute_tr1:1;  /* [13] */
		uint32_t sar_aroute_tr2:1;  /* [14] */
		uint32_t sar_aroute_tr3:1;  /* [15] */
		uint32_t sar_aroute_sel0:2; /* [17:16] per-mux chan_idx */
		uint32_t sar_aroute_sel1:2; /* [19:18] */
		uint32_t sar_aroute_sel2:2; /* [21:20] */
		uint32_t sar_aroute_sel3:2; /* [23:22] */
		uint32_t _rsv1:8;           /* [31:24] rsvd */
	};
	uint32_t raw;
};

BUILD_ASSERT(IFX_HPPASS_CSG_NUM == 5,
	     "TT_CFG bitfield widths assume 5 CSG channels");
BUILD_ASSERT(IFX_HPPASS_SAR_MUX_NUM == 4,
	     "TT_CFG3 bitfield layout assumes 4 SAR muxes");

/*
 * INFRA_STARTUP_CFG[0..3]: per-state count value + SAR/CSG enables for the
 * AC startup cascade.  All four registers share this layout (TRM 002-39445
 * §20.1.80).
 */
union ifx_hppass_infra_startup_cfg {
	struct {
		uint32_t count_val:8;        /* [7:0] */
		uint32_t sar_en:1;           /* [8] */
		uint32_t csg_ch_en:1;        /* [9] */
		uint32_t csg_pwr_en_slice:1; /* [10] */
		uint32_t csg_ready:1;        /* [11] */
		uint32_t _rsv0:20;
	};
	uint32_t raw;
};

/*
 * MMIO_TR_LEVEL_OUT[0..7]: comparator and SAR-range bitmasks ORed into a
 * level-trigger output (TRM 002-39445 §20.1.101).
 */
union ifx_hppass_mmio_tr_level_out {
	struct {
		uint32_t comp_msk:8;     /* [7:0]  CMP_TR */
		uint32_t limit_msk:8;    /* [15:8] SAR_RANGE_TR */
		uint32_t _rsv0:16;
	};
	uint32_t raw;
};

/*
 * 8 trigger slots packed into TR_IN_SEL and HW_TR_MODE.
 *   - HW_TR_MODE (§20.1.76): 4-bit field, 4-bit stride, contiguous.
 *   - TR_IN_SEL  (§20.1.75): 3-bit field at the low end of each 4-bit
 *     stride; bits 3, 7, 11, … 31 are reserved.
 */
#define IFX_HPPASS_TR_BITS_PER_SLOT  4
#define IFX_HPPASS_TR_IN_SEL_WIDTH   3
#define IFX_HPPASS_HW_TR_MODE_WIDTH  4

/*
 * MFD-internal HW configuration: the AC startup table and trigger
 * configuration the MFD itself programs at init.  CSG/SAR child drivers
 * own their own configuration and contribute STT entries via
 * ifx_hppass_mfd_config.
 *
 * Field shapes mirror the target registers so init writes them with no
 * runtime translation: STARTUP_CFG and TR_LEVEL_OUT use the bitfield
 * unions; TR_IN_SEL, HW_TR_MODE, and TR_LEVEL_CFG are pre-packed at
 * compile time from the per-slot DT props.
 */
struct ifx_hppass_hw_cfg {
	/* Divisor in cycle units (1..256, per DT binding).  Written as N-1
	 * to INFRA_CLOCK_STARTUP_DIV.DIV_VAL by init_hw().
	 * TRM 002-39445 §20.1.79.
	 */
	uint16_t startup_clk_div;

	/* Per-line enable mask for AC GPIO outputs (one bit per line that an
	 * STT entry may drive).  Written to ACTRLR_CFG.DOUT_EN.
	 * TRM 002-39445 §20.1.69.
	 */
	uint8_t gpio_out_en_msk;

	union ifx_hppass_infra_startup_cfg startup[CY_HPPASS_STARTUP_CFG_NUM];

	/* INFRA_TR_IN_SEL.  TRM 002-39445 §20.1.75. */
	uint32_t trig_in_sel;

	/* INFRA_HW_TR_MODE.  TRM 002-39445 §20.1.76. */
	uint32_t hw_tr_mode;

	/* MMIO_TR_PULSE_OUT[N].SEL.  TRM 002-39445 §20.1.102. */
	cy_en_hppass_trig_out_pulse_t trig_pulse[CY_HPPASS_TRIG_NUM];

	/* MMIO_TR_LEVEL_OUT[N].  TRM 002-39445 §20.1.101. */
	union ifx_hppass_mmio_tr_level_out trig_level[CY_HPPASS_TRIG_NUM];

	/* MMIO_TR_LEVEL_CFG.BYPASS_SYNC.  Only safe when the consumer treats
	 * the trigger as asynchronous and has a double-sync input.
	 * TRM 002-39445 §20.1.100.
	 */
	uint32_t trig_level_sync_bypass_msk;
};

struct ifx_hppass_mfd_config {
	mem_addr_t base;
	void (*irq_config_func)(const struct device *dev);
	const struct ifx_hppass_hw_cfg *init_cfg;
	const struct ifx_hppass_ac_stt_entry *stt;
	uint8_t num_stt;
};

/* Init helpers */

/* ACTRLR_BLOCK_STATUS.READY non-zero (TRM 002-39445 §20.1.67). */
static inline bool ifx_hppass_ac_is_block_ready(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;

	return (sys_read32(config->base + IFX_HPPASS_AC_BLOCK_STATUS) &
		HPPASS_ACTRLR_BLOCK_STATUS_READY_Msk) != 0;
}

/*
 * Replay the SFLASH-resident trim table into HPPASS infra registers, plus
 * the 16-entry SAR linearity calibration table.  Layout of SFLASH table is
 * fixed by silicon (Regs TRM 5.1.21).
 */
#define IFX_HPPASS_TRIMS_ENTRIES_NUM 7

static int ifx_hppass_apply_factory_trims(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;
	mem_addr_t base = config->base;
	struct trim_entry {
		volatile uint32_t *addr;
		uint32_t value;
	};
	struct trims {
		uint32_t num_entries;
		struct trim_entry entry[IFX_HPPASS_TRIMS_ENTRIES_NUM];
	};

	const struct trims *infra_trims = (const struct trims *)&SFLASH_SAR_INFRA_TRIM_TABLE;

	if (infra_trims->num_entries != IFX_HPPASS_TRIMS_ENTRIES_NUM) {
		return -EIO;
	}

	for (uint32_t i = 0; i < IFX_HPPASS_TRIMS_ENTRIES_NUM; i++) {
		*infra_trims->entry[i].addr = infra_trims->entry[i].value;
	}

	for (uint8_t i = 0; i < IFX_HPPASS_SAR_CAL_LIN_TABLE_SIZE; i++) {
		sys_write32(SFLASH_SAR_CAL_LIN_TABLE(i), base + IFX_HPPASS_SARADC_CALLIN(i));
	}

	return 0;
}

/* AREF "normal operation, internal Vref" enable.  See Regs TRM 20.1.83. */
static inline void ifx_hppass_aref_enable(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;

	sys_write32(IFX_HPPASS_AREF_CTRL_NORMAL_INTERNAL,
		    config->base + IFX_HPPASS_INFRA_AREF_CTRL);
}

/* Pack one STT entry into TT_CFG{0..3}[idx] (Arch TRM §27.2.4.3 Tables 254-256).
 * AC must be stopped before calling.
 */
static void ifx_hppass_stt_pack_one(const struct device *dev, uint8_t idx,
				    const struct ifx_hppass_ac_stt_entry *entry)
{
	union ifx_hppass_stt_tt_cfg0 cfg0 = {.raw = 0};
	union ifx_hppass_stt_tt_cfg1 cfg1 = {.raw = 0};
	union ifx_hppass_stt_tt_cfg2 cfg2 = {.raw = 0};
	union ifx_hppass_stt_tt_cfg3 cfg3 = {.raw = 0};
	uint8_t csg_unlock_msk = 0;
	uint8_t csg_enable_msk = 0;
	uint8_t csg_dac_trig_msk = 0;

	/*
	 * Fold the three per-CSG-slice boolean arrays into packed bitmasks
	 * (one bit per slice) for TT_CFG0.csg_unlock and TT_CFG2.{csg_enable,
	 * csg_dac_tr}, which is the layout the AC hardware expects.
	 */
	for (uint8_t slice_idx = 0; slice_idx < IFX_HPPASS_CSG_NUM; slice_idx++) {
		if (entry->csg_unlock[slice_idx]) {
			csg_unlock_msk |= (uint8_t)BIT(slice_idx);
		}
		if (entry->csg_enable[slice_idx]) {
			csg_enable_msk |= (uint8_t)BIT(slice_idx);
		}
		if (entry->csg_dac_trig[slice_idx]) {
			csg_dac_trig_msk |= (uint8_t)BIT(slice_idx);
		}
	}

	cfg0.csg_unlock  = csg_unlock_msk;
	cfg0.sar_unlock  = entry->sar_unlock ? 1 : 0;
	cfg0.dout_unlock = entry->gpio_out_unlock ? 1 : 0;
	cfg0.dout        = entry->gpio_out_msk;

	cfg1.br_addr  = entry->branch_state_idx;
	cfg1.cond     = (uint32_t)entry->condition;
	cfg1.action   = (uint32_t)entry->action;
	cfg1.intr_set = entry->interrupt ? 1 : 0;
	if (entry->condition == IFX_HPPASS_CONDITION_CNT_DONE && entry->count > 0) {
		cfg1.cnt = (uint32_t)entry->count - 1;
	}

	cfg2.csg_enable = csg_enable_msk;
	cfg2.csg_dac_tr = csg_dac_trig_msk;

	cfg3.sar_tr          = entry->sar_grp_msk;
	cfg3.sar_en          = entry->sar_enable ? 1 : 0;
	cfg3.sar_aroute_tr0  = entry->sar_mux[0].unlock ? 1 : 0;
	cfg3.sar_aroute_tr1  = entry->sar_mux[1].unlock ? 1 : 0;
	cfg3.sar_aroute_tr2  = entry->sar_mux[2].unlock ? 1 : 0;
	cfg3.sar_aroute_tr3  = entry->sar_mux[3].unlock ? 1 : 0;
	cfg3.sar_aroute_sel0 = entry->sar_mux[0].chan_idx;
	cfg3.sar_aroute_sel1 = entry->sar_mux[1].chan_idx;
	cfg3.sar_aroute_sel2 = entry->sar_mux[2].chan_idx;
	cfg3.sar_aroute_sel3 = entry->sar_mux[3].chan_idx;

	const struct ifx_hppass_mfd_config *config = dev->config;
	mem_addr_t base = config->base;

	sys_write32(cfg0.raw, base + IFX_HPPASS_AC_TT_CFG0(idx));
	sys_write32(cfg1.raw, base + IFX_HPPASS_AC_TT_CFG1(idx));
	sys_write32(cfg2.raw, base + IFX_HPPASS_AC_TT_CFG2(idx));
	sys_write32(cfg3.raw, base + IFX_HPPASS_AC_TT_CFG3(idx));
} /* ifx_hppass_stt_pack_one() */

/*
 * Write num_entries STT slots starting at start.  Returns -EBUSY if the
 * AC is running; the hardware drops STT writes while it executes.
 * Caller must validate stt != NULL, num_entries > 0, and range bounds.
 */
static int ifx_hppass_stt_write_range(const struct device *dev, uint8_t num_entries,
				      const struct ifx_hppass_ac_stt_entry *stt,
				      uint8_t start)
{
	if (ifx_hppass_ac_is_running(dev)) {
		return -EBUSY;
	}

	for (uint8_t i = 0; i < num_entries; i++) {
		ifx_hppass_stt_pack_one(dev, start + i, &stt[i]);
	}

	return 0;
}

/*
 * TR_IN_SEL picks HW/FW source per slot; HW_TR_MODE picks edge/level.
 * Both are pre-packed at compile time from per-slot DT props.
 * Architecture TRM 002-39348 §27.2.1.2.
 */
static void ifx_hppass_trigger_in_configure(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;
	mem_addr_t base = config->base;

	sys_write32(config->init_cfg->trig_in_sel, base + IFX_HPPASS_INFRA_TR_IN_SEL);
	sys_write32(config->init_cfg->hw_tr_mode, base + IFX_HPPASS_INFRA_HW_TR_MODE);
}

/*
 * TR_PULSE_OUT picks the AC pulse source; TR_LEVEL_OUT gates compare/limit
 * channels into a level signal; TR_LEVEL_CFG bit picks sync vs bypass per
 * slot (pre-packed from DT).  Registers TRM 002-39445 §20.1.100
 * (TR_LEVEL_CFG), §20.1.101 (TR_LEVEL_OUT), §20.1.102 (TR_PULSE_OUT).
 */
static void ifx_hppass_trigger_out_configure(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;
	const struct ifx_hppass_hw_cfg *cfg = config->init_cfg;
	mem_addr_t base = config->base;

	for (uint8_t i = 0; i < CY_HPPASS_TRIG_NUM; i++) {
		sys_write32((uint32_t)cfg->trig_pulse[i], base + IFX_HPPASS_MMIO_TR_PULSE_OUT(i));
		sys_write32(cfg->trig_level[i].raw, base + IFX_HPPASS_MMIO_TR_LEVEL_OUT(i));
	}

	sys_write32(cfg->trig_level_sync_bypass_msk, base + IFX_HPPASS_MMIO_TR_LEVEL_CFG);
}

/*
 * Bring up HPPASS: enable block, replay factory trims, configure AREF and
 * the startup-clock divider, load the STT, then write trigger config.
 * The CSG and SAR drivers own their own init.
 */
static int ifx_hppass_init_hw(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;
	const struct ifx_hppass_hw_cfg *cfg = config->init_cfg;
	mem_addr_t base = config->base;
	int ret;

	if (cfg == NULL) {
		return -EINVAL;
	}

	if (ifx_hppass_ac_is_running(dev) || ifx_hppass_ac_is_block_ready(dev)) {
		return -EBUSY;
	}

	sys_write32(HPPASS_ACTRLR_CTRL_ENABLED_Msk, base + IFX_HPPASS_AC_CTRL);

	ret = ifx_hppass_apply_factory_trims(dev);
	if (ret != 0) {
		return ret;
	}

	if (!CY_HPPASS_DIVSTCLK_VALID(cfg->startup_clk_div)) {
		return -EINVAL;
	}

	ifx_hppass_aref_enable(dev);

	sys_write32(FIELD_PREP(HPPASS_INFRA_CLOCK_STARTUP_DIV_DIV_VAL_Msk,
			       (uint32_t)cfg->startup_clk_div - 1),
		    base + IFX_HPPASS_INFRA_CLOCK_STARTUP_DIV);

	sys_write32(FIELD_PREP(HPPASS_ACTRLR_CFG_DOUT_EN_Msk, cfg->gpio_out_en_msk),
		    base + IFX_HPPASS_AC_CFG);

	if (config->stt == NULL || config->num_stt == 0 ||
	    config->num_stt > IFX_HPPASS_AC_STT_SIZE) {
		return -EINVAL;
	}
	ret = ifx_hppass_stt_write_range(dev, config->num_stt, config->stt, 0);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t i = 0; i < CY_HPPASS_STARTUP_CFG_NUM; i++) {
		sys_write32(cfg->startup[i].raw, base + IFX_HPPASS_INFRA_STARTUP_CFG(i));
	}

	ifx_hppass_trigger_in_configure(dev);
	ifx_hppass_trigger_out_configure(dev);

	return 0;
}

/*
 * Start the AC at start_state.  With timeout_us > 0, busy-wait up to that
 * many microseconds for BLOCK_READY.  Pass 0 for restarts where the block
 * is already powered.
 */
int ifx_hppass_ac_start(const struct device *dev, uint8_t start_state,
		     uint16_t timeout_us)
{
	const struct ifx_hppass_mfd_config *config = dev->config;
	mem_addr_t base = config->base;

	if (start_state >= IFX_HPPASS_AC_STT_SIZE) {
		return -EINVAL;
	}
	if (ifx_hppass_ac_is_running(dev)) {
		return -EALREADY;
	}
	if ((sys_read32(base + IFX_HPPASS_INFRA_VDDA_STATUS) &
	     HPPASS_INFRA_VDDA_STATUS_VDDA_OK_Msk) == 0) {
		return -EIO;
	}

	sys_write32(FIELD_PREP(HPPASS_ACTRLR_CMD_STATE_STATE_Msk, start_state),
		    base + IFX_HPPASS_AC_CMD_STATE);
	sys_write32(HPPASS_ACTRLR_CMD_RUN_RUN_Msk, base + IFX_HPPASS_AC_CMD_RUN);

	if (timeout_us != 0) {
		while (!ifx_hppass_ac_is_block_ready(dev) && (timeout_us != 0)) {
			timeout_us--;
			k_busy_wait(1);
		}
		if (timeout_us == 0) {
			return -ETIMEDOUT;
		}
	}

	return 0;
} /* ifx_hppass_ac_start() */

/*
 * AC start gate.
 *
 * ifx_hppass_late_start() runs at APPLICATION init, after all POST_KERNEL
 * device inits, so child CFG writes land inside the AC unlock window.
 * The basic-mode STT then self-stops via ACTION_STOP.
 *
 * Children that need the AC executing (one-shot custom STT, runtime BIST,
 * etc.) call ifx_hppass_ensure_running().  It serialises the start
 * sequence across children and re-kicks the AC whenever the hardware
 * reads stopped, since basic-mode STTs may have self-stopped between
 * callers.
 *
 * No firmware-side stop is available.  CMD_RUN.RUN is RW1S
 * (Regs TRM 002-39445 §20.1.65); the AC stops only via STT
 * STOP action (Arch TRM §27.2.4.3 Table 256).  On-demand halt must be
 * encoded in the STT, driven by ifx_hppass_ac_set_fw_trigger_level().
 */

#define IFX_HPPASS_ENSURE_RUNNING_TIMEOUT_US 1000

int ifx_hppass_ensure_running(const struct device *dev)
{
	struct ifx_hppass_mfd_data *data;
	int ret = 0;

	if (dev == NULL) {
		return -EINVAL;
	}

	data = dev->data;

	k_mutex_lock(&data->start_lock, K_FOREVER);
	if (!ifx_hppass_ac_is_running(dev)) {
		ret = ifx_hppass_ac_start(dev, 0, IFX_HPPASS_ENSURE_RUNNING_TIMEOUT_US);
		/* Out-of-band caller may have already started it. */
		if (ret == -EALREADY) {
			ret = 0;
		}
	}
	k_mutex_unlock(&data->start_lock);
	return ret;
} /* ifx_hppass_ensure_running() */

/*
 * Replace the STT in place.  Caller must ensure the AC is stopped — TT_CFG
 * writes while the AC is executing have indeterminate effect on the
 * in-flight program.
 */
int ifx_hppass_ac_load_stt(const struct device *dev, uint8_t num_entries,
			const struct ifx_hppass_ac_stt_entry *stt)
{
	if (stt == NULL || num_entries == 0 ||
	    num_entries > IFX_HPPASS_AC_STT_SIZE) {
		return -EINVAL;
	}

	return ifx_hppass_stt_write_range(dev, num_entries, stt, 0);
} /* ifx_hppass_ac_load_stt() */

/*
 * Patch a contiguous STT range without touching the rest of the table.
 * Caller must ensure the AC is stopped.
 */
int ifx_hppass_ac_update_stt(const struct device *dev, uint8_t num_entries,
			  const struct ifx_hppass_ac_stt_entry *stt,
			  uint8_t start_idx)
{
	if (stt == NULL || num_entries == 0 ||
	    start_idx >= IFX_HPPASS_AC_STT_SIZE ||
	    (uint16_t)num_entries + start_idx > IFX_HPPASS_AC_STT_SIZE) {
		return -EINVAL;
	}

	return ifx_hppass_stt_write_range(dev, num_entries, stt, start_idx);
} /* ifx_hppass_ac_update_stt() */

/* AC state query */

bool ifx_hppass_ac_is_running(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;

	return (sys_read32(config->base + IFX_HPPASS_AC_STATUS) &
		HPPASS_ACTRLR_STATUS_STATUS_Msk) == IFX_HPPASS_AC_STATUS_RUNNING;
} /* ifx_hppass_ac_is_running() */

/* True when VDDA is at or above the analog operating threshold. */
bool ifx_hppass_is_vdda_ok(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;

	return (sys_read32(config->base + IFX_HPPASS_INFRA_VDDA_STATUS) &
		HPPASS_INFRA_VDDA_STATUS_VDDA_OK_Msk) != 0;
} /* ifx_hppass_is_vdda_ok() */

/* FW trigger control */

/*
 * Pulse the selected FW trigger lines for one cycle.  Only takes effect
 * when the corresponding trigger input is configured CY_HPPASS_TR_FW_PULSE.
 * Safe while the AC is running.
 */
void ifx_hppass_ac_set_fw_trigger_pulse(const struct device *dev, uint8_t mask)
{
	const struct ifx_hppass_mfd_config *config = dev->config;

	sys_write32(mask, config->base + IFX_HPPASS_INFRA_FW_TR_PULSE);
} /* ifx_hppass_ac_set_fw_trigger_pulse() */

/*
 * Latching level trigger.  Stays asserted until cleared via
 * ifx_hppass_ac_clear_fw_trigger_level(); use this for cooperative stops
 * where the AC must observe the trigger no matter where it is in the STT.
 */
void ifx_hppass_ac_set_fw_trigger_level(const struct device *dev, uint8_t mask)
{
	const struct ifx_hppass_mfd_config *config = dev->config;

	sys_set_bits(config->base + IFX_HPPASS_INFRA_FW_TR_LEVEL, mask);
} /* ifx_hppass_ac_set_fw_trigger_level() */

/* Clear an asserted FW level trigger bit. */
void ifx_hppass_ac_clear_fw_trigger_level(const struct device *dev, uint8_t mask)
{
	const struct ifx_hppass_mfd_config *config = dev->config;

	sys_clear_bits(config->base + IFX_HPPASS_INFRA_FW_TR_LEVEL, mask);
} /* ifx_hppass_ac_clear_fw_trigger_level() */

/* Rebuild the HPPASS interrupt mask from the registered callback list. */
static void ifx_hppass_update_hw_mask(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;
	struct ifx_hppass_mfd_data *data = dev->data;
	struct ifx_hppass_event_callback *cb;
	uint32_t mask = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&data->callbacks, cb, node) {
		mask |= cb->event_mask;
	}

	sys_write32(IFX_HPPASS_INTR_ALL_MSK & mask, config->base + IFX_HPPASS_MMIO_INTR_MASK);
} /* ifx_hppass_update_hw_mask() */

/*
 * Add an event callback to the dispatch list.  The hardware interrupt
 * mask is automatically updated to include the new callback's events.
 */
int ifx_hppass_add_callback(const struct device *dev,
			    struct ifx_hppass_event_callback *callback)
{
	struct ifx_hppass_mfd_data *data = dev->data;

	if (callback == NULL || callback->handler == NULL ||
	    callback->event_mask == 0) {
		return -EINVAL;
	}

	sys_slist_prepend(&data->callbacks, &callback->node);
	ifx_hppass_update_hw_mask(dev);

	return 0;
} /* ifx_hppass_add_callback() */

/*
 * Remove a previously registered event callback.  The hardware interrupt
 * mask is updated to disable events no longer needed by any callback.
 */
int ifx_hppass_remove_callback(const struct device *dev,
			       struct ifx_hppass_event_callback *callback)
{
	struct ifx_hppass_mfd_data *data = dev->data;

	if (callback == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&data->callbacks, &callback->node)) {
		return -ENOENT;
	}

	ifx_hppass_update_hw_mask(dev);

	return 0;
} /* ifx_hppass_remove_callback() */

/*
 * Combined MCPASS ISR.  Latches the masked status, clears it at the peripheral,
 * and dispatches to every callback whose mask intersects the status word.
 */
static void ifx_hppass_mfd_isr(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;
	struct ifx_hppass_mfd_data *data = dev->data;
	struct ifx_hppass_event_callback *cb;
	struct ifx_hppass_event_callback *tmp;
	mem_addr_t base = config->base;
	uint32_t status;

	status = sys_read32(base + IFX_HPPASS_MMIO_INTR_MASKED);
	sys_write32(IFX_HPPASS_INTR_ALL_MSK & status, base + IFX_HPPASS_MMIO_INTR);
	(void)sys_read32(base + IFX_HPPASS_MMIO_INTR);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->callbacks, cb, tmp, node) {
		if (cb->event_mask & status) {
			cb->handler(dev, cb, cb->event_mask & status);
		}
	}
} /* ifx_hppass_mfd_isr() */

/*
 * Sets up the peripheral clock, programs the HPPASS subsystem
 * via the in-driver init helpers (basic or advanced mode STT, triggers, and
 * startup timing), and enables the combined MCPASS IRQ.
 */
static int ifx_hppass_mfd_init(const struct device *dev)
{
	const struct ifx_hppass_mfd_config *config = dev->config;
	struct ifx_hppass_mfd_data *data = dev->data;
	int ret;

	sys_slist_init(&data->callbacks);
	k_mutex_init(&data->start_lock);

	ret = ifx_hppass_init_hw(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialise HPPASS subsystem (%d)", ret);
		return ret;
	}

	config->irq_config_func(dev);

	LOG_DBG("HPPASS MFD initialised");
	return 0;
} /* ifx_hppass_mfd_init() */

/*
 * Per-instance STT is basic or advanced, selected by whether ac-states is
 * present in DT.  Basic mode generates a 2-state STT: WAIT_FOR BLOCK_READY
 * then STOP, enabling whichever SAR and CSG children are marked "okay" in DT.
 * Advanced mode builds the STT from the ac-states phandle list.
 *
 * Per Arch TRM 002-39348 §27.2.4.4, users of basic mode must still set
 * the startup-N-csg-{chan,slice,ready} props on the analog parent so the
 * STARTUP_CFG cascade matches the silicon-fixed CSG bring-up:
 *   CFG0: SAR_EN+CSG_CH_EN after AREF
 *   CFG1: CSG_PWR_EN_SLICE +10 us
 *   CFG2: CSG_READY +32 clk_csg
 * The binding defaults each of these props to 0 (CSG disabled at every
 * startup phase).  Boards with CSG channels must override them on the
 * &hppass_analog0 node in DTS, e.g.:
 *
 *	&hppass_analog0 {
 *		startup-0-csg-chan = <1>;
 *		startup-1-csg-slice = <1>;
 *		startup-2-csg-ready = <1>;
 *	};
 */

/* clang-format off */

/** 1 when the ac-states property is present on instance n */
#define HAS_AC_STATES(n) DT_INST_NODE_HAS_PROP(n, ac_states)

/* 1 when an enabled child of instance n has the SAR ADC compatible. */
#define _SAR_CHILD_OK(node) DT_NODE_HAS_COMPAT_STATUS(node, infineon_hppass_sar_adc, okay)
#define _OR_SAR(node)       _SAR_CHILD_OK(node) ||
#define SAR_IS_USED(n)      (DT_FOREACH_CHILD(DT_DRV_INST(n), _OR_SAR) 0)

/*
 * A CSG slice is in-use when the CSG parent is okay AND either its
 * comparator (comp@N) or DAC (dac@N+4) child is okay.  Slice register
 * spacing is 0x40.
 */
#define CSG_NODE(n) DT_CHILD(DT_DRV_INST(n), csg_b0000)

#define _CSG_SLICE_USED(n, comp_tok, dac_tok)                                                  \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_CHILD(CSG_NODE(n), comp_tok), okay), (1),            \
		    (DT_NODE_HAS_STATUS(DT_CHILD(CSG_NODE(n), dac_tok), okay)))

#define _CSG_IS_USED_0(n) _CSG_SLICE_USED(n, comp_0,   dac_4)
#define _CSG_IS_USED_1(n) _CSG_SLICE_USED(n, comp_40,  dac_44)
#define _CSG_IS_USED_2(n) _CSG_SLICE_USED(n, comp_80,  dac_84)
#define _CSG_IS_USED_3(n) _CSG_SLICE_USED(n, comp_c0,  dac_c4)
#define _CSG_IS_USED_4(n) _CSG_SLICE_USED(n, comp_100, dac_104)

#define CSG_IS_USED(n, slice)                                                                  \
	COND_CODE_1(DT_NODE_HAS_STATUS(CSG_NODE(n), okay),                                     \
		    (_CSG_IS_USED_##slice(n)), (0))

/* Basic-mode STT: WAIT_FOR BLOCK_READY then STOP (TRM §27.2.4.4). */

/*
 * Sequence generated (TRM 002-39348 §27.2.4.4 canonical 2-state form):
 *
 *   [0] WAIT_FOR BLOCK_READY  — sar_unlock/sar_enable when SAR_IS_USED(n);
 *                               csg_unlock[X]/csg_enable[X] for every X
 *                               where CSG_IS_USED(n, X) == 1
 *   [1] FALSE STOP            — terminal state, raises interrupt
 *
 * The user is responsible for setting startup-N-csg-{chan,slice,ready} on
 * the analog parent (see infineon,hppass-analog.yaml lines 78–91, 103–113,
 * 128–138) so that the STARTUP_CFG0/1/2 cascade written at lines 290–302
 * matches the hardware-fixed CSG bring-up timing in TRM 002-39348 §27
 * (CFG0: SAR_EN+CSG_CH_EN after AREF; CFG1: CSG_PWR_EN_SLICE +10 us;
 * CFG2: CSG_READY +32 clk_csg).  The binding defaults the
 * startup-N-csg-{chan,slice,ready} props to 0 (CSG disabled at every
 * phase); boards with CSG channels must override them on the
 * &hppass_analog0 node in DTS.
 */

#define IFX_HPPASS_BASIC_NUM_STT 2

/*
 * Basic-mode STT: two-entry table (WAIT_FOR(BLOCK_READY) → STOP).  Builds
 * software ifx_hppass_ac_stt_entry initialisers; stt_pack_one() later
 * packs each into TT_CFG0..3 (TRM 002-39445 §20.1.71-74).
 */
#define IFX_HPPASS_BASIC_STT(n)                                                                \
	static const struct ifx_hppass_ac_stt_entry ifx_hppass_stt_##n[] = {                   \
		{                                                                              \
			.condition = IFX_HPPASS_CONDITION_BLOCK_READY,                         \
			.action = IFX_HPPASS_ACTION_WAIT_FOR,                                  \
			.branch_state_idx = 0,                                                \
			.interrupt = false,                                                    \
			.count = 1,                                                           \
			.gpio_out_unlock = false,                                              \
			.gpio_out_msk = 0,                                                    \
			.csg_unlock = {                                                        \
				(bool)CSG_IS_USED(n, 0), (bool)CSG_IS_USED(n, 1),              \
				(bool)CSG_IS_USED(n, 2), (bool)CSG_IS_USED(n, 3),              \
				(bool)CSG_IS_USED(n, 4),                                       \
			},                                                                     \
			.csg_enable = {                                                        \
				(bool)CSG_IS_USED(n, 0), (bool)CSG_IS_USED(n, 1),              \
				(bool)CSG_IS_USED(n, 2), (bool)CSG_IS_USED(n, 3),              \
				(bool)CSG_IS_USED(n, 4),                                       \
			},                                                                     \
			.csg_dac_trig = {false, false, false, false, false},                   \
			.sar_unlock = SAR_IS_USED(n),                                          \
			.sar_enable = SAR_IS_USED(n),                                          \
			.sar_grp_msk = 0,                                                     \
			.sar_mux = {{false, 0}, {false, 0}, {false, 0}, {false, 0}},       \
		},                                                                             \
		{                                                                              \
			.condition = IFX_HPPASS_CONDITION_FALSE,                               \
			.action = IFX_HPPASS_ACTION_STOP,                                      \
			.branch_state_idx = 1,                                                \
			.interrupt = true,                                                     \
			.count = 1,                                                           \
			.gpio_out_unlock = false,                                              \
			.gpio_out_msk = 0,                                                    \
			.csg_unlock = {false, false, false, false, false},                     \
			.csg_enable = {false, false, false, false, false},                     \
			.csg_dac_trig = {false, false, false, false, false},                   \
			.sar_unlock = false,                                                   \
			.sar_enable = false,                                                   \
			.sar_grp_msk = 0,                                                     \
			.sar_mux = {{false, 0}, {false, 0}, {false, 0}, {false, 0}},       \
		},                                                                             \
	};

/* Advanced-mode STT: built from ac-states phandle list. */

/** Get the DT node id of the i-th ac-state phandle for instance n */
#define AC_STATE_NODE(n, i) DT_INST_PHANDLE_BY_IDX(n, ac_states, i)

/** Number of AC states derived from the ac-states phandle list length */
#define NUM_STT_STATES(n) DT_INST_PROP_LEN(n, ac_states)

/**
 * Build one struct ifx_hppass_ac_stt_entry initialiser from an ac-state
 * node.  Packed into TT_CFG0..3 (TRM 002-39445 §20.1.71-74) by
 * stt_pack_one().
 */
#define IFX_HPPASS_STT_ENTRY(i, n)                                                             \
	{                                                                                      \
		.condition = (enum ifx_hppass_condition)DT_PROP(AC_STATE_NODE(n, i),           \
								ac_condition),                 \
		.action = (enum ifx_hppass_action)DT_PROP(AC_STATE_NODE(n, i), ac_action),     \
		.branch_state_idx = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), branch_state_idx),   \
		.interrupt = (bool)DT_PROP(AC_STATE_NODE(n, i), ac_interrupt),                 \
		.count = (uint16_t)DT_PROP(AC_STATE_NODE(n, i), ac_count),                     \
		.gpio_out_unlock = (bool)DT_PROP(AC_STATE_NODE(n, i), gpio_out_unlock),        \
		.gpio_out_msk = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), gpio_out_msk),           \
		.csg_unlock = {                                                                \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg0_unlock),                       \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg1_unlock),                       \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg2_unlock),                       \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg3_unlock),                       \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg4_unlock),                       \
		},                                                                             \
		.csg_enable = {                                                                \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg0_enable),                       \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg1_enable),                       \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg2_enable),                       \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg3_enable),                       \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg4_enable),                       \
		},                                                                             \
		.csg_dac_trig = {                                                              \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg0_dac_trig),                     \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg1_dac_trig),                     \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg2_dac_trig),                     \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg3_dac_trig),                     \
			(bool)DT_PROP(AC_STATE_NODE(n, i), csg4_dac_trig),                     \
		},                                                                             \
		.sar_unlock = (bool)DT_PROP(AC_STATE_NODE(n, i), sar_unlock),                  \
		.sar_enable = (bool)DT_PROP(AC_STATE_NODE(n, i), sar_enable),                  \
		.sar_grp_msk = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), sar_grp_msk),             \
		.sar_mux = {                                                                   \
			{(bool)DT_PROP(AC_STATE_NODE(n, i), sar_mux0_unlock),                  \
			 (uint8_t)DT_PROP(AC_STATE_NODE(n, i), sar_mux0_chan_idx)},            \
			{(bool)DT_PROP(AC_STATE_NODE(n, i), sar_mux1_unlock),                  \
			 (uint8_t)DT_PROP(AC_STATE_NODE(n, i), sar_mux1_chan_idx)},            \
			{(bool)DT_PROP(AC_STATE_NODE(n, i), sar_mux2_unlock),                  \
			 (uint8_t)DT_PROP(AC_STATE_NODE(n, i), sar_mux2_chan_idx)},            \
			{(bool)DT_PROP(AC_STATE_NODE(n, i), sar_mux3_unlock),                  \
			 (uint8_t)DT_PROP(AC_STATE_NODE(n, i), sar_mux3_chan_idx)},            \
		},                                                                             \
	}

/** Generate the advanced-mode STT array from ac-states phandles */
#define IFX_HPPASS_ADVANCED_STT(n)                                                             \
	static const struct ifx_hppass_ac_stt_entry ifx_hppass_stt_##n[] = {                   \
		LISTIFY(NUM_STT_STATES(n), IFX_HPPASS_STT_ENTRY, (,), n)                       \
	};

/** Number of STT entries — basic or advanced */
#define IFX_HPPASS_NUM_STT(n)                                                                  \
	COND_CODE_1(HAS_AC_STATES(n), (NUM_STT_STATES(n)), (IFX_HPPASS_BASIC_NUM_STT))

/* Packs INFRA_STARTUP_CFG[idx] (TRM 002-39445 §20.1.80). */
#define IFX_HPPASS_STARTUP_PHASE(n, idx)                                                       \
	{ .raw =                                                                              \
		FIELD_PREP(GENMASK(7, 0),                                                     \
			   (uint32_t)DT_INST_PROP_OR(n, startup_##idx##_count, 0)) |          \
		((uint32_t)(bool)DT_INST_PROP_OR(n, startup_##idx##_sar, 0)       << 8) |    \
		((uint32_t)(bool)DT_INST_PROP_OR(n, startup_##idx##_csg_chan, 0)  << 9) |    \
		((uint32_t)(bool)DT_INST_PROP_OR(n, startup_##idx##_csg_slice, 0) << 10) |   \
		((uint32_t)(bool)DT_INST_PROP_OR(n, startup_##idx##_csg_ready, 0) << 11)     \
	}

/* Packs MMIO_TR_LEVEL_OUT[idx] (TRM 002-39445 §20.1.101). */
#define IFX_HPPASS_TRIG_LEVEL(n, idx)                                                          \
	{ .raw =                                                                              \
		FIELD_PREP(GENMASK(7, 0),                                                     \
			   (uint32_t)DT_INST_PROP_OR(n, trig_level_##idx##_comp_msk, 0)) |    \
		FIELD_PREP(GENMASK(15, 8),                                                    \
			   (uint32_t)DT_INST_PROP_OR(n, trig_level_##idx##_limit_msk, 0))     \
	}

/*
 * INFRA_TR_IN_SEL slot field: 3-bit type code per slot at the low end of
 * a 4-bit stride; bit 3 of each stride is reserved (TRM 002-39445 §20.1.75).
 */
#define IFX_HPPASS_TRIG_IN_TYPE_SLOT(n, slot)                                                  \
	(((uint32_t)DT_INST_PROP_OR(n, trig_in_##slot##_type, 0) &                            \
	  BIT_MASK(IFX_HPPASS_TR_IN_SEL_WIDTH)) <<                                            \
	 (IFX_HPPASS_TR_BITS_PER_SLOT * (slot)))

/* INFRA_HW_TR_MODE slot field: 4-bit mode code per slot (TRM 002-39445 §20.1.76). */
#define IFX_HPPASS_TRIG_IN_HW_MODE_SLOT(n, slot)                                               \
	(((uint32_t)DT_INST_PROP_OR(n, trig_in_##slot##_hw_mode, 0) &                         \
	  BIT_MASK(IFX_HPPASS_HW_TR_MODE_WIDTH)) <<                                           \
	 (IFX_HPPASS_TR_BITS_PER_SLOT * (slot)))

/* Pack 8 slot fields into INFRA_TR_IN_SEL (TRM 002-39445 §20.1.75). */
#define IFX_HPPASS_PACK_TRIG_IN_SEL(n)                                                         \
	(IFX_HPPASS_TRIG_IN_TYPE_SLOT(n, 0) | IFX_HPPASS_TRIG_IN_TYPE_SLOT(n, 1) |            \
	 IFX_HPPASS_TRIG_IN_TYPE_SLOT(n, 2) | IFX_HPPASS_TRIG_IN_TYPE_SLOT(n, 3) |            \
	 IFX_HPPASS_TRIG_IN_TYPE_SLOT(n, 4) | IFX_HPPASS_TRIG_IN_TYPE_SLOT(n, 5) |            \
	 IFX_HPPASS_TRIG_IN_TYPE_SLOT(n, 6) | IFX_HPPASS_TRIG_IN_TYPE_SLOT(n, 7))

/* Pack 8 slot fields into INFRA_HW_TR_MODE (TRM 002-39445 §20.1.76). */
#define IFX_HPPASS_PACK_HW_TR_MODE(n)                                                          \
	(IFX_HPPASS_TRIG_IN_HW_MODE_SLOT(n, 0) | IFX_HPPASS_TRIG_IN_HW_MODE_SLOT(n, 1) |      \
	 IFX_HPPASS_TRIG_IN_HW_MODE_SLOT(n, 2) | IFX_HPPASS_TRIG_IN_HW_MODE_SLOT(n, 3) |      \
	 IFX_HPPASS_TRIG_IN_HW_MODE_SLOT(n, 4) | IFX_HPPASS_TRIG_IN_HW_MODE_SLOT(n, 5) |      \
	 IFX_HPPASS_TRIG_IN_HW_MODE_SLOT(n, 6) | IFX_HPPASS_TRIG_IN_HW_MODE_SLOT(n, 7))

/* MMIO_TR_LEVEL_CFG.BYPASS_SYNC slot bit (TRM 002-39445 §20.1.100). */
#define IFX_HPPASS_PACK_SYNC_BYPASS_BIT(n, slot)                                               \
	(((uint32_t)(bool)DT_INST_PROP_OR(n, trig_level_##slot##_sync_bypass, 0)) << (slot))

/* Pack 8 slot bits into MMIO_TR_LEVEL_CFG.BYPASS_SYNC (TRM 002-39445 §20.1.100). */
#define IFX_HPPASS_PACK_SYNC_BYPASS_MSK(n)                                                     \
	(IFX_HPPASS_PACK_SYNC_BYPASS_BIT(n, 0) |                                              \
	 IFX_HPPASS_PACK_SYNC_BYPASS_BIT(n, 1) |                                              \
	 IFX_HPPASS_PACK_SYNC_BYPASS_BIT(n, 2) |                                              \
	 IFX_HPPASS_PACK_SYNC_BYPASS_BIT(n, 3) |                                              \
	 IFX_HPPASS_PACK_SYNC_BYPASS_BIT(n, 4) |                                              \
	 IFX_HPPASS_PACK_SYNC_BYPASS_BIT(n, 5) |                                              \
	 IFX_HPPASS_PACK_SYNC_BYPASS_BIT(n, 6) |                                              \
	 IFX_HPPASS_PACK_SYNC_BYPASS_BIT(n, 7))

/* clang-format on */

#define IFX_HPPASS_MFD_INIT(n)                                                                 \
                                                                                               \
	/* Per-instance STT, basic or advanced. */                                            \
	COND_CODE_1(HAS_AC_STATES(n),                                                         \
		    (IFX_HPPASS_ADVANCED_STT(n)),                                              \
		    (IFX_HPPASS_BASIC_STT(n)))                                                 \
                                                                                               \
	/* Register-shaped DT config; STT array is held in ifx_hppass_mfd_config. */    \
	static const struct ifx_hppass_hw_cfg ifx_hppass_cfg_##n = {                           \
		.startup_clk_div = (uint16_t)DT_INST_PROP_OR(n, startup_clk_div, 1),          \
		.gpio_out_en_msk = (uint8_t)DT_INST_PROP_OR(n, ac_gpio_out_en_msk, 0),        \
		.startup = {                                                                   \
			IFX_HPPASS_STARTUP_PHASE(n, 0),                                        \
			IFX_HPPASS_STARTUP_PHASE(n, 1),                                        \
			IFX_HPPASS_STARTUP_PHASE(n, 2),                                        \
			IFX_HPPASS_STARTUP_PHASE(n, 3),                                        \
		},                                                                             \
		.trig_in_sel = IFX_HPPASS_PACK_TRIG_IN_SEL(n),                                \
		.hw_tr_mode  = IFX_HPPASS_PACK_HW_TR_MODE(n),                                 \
		.trig_pulse = {                                                                \
			(cy_en_hppass_trig_out_pulse_t)DT_INST_PROP_OR(n, trig_pulse_0, 0),   \
			(cy_en_hppass_trig_out_pulse_t)DT_INST_PROP_OR(n, trig_pulse_1, 0),   \
			(cy_en_hppass_trig_out_pulse_t)DT_INST_PROP_OR(n, trig_pulse_2, 0),   \
			(cy_en_hppass_trig_out_pulse_t)DT_INST_PROP_OR(n, trig_pulse_3, 0),   \
			(cy_en_hppass_trig_out_pulse_t)DT_INST_PROP_OR(n, trig_pulse_4, 0),   \
			(cy_en_hppass_trig_out_pulse_t)DT_INST_PROP_OR(n, trig_pulse_5, 0),   \
			(cy_en_hppass_trig_out_pulse_t)DT_INST_PROP_OR(n, trig_pulse_6, 0),   \
			(cy_en_hppass_trig_out_pulse_t)DT_INST_PROP_OR(n, trig_pulse_7, 0),   \
		},                                                                             \
		.trig_level = {                                                                \
			IFX_HPPASS_TRIG_LEVEL(n, 0), IFX_HPPASS_TRIG_LEVEL(n, 1),            \
			IFX_HPPASS_TRIG_LEVEL(n, 2), IFX_HPPASS_TRIG_LEVEL(n, 3),            \
			IFX_HPPASS_TRIG_LEVEL(n, 4), IFX_HPPASS_TRIG_LEVEL(n, 5),            \
			IFX_HPPASS_TRIG_LEVEL(n, 6), IFX_HPPASS_TRIG_LEVEL(n, 7),            \
		},                                                                             \
		.trig_level_sync_bypass_msk = IFX_HPPASS_PACK_SYNC_BYPASS_MSK(n),             \
	};                                                                                     \
                                                                                               \
	static void ifx_hppass_mfd_irq_config_##n(const struct device *dev);                   \
                                                                                               \
	static const struct ifx_hppass_mfd_config ifx_hppass_mfd_config_##n = {                \
		.base = (mem_addr_t)DT_INST_REG_ADDR(n),                                       \
		.irq_config_func = ifx_hppass_mfd_irq_config_##n,                              \
		.init_cfg = &ifx_hppass_cfg_##n,                                               \
		.stt = ifx_hppass_stt_##n,                                                     \
		.num_stt = IFX_HPPASS_NUM_STT(n),                                              \
	};                                                                                     \
                                                                                               \
	static struct ifx_hppass_mfd_data ifx_hppass_mfd_data_##n;                             \
                                                                                               \
	DEVICE_DT_INST_DEFINE(n, &ifx_hppass_mfd_init, NULL,                                   \
			      &ifx_hppass_mfd_data_##n,                                        \
			      &ifx_hppass_mfd_config_##n, POST_KERNEL,                         \
			      CONFIG_MFD_INFINEON_HPPASS_INIT_PRIORITY, NULL);                 \
                                                                                               \
	static void ifx_hppass_mfd_irq_config_##n(const struct device *dev)                    \
	{                                                                                      \
		ARG_UNUSED(dev);                                                               \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                        \
			    ifx_hppass_mfd_isr, DEVICE_DT_INST_GET(n), 0);                    \
		irq_enable(DT_INST_IRQN(n));                                                   \
	}

DT_INST_FOREACH_STATUS_OKAY(IFX_HPPASS_MFD_INIT)

/*
 * APPLICATION-init hook for the AC one-shot unlock sweep.  Must run after
 * all POST_KERNEL device inits so child CFG writes happen
 * before the AC sees them.  Errors are logged; dependent drivers read
 * register reset values rather than failing to initialize.
 */
#define IFX_HPPASS_LATE_START(n)                                                                   \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(n);                                  \
		int ret;                                                                           \
                                                                                                   \
		if (!device_is_ready(dev)) {                                                       \
			LOG_ERR("HPPASS instance %d not ready; AC not started", (n));              \
		} else {                                                                           \
			ret = ifx_hppass_ac_start(dev, 0, IFX_HPPASS_ENSURE_RUNNING_TIMEOUT_US);   \
			if (ret != 0 && ret != -EALREADY) {                                        \
				LOG_ERR("ifx_hppass_ac_start(%s) -> %d", dev->name, ret);          \
			}                                                                          \
		}                                                                                  \
	}

static int ifx_hppass_late_start(void)
{
	DT_INST_FOREACH_STATUS_OKAY(IFX_HPPASS_LATE_START)
	return 0;
}

SYS_INIT(ifx_hppass_late_start, APPLICATION, CONFIG_MFD_INFINEON_HPPASS_AC_START_PRIORITY);
