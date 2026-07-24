/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_smif_memslot

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include <infineon_kconfig.h>
#include "cy_pdl.h"
#include "cy_device_headers.h"

LOG_MODULE_REGISTER(flash_infineon, CONFIG_FLASH_LOG_LEVEL);

#define IFX_SMIF_TIMEOUT_1_MS    (1000UL)
/* Poll timeout (us) after issuing the (non-volatile) quad-enable command. */
#define IFX_SMIF_QE_TIMEOUT_US   (5000UL)

/* The QSPI Configurator (cycfg_qspi_memslot.c) generates one block-config
 * symbol per SMIF instance. Each controller node is bound to the symbol
 * whose SMIF base address matches the node's reg property. Both the
 * non-secure and secure CBUS aliases for a given SMIF map to the same
 * block-config:
 *   0x44460000 / 0x54460000 -> smif0BlockConfig
 *   0x444A0000 / 0x544A0000 -> smif1BlockConfig
 */
#if defined(SMIF0_CORE)
#define IFX_SMIF0_BASE SMIF0_CORE
#elif defined(SMIF0_BASE)
#define IFX_SMIF0_BASE SMIF0_BASE
#endif
#if defined(SMIF1_CORE)
#define IFX_SMIF1_BASE SMIF1_CORE
#elif defined(SMIF1_BASE)
#define IFX_SMIF1_BASE SMIF1_BASE
#endif

extern cy_stc_smif_block_config_t smif0BlockConfig;
#if defined(IFX_SMIF1_BASE)
extern cy_stc_smif_block_config_t smif1BlockConfig;
#endif


#define IFX_INST_USES_SMIF0(n)                                                                     \
	(DT_INST_REG_ADDR(n) == (uintptr_t)IFX_SMIF0_BASE)
#if defined(IFX_SMIF1_BASE)
#define IFX_INST_USES_SMIF1(n)                                                                     \
	(DT_INST_REG_ADDR(n) == (uintptr_t)IFX_SMIF1_BASE)
#else
#define IFX_INST_USES_SMIF1(n) 0
#endif

#if defined(IFX_SMIF1_BASE)
#define IFX_SMIF_BLOCK_CFG(n)                                                                      \
	(IFX_INST_USES_SMIF0(n) ? &smif0BlockConfig :                                              \
	 IFX_INST_USES_SMIF1(n) ? &smif1BlockConfig :                                              \
				  NULL)
#else
#define IFX_SMIF_BLOCK_CFG(n)                                                                      \
	(IFX_INST_USES_SMIF0(n) ? &smif0BlockConfig : NULL)
#endif

/* Peripheral-group slave parameters (PERI, MMIO group, slave, CLK_HF) used by
 * Cy_SysClk_PeriGroupSlaveInit() to release the SMIF core from reset and route
 * its interface clock. The device headers expose one set per SMIF core; select
 * the set that matches the node's SMIF base. SoCs with a single SMIF only
 * expose the SMIF0 set, so the SMIF1 alternative is compiled in only when a
 * second instance exists.
 */
#if defined(IFX_SMIF1_BASE)
#define IFX_SMIF_CLK_PERI_NR(n)                                                                   \
	(IFX_INST_USES_SMIF0(n) ? CY_MMIO_SMIF0_PERI_NR : CY_MMIO_SMIF01_PERI_NR)
#define IFX_SMIF_CLK_GROUP_NR(n)                                                                  \
	(IFX_INST_USES_SMIF0(n) ? CY_MMIO_SMIF0_GROUP_NR : CY_MMIO_SMIF01_GROUP_NR)
#define IFX_SMIF_CLK_SLAVE_NR(n)                                                                  \
	(IFX_INST_USES_SMIF0(n) ? CY_MMIO_SMIF0_SLAVE_NR : CY_MMIO_SMIF01_SLAVE_NR)
#define IFX_SMIF_CLK_HF_NR(n)                                                                     \
	(IFX_INST_USES_SMIF0(n) ? CY_MMIO_SMIF0_CLK_HF_NR : CY_MMIO_SMIF01_CLK_HF_NR)
#else
#define IFX_SMIF_CLK_PERI_NR(n)  CY_MMIO_SMIF0_PERI_NR
#define IFX_SMIF_CLK_GROUP_NR(n) CY_MMIO_SMIF0_GROUP_NR
#define IFX_SMIF_CLK_SLAVE_NR(n) CY_MMIO_SMIF0_SLAVE_NR
#define IFX_SMIF_CLK_HF_NR(n)    CY_MMIO_SMIF0_CLK_HF_NR
#endif


/* Flash geometry is taken from the memory node mapped by each instance's
 * zephyr,mapped-partition, so every SMIF instance reflects the size and block
 * layout of its own memory part rather than instance 0's.
 */
#define IFX_SMIF_MEMSLOT_FLASH_NODE(n)                                                            \
	DT_MEM_FROM_MAPPED_PARTITION(DT_INST(n, zephyr_mapped_partition))

#define IFX_SMIF_MEMSLOT_WRITE_BLOCK_SIZE(n)                                                      \
	DT_PROP(IFX_SMIF_MEMSLOT_FLASH_NODE(n), write_block_size)
#define IFX_SMIF_MEMSLOT_ERASE_BLOCK_SIZE(n)                                                      \
	DT_PROP(IFX_SMIF_MEMSLOT_FLASH_NODE(n), erase_block_size)
#define IFX_SMIF_MEMSLOT_TOTAL_SIZE(n)  DT_REG_SIZE(IFX_SMIF_MEMSLOT_FLASH_NODE(n))

struct ifx_smif_memslot_config {
	SMIF_Type *smif_base;
	cy_stc_smif_block_config_t *block_config;
	uint8_t mem_num;
	bool hw_init;
	uint32_t total_size;
	uint32_t erase_block_size;
	uint32_t clk_peri_nr;
	uint32_t clk_group_nr;
	uint32_t clk_slave_nr;
	uint32_t clk_hf_nr;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout pages_layout;
#endif
	struct flash_parameters params;
};

struct ifx_smif_memslot_data {
	cy_stc_smif_mem_context_t mem_context;
	struct k_sem sem;
};

static int ifx_smif_block_slot_idx(const cy_stc_smif_block_config_t *block,
								   uint8_t mem_num)
{
	const cy_en_smif_slave_select_t ss = (cy_en_smif_slave_select_t)BIT(mem_num);

	for (uint32_t i = 0; i < block->memCount; i++) {
		if (block->memConfig[i]->slaveSelect == ss) {
			return (int)i;
		}
	}
	return -ENOENT;
}

static int ifx_smif_memslot_validate_range(const struct ifx_smif_memslot_config *cfg,
				off_t offset, size_t len)
{
	if (offset < 0 || (off_t)cfg->total_size < offset) {
		LOG_ERR("Offset %ld is out of bounds", (long)offset);
		return -EINVAL;
	}

	if ((cfg->total_size - (size_t)offset) < len) {
		LOG_ERR("Length %zu exceeds memory bounds at offset %ld", len, (long)offset);
		return -EINVAL;
	}

	return 0;
} /* ifx_smif_memslot_validate_range() */


static int ifx_smif_memslot_read(const struct device *dev, off_t offset, void *buf,
				 size_t len)
{
	const struct ifx_smif_memslot_config *cfg = dev->config;
	struct ifx_smif_memslot_data *data = dev->data;
	cy_en_smif_status_t rslt;
	int ret = 0;

	if (!len) {
		return 0;
	}

	ret = ifx_smif_memslot_validate_range(cfg, offset, len);
	if (ret) {
		return ret;
	}

	k_sem_take(&data->sem, K_FOREVER);

	rslt = Cy_SMIF_MemNumRead(&data->mem_context, cfg->mem_num, (uint32_t)offset,
							  (uint8_t *)buf, (uint32_t)len);
	if (rslt != CY_SMIF_SUCCESS) {
		LOG_ERR("Error reading @ %lu (Err:0x%x)", offset, rslt);
		ret = -EIO;
	}

	k_sem_give(&data->sem);
	return ret;
}

static int ifx_smif_memslot_write(const struct device *dev, off_t offset, const void *buf,
				  size_t len)
{
	const struct ifx_smif_memslot_config *cfg = dev->config;
	struct ifx_smif_memslot_data *data = dev->data;
	cy_en_smif_status_t rslt;
	int ret = 0;

	if (len == 0) {
		return 0;
	}

	ret = ifx_smif_memslot_validate_range(cfg, offset, len);
	if (ret) {
		return ret;
	}

	k_sem_take(&data->sem, K_FOREVER);

	rslt = Cy_SMIF_MemNumWrite(&data->mem_context, cfg->mem_num, (uint32_t)offset,
							   (const uint8_t *)buf, (uint32_t)len);
	if (rslt != CY_SMIF_SUCCESS) {
		LOG_ERR("Error in writing @ %lu (Err:0x%x)", offset, rslt);
		ret = -EIO;
	}

	k_sem_give(&data->sem);
	return ret;
}

static int ifx_smif_memslot_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct ifx_smif_memslot_config *cfg = dev->config;
	struct ifx_smif_memslot_data *data = dev->data;
	cy_en_smif_status_t rslt;
	int ret = 0;

	if (size == 0) {
		return 0;
	}

	ret = ifx_smif_memslot_validate_range(cfg, offset, size);
	if (ret) {
		return ret;
	}

	if (((offset % cfg->erase_block_size) != 0) ||
	    ((size % cfg->erase_block_size) != 0)) {
		LOG_ERR("Erase range (offset %ld, size %zu) not aligned to sector size %u",
			(long)offset, size, cfg->erase_block_size);
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	if ((offset == 0) && (size == cfg->total_size)) {
		rslt = Cy_SMIF_MemNumEraseChip(&data->mem_context, cfg->mem_num);
		if (rslt != CY_SMIF_SUCCESS) {
			LOG_ERR("Error in erasing : 0x%x", rslt);
			ret = -EIO;
		}
	} else {
		rslt = Cy_SMIF_MemNumEraseSector(&data->mem_context, cfg->mem_num, offset,
						 size);
		if (rslt != CY_SMIF_SUCCESS) {
			LOG_ERR("Error in erasing : 0x%x", rslt);
			ret = -EIO;
		}
	}

	k_sem_give(&data->sem);
	return ret;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static void ifx_smif_memslot_page_layout(const struct device *dev,
					 const struct flash_pages_layout **layout,
					 size_t *layout_size)
{
	const struct ifx_smif_memslot_config *cfg = dev->config;

	*layout = &cfg->pages_layout;
	*layout_size = 1;
}
#endif

static const struct flash_parameters *ifx_smif_memslot_get_parameters(const struct device *dev)
{
	const struct ifx_smif_memslot_config *cfg = dev->config;

	return &cfg->params;
}

#ifdef CONFIG_PM
#ifdef CONFIG_SOC_SERIES_PSE84
static uint32_t smif0_crypto_input1;
static uint32_t smif0_crypto_input2;
static uint32_t smif0_crypto_input3;
static uint32_t smif1_crypto_input1;
static uint32_t smif1_crypto_input2;
static uint32_t smif1_crypto_input3;
#endif /* CONFIG_SOC_SERIES_PSE84 */

static cy_en_syspm_status_t ifx_smif_memslot_pm_callback(cy_stc_syspm_callback_params_t *params,
							 cy_en_syspm_callback_mode_t mode)
{
	ARG_UNUSED(params);

	if (mode == CY_SYSPM_CHECK_READY) {
#ifdef CONFIG_SOC_SERIES_PSE84
		smif0_crypto_input1 = SMIF_CRYPTO_INPUT1(SMIF0_CORE);
		smif0_crypto_input2 = SMIF_CRYPTO_INPUT2(SMIF0_CORE);
		smif0_crypto_input3 = SMIF_CRYPTO_INPUT3(SMIF0_CORE);
		smif1_crypto_input1 = SMIF_CRYPTO_INPUT1(SMIF1_CORE);
		smif1_crypto_input2 = SMIF_CRYPTO_INPUT2(SMIF1_CORE);
		smif1_crypto_input3 = SMIF_CRYPTO_INPUT3(SMIF1_CORE);
#endif
	}

	if (mode == CY_SYSPM_AFTER_TRANSITION) {
#ifdef CONFIG_SOC_SERIES_PSE84
		SMIF_CRYPTO_INPUT1(SMIF0_CORE) = smif0_crypto_input1;
		SMIF_CRYPTO_INPUT2(SMIF0_CORE) = smif0_crypto_input2;
		SMIF_CRYPTO_INPUT3(SMIF0_CORE) = smif0_crypto_input3;
		SMIF_CRYPTO_INPUT1(SMIF1_CORE) = smif1_crypto_input1;
		SMIF_CRYPTO_INPUT2(SMIF1_CORE) = smif1_crypto_input2;
		SMIF_CRYPTO_INPUT3(SMIF1_CORE) = smif1_crypto_input3;

		smif0_crypto_input1 = 0;
		smif0_crypto_input2 = 0;
		smif0_crypto_input3 = 0;
		smif1_crypto_input1 = 0;
		smif1_crypto_input2 = 0;
		smif1_crypto_input3 = 0;
#endif
	}

	return 0;
}

static cy_stc_syspm_callback_params_t ifx_smif_memslot_pm_params = {NULL, NULL};
static cy_stc_syspm_callback_t ifx_smif_memslot_pm_cb = {
	.callback         = &ifx_smif_memslot_pm_callback,
	.type             = CY_SYSPM_DEEPSLEEP,
	.skipMode         = CY_SYSPM_SKIP_BEFORE_TRANSITION,
	.callbackParams   = &ifx_smif_memslot_pm_params,
	.prevItm          = NULL,
	.nextItm          = NULL,
	.order            = 0,
};

/* The PM callback is global (saves/restores both SMIF cores). Register it
 * exactly once across all instances. Driver init runs in the single-threaded
 * kernel-init context, so a plain flag is enough.
 */
static void ifx_smif_memslot_pm_register_once(void)
{
	static bool registered;

	if (!registered) {
		registered = true;
		Cy_SysPm_RegisterCallback(&ifx_smif_memslot_pm_cb);
	}
}
#endif /* CONFIG_PM */

/* Default SMIF PHY/clock configuration applied when a node opts into HW init
 * with "infineon,hw-init". The values match the QSPI Configurator output for
 * the kit_pse84_eval and are appropriate for any SMIF core driven from the
 * same 200 MHz HF clock. Boards needing different timings should override
 * this driver or extend the binding to expose the relevant fields.
 */
static const cy_stc_smif_config_t ifx_smif_hw_cfg = {
	.mode = (uint32_t)CY_SMIF_NORMAL,
	.deselectDelay = 7,
	.blockEvent = (uint32_t)CY_SMIF_BUS_ERROR,
#if (CY_IP_MXSMIF_VERSION >= 4)
	.inputFrequencyMHz = 200,
	.enable_internal_dll = false,
	.dll_divider_value = CY_SMIF_DLL_DIVIDE_BY_2,
	.rx_capture_mode = CY_SMIF_SEL_NORMAL_SPI,
	.mdl_tap = CY_SMIF_MDL_8_TAP_DELAY,
	.device0_sdl_tap = CY_SMIF_SDL_8_TAP_DELAY,
	.device1_sdl_tap = CY_SMIF_SDL_8_TAP_DELAY,
	.device2_sdl_tap = CY_SMIF_SDL_8_TAP_DELAY,
	.device3_sdl_tap = CY_SMIF_SDL_8_TAP_DELAY,
	.tx_sdr_extra = CY_SMIF_TX_TWO_PERIOD_AHEAD,
#endif
};

/* Bring SMIF pins, peripheral and active slot into a known state.
 *
 * Required when the boot ROM / earlier stage has not configured this SMIF
 * core (e.g. when the first stage boots from internal RRAM under MCUboot).
 *
 * SMIF data pins live on the dedicated SMIF GPIO port and the chip-select
 * pin on a standard IOSS GPIO port. Both are described by the node's DT
 * pinctrl entries and applied here through the Zephyr pinctrl driver.
 */
static int ifx_smif_memslot_hw_init(const struct device *dev)
{
	const struct ifx_smif_memslot_config *cfg = dev->config;
	struct ifx_smif_memslot_data *data = dev->data;
	const cy_stc_smif_mem_config_t *active_mem_cfg;
	cy_en_smif_status_t smif_status;
	int slot;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("SMIF pinctrl apply failed: %d", ret);
		return ret;
	}

	/* Release the SMIF peripheral-group slave from reset and route its
	 * interface clock. Normally done by SoC startup; issue it here so the
	 * SMIF core is usable without relying on that.
	 */
	Cy_SysClk_PeriGroupSlaveInit(cfg->clk_peri_nr, cfg->clk_group_nr, cfg->clk_slave_nr,
				     cfg->clk_hf_nr);

	slot = ifx_smif_block_slot_idx(cfg->block_config, cfg->mem_num);
	if (slot < 0) {
		LOG_ERR("SMIF block has no slot for mem_num %u", cfg->mem_num);
		return -ENODEV;
	}
	active_mem_cfg = cfg->block_config->memConfig[slot];

	Cy_SMIF_Disable(cfg->smif_base);
	Cy_SMIF_DeInit(cfg->smif_base);
	Cy_SMIF_MemDeInit(cfg->smif_base);
	Cy_SMIF_Reset_Memory(cfg->smif_base, active_mem_cfg->slaveSelect);

	smif_status = Cy_SMIF_Init(cfg->smif_base, &ifx_smif_hw_cfg, IFX_SMIF_TIMEOUT_1_MS,
				   &data->mem_context.smif_context);
	if (smif_status != CY_SMIF_SUCCESS) {
		return -EIO;
	}

	Cy_SMIF_SetDataSelect(cfg->smif_base, active_mem_cfg->slaveSelect,
			      active_mem_cfg->dataSelect);
	Cy_SMIF_Enable(cfg->smif_base, &data->mem_context.smif_context);

	return 0;
}

static int ifx_smif_memslot_init(const struct device *dev)
{
	const struct ifx_smif_memslot_config *cfg = dev->config;
	struct ifx_smif_memslot_data *data = dev->data;
	const cy_stc_smif_mem_config_t *active_mem_cfg;
	cy_en_smif_status_t smif_status;
	int slot;

	if (cfg->hw_init) {
		int ret = ifx_smif_memslot_hw_init(dev);

		if (ret) {
			LOG_ERR("SMIF HW init failed: %d", ret);
			return ret;
		}
	}

	for (uint32_t i = 0; i < cfg->block_config->memCount; i++) {
		Cy_SMIF_SetDataSelect(cfg->smif_base,
				      cfg->block_config->memConfig[i]->slaveSelect,
				      cfg->block_config->memConfig[i]->dataSelect);
	}

	smif_status = Cy_SMIF_MemNumInit(cfg->smif_base, cfg->block_config, &data->mem_context);
	if (smif_status != CY_SMIF_SUCCESS) {
		LOG_ERR("SMIF MemNumInit failed: 0x%x", smif_status);
		return -EIO;
	}

	slot = ifx_smif_block_slot_idx(cfg->block_config, cfg->mem_num);
	if (slot < 0) {
		LOG_ERR("SMIF block has no slot for mem_num %u", cfg->mem_num);
		return -ENODEV;
	}
	active_mem_cfg = cfg->block_config->memConfig[slot];

	/* For non-SFDP memories the Quad-Enable bit isn't programmed by MemInit.
	 * With SFDP-probed memories readCmd/programCmd may be NULL, so guard each
	 * pointer before inspecting its data width.
	 */
	if ((active_mem_cfg->deviceCfg != NULL) &&
	    (((active_mem_cfg->deviceCfg->readCmd != NULL) &&
	      (active_mem_cfg->deviceCfg->readCmd->dataWidth == CY_SMIF_WIDTH_QUAD)) ||
	     ((active_mem_cfg->deviceCfg->programCmd != NULL) &&
	      (active_mem_cfg->deviceCfg->programCmd->dataWidth == CY_SMIF_WIDTH_QUAD)))) {
		bool is_quad = false;

		smif_status = Cy_SMIF_MemIsQuadEnabled(cfg->smif_base, active_mem_cfg, &is_quad,
						       &data->mem_context.smif_context);
		if ((smif_status == CY_SMIF_SUCCESS) && !is_quad) {
			smif_status = Cy_SMIF_MemEnableQuadMode(cfg->smif_base, active_mem_cfg,
								IFX_SMIF_QE_TIMEOUT_US,
								&data->mem_context.smif_context);
		}
		if (smif_status != CY_SMIF_SUCCESS) {
			LOG_ERR("SMIF quad enable failed: 0x%x", smif_status);
			return -EIO;
		}
	}

#if defined(CONFIG_MCUBOOT)
	/* Enable XIP/memory-mapped mode so apps can execute from external flash
	 * after MCUboot jumps to them.
	 */
	Cy_SMIF_SetMode(cfg->smif_base, CY_SMIF_MEMORY);
#endif

	k_sem_init(&data->sem, 1, 1);

#ifdef CONFIG_PM
	ifx_smif_memslot_pm_register_once();
#endif

	return 0;
}

static DEVICE_API(flash, ifx_smif_memslot_driver_api) = {
	.read = ifx_smif_memslot_read,
	.write = ifx_smif_memslot_write,
	.erase = ifx_smif_memslot_erase,
	.get_parameters = ifx_smif_memslot_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = ifx_smif_memslot_page_layout,
#endif
};

#define IFX_SMIF_MEMSLOT_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define IFX_SMIF_MEMSLOT_PINCTRL_INIT(n)   .pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),

#ifdef CONFIG_FLASH_PAGE_LAYOUT
#define IFX_SMIF_MEMSLOT_PAGES_LAYOUT_INIT(n)                                                     \
	.pages_layout = {                                                                         \
		.pages_count = IFX_SMIF_MEMSLOT_TOTAL_SIZE(n) /                                    \
			       IFX_SMIF_MEMSLOT_ERASE_BLOCK_SIZE(n),                              \
		.pages_size = IFX_SMIF_MEMSLOT_ERASE_BLOCK_SIZE(n),                                \
	},
#else
#define IFX_SMIF_MEMSLOT_PAGES_LAYOUT_INIT(n)
#endif

#define IFX_SMIF_MEMSLOT_INST(n)                                                                  \
	IFX_SMIF_MEMSLOT_PINCTRL_DEFINE(n)                                                        \
                                                                                                  \
	BUILD_ASSERT(IFX_INST_USES_SMIF0(n) || IFX_INST_USES_SMIF1(n),                            \
		     "infineon,smif-memslot reg must match a known SMIF base ");                  \
                                                                                                  \
	static struct ifx_smif_memslot_data ifx_smif_memslot_data_##n;                            \
                                                                                                  \
	static const struct ifx_smif_memslot_config ifx_smif_memslot_cfg_##n = {                  \
		.smif_base    = (SMIF_Type *)DT_INST_REG_ADDR(n),                                 \
		.block_config = IFX_SMIF_BLOCK_CFG(n),                                            \
		.mem_num      = DT_INST_PROP(n, infineon_chip_select),                            \
		.hw_init      = DT_INST_PROP(n, infineon_hw_init),                                \
		.total_size   = IFX_SMIF_MEMSLOT_TOTAL_SIZE(n),                                   \
		.erase_block_size = IFX_SMIF_MEMSLOT_ERASE_BLOCK_SIZE(n),                          \
		.clk_peri_nr  = IFX_SMIF_CLK_PERI_NR(n),                                          \
		.clk_group_nr = IFX_SMIF_CLK_GROUP_NR(n),                                         \
		.clk_slave_nr = IFX_SMIF_CLK_SLAVE_NR(n),                                         \
		.clk_hf_nr    = IFX_SMIF_CLK_HF_NR(n),                                            \
		IFX_SMIF_MEMSLOT_PINCTRL_INIT(n)                                                  \
		IFX_SMIF_MEMSLOT_PAGES_LAYOUT_INIT(n)                                             \
		.params = {                                                                       \
			.write_block_size = IFX_SMIF_MEMSLOT_WRITE_BLOCK_SIZE(n),                 \
			.erase_value      = 0xFF,                                                 \
		},                                                                                \
	};                                                                                        \
                                                                                                  \
	DEVICE_DT_INST_DEFINE(n, ifx_smif_memslot_init, NULL, &ifx_smif_memslot_data_##n,         \
			      &ifx_smif_memslot_cfg_##n, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,  \
			      &ifx_smif_memslot_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_SMIF_MEMSLOT_INST)
