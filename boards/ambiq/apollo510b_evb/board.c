/*
 * Copyright 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/pm/device_runtime.h>
#include <am_mcu_apollo.h>

#if DT_HAS_CHOSEN(ambiq_xo32m)
#define XTAL_HS_FREQ DT_PROP(DT_CHOSEN(ambiq_xo32m), clock_frequency)
#if DT_SAME_NODE(DT_CHOSEN(ambiq_xo32m), DT_NODELABEL(xo32m_xtal))
#define XTAL_HS_MODE AM_HAL_CLKMGR_XTAL_HS_MODE_XTAL
#elif DT_SAME_NODE(DT_CHOSEN(ambiq_xo32m), DT_NODELABEL(xo32m_ext))
#define XTAL_HS_MODE AM_HAL_CLKMGR_XTAL_HS_MODE_EXT
#endif
#else
#define XTAL_HS_FREQ 0
#define XTAL_HS_MODE AM_HAL_CLKMGR_XTAL_HS_MODE_XTAL
#endif

#if DT_HAS_CHOSEN(ambiq_xo32k)
#define XTAL_LS_FREQ DT_PROP(DT_CHOSEN(ambiq_xo32k), clock_frequency)
#if DT_SAME_NODE(DT_CHOSEN(ambiq_xo32k), DT_NODELABEL(xo32k_xtal))
#define XTAL_LS_MODE AM_HAL_CLKMGR_XTAL_LS_MODE_XTAL
#elif DT_SAME_NODE(DT_CHOSEN(ambiq_xo32k), DT_NODELABEL(xo32k_ext))
#define XTAL_LS_MODE AM_HAL_CLKMGR_XTAL_LS_MODE_EXT
#endif
#else
#define XTAL_LS_FREQ 0
#define XTAL_LS_MODE AM_HAL_CLKMGR_XTAL_LS_MODE_XTAL
#endif

#if DT_HAS_CHOSEN(ambiq_extrefclk)
#define EXTREFCLK_FREQ DT_PROP(DT_CHOSEN(ambiq_extrefclk), clock_frequency)
#else
#define EXTREFCLK_FREQ 0
#endif

void board_early_init_hook(void)
{
	/* Set board related info into clock manager */
	am_hal_clkmgr_board_info_t sClkmgrBoardInfo = {.sXtalHs.eXtalHsMode = XTAL_HS_MODE,
						       .sXtalHs.ui32XtalHsFreq = XTAL_HS_FREQ,
						       .sXtalLs.eXtalLsMode = XTAL_LS_MODE,
						       .sXtalLs.ui32XtalLsFreq = XTAL_LS_FREQ,
						       .ui32ExtRefClkFreq = EXTREFCLK_FREQ};
	am_hal_clkmgr_board_info_set(&sClkmgrBoardInfo);

	/* Default HFRC and HFRC2 to Free Running clocks */
	am_hal_clkmgr_clock_config(AM_HAL_CLKMGR_CLK_ID_HFRC,
				   AM_HAL_CLKMGR_HFRC_FREQ_FREE_RUN_APPROX_48MHZ, NULL);
	am_hal_clkmgr_clock_config(AM_HAL_CLKMGR_CLK_ID_HFRC2,
				   AM_HAL_CLKMGR_HFRC2_FREQ_FREE_RUN_APPROX_250MHZ, NULL);

}

#if defined(CONFIG_BOARD_ENABLE_GPU_ASSET_RELOCATION)

/* Symbols defined in the board-level linker.ld */
extern char __gfx_assets_start[];
extern char __gfx_assets_load_start[];
extern char __gfx_assets_size[];

void board_late_init_hook(void)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(ambiq_psram)) &&                                             \
	DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(ambiq_external_ram_region))
	pm_device_runtime_get(DEVICE_DT_GET(DT_CHOSEN(ambiq_psram)));
#endif

	memcpy(__gfx_assets_start, __gfx_assets_load_start, (size_t)&__gfx_assets_size);

	sys_cache_data_flush_range(__gfx_assets_start, (size_t)&__gfx_assets_size);
}
#endif /* CONFIG_BOARD_ENABLE_GPU_ASSET_RELOCATION */
