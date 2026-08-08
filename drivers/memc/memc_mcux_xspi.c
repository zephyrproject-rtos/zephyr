/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_xspi

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <soc.h>
#include "memc_mcux_xspi.h"

LOG_MODULE_REGISTER(memc_mcux_xspi, CONFIG_MEMC_LOG_LEVEL);

/*
 * XSPI target-group and FRAD-region counts.
 *
 * LISTIFY()/FOR_EACH_*() need the loop bound (and the region-index list) to be
 * plain decimal integer literals, so they cannot consume the HAL macros
 * XSPI_TARGET_GROUP_COUNT / XSPI_SFP_FRAD_COUNT directly (those expand to a
 * parenthesised, U-suffixed value such as "(2U)"). To avoid silently assuming a
 * count, the literals below are pinned to the HAL values with BUILD_ASSERTs: if
 * a future HAL changes either count, the build breaks here instead of emitting
 * a wrong config. Update the literals (and MEMC_XSPI_FRAD_REGION_IDS) to match.
 */
#define MEMC_XSPI_TARGET_GROUP_COUNT 2
#define MEMC_XSPI_SFP_FRAD_COUNT     8
#define MEMC_XSPI_FRAD_REGION_IDS    0, 1, 2, 3, 4, 5, 6, 7

BUILD_ASSERT(MEMC_XSPI_TARGET_GROUP_COUNT == XSPI_TARGET_GROUP_COUNT,
	     "MEMC_XSPI_TARGET_GROUP_COUNT is out of sync with the HAL "
	     "XSPI_TARGET_GROUP_COUNT; update the literal and the tgConfig/tgMdad "
	     "initializers to match.");
BUILD_ASSERT(MEMC_XSPI_SFP_FRAD_COUNT == XSPI_SFP_FRAD_COUNT,
	     "MEMC_XSPI_SFP_FRAD_COUNT is out of sync with the HAL "
	     "XSPI_SFP_FRAD_COUNT; update the literal and MEMC_XSPI_FRAD_REGION_IDS.");
BUILD_ASSERT(NUM_VA_ARGS(MEMC_XSPI_FRAD_REGION_IDS) == MEMC_XSPI_SFP_FRAD_COUNT,
	     "MEMC_XSPI_FRAD_REGION_IDS must enumerate every FRAD region.");

/*
 * The byteOrder and enableDoze members only exist in the HAL xspi_config_t when
 * the SoC provides the END_CFG / DOZE_MODE XSPI features. Guard the initializers
 * with the same feature macros so the driver builds on variants that lack them.
 */
#if (defined(FSL_FEATURE_XSPI_HAS_END_CFG) && FSL_FEATURE_XSPI_HAS_END_CFG)
#define MCUX_XSPI_BYTE_ORDER(n) .byteOrder = DT_INST_PROP(n, byte_order),
#else
#define MCUX_XSPI_BYTE_ORDER(n)
#endif

#if (defined(FSL_FEATURE_XSPI_HAS_DOZE_MODE) && FSL_FEATURE_XSPI_HAS_DOZE_MODE)
#define MCUX_XSPI_DOZE .enableDoze = false,
#else
#define MCUX_XSPI_DOZE
#endif


struct memc_mcux_xspi_config {
	const struct pinctrl_dev_config *pincfg;
	xspi_config_t xspi_config;
	xspi_sfp_mdad_config_t mdad_configs;
	bool mdad_valid;
	xspi_sfp_frad_config_t frad_configs;
	bool frad_valid;
};

struct memc_mcux_xspi_data {
	XSPI_Type *base;
	bool xip;
	uint32_t amba_address;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

void memc_mcux_xspi_update_device_addr_mode(const struct device *dev,
						xspi_device_addr_mode_t addr_mode)
{
	XSPI_Type *base = ((struct memc_mcux_xspi_data *)dev->data)->base;

	XSPI_UpdateDeviceAddrMode(base, addr_mode);
}

int memc_mcux_xspi_get_root_clock(const struct device *dev, uint32_t *clock_rate)
{
	struct memc_mcux_xspi_data *data = dev->data;

	return clock_control_get_rate(data->clock_dev, data->clock_subsys, clock_rate);
}

void memc_xspi_wait_bus_idle(const struct device *dev)
{
	struct memc_mcux_xspi_data *data = dev->data;

	while (!XSPI_GetBusIdleStatus(data->base)) {
	}
}

int memc_xspi_set_device_config(const struct device *dev, const xspi_device_config_t *device_config,
				const uint32_t *lut_array, uint8_t lut_count)
{
	struct memc_mcux_xspi_data *data = dev->data;
	XSPI_Type *base = data->base;
	status_t status;

	/* Configure flash settings according to serial flash feature. */
	status = XSPI_SetDeviceConfig(base, (xspi_device_config_t *)device_config);
	if (status != kStatus_Success) {
		LOG_ERR("XSPI_SetDeviceConfig failed with status %u\n", status);
		return -ENODEV;
	}

	XSPI_UpdateLUT(base, 0, lut_array, lut_count);

	return 0;
}

uint32_t memc_mcux_xspi_get_ahb_address(const struct device *dev)
{
	return ((const struct memc_mcux_xspi_data *)dev->data)->amba_address;
}

void memc_mcux_xspi_clear_ahb_buffer(const struct device *dev)
{
	XSPI_Type *base = ((struct memc_mcux_xspi_data *)dev->data)->base;

	XSPI_ClearAhbBuffer(base);
}

int memc_mcux_xspi_transfer(const struct device *dev, xspi_transfer_t *xfer)
{
	XSPI_Type *base = ((struct memc_mcux_xspi_data *)dev->data)->base;
	status_t status;

	status = XSPI_TransferBlocking(base, xfer);

	return (status == kStatus_Success) ? 0 : -EIO;
}

bool memc_xspi_is_running_xip(const struct device *dev)
{
	return ((const struct memc_mcux_xspi_data *)dev->data)->xip;
}

static int memc_mcux_xspi_init(const struct device *dev)
{
	const struct memc_mcux_xspi_config *memc_xspi_config = dev->config;
	const struct pinctrl_dev_config *pincfg = memc_xspi_config->pincfg;
	xspi_config_t config = memc_xspi_config->xspi_config;
	XSPI_Type *base = ((struct memc_mcux_xspi_data *)dev->data)->base;
	int ret;

	if ((memc_xspi_is_running_xip(dev)) && (!IS_ENABLED(CONFIG_MEMC_MCUX_XSPI_INIT_XIP))) {
		LOG_DBG("XIP active on %s, skipping init\n", dev->name);
		return 0;
	}

	ret = pinctrl_apply_state(pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d", ret);
		return ret;
	}

	config.ptrAhbAccessConfig->ahbAlignment = kXSPI_AhbAlignmentNoLimit;
	config.ptrAhbAccessConfig->ahbSplitSize = kXSPI_AhbSplitSizeDisabled;

	for (uint8_t i = 0U; i < XSPI_BUFCR_COUNT; i++) {
		config.ptrAhbAccessConfig->buffer[i].masterId = i;
		if (i == (XSPI_BUFCR_COUNT - 1U)) {
			config.ptrAhbAccessConfig->buffer[i].enaPri.enableAllMaster = true;
		} else {
			config.ptrAhbAccessConfig->buffer[i].enaPri.enablePriority = false;
		}
		config.ptrAhbAccessConfig->buffer[i].bufferSize = 0x80U;
		config.ptrAhbAccessConfig->buffer[i].ptrSubBuffer0Config = NULL;
		config.ptrAhbAccessConfig->buffer[i].ptrSubBuffer1Config = NULL;
		config.ptrAhbAccessConfig->buffer[i].ptrSubBuffer2Config = NULL;
		config.ptrAhbAccessConfig->buffer[i].ptrSubBuffer3Config = NULL;
	}

	if (memc_xspi_config->mdad_valid) {
		config.ptrIpAccessConfig->ptrSfpMdadConfig =
			(xspi_sfp_mdad_config_t *)&memc_xspi_config->mdad_configs;
	}
	if (memc_xspi_config->frad_valid) {
		config.ptrIpAccessConfig->ptrSfpFradConfig =
			(xspi_sfp_frad_config_t *)&memc_xspi_config->frad_configs;
	}

	XSPI_Init(base, &config);

	return 0;
}

#if defined(CONFIG_XIP) && defined(CONFIG_FLASH_MCUX_XSPI_XIP)
/* Checks if image flash base address is in the XSPI AHB base region */
#define MEMC_XSPI_CFG_XIP(n) \
	((CONFIG_FLASH_BASE_ADDRESS) >= DT_INST_REG_ADDR_BY_IDX(n, 1)) && \
		((CONFIG_FLASH_BASE_ADDRESS) < \
		 (DT_INST_REG_ADDR_BY_IDX(n, 1) + DT_INST_REG_SIZE_BY_IDX(n, 1)))

#else
#define MEMC_XSPI_CFG_XIP(node_id) false
#endif

/*
 * Resolve the mdad@<idx> child node under the sfp-mdad container. The MDAD
 * descriptors cannot be placed at the xspi top level (a top-level mdad@0 would
 * collide with the flash@0 chip-select unit-address), so they live in a
 * dedicated sfp-mdad container, mirroring how target-group@N lives under a
 * frad_region node. Each child's unit-address (reg) is the tgMdad[] array
 * subscript. DT_CHILD sanitises "sfp-mdad" to "sfp_mdad" and "mdad@0" to
 * "mdad_0".
 */
#define MCUX_XSPI_MDAD_NODE(n, idx)	\
	DT_CHILD(DT_CHILD(DT_DRV_INST(n), sfp_mdad), mdad_ ## idx)
#define MCUX_XSPI_GET_MDAD(n, idx, x)	\
	DT_PROP(MCUX_XSPI_MDAD_NODE(n, idx), x)
#define MCUX_XSPI_GET_FRAD(n, idx, x)	\
	DT_PROP(DT_CHILD(DT_DRV_INST(n), frad_region ## idx), x)


/*
 * Resolve the target-group@<tgidx> child node of FRAD region <ridx>. Each FRAD
 * region carries one target-group@N child per target group; the unit-address N
 * (== the child's reg) is the tgConfig[] array subscript. DT_CHILD sanitises
 * "target-group@0" to the token "target_group_0".
 */
#define MCUX_XSPI_FRAD_TG_NODE(n, ridx, tgidx)	\
	DT_CHILD(DT_CHILD(DT_DRV_INST(n), frad_region ## ridx),	\
		 target_group_ ## tgidx)


/*
 * Emit one xspi_mdad_config_t entry for target group <idx>, sourced from the
 * mdad@<idx> child of the sfp-mdad container. The array subscript is the
 * child's unit-address (reg), matching the target-group index, so the correct
 * tgMdad[] slot is populated. Absent mdad@<idx> nodes zero-initialise.
 */
#define MCUX_XSPI_MDAD_INIT(idx, n)	\
	COND_CODE_1(DT_NODE_EXISTS(MCUX_XSPI_MDAD_NODE(n, idx)),	\
		([idx] = {	\
			.enableDescriptorLock =	\
				MCUX_XSPI_GET_MDAD(n, idx, enable_descriptor_lock),	\
			.maskType = MCUX_XSPI_GET_MDAD(n, idx, mask_type),	\
			.assignIsValid = MCUX_XSPI_GET_MDAD(n, idx, assign_valid),	\
			.mask = MCUX_XSPI_GET_MDAD(n, idx, mask),	\
			.masterIdReference =	\
				MCUX_XSPI_GET_MDAD(n, idx, master_id_reference),	\
			.secureAttribute =	\
				MCUX_XSPI_GET_MDAD(n, idx, secure_attribute),	\
		}),	\
		({	\
			0	\
		}))


/*
 * On XSPI variants that provide the EENV feature (e.g. i.MX943), the HAL
 * xspi_frad_config_t nests the per-target-group access control inside a
 * tgConfig[] array (xspi_frad_tg_config_t), one entry per target group. On
 * variants without EENV the same fields are exposed as flat members. Emit the
 * layout that matches the HAL struct in use.
 */
#if (defined(FSL_FEATURE_XSPI_HAS_EENV) && FSL_FEATURE_XSPI_HAS_EENV)
/*
 * Fetch a property from the target-group@<tgidx> child of FRAD region <ridx>:
 * frad_region<ridx> { target-group@<tgidx> { <x> = <..>; }; }.
 */
#define MCUX_XSPI_GET_FRAD_TG(n, ridx, tgidx, x)	\
	DT_PROP(MCUX_XSPI_FRAD_TG_NODE(n, ridx, tgidx), x)

/*
 * Emit one xspi_frad_tg_config_t entry for target group <tgidx> of FRAD region
 * <ridx>. The array subscript is the target-group index itself (the child's
 * unit-address / reg), so the correct slot is populated without hard-coding a
 * specific kXSPI_TargetGroupN value. The tgMasterAccess[] array is initialised
 * from the child's master-access DT array property, so it scales with the HAL
 * target-group count too.
 */
#define MCUX_XSPI_FRAD_TG_INIT(tgidx, ridx, n)	\
	[tgidx] = {	\
		.tgMasterAccess = MCUX_XSPI_GET_FRAD_TG(n, ridx, tgidx, master_access),	\
		.assignIsValid =	\
			MCUX_XSPI_GET_FRAD_TG(n, ridx, tgidx, assign_valid),	\
		.descriptorLock =	\
			MCUX_XSPI_GET_FRAD_TG(n, ridx, tgidx, descriptor_lock),	\
		.exclusiveAccessLock =	\
			MCUX_XSPI_GET_FRAD_TG(n, ridx, tgidx, exclusive_access_lock),	\
	}

/*
 * Populate every target-group slot of a FRAD region's tgConfig[] array. The
 * number of slots is driven by the HAL target-group count (LISTIFY index),
 * never a hard-coded value. LISTIFY() can be used here because the enclosing
 * FRAD-region loop below uses the FOR_EACH() macro family (a different
 * expansion mechanism), so there is no LISTIFY-within-LISTIFY recursion.
 */
#define MCUX_XSPI_FRAD_TGCONFIG(n, ridx)	\
	LISTIFY(MEMC_XSPI_TARGET_GROUP_COUNT, MCUX_XSPI_FRAD_TG_INIT, (,), ridx, n)

/*
 * Emit one xspi_frad_config_t entry for FRAD region <ridx>. Invoked by
 * FOR_EACH_IDX_FIXED_ARG as F(loop_index, ridx, n); loop_index is unused.
 */
#define MCUX_XSPI_FRAD_INIT(unused, ridx, n)	\
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(n), frad_region ## ridx)),	\
		({	\
			.startAddress = MCUX_XSPI_GET_FRAD(n, ridx, start_address), \
			.endAddress = MCUX_XSPI_GET_FRAD(n, ridx, end_address),	\
			.tgConfig = {	\
				MCUX_XSPI_FRAD_TGCONFIG(n, ridx)	\
			},	\
		}),	\
		({	\
			0	\
		}))
#else
/*
 * Non-EENV flat layout: xspi_frad_config_t exposes tg0/tg1 master access as
 * flat members. Reuse element 0 of each target-group@N child's master-access
 * array; the descriptor / exclusive-access lock come from target-group@0.
 */
#define MCUX_XSPI_GET_FRAD_M0(n, ridx, tgidx)	\
	DT_PROP_BY_IDX(MCUX_XSPI_FRAD_TG_NODE(n, ridx, tgidx), master_access, 0)

#define MCUX_XSPI_FRAD_INIT(unused, ridx, n)	\
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(n), frad_region ## ridx)),	\
		({	\
			.startAddress = MCUX_XSPI_GET_FRAD(n, ridx, start_address), \
			.endAddress = MCUX_XSPI_GET_FRAD(n, ridx, end_address),	\
			.tg0MasterAccess = MCUX_XSPI_GET_FRAD_M0(n, ridx, 0),	\
			.tg1MasterAccess = MCUX_XSPI_GET_FRAD_M0(n, ridx, 1),	\
			.assignIsValid = true,	\
			.descriptorLock =	\
				DT_PROP(MCUX_XSPI_FRAD_TG_NODE(n, ridx, 0),	\
					descriptor_lock),	\
			.exclusiveAccessLock =	\
				DT_PROP(MCUX_XSPI_FRAD_TG_NODE(n, ridx, 0),	\
					exclusive_access_lock),	\
		}),	\
		({	\
			0	\
		}))
#endif


/*
 * Compile-time validation of the FRAD master-access arrays.
 *
 * The HAL sizes xspi_frad_tg_config_t::tgMasterAccess[] with
 * XSPI_TARGET_GROUP_COUNT, so the number of masters is not hard-coded here: the
 * DT tg<N>-master-access array supplies the values and its length must not
 * exceed the HAL array dimension. A too-long array would silently overflow the
 * initializer, so it is rejected at build time. Shorter arrays are allowed and
 * leave the remaining masters zero-initialised.
 */
#if (defined(FSL_FEATURE_XSPI_HAS_EENV) && FSL_FEATURE_XSPI_HAS_EENV)
#define MCUX_XSPI_FRAD_TG_CHECK(tgidx, ridx, n)	\
	BUILD_ASSERT(DT_PROP_LEN(MCUX_XSPI_FRAD_TG_NODE(n, ridx, tgidx),	\
				 master_access) <=	\
		     XSPI_TARGET_GROUP_COUNT,	\
		     "target-group@N master-access has more entries than the HAL "	\
		     "tgMasterAccess[] can hold (XSPI_TARGET_GROUP_COUNT).");


#define MCUX_XSPI_FRAD_REGION_CHECK(unused, ridx, n)	\
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(n), frad_region ## ridx)),	\
		(LISTIFY(MEMC_XSPI_TARGET_GROUP_COUNT,	\
			 MCUX_XSPI_FRAD_TG_CHECK, (), ridx, n)),	\
		())

#define MCUX_XSPI_VALIDATE(n)	\
	FOR_EACH_IDX_FIXED_ARG(MCUX_XSPI_FRAD_REGION_CHECK, (), n,	\
			       MEMC_XSPI_FRAD_REGION_IDS)
#else
#define MCUX_XSPI_VALIDATE(n)
#endif

#define MCUX_XSPI_INIT(n)	\
	PINCTRL_DT_INST_DEFINE(n);	\
	static const struct memc_mcux_xspi_config memc_mcux_xspi_config_##n = {	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),	\
		.xspi_config = {	\
			MCUX_XSPI_BYTE_ORDER(n)	\
			MCUX_XSPI_DOZE	\
			.ptrAhbAccessConfig = &(xspi_ahb_access_config_t){	\
				.ahbErrorPayload = {	\
					.highPayload = 0x5A5A5A5A,	\
					.lowPayload = 0x5A5A5A5A,	\
				},	\
				.ARDSeqIndex = 0,	\
				.enableAHBBufferWriteFlush =	\
					DT_INST_PROP(n, ahb_buffer_write_flush),	\
				.enableAHBPrefetch = DT_INST_PROP(n, ahb_prefetch),	\
				.ptrAhbWriteConfig = DT_INST_PROP(n, enable_ahb_write) ?	\
					&(xspi_ahb_write_config_t){	\
						.AWRSeqIndex = 1,	\
						.ARDSRSeqIndex = 0,	\
						.blockRead = false,	\
						.blockSequenceWrite = false,	\
					} : NULL,	\
			},	\
			.ptrIpAccessConfig = &(xspi_ip_access_config_t){	\
				.ipAccessTimeoutValue = 0xFFFFFFFF,	\
				.ptrSfpFradConfig = NULL,	\
				.ptrSfpMdadConfig = NULL,	\
				.sfpArbitrationLockTimeoutValue = 0xFFFFFF,	\
			},	\
		},	\
		.mdad_configs = {	\
			.tgMdad = {	\
				LISTIFY(MEMC_XSPI_TARGET_GROUP_COUNT,	\
					MCUX_XSPI_MDAD_INIT, (,), n)	\
			},	\
		},	\
		.mdad_valid = DT_NODE_EXISTS(MCUX_XSPI_MDAD_NODE(n, 0)),	\
		.frad_configs = {	\
			.fradConfig = {	\
				FOR_EACH_IDX_FIXED_ARG(MCUX_XSPI_FRAD_INIT, (,), n,	\
					MEMC_XSPI_FRAD_REGION_IDS)	\
			},	\
		},	\
		.frad_valid = DT_NODE_EXISTS(DT_CHILD(DT_DRV_INST(n), frad_region0)),	\
	};	\
	static struct memc_mcux_xspi_data memc_mcux_xspi_data_##n = {	\
		.base = (XSPI_Type *)DT_INST_REG_ADDR(n),	\
		.xip = MEMC_XSPI_CFG_XIP(n),	\
		.amba_address = DT_INST_REG_ADDR_BY_IDX(n, 1),	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
	};	\
	DEVICE_DT_INST_DEFINE(n, &memc_mcux_xspi_init, NULL,	\
		&memc_mcux_xspi_data_##n, &memc_mcux_xspi_config_##n,	\
		POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MCUX_XSPI_VALIDATE)
DT_INST_FOREACH_STATUS_OKAY(MCUX_XSPI_INIT)
