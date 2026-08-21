/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Infineon SMIF (Serial Memory InterFace) controller driver.
 *
 * This driver owns the SMIF controller instance: it enumerates the external
 * memory devices attached to the controller (described as child nodes on the
 * "smif" bus) through the vendor PDL, and owns the shared PDL memory context.
 * The per-device Zephyr APIs are implemented by separate child drivers that
 * reference this parent for the controller base and context.
 *
 * The per-device command set is described statically from devicetree, so this
 * driver builds the PDL block configuration directly from devicetree and does not
 * depend on the QSPI Configurator output (no smif0BlockConfig /
 * smif1BlockConfig). SFDP auto-discovery is intentionally not used: it issues
 * command-mode transactions that can be unsafe on a device that is executed in
 * place from and already configured by the boot ROM.
 *
 * When the boot ROM has already configured this SMIF instance and placed it in
 * XIP mode (the common case on parts that execute in place from external
 * flash), the PDL leaves the running XIP configuration untouched during
 * enumeration. Full controller bring-up (peripheral-group release,
 * Cy_SMIF_Init) is performed only when the node opts in with
 * "not-pre-initialized:", e.g. when the first stage boots from internal RRAM or
 * when the targeted SMIF instance differs from the one from which the device
 * is executing in place.
 */

#define DT_DRV_COMPAT infineon_smif

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include <infineon_kconfig.h>
#include "cy_pdl.h"
#include "cy_device_headers.h"

#include "flash_infineon_smif.h"

LOG_MODULE_REGISTER(flash_infineon_smif, CONFIG_FLASH_LOG_LEVEL);

#define IFX_SMIF_TIMEOUT_1_MS  (1000UL)
/* Poll timeout (us) after issuing the (non-volatile) quad-enable command. */
#define IFX_SMIF_QE_TIMEOUT_US (5000UL)

/* Force the SMIF register base into the correct alias for the current core.
 * Secure execution must access the SMIF block through the secure alias
 * (bit 28 set); non-secure execution must use the non-secure alias (bit 28
 * cleared). SECURE_ALIAS_OFFSET (0x10000000) comes from PDL's cy_device.h.
 */
#if defined(CONFIG_TRUSTED_EXECUTION_SECURE)
#define IFX_SMIF_ALIAS_ADDR(addr) ((uint32_t)(addr) | SECURE_ALIAS_OFFSET)
#else
#define IFX_SMIF_ALIAS_ADDR(addr) ((uint32_t)(addr) & ~SECURE_ALIAS_OFFSET)
#endif

/* SMIF core base addresses exposed by the device headers, used to select the
 * peripheral-group clock parameters for the hardware-init path.
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

#define IFX_SMIF_INST_BASE(n) IFX_SMIF_ALIAS_ADDR(DT_INST_REG_ADDR(n))

#define IFX_INST_USES_SMIF0(n) (IFX_SMIF_INST_BASE(n) == (uintptr_t)IFX_SMIF0_BASE)
#if defined(IFX_SMIF1_BASE)
#define IFX_INST_USES_SMIF1(n) (IFX_SMIF_INST_BASE(n) == (uintptr_t)IFX_SMIF1_BASE)
#else
#define IFX_INST_USES_SMIF1(n) 0
#endif

/* Peripheral-group slave parameters (PERI, MMIO group, slave, CLK_HF) used by
 * Cy_SysClk_PeriGroupSlaveInit() to release the SMIF core from reset and route
 * its interface clock during hardware init. Selected per instance from its SMIF
 * base; SoCs with a single SMIF only expose the SMIF0 set.
 */
#if defined(IFX_SMIF1_BASE)
#define IFX_SMIF_CLK_PERI_NR(n)                                                                 \
	(IFX_INST_USES_SMIF0(n) ? CY_MMIO_SMIF0_PERI_NR : CY_MMIO_SMIF01_PERI_NR)
#define IFX_SMIF_CLK_GROUP_NR(n)                                                                \
	(IFX_INST_USES_SMIF0(n) ? CY_MMIO_SMIF0_GROUP_NR : CY_MMIO_SMIF01_GROUP_NR)
#define IFX_SMIF_CLK_SLAVE_NR(n)                                                                \
	(IFX_INST_USES_SMIF0(n) ? CY_MMIO_SMIF0_SLAVE_NR : CY_MMIO_SMIF01_SLAVE_NR)
#define IFX_SMIF_CLK_HF_NR(n)                                                                   \
	(IFX_INST_USES_SMIF0(n) ? CY_MMIO_SMIF0_CLK_HF_NR : CY_MMIO_SMIF01_CLK_HF_NR)
#else
#define IFX_SMIF_CLK_PERI_NR(n)  CY_MMIO_SMIF0_PERI_NR
#define IFX_SMIF_CLK_GROUP_NR(n) CY_MMIO_SMIF0_GROUP_NR
#define IFX_SMIF_CLK_SLAVE_NR(n) CY_MMIO_SMIF0_SLAVE_NR
#define IFX_SMIF_CLK_HF_NR(n)    CY_MMIO_SMIF0_CLK_HF_NR
#endif

struct ifx_smif_controller_config {
	SMIF_Type *base;
	cy_stc_smif_block_config_t *block_config;
	const struct pinctrl_dev_config *pcfg;
	bool hw_init;
	uint32_t input_freq_mhz;
	uint32_t deselect_delay;
	uint32_t clk_peri_nr;
	uint32_t clk_group_nr;
	uint32_t clk_slave_nr;
	uint32_t clk_hf_nr;
};

#ifdef CONFIG_PM
#ifdef CONFIG_SOC_SERIES_PSE84
static uint32_t smif0_crypto_input1;
static uint32_t smif0_crypto_input2;
static uint32_t smif0_crypto_input3;
static uint32_t smif1_crypto_input1;
static uint32_t smif1_crypto_input2;
static uint32_t smif1_crypto_input3;
#endif /* CONFIG_SOC_SERIES_PSE84 */

static cy_en_syspm_status_t ifx_smif_pm_callback(cy_stc_syspm_callback_params_t *params,
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

	return CY_SYSPM_SUCCESS;
}

static cy_stc_syspm_callback_params_t ifx_smif_pm_params = {NULL, NULL};
static cy_stc_syspm_callback_t ifx_smif_pm_cb = {
	.callback = &ifx_smif_pm_callback,
	.type = CY_SYSPM_DEEPSLEEP,
	.skipMode = CY_SYSPM_SKIP_BEFORE_TRANSITION,
	.callbackParams = &ifx_smif_pm_params,
	.prevItm = NULL,
	.nextItm = NULL,
	.order = 0,
};

/* The PM callback is global (saves/restores both SMIF cores). Register it
 * exactly once across all controller instances. Driver init runs in the
 * single-threaded kernel-init context, so a plain flag is enough.
 */
static void ifx_smif_pm_register_once(void)
{
	static bool registered;

	if (!registered) {
		registered = true;
		Cy_SysPm_RegisterCallback(&ifx_smif_pm_cb);
	}
}
#endif /* CONFIG_PM */

/* Bring the SMIF controller up from reset. Only used when the node opts in
 * with "not-pre-initialized".
 */
static int ifx_smif_hw_init(const struct device *dev)
{
	const struct ifx_smif_controller_config *cfg = dev->config;
	struct ifx_smif_controller_data *data = dev->data;
	cy_en_smif_status_t status;

	const cy_stc_smif_config_t smif_hw_cfg = {
		.mode = (uint32_t)CY_SMIF_NORMAL,
		.deselectDelay = cfg->deselect_delay,
		.blockEvent = (uint32_t)CY_SMIF_BUS_ERROR,
		.inputFrequencyMHz = cfg->input_freq_mhz,
		.enable_internal_dll = false,
		.dll_divider_value = CY_SMIF_DLL_DIVIDE_BY_2,
		.rx_capture_mode = CY_SMIF_SEL_NORMAL_SPI,
		.mdl_tap = CY_SMIF_MDL_8_TAP_DELAY,
		.device0_sdl_tap = CY_SMIF_SDL_8_TAP_DELAY,
		.device1_sdl_tap = CY_SMIF_SDL_8_TAP_DELAY,
		.device2_sdl_tap = CY_SMIF_SDL_8_TAP_DELAY,
		.device3_sdl_tap = CY_SMIF_SDL_8_TAP_DELAY,
		.tx_sdr_extra = CY_SMIF_TX_TWO_PERIOD_AHEAD,
	};

	/* Release the SMIF peripheral-group slave from reset and route its
	 * interface clock. Normally done by SoC startup; issue it here so the
	 * SMIF core is usable without relying on that.
	 *
	 * Under TF-M this programs secure clock registers owned by the
	 * secure firmware (which already performs it); a non-secure access here
	 * would fault, so skip it.
	 */
#if !defined(CONFIG_BUILD_WITH_TFM)
	Cy_SysClk_PeriGroupSlaveInit(cfg->clk_peri_nr, cfg->clk_group_nr, cfg->clk_slave_nr,
				     cfg->clk_hf_nr);
#endif

	Cy_SMIF_Disable(cfg->base);
	Cy_SMIF_DeInit(cfg->base);
	Cy_SMIF_MemDeInit(cfg->base);

	status = Cy_SMIF_Init(cfg->base, &smif_hw_cfg, IFX_SMIF_TIMEOUT_1_MS,
			      &data->mem_context.smif_context);
	if (status != CY_SMIF_SUCCESS) {
		LOG_ERR("Cy_SMIF_Init failed: 0x%x", status);
		return -EIO;
	}

	for (uint32_t i = 0; i < cfg->block_config->memCount; i++) {
		Cy_SMIF_SetDataSelect(cfg->base, cfg->block_config->memConfig[i]->slaveSelect,
				      cfg->block_config->memConfig[i]->dataSelect);
	}

	Cy_SMIF_Enable(cfg->base, &data->mem_context.smif_context);

	return 0;
}

/* Program the quad-enable bit for any device whose read or program
 * command uses quad width.
 */
static int ifx_smif_quad_enable(const struct device *dev)
{
	const struct ifx_smif_controller_config *cfg = dev->config;
	struct ifx_smif_controller_data *data = dev->data;

	for (uint32_t i = 0; i < cfg->block_config->memCount; i++) {
		const cy_stc_smif_mem_config_t *mem = cfg->block_config->memConfig[i];
		cy_en_smif_status_t status;
		bool is_quad = false;

		if (mem->deviceCfg == NULL) {
			continue;
		}

		if (!(((mem->deviceCfg->readCmd != NULL) &&
		       (mem->deviceCfg->readCmd->dataWidth == CY_SMIF_WIDTH_QUAD)) ||
		      ((mem->deviceCfg->programCmd != NULL) &&
		       (mem->deviceCfg->programCmd->dataWidth == CY_SMIF_WIDTH_QUAD)))) {
			continue;
		}

		status = Cy_SMIF_MemIsQuadEnabled(cfg->base, mem, &is_quad,
						  &data->mem_context.smif_context);
		if ((status == CY_SMIF_SUCCESS) && !is_quad) {
			status = Cy_SMIF_MemEnableQuadMode(cfg->base, mem, IFX_SMIF_QE_TIMEOUT_US,
							   &data->mem_context.smif_context);
		}
		if (status != CY_SMIF_SUCCESS) {
			LOG_ERR("SMIF quad enable failed (slot %u): 0x%x", i, status);
			return -EIO;
		}
	}

	return 0;
}

static int ifx_smif_controller_init(const struct device *dev)
{
	const struct ifx_smif_controller_config *cfg = dev->config;
	struct ifx_smif_controller_data *data = dev->data;
	cy_en_smif_status_t status;
	int ret;

	k_sem_init(&data->lock, 1, 1);

	/* Configure the SMIF pins for every controller instance, independent of
	 * not-pre-initialized, so that chip-select and data lines for devices the
	 * boot ROM did not set up (for example a second device added on another
	 * chip-select) get muxed. Re-applying the pins the boot ROM already
	 * configured writes the same mux and drive settings, so it does not
	 * disturb a device being executed in place from another chip-select.
	 *
	 */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("SMIF pinctrl apply failed: %d", ret);
		return ret;
	}

	if (cfg->hw_init) {
		ret = ifx_smif_hw_init(dev);
		if (ret) {
			LOG_ERR("SMIF hardware init failed: %d", ret);
			return ret;
		}
	} else {
		/* Configure the SMIF context timeout for non-initialized hardware
		 * to a default value of 1 ms
		 */
		data->mem_context.smif_context.timeout = IFX_SMIF_TIMEOUT_1_MS;
	}

	/* Enumerate the attached devices. When the controller is already in XIP
	 * mode (boot ROM configured), the PDL leaves the running XIP registers
	 * untouched and only sets up the software context and command sets.
	 */
	status = Cy_SMIF_MemNumInit(cfg->base, cfg->block_config, &data->mem_context);
	if (status != CY_SMIF_SUCCESS) {
		LOG_ERR("Cy_SMIF_MemNumInit failed: 0x%x", status);
		return -EIO;
	}

	ret = ifx_smif_quad_enable(dev);
	if (ret) {
		return ret;
	}

#if defined(CONFIG_MCUBOOT)
	/* Enable XIP/memory-mapped mode so applications can execute in place
	 * from external flash after MCUboot jumps to them.
	 */
	Cy_SMIF_SetMode(cfg->base, CY_SMIF_MEMORY);
#endif

#ifdef CONFIG_PM
	ifx_smif_pm_register_once();
#endif

	data->ready = true;

	return 0;
}

/* Data-rate and field-presence members of cy_stc_smif_mem_cmd_t shared by the
 * single-line command initializers. All commands here are single-data-rate
 * (SDR); the command byte is always present, while dummy and mode presence are
 * supplied per command.
 */
#define IFX_SMIF_CMD(dummy_present, mode_present)                                          \
	.dataRate = CY_SMIF_SDR, .dummyCyclesPresence = (dummy_present),                       \
	.modePresence = (mode_present), .modeH = 0x00U, .modeRate = CY_SMIF_SDR,               \
	.addrRate = CY_SMIF_SDR, .cmdPresence = CY_SMIF_PRESENT_1BYTE, .commandH = 0x00U,      \
	.cmdRate = CY_SMIF_SDR,

/* Optional hybrid (mixed) erase-sector support. A device may expose regions
 * with different sector sizes and erase opcodes (for example, small parameter
 * sectors at the bottom of the array). These are described with five parallel
 * devicetree arrays (address, sector count, erase opcode, sector size and
 * per-sector erase time), one entry per region. When present, the PDL uses
 * the per-region opcode / size so that Cy_SMIF_MemEraseSector erases every
 * region correctly; otherwise the uniform eraseCmd / eraseSize is used.
 */
#define IFX_SMIF_HYBRID_REGION(i, child)                                                 \
	{                                                                                    \
		.regionAddress = DT_PROP_BY_IDX(child, hybrid_region_address, i),       \
		.sectorsCount = DT_PROP_BY_IDX(child, hybrid_region_sectors, i),        \
		.eraseCmd = DT_PROP_BY_IDX(child, hybrid_region_erase_command, i),      \
		.eraseSize = DT_PROP_BY_IDX(child, hybrid_region_erase_size, i),        \
		.eraseTime = DT_PROP_BY_IDX(child, hybrid_region_erase_time_ms, i),     \
	},
#define IFX_SMIF_HYBRID_PTR(i, child) &child##_hybrid[i],
/* All five hybrid-region arrays are indexed in parallel using the address array
 * length; a mismatched length would over-count regions and read past the end of
 * the shorter arrays. Guard against that with a compile-time length check.
 */
#define IFX_SMIF_HYBRID_ASSERT(child)                                                   \
	BUILD_ASSERT(                                                                       \
		(DT_PROP_LEN(child, hybrid_region_sectors) ==                         \
		 DT_PROP_LEN(child, hybrid_region_address)) &&                        \
			(DT_PROP_LEN(child, hybrid_region_erase_command) ==               \
			 DT_PROP_LEN(child, hybrid_region_address)) &&                    \
			(DT_PROP_LEN(child, hybrid_region_erase_size) ==                  \
			 DT_PROP_LEN(child, hybrid_region_address)) &&                    \
			(DT_PROP_LEN(child, hybrid_region_erase_time_ms) ==               \
			 DT_PROP_LEN(child, hybrid_region_address)),                      \
		"hybrid-region-* arrays must all have the same length");
#define IFX_SMIF_HYBRID_DEFINE(child)                                                   \
	COND_CODE_1(                                                                        \
		DT_NODE_HAS_PROP(child, hybrid_region_address),                        \
		(IFX_SMIF_HYBRID_ASSERT(child)                                                  \
		 static cy_stc_smif_hybrid_region_info_t child##_hybrid[] = {LISTIFY(           \
			 DT_PROP_LEN(child, hybrid_region_address),                       \
			 IFX_SMIF_HYBRID_REGION, (), child)};                                      \
		 static cy_stc_smif_hybrid_region_info_t *child##_hybrid_ptrs[] = {LISTIFY(     \
			 DT_PROP_LEN(child, hybrid_region_address), IFX_SMIF_HYBRID_PTR,  \
			 (), child)};),                                                            \
		())
#define IFX_SMIF_HYBRID_CFG(child)                                                      \
	COND_CODE_1(DT_NODE_HAS_PROP(child, hybrid_region_address),                \
		    (.hybridRegionCount = DT_PROP_LEN(child, hybrid_region_address),   \
		     .hybridRegionInfo = child##_hybrid_ptrs,),                                 \
		    ())

/* Per-device read protocol. A device may be driven in plain single-line SPI
 * (1-1-1) or in quad SPI (1-4-4 with a mode byte), selected by the
 * "protocol" devicetree enum. Only the read command differs between
 * protocols; program, erase and status commands are single-line in both cases
 * (which keeps controller-wide settings unchanged, so several devices with
 * different protocols can share one SMIF instance). The individual read fields
 * are selected here so a single read-command initializer serves both.
 */
#define IFX_SMIF_IS_SINGLE(child) DT_ENUM_HAS_VALUE(child, protocol, single_spi)

#define IFX_SMIF_READ_XFER_WIDTH(child)                                                   \
	COND_CODE_1(IFX_SMIF_IS_SINGLE(child), (CY_SMIF_WIDTH_SINGLE), (CY_SMIF_WIDTH_QUAD))
#define IFX_SMIF_READ_MODE(child)                                                         \
	COND_CODE_1(IFX_SMIF_IS_SINGLE(child), (CY_SMIF_NO_COMMAND_OR_MODE), (0x01U))
#define IFX_SMIF_READ_MODE_PRESENT(child)                                                 \
	COND_CODE_1(IFX_SMIF_IS_SINGLE(child), (CY_SMIF_NOT_PRESENT), (CY_SMIF_PRESENT_1BYTE))

/* Build the PDL memory configuration for a single child device node from its
 * devicetree properties.
 */
#define IFX_SMIF_CHILD_NOR_DEFINE(child)                                               \
	static cy_stc_smif_mem_cmd_t child##_read_cmd = {                                  \
		.command = DT_PROP(child, read_command),                              \
		.cmdWidth = CY_SMIF_WIDTH_SINGLE,                                              \
		.addrWidth = IFX_SMIF_READ_XFER_WIDTH(child),                                  \
		.mode = IFX_SMIF_READ_MODE(child),                                             \
		.modeWidth = IFX_SMIF_READ_XFER_WIDTH(child),                                  \
		.dummyCycles = DT_PROP(child, read_dummy_cycles),                   \
		.dataWidth = IFX_SMIF_READ_XFER_WIDTH(child),                                  \
		IFX_SMIF_CMD(CY_SMIF_PRESENT_1BYTE, IFX_SMIF_READ_MODE_PRESENT(child))};       \
	static cy_stc_smif_mem_cmd_t child##_write_en_cmd = {                              \
		.command = 0x06U,                                                              \
		.cmdWidth = CY_SMIF_WIDTH_SINGLE,                                              \
		.addrWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.mode = CY_SMIF_NO_COMMAND_OR_MODE,                                            \
		.modeWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.dummyCycles = 0U,                                                             \
		.dataWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		IFX_SMIF_CMD(CY_SMIF_NOT_PRESENT, CY_SMIF_NOT_PRESENT)};                       \
	static cy_stc_smif_mem_cmd_t child##_write_dis_cmd = {                             \
		.command = 0x04U,                                                              \
		.cmdWidth = CY_SMIF_WIDTH_SINGLE,                                              \
		.addrWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.mode = CY_SMIF_NO_COMMAND_OR_MODE,                                            \
		.modeWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.dummyCycles = 0U,                                                             \
		.dataWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		IFX_SMIF_CMD(CY_SMIF_NOT_PRESENT, CY_SMIF_NOT_PRESENT)};                       \
	static cy_stc_smif_mem_cmd_t child##_erase_cmd = {                                 \
		.command = DT_PROP(child, erase_command),                       \
		.cmdWidth = CY_SMIF_WIDTH_SINGLE,                                              \
		.addrWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.mode = CY_SMIF_NO_COMMAND_OR_MODE,                                            \
		.modeWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.dummyCycles = 0U,                                                             \
		.dataWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		IFX_SMIF_CMD(CY_SMIF_NOT_PRESENT, CY_SMIF_NOT_PRESENT)};                       \
	static cy_stc_smif_mem_cmd_t child##_chip_erase_cmd = {                            \
		.command = 0x60U,                                                              \
		.cmdWidth = CY_SMIF_WIDTH_SINGLE,                                              \
		.addrWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.mode = CY_SMIF_NO_COMMAND_OR_MODE,                                            \
		.modeWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.dummyCycles = 0U,                                                             \
		.dataWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		IFX_SMIF_CMD(CY_SMIF_NOT_PRESENT, CY_SMIF_NOT_PRESENT)};                       \
	static cy_stc_smif_mem_cmd_t child##_program_cmd = {                               \
		.command = DT_PROP(child, program_command),                      \
		.cmdWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.addrWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		.mode = CY_SMIF_NO_COMMAND_OR_MODE,                                           \
		.modeWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		.dummyCycles = 0U,                                                            \
		.dataWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		IFX_SMIF_CMD(CY_SMIF_NOT_PRESENT, CY_SMIF_NOT_PRESENT)};                      \
	static cy_stc_smif_mem_cmd_t child##_read_sts_wip_cmd = {                         \
		.command = 0x05U,                                                             \
		.cmdWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.addrWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		.mode = CY_SMIF_NO_COMMAND_OR_MODE,                                           \
		.modeWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		.dummyCycles = 0U,                                                            \
		.dataWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		IFX_SMIF_CMD(CY_SMIF_NOT_PRESENT, CY_SMIF_NOT_PRESENT)};                      \
	static cy_stc_smif_mem_cmd_t child##_read_sts_qe_cmd = {                          \
		.command = 0x35U,                                                             \
		.cmdWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.addrWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		.mode = CY_SMIF_NO_COMMAND_OR_MODE,                                           \
		.modeWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		.dummyCycles = 0U,                                                            \
		.dataWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		IFX_SMIF_CMD(CY_SMIF_NOT_PRESENT, CY_SMIF_NOT_PRESENT)};                      \
	static cy_stc_smif_mem_cmd_t child##_write_sts_qe_cmd = {                         \
		.command = 0x01U,                                                             \
		.cmdWidth = CY_SMIF_WIDTH_SINGLE,                                             \
		.addrWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		.mode = CY_SMIF_NO_COMMAND_OR_MODE,                                           \
		.modeWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		.dummyCycles = 0U,                                                            \
		.dataWidth = CY_SMIF_WIDTH_SINGLE,                                            \
		IFX_SMIF_CMD(CY_SMIF_NOT_PRESENT, CY_SMIF_NOT_PRESENT)};                      \
	IFX_SMIF_HYBRID_DEFINE(child)                                                     \
	static cy_stc_smif_mem_device_cfg_t child##_dev_cfg = {                           \
		.numOfAddrBytes = DT_PROP(child, address_bytes),                     \
		.memSize = DT_REG_SIZE(child),                                                \
		.readCmd = &child##_read_cmd,                                                 \
		.writeEnCmd = &child##_write_en_cmd,                                          \
		.writeDisCmd = &child##_write_dis_cmd,                                        \
		.eraseCmd = &child##_erase_cmd,                                               \
		.eraseSize = DT_PROP(child, erase_block_size),                         \
		.chipEraseCmd = &child##_chip_erase_cmd,                                      \
		.programCmd = &child##_program_cmd,                                           \
		.programSize = DT_PROP(child, page_size),                         \
		.readStsRegQeCmd = &child##_read_sts_qe_cmd,                                  \
		.readStsRegWipCmd = &child##_read_sts_wip_cmd,                                \
		.writeStsRegQeCmd = &child##_write_sts_qe_cmd,                                \
		.stsRegBusyMask = 0x01U,                                                      \
		.stsRegQuadEnableMask = 0x02U,                                                \
		.eraseTime = DT_PROP(child, erase_time_ms),                       \
		.chipEraseTime = DT_PROP(child, chip_erase_time_ms),           \
		.programTime = DT_PROP(child, program_time_us),                  \
		IFX_SMIF_HYBRID_CFG(child)                                                    \
	};                                                                                \
	static cy_stc_smif_mem_config_t child##_mem_cfg = {                               \
		.slaveSelect =                                                              \
			(cy_en_smif_slave_select_t)BIT(DT_PROP(child, chip_select)), \
		.flags = CY_SMIF_FLAG_SMIF_REV_3 | CY_SMIF_FLAG_WR_EN,                        \
		.dataSelect = (cy_en_smif_data_select_t)DT_PROP(child, data_select), \
		.baseAddress = DT_REG_ADDR(child),                                            \
		.memMappedSize = DT_REG_SIZE(child),                                          \
		.deviceCfg = &child##_dev_cfg,                                                \
	};

#define IFX_SMIF_CHILD_MEM_PTR(child) &child##_mem_cfg,

#define IFX_SMIF_CONTROLLER_INST(n)                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                        \
                                                                                      \
	BUILD_ASSERT(IFX_INST_USES_SMIF0(n) || IFX_INST_USES_SMIF1(n),                    \
		     "infineon,smif reg must match a known SMIF base");                       \
                                                                                      \
                                                                                      \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, IFX_SMIF_CHILD_NOR_DEFINE)                   \
                                                                                      \
	static cy_stc_smif_mem_config_t *ifx_smif_mem_cfgs_##n[] = {                      \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, IFX_SMIF_CHILD_MEM_PTR)};                \
                                                                                      \
	static cy_stc_smif_block_config_t ifx_smif_block_##n = {                          \
		.memCount = ARRAY_SIZE(ifx_smif_mem_cfgs_##n),                                \
		.memConfig = ifx_smif_mem_cfgs_##n,                                           \
		.majorVersion = CY_SMIF_DRV_VERSION_MAJOR,                                    \
		.minorVersion = CY_SMIF_DRV_VERSION_MINOR,                                    \
	};                                                                                \
                                                                                      \
	static struct ifx_smif_controller_data ifx_smif_controller_data_##n;              \
                                                                                      \
	static const struct ifx_smif_controller_config ifx_smif_controller_config_##n = {   \
		.base = (SMIF_Type *)IFX_SMIF_INST_BASE(n),                                     \
		.block_config = &ifx_smif_block_##n,                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.hw_init = DT_INST_PROP(n, not_pre_initialized),                                   \
		.input_freq_mhz = DT_INST_PROP(n, input_frequency_mhz),                \
		.deselect_delay = DT_INST_PROP(n, deselect_delay),                     \
		.clk_peri_nr = IFX_SMIF_CLK_PERI_NR(n),                                         \
		.clk_group_nr = IFX_SMIF_CLK_GROUP_NR(n),                                       \
		.clk_slave_nr = IFX_SMIF_CLK_SLAVE_NR(n),                                       \
		.clk_hf_nr = IFX_SMIF_CLK_HF_NR(n),                                             \
	};                                                                                  \
                                                                                        \
	DEVICE_DT_INST_DEFINE(n, ifx_smif_controller_init, NULL, &ifx_smif_controller_data_##n,  \
			      &ifx_smif_controller_config_##n, POST_KERNEL,                       \
			      CONFIG_FLASH_INFINEON_SMIF_CTRL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(IFX_SMIF_CONTROLLER_INST)
