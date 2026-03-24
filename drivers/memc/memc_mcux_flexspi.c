/*
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_imx_flexspi

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>
#include <soc.h>

#include "memc_mcux_flexspi.h"


/*
 * NOTE: If CONFIG_FLASH_MCUX_FLEXSPI_XIP is selected, Any external functions
 * called while interacting with the flexspi MUST be relocated to SRAM or ITCM
 * at runtime, so that the chip does not access the flexspi to read program
 * instructions while it is being written to
 */
#if defined(CONFIG_FLASH_MCUX_FLEXSPI_XIP) && (CONFIG_MEMC_LOG_LEVEL > 0)
#warning "Enabling memc driver logging and XIP mode simultaneously can cause \
	read-while-write hazards. This configuration is not recommended."
#endif

#define FLEXSPI_MAX_LUT 64U

LOG_MODULE_REGISTER(memc_flexspi, CONFIG_MEMC_LOG_LEVEL);

struct memc_flexspi_buf_cfg {
	uint16_t prefetch;
	uint16_t priority;
	uint16_t master_id;
	uint16_t buf_size;
} __packed;

/* Structure tracking LUT offset and usage per each port */
struct port_lut {
	uint8_t lut_offset;
	uint8_t lut_used;
};

#define DEV_CFG(_dev) ((const struct memc_flexspi_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct memc_flexspi_data *)(_dev)->data)

struct memc_flexspi_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	DEVICE_MMIO_NAMED_ROM(ahb);
};

/* flexspi device data should be stored in RAM to avoid read-while-write hazards */
struct memc_flexspi_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	DEVICE_MMIO_NAMED_RAM(ahb);
	bool xip;
	bool ahb_bufferable;
	bool ahb_cacheable;
	bool ahb_prefetch;
	bool ahb_read_addr_opt;
	uint8_t ahb_boundary;
	bool combination_mode;
	bool sck_differential_clock;
	flexspi_read_sample_clock_t rx_sample_clock;
#if defined(FSL_FEATURE_FLEXSPI_SUPPORT_SEPERATE_RXCLKSRC_PORTB) && \
FSL_FEATURE_FLEXSPI_SUPPORT_SEPERATE_RXCLKSRC_PORTB
	flexspi_read_sample_clock_t rx_sample_clock_b;
#endif
	const struct pinctrl_dev_config *pincfg;
	size_t size[kFLEXSPI_PortCount];
	struct port_lut port_luts[kFLEXSPI_PortCount];
	struct memc_flexspi_buf_cfg *buf_cfg;
	uint8_t buf_cfg_cnt;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

static inline FLEXSPI_Type *get_base(const struct device *dev)
{
	return (FLEXSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
}

static inline uint8_t *get_ahb(const struct device *dev)
{
	return (uint8_t *)DEVICE_MMIO_NAMED_GET(dev, ahb);
}

void memc_flexspi_wait_bus_idle(const struct device *dev)
{
	FLEXSPI_Type *base = get_base(dev);

	while (false == FLEXSPI_GetBusIdleStatus(base)) {
	}
}

bool memc_flexspi_is_running_xip(const struct device *dev)
{
	struct memc_flexspi_data *data = dev->data;

	return data->xip;
}

int memc_flexspi_update_clock(const struct device *dev,
		flexspi_device_config_t *device_config,
		flexspi_port_t port, uint32_t freq_hz)
{
	struct memc_flexspi_data *data = dev->data;
	FLEXSPI_Type *base = get_base(dev);
	uint32_t key;
	uint32_t divider, actual_freq, flexspiRootClk_copy, ccm_clock;

	int ret = clock_control_get_rate(data->clock_dev, data->clock_subsys, &ccm_clock);

	if (ret < 0) {
		LOG_ERR("memc flexspi get root clock error: %d", ret);
		return ret;
	}

	/* Freq required shall not exceed the max freq flash can support. */
	freq_hz = MIN(freq_hz, device_config->flexspiRootClk);

	/* Get the real freq on going. */
	divider = (base->MCR0 & FLEXSPI_MCR0_SERCLKDIV_MASK) >> FLEXSPI_MCR0_SERCLKDIV_SHIFT;
	actual_freq = ccm_clock / (divider + 1);
	if (freq_hz ==  actual_freq) {
		return 0;
	}


	/* To reclock the FlexSPI, we should:
	 * - disable the module
	 * - set the new clock
	 * - reenable the module
	 * - reset the module
	 * We CANNOT XIP at any point during this process
	 */
	key = irq_lock();
	memc_flexspi_wait_bus_idle(dev);
	FLEXSPI_Enable(base, false);

	 /* Select a divider based on root frequency.
	  * if we can't get an exact divider, round down
	  */
	divider = ((ccm_clock + (freq_hz - 1)) / freq_hz) - 1;
	/* Cap divider to max value */
	divider = MIN(divider, FLEXSPI_MCR0_SERCLKDIV_MASK >> FLEXSPI_MCR0_SERCLKDIV_SHIFT);
	/* Update the internal divider*/
	base->MCR0 &= ~FLEXSPI_MCR0_SERCLKDIV_MASK;
	base->MCR0 |= FLEXSPI_MCR0_SERCLKDIV(divider);

	/*
	 * We don't want to modify the root clock variable, but we have to use this
	 * parameter for DLL updating purpose(FLEXSPI_CalculateDll use flexspiRootClk as
	 * the real frequency of FlexSPI serial clock). Back up it and restore it after DLL
	 * Updating.
	 */
	flexspiRootClk_copy = device_config->flexspiRootClk;
	device_config->flexspiRootClk = ccm_clock/(divider + 1);
	FLEXSPI_UpdateDllValue(base, device_config, port);
	/* Restore root clock */
	device_config->flexspiRootClk = flexspiRootClk_copy;

	FLEXSPI_Enable(base, true);
	memc_flexspi_reset(dev);

	irq_unlock(key);
	return 0;
}

int memc_flexspi_set_device_config(const struct device *dev,
		const flexspi_device_config_t *device_config,
		const uint32_t *lut_array,
		uint8_t lut_count,
		flexspi_port_t port)
{
	FLEXSPI_Type *base = get_base(dev);
	flexspi_device_config_t tmp_config;
	uint32_t tmp_lut[FLEXSPI_MAX_LUT];
	struct memc_flexspi_data *data = dev->data;
	const uint32_t *lut_ptr = lut_array;
	uint8_t lut_used = 0U;
	unsigned int key = 0;
	int ret;
	uint32_t divider;

	if (port >= kFLEXSPI_PortCount) {
		LOG_ERR("Invalid port number");
		return -EINVAL;
	}

	if (data->port_luts[port].lut_used < lut_count) {
		/* We cannot reuse the existing LUT slot,
		 * Check if the LUT table will fit into the remaining LUT slots
		 */
		for (uint8_t i = 0; i < kFLEXSPI_PortCount; i++) {
			lut_used += data->port_luts[i].lut_used;
		}

		if ((lut_used + lut_count) > FLEXSPI_MAX_LUT) {
			return -ENOBUFS;
		}
	}

	data->size[port] = device_config->flashSize * KB(1);

	if (memc_flexspi_is_running_xip(dev)) {
		/* We need to avoid flash access while configuring the FlexSPI.
		 * To do this, we will copy the LUT array into stack-allocated
		 * temporary memory
		 */
		memcpy(tmp_lut, lut_array, lut_count * MEMC_FLEXSPI_CMD_SIZE);
		lut_ptr = tmp_lut;
	}

	memcpy(&tmp_config, device_config, sizeof(tmp_config));
	/* Update FlexSPI AWRSEQID and ARDSEQID values based on where the LUT
	 * array will actually be loaded.
	 */
	if (data->port_luts[port].lut_used < lut_count) {
		/* Update lut offset with new value */
		data->port_luts[port].lut_offset = lut_used;
	}
	/* LUTs should only be installed on sequence boundaries, every
	 * 4 entries. Round LUT usage up to nearest sequence
	 */
	data->port_luts[port].lut_used = ROUND_UP(lut_count, 4);
	tmp_config.ARDSeqIndex += data->port_luts[port].lut_offset / MEMC_FLEXSPI_CMD_PER_SEQ;
	tmp_config.AWRSeqIndex += data->port_luts[port].lut_offset / MEMC_FLEXSPI_CMD_PER_SEQ;

	/* Set FlexSPI clock to the max frequency flash can support.
	 * FLEXSPI_SetFlashConfig only update DLL but not the freq divider.
	 */
	ret = memc_flexspi_update_clock(dev, &tmp_config,
					port, device_config->flexspiRootClk);
	if (ret < 0) {
		LOG_ERR("memc flexspi update clock error: %d", ret);
		return ret;
	}

	/* Get the real clock for DLL updating. */
	ret = clock_control_get_rate(data->clock_dev, data->clock_subsys,
				&tmp_config.flexspiRootClk);
	if (ret < 0) {
		LOG_ERR("memc flexspi get root clock error: %d", ret);
		return ret;
	}
	divider = (base->MCR0 & FLEXSPI_MCR0_SERCLKDIV_MASK) >> FLEXSPI_MCR0_SERCLKDIV_SHIFT;
	tmp_config.flexspiRootClk /= (divider + 1);

	/* Lock IRQs before reconfiguring FlexSPI, to prevent XIP */
	key = irq_lock();
	FLEXSPI_SetFlashConfig(base, &tmp_config, port);

#if (CONFIG_FLASH_MCUX_FLEXSPI_FORCE_USING_OVRDVAL == 1)
	base->DLLCR[port >> 1U] = FLEXSPI_DLLCR_OVRDEN(1) |
					FLEXSPI_DLLCR_OVRDVAL(CONFIG_FLASH_MCUX_FLEXSPI_OVRDVAL);
#endif

	FLEXSPI_UpdateLUT(base, data->port_luts[port].lut_offset,
			  lut_ptr, lut_count);
	irq_unlock(key);

	return 0;
}

int memc_flexspi_reset(const struct device *dev)
{
	FLEXSPI_Type *base = get_base(dev);

	FLEXSPI_SoftwareReset(base);

	return 0;
}

int memc_flexspi_transfer(const struct device *dev,
		flexspi_transfer_t *transfer)
{
	FLEXSPI_Type *base = get_base(dev);
	flexspi_transfer_t tmp;
	struct memc_flexspi_data *data = dev->data;
	status_t status;
	uint32_t seq_off, addr_offset = 0U;
	int i;

	/* Calculate sequence offset and address offset based on port */
	seq_off = data->port_luts[transfer->port].lut_offset /
				MEMC_FLEXSPI_CMD_PER_SEQ;
	for (i = 0; i < transfer->port; i++) {
		addr_offset += data->size[i];
	}

	if ((seq_off != 0) || (addr_offset != 0)) {
		/* Adjust device address and sequence index for transfer */
		memcpy(&tmp, transfer, sizeof(tmp));
		tmp.seqIndex += seq_off;
		tmp.deviceAddress += addr_offset;
		status = FLEXSPI_TransferBlocking(base, &tmp);
	} else {
		/* Transfer does not need adjustment */
		status = FLEXSPI_TransferBlocking(base, transfer);
	}

	if (status != kStatus_Success) {
		LOG_ERR("Transfer error: %d", status);
		return -EIO;
	}

	return 0;
}

void *memc_flexspi_get_ahb_address(const struct device *dev,
		flexspi_port_t port, off_t offset)
{
	struct memc_flexspi_data *data = dev->data;
	uint8_t *ahb_base = (uint8_t *)DEVICE_MMIO_NAMED_GET(dev, ahb);
	int i;

	if (port >= kFLEXSPI_PortCount) {
		LOG_ERR("Invalid port number: %u", port);
		return NULL;
	}

	for (i = 0; i < port; i++) {
		offset += data->size[i];
	}

	return ahb_base + offset;
}

static int memc_flexspi_init(const struct device *dev)
{
	struct memc_flexspi_data *data = dev->data;
	flexspi_config_t flexspi_config;
	uint32_t flash_sizes[kFLEXSPI_PortCount] = { 0 };
	FLEXSPI_Type *base;
	int ret;
	uint8_t i;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	DEVICE_MMIO_NAMED_MAP(dev, ahb, data->ahb_cacheable ? K_MEM_DIRECT_MAP
				: (K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP));
	base = get_base(dev);

	/* we should not configure the device we are running on */
	if (memc_flexspi_is_running_xip(dev)) {
		if (!IS_ENABLED(CONFIG_MEMC_MCUX_FLEXSPI_INIT_XIP)) {
			LOG_DBG("XIP active on %s, skipping init", dev->name);
			return 0;
		}
	}
	/*
	 * SOCs such as the RT1064 and RT1024 have internal flash, and no pinmux
	 * settings, continue if no pinctrl state found.
	 */
	ret = pinctrl_apply_state(data->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	FLEXSPI_GetDefaultConfig(&flexspi_config);

	flexspi_config.ahbConfig.enableAHBBufferable = data->ahb_bufferable;
	flexspi_config.ahbConfig.enableAHBCachable = data->ahb_cacheable;
	flexspi_config.ahbConfig.enableAHBPrefetch = data->ahb_prefetch;
	flexspi_config.ahbConfig.enableReadAddressOpt = data->ahb_read_addr_opt;
#if !(defined(FSL_FEATURE_FLEXSPI_HAS_NO_MCR0_COMBINATIONEN) && \
	FSL_FEATURE_FLEXSPI_HAS_NO_MCR0_COMBINATIONEN)
	flexspi_config.enableCombination = data->combination_mode;
#endif

#if !(defined(FSL_FEATURE_FLEXSPI_HAS_NO_MCR2_SCKBDIFFOPT) && \
	FSL_FEATURE_FLEXSPI_HAS_NO_MCR2_SCKBDIFFOPT)
	flexspi_config.enableSckBDiffOpt = data->sck_differential_clock;
#endif
	flexspi_config.rxSampleClock = data->rx_sample_clock;
#if defined(FSL_FEATURE_FLEXSPI_SUPPORT_SEPERATE_RXCLKSRC_PORTB) && \
FSL_FEATURE_FLEXSPI_SUPPORT_SEPERATE_RXCLKSRC_PORTB
	flexspi_config.rxSampleClockPortB = data->rx_sample_clock_b;
#if defined(FSL_FEATURE_FLEXSPI_SUPPORT_RXCLKSRC_DIFF) && \
	FSL_FEATURE_FLEXSPI_SUPPORT_RXCLKSRC_DIFF
	if (flexspi_config.rxSampleClock != flexspi_config.rxSampleClockPortB) {
		flexspi_config.rxSampleClockDiff = true;
	}
#endif
#endif

	/* Configure AHB RX buffers, if any configuration settings are present */
	__ASSERT(data->buf_cfg_cnt < FSL_FEATURE_FLEXSPI_AHB_BUFFER_COUNT,
		"Maximum RX buffer configuration count exceeded");
	for (i = 0; i < data->buf_cfg_cnt; i++) {
		/* Should AHB prefetch up to buffer size? */
		flexspi_config.ahbConfig.buffer[i].enablePrefetch = data->buf_cfg[i].prefetch;
		/* AHB access priority (used for suspending control of AHB prefetching )*/
		flexspi_config.ahbConfig.buffer[i].priority = data->buf_cfg[i].priority;
		/* AHB master index, SOC specific */
		flexspi_config.ahbConfig.buffer[i].masterIndex = data->buf_cfg[i].master_id;
		/* RX buffer allocation (total available buffer space is instance/SOC specific) */
		flexspi_config.ahbConfig.buffer[i].bufferSize = data->buf_cfg[i].buf_size;
	}

	if (memc_flexspi_is_running_xip(dev)) {
		/* Save flash sizes- FlexSPI init will reset them */
		for (i = 0; i < kFLEXSPI_PortCount; i++) {
			flash_sizes[i] = base->FLSHCR0[i];
		}
	}

	FLEXSPI_Init(base, &flexspi_config);

#if defined(FLEXSPI_AHBCR_ALIGNMENT_MASK)
	/* Configure AHB alignment boundary */
	base->AHBCR = (base->AHBCR & ~FLEXSPI_AHBCR_ALIGNMENT_MASK) |
		FLEXSPI_AHBCR_ALIGNMENT(data->ahb_boundary);
#endif

	if (memc_flexspi_is_running_xip(dev)) {
		/* Restore flash sizes */
		for (i = 0; i < kFLEXSPI_PortCount; i++) {
			base->FLSHCR0[i] = flash_sizes[i];
		}

		/* Reenable FLEXSPI module */
		base->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int memc_flexspi_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct memc_flexspi_data *data = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(data->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pinctrl_apply_state(data->pincfg, PINCTRL_STATE_SLEEP);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#if defined(FSL_FEATURE_FLEXSPI_SUPPORT_SEPERATE_RXCLKSRC_PORTB) && \
	FSL_FEATURE_FLEXSPI_SUPPORT_SEPERATE_RXCLKSRC_PORTB
#define MEMC_FLEXSPI_RXCLK_B(inst) .rx_sample_clock_b = DT_INST_PROP(inst, rx_clock_source_b),
#else
#define MEMC_FLEXSPI_RXCLK_B(inst)
#endif

#if defined(CONFIG_XIP) && defined(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
/* Checks if image flash base address is in the FlexSPI AHB base region */
#define MEMC_FLEXSPI_CFG_XIP(node_id)						\
	((CONFIG_FLASH_BASE_ADDRESS) >= DT_REG_ADDR_BY_IDX(node_id, 1)) &&	\
	((CONFIG_FLASH_BASE_ADDRESS) < (DT_REG_ADDR_BY_IDX(node_id, 1) +	\
					DT_REG_SIZE_BY_IDX(node_id, 1)))

#else
#define MEMC_FLEXSPI_CFG_XIP(node_id) false
#endif

#define MEMC_FLEXSPI(n)							\
	PINCTRL_DT_INST_DEFINE(n);					\
	static uint16_t  buf_cfg_##n[] =				\
		DT_INST_PROP_OR(n, rx_buffer_config, {0});		\
									\
	static const struct memc_flexspi_config memc_flexspi_config_##n = {	\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(reg_base, DT_DRV_INST(n)),	\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ahb, DT_DRV_INST(n)),	\
	};								\
									\
	static struct memc_flexspi_data					\
		memc_flexspi_data_##n = {				\
		.xip = MEMC_FLEXSPI_CFG_XIP(DT_DRV_INST(n)),		\
		.ahb_bufferable = DT_INST_PROP(n, ahb_bufferable),	\
		.ahb_cacheable = DT_INST_PROP(n, ahb_cacheable),	\
		.ahb_prefetch = DT_INST_PROP(n, ahb_prefetch),		\
		.ahb_read_addr_opt = DT_INST_PROP(n, ahb_read_addr_opt),\
		.ahb_boundary = DT_INST_ENUM_IDX(n, ahb_boundary),	\
		.combination_mode = DT_INST_PROP(n, combination_mode),	\
		.sck_differential_clock = DT_INST_PROP(n, sck_differential_clock),	\
		.rx_sample_clock = DT_INST_PROP(n, rx_clock_source),	\
		MEMC_FLEXSPI_RXCLK_B(n)                                 \
		.buf_cfg = (struct memc_flexspi_buf_cfg *)buf_cfg_##n,	\
		.buf_cfg_cnt = sizeof(buf_cfg_##n) /			\
			sizeof(struct memc_flexspi_buf_cfg),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),     \
		.clock_subsys = (clock_control_subsys_t)                \
			DT_INST_CLOCKS_CELL(n, name),                   \
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(n, memc_flexspi_pm_action);		\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      memc_flexspi_init,			\
			      PM_DEVICE_DT_INST_GET(n),			\
			      &memc_flexspi_data_##n,			\
			      &memc_flexspi_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_MEMC_MCUX_FLEXSPI_INIT_PRIORITY,	\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_FLEXSPI)
