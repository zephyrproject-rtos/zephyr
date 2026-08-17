/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Comparator driver for one HPPASS CSG slice.
 *
 * One device per @c infineon,hppass-csg-comp node.  The CSG MFD parent
 * wires the combined IRQ and dispatches per-slice callbacks.  This
 * driver writes CMP_CFG at init and CMP_CFG.CMP_EDGE_MODE at runtime,
 * arms its slice's comparator interrupt through the CSG MFD parent, and
 * tracks a software pending flag for comparator_trigger_is_pending().
 *
 * HPPASS is shared across multiple Infineon device families.  The TRM
 * citations throughout this file refer to the PSOC Control C3 Mainline
 * documentation:
 * References:
 *   - <em>PSOC Control C3 Mainline Architecture TRM</em>, 002-39348
 *     (§27.2.3 CSG)
 *   - <em>PSOC Control C3 Mainline Registers TRM</em>, 002-39445
 *     (HPPASS_CSG_SLICE_CMP_CFG §20.1.42, CMP_STATUS §20.1.51)
 */

#define DT_DRV_COMPAT infineon_hppass_csg_comp

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <infineon_kconfig.h>
#include <infineon_hppass_csg.h>
#include "mfd_infineon_hppass_regs.h"
#include <cy_device.h>

LOG_MODULE_REGISTER(comparator_infineon_hppass_csg, CONFIG_COMPARATOR_LOG_LEVEL);

#define IFX_CSG_COMP_PARENT(n) DT_INST_PARENT(n)

#define IFX_CSG_COMP_SLICE_OF(n)                                                          \
	((uint8_t)((DT_INST_REG_ADDR(n) - DT_REG_ADDR(IFX_CSG_COMP_PARENT(n))) /          \
		   DT_PROP(IFX_CSG_COMP_PARENT(n), infineon_slice_reg_spacing)))

/*
 * IFX_CSG_<FIELD>_REG_<VAL> constants encode DT enum strings to CMP_CFG
 * bit values.  DT_STRING_UPPER_TOKEN concatenates with the prefix at
 * preprocess time, so ifx_csg_comp_config.cmp_cfg holds a ready-to-write
 * register word.
 */

/* positive-input */
#define IFX_CSG_POS_REG_AIN_PA 0U
#define IFX_CSG_POS_REG_AIN_PB 1U
#define IFX_CSG_POS_REG(val) CONCAT(IFX_CSG_POS_REG_, val)

/* negative-input (encoding skips 1) */
#define IFX_CSG_NEG_REG_DAC 0U
#define IFX_CSG_NEG_REG_AIN_NA 2U
#define IFX_CSG_NEG_REG_AIN_NB 3U
#define IFX_CSG_NEG_REG(val) CONCAT(IFX_CSG_NEG_REG_, val)

/* blank-mode */
#define IFX_CSG_BLANK_REG_DISABLED 0U
#define IFX_CSG_BLANK_REG_GATE_ON_ONE 1U
#define IFX_CSG_BLANK_REG_GATE_ON_ZERO 2U
#define IFX_CSG_BLANK_REG(val) CONCAT(IFX_CSG_BLANK_REG_, val)

/* blank-trigger-sel: "disabled"=0, "tr0"=1 ... "tr7"=8 */
#define IFX_CSG_BLANK_TR_REG_DISABLED 0U
#define IFX_CSG_BLANK_TR_REG_TR0 1U
#define IFX_CSG_BLANK_TR_REG_TR1 2U
#define IFX_CSG_BLANK_TR_REG_TR2 3U
#define IFX_CSG_BLANK_TR_REG_TR3 4U
#define IFX_CSG_BLANK_TR_REG_TR4 5U
#define IFX_CSG_BLANK_TR_REG_TR5 6U
#define IFX_CSG_BLANK_TR_REG_TR6 7U
#define IFX_CSG_BLANK_TR_REG_TR7 8U
#define IFX_CSG_BLANK_TR_REG(val) CONCAT(IFX_CSG_BLANK_TR_REG_, val)

/* CMP_CFG field masks (Regs TRM 002-39445 §20.1.42), composed with FIELD_PREP(). */
#define IFX_CSG_CMP_CFG_POS_SEL      BIT(0)          /* [0]     positive-input select */
#define IFX_CSG_CMP_CFG_NEG_SEL      GENMASK(3, 2)   /* [3:2]   negative-input select */
#define IFX_CSG_CMP_CFG_BLANK_MODE   GENMASK(5, 4)   /* [5:4]   blank mode */
#define IFX_CSG_CMP_CFG_BLANK_TR_SEL GENMASK(11, 8)  /* [11:8]  blank trigger select */
#define IFX_CSG_CMP_CFG_EDGE_MODE    GENMASK(13, 12) /* [13:12] edge detection mode */
#define IFX_CSG_CMP_CFG_POLARITY     BIT(16)         /* [16]    output polarity */

/*
 * CMP_CFG.CMP_EDGE_MODE encoding:
 *   0 = disabled (no edge detection)
 *   1 = rising edge
 *   2 = falling edge
 *   3 = both edges
 */
#define IFX_CSG_EDGE_MODE_NONE          0U
#define IFX_CSG_EDGE_MODE_RISING_EDGE   1U
#define IFX_CSG_EDGE_MODE_FALLING_EDGE  2U
#define IFX_CSG_EDGE_MODE_BOTH_EDGES    3U

struct ifx_csg_comp_config {
	const struct device *parent;
	mem_addr_t slice_base;
	uint8_t slice;
	uint32_t cmp_cfg;
};

struct ifx_csg_comp_data {
	comparator_callback_t cb;
	void *user_data;
	atomic_t pending;
};

/*
 * Sync this slice's CMP_INTR_MASK bit to whether its comparator is armed
 * for edge IRQs.  CMP_INTR_MASK is CSG-wide and owned by the MFD parent;
 * the caller must hold the CSG spinlock via ifx_hppass_csg_cmp_lock().
 */
static void ifx_csg_comp_apply_intr_mask(const struct device *dev)
{
	const struct ifx_csg_comp_config *cfg = dev->config;
	uint32_t cmp_cfg;
	bool enable;

	cmp_cfg = sys_read32(cfg->slice_base + IFX_HPPASS_CSG_SLICE_CMP_CFG);
	enable = (FIELD_GET(IFX_CSG_CMP_CFG_EDGE_MODE, cmp_cfg) != IFX_CSG_EDGE_MODE_NONE);
	(void)ifx_hppass_csg_set_cmp_intr_mask(cfg->parent, cfg->slice, enable);
} /* ifx_csg_comp_apply_intr_mask() */

/* Invoked by the CSG MFD ISR.  The parent has already cleared the slice's CMP_INTR latch. */
static void ifx_csg_comp_dispatch(void *user_data)
{
	const struct device *dev = user_data;
	struct ifx_csg_comp_data *data = dev->data;

	if (data->cb != NULL) {
		data->cb(dev, data->user_data);
	} else {
		(void)atomic_set(&data->pending, 1);
	}
} /* ifx_csg_comp_dispatch() */

static int ifx_csg_comp_get_output(const struct device *dev)
{
	const struct ifx_csg_comp_config *cfg = dev->config;
	uint32_t status = sys_read32(cfg->slice_base + IFX_HPPASS_CSG_SLICE_CMP_STATUS);

	return (int)(status & HPPASS_CSG_SLICE_CMP_STATUS_CMP_VAL_Msk);
} /* ifx_csg_comp_get_output() */

static int ifx_csg_comp_set_trigger(const struct device *dev,
				    enum comparator_trigger trigger)
{
	const struct ifx_csg_comp_config *cfg = dev->config;
	struct ifx_csg_comp_data *data = dev->data;
	uint32_t cmp_cfg;
	uint8_t edge_mode;
	k_spinlock_key_t key;

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		edge_mode = IFX_CSG_EDGE_MODE_NONE;
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		edge_mode = IFX_CSG_EDGE_MODE_RISING_EDGE;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		edge_mode = IFX_CSG_EDGE_MODE_FALLING_EDGE;
		break;
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		edge_mode = IFX_CSG_EDGE_MODE_BOTH_EDGES;
		break;
	default:
		return -EINVAL;
	}

	key = ifx_hppass_csg_cmp_lock(cfg->parent);
	cmp_cfg = sys_read32(cfg->slice_base + IFX_HPPASS_CSG_SLICE_CMP_CFG);
	cmp_cfg &= ~IFX_CSG_CMP_CFG_EDGE_MODE;
	cmp_cfg |= FIELD_PREP(IFX_CSG_CMP_CFG_EDGE_MODE, edge_mode);
	sys_write32(cmp_cfg, cfg->slice_base + IFX_HPPASS_CSG_SLICE_CMP_CFG);

	/*
	 * Clear any edge latched by the edge_mode write or from a prior
	 * arming, and discard stale software pending state.
	 */
	(void)ifx_hppass_csg_clear_cmp_intr(cfg->parent, cfg->slice);
	atomic_clear(&data->pending);
	ifx_csg_comp_apply_intr_mask(dev);
	ifx_hppass_csg_cmp_unlock(cfg->parent, key);

	return 0;
} /* ifx_csg_comp_set_trigger() */

static int ifx_csg_comp_set_trigger_callback(const struct device *dev,
					     comparator_callback_t callback,
					     void *user_data)
{
	struct ifx_csg_comp_data *data = dev->data;
	const struct ifx_csg_comp_config *cfg = dev->config;
	bool fire_now;
	k_spinlock_key_t key;

	key = ifx_hppass_csg_cmp_lock(cfg->parent);
	data->cb = callback;
	data->user_data = user_data;
	/* Deliver any latched trigger to a freshly bound callback. */
	fire_now = (callback != NULL) && (atomic_clear(&data->pending) == 1);
	ifx_csg_comp_apply_intr_mask(dev);
	ifx_hppass_csg_cmp_unlock(cfg->parent, key);

	if (fire_now) {
		callback(dev, user_data);
	}

	return 0;
} /* ifx_csg_comp_set_trigger_callback() */

static int ifx_csg_comp_trigger_is_pending(const struct device *dev)
{
	struct ifx_csg_comp_data *data = dev->data;

	return (int)atomic_clear(&data->pending);
} /* ifx_csg_comp_trigger_is_pending() */

static DEVICE_API(comparator, ifx_csg_comp_api) = {
	.get_output           = ifx_csg_comp_get_output,
	.set_trigger          = ifx_csg_comp_set_trigger,
	.set_trigger_callback = ifx_csg_comp_set_trigger_callback,
	.trigger_is_pending   = ifx_csg_comp_trigger_is_pending,
};

/*
 * Runs after the CSG MFD parent.  Writes CMP_CFG from cfg->cmp_cfg;
 * edge_mode stays 0 until comparator_set_trigger().  The combined CSG
 * IRQ is wired by the parent; this function registers the per-slice
 * dispatch hook and leaves CMP_INTR_MASK clear.
 */
static int ifx_csg_comp_init(const struct device *dev)
{
	const struct ifx_csg_comp_config *cfg = dev->config;
	struct ifx_csg_comp_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("CSG MFD parent not ready");
		return -ENODEV;
	}

	sys_write32(cfg->cmp_cfg, cfg->slice_base + IFX_HPPASS_CSG_SLICE_CMP_CFG);

	data->cb = NULL;
	data->user_data = NULL;
	atomic_clear(&data->pending);

	ret = ifx_hppass_csg_register_cmp_cb(cfg->parent, cfg->slice,
					     ifx_csg_comp_dispatch, (void *)dev);
	if (ret < 0) {
		LOG_ERR("CSG slice %u callback register failed (%d)",
			cfg->slice, ret);
		return ret;
	}

	/* Slice IRQ stays masked until comparator_set_trigger() arms it. */
	return 0;
} /* ifx_csg_comp_init() */

/*
 * Compose CMP_CFG for one comparator child.  edge_mode is left at 0 and
 * filled in at runtime by comparator_set_trigger().
 */
#define IFX_CSG_COMP_CFG_INIT(node_id)                                                    \
	(FIELD_PREP(IFX_CSG_CMP_CFG_POS_SEL,                                              \
		    IFX_CSG_POS_REG(DT_STRING_UPPER_TOKEN(node_id, positive_input))) |    \
	 FIELD_PREP(IFX_CSG_CMP_CFG_NEG_SEL,                                              \
		    IFX_CSG_NEG_REG(DT_STRING_UPPER_TOKEN(node_id, negative_input))) |    \
	 FIELD_PREP(IFX_CSG_CMP_CFG_BLANK_MODE,                                           \
		    IFX_CSG_BLANK_REG(                                                    \
			    DT_STRING_UPPER_TOKEN_OR(node_id, blank_mode, DISABLED))) |   \
	 FIELD_PREP(IFX_CSG_CMP_CFG_BLANK_TR_SEL,                                         \
		    IFX_CSG_BLANK_TR_REG(                                                 \
			    DT_STRING_UPPER_TOKEN_OR(node_id, blank_trigger_sel,          \
						     DISABLED))) |                        \
	 FIELD_PREP(IFX_CSG_CMP_CFG_POLARITY, DT_PROP(node_id, invert_output)))

#define IFX_CSG_COMP_INIT(n)                                                              \
	BUILD_ASSERT(IFX_CSG_COMP_SLICE_OF(n) <                                           \
			DT_PROP(IFX_CSG_COMP_PARENT(n), infineon_num_slices),             \
		     "CSG comparator slice index out of range");                          \
	static struct ifx_csg_comp_data ifx_csg_comp_data_##n;                            \
	static const struct ifx_csg_comp_config ifx_csg_comp_config_##n = {               \
		.parent     = DEVICE_DT_GET(IFX_CSG_COMP_PARENT(n)),                      \
		.slice_base = (mem_addr_t)DT_INST_REG_ADDR(n),                            \
		.slice      = IFX_CSG_COMP_SLICE_OF(n),                                   \
		.cmp_cfg    = IFX_CSG_COMP_CFG_INIT(DT_DRV_INST(n)),                      \
	};                                                                                \
	DEVICE_DT_INST_DEFINE(n, ifx_csg_comp_init, NULL,                                 \
			      &ifx_csg_comp_data_##n, &ifx_csg_comp_config_##n,           \
			      POST_KERNEL,                                                \
			      CONFIG_COMPARATOR_INFINEON_HPPASS_CSG_INIT_PRIORITY,        \
			      &ifx_csg_comp_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_CSG_COMP_INIT)
