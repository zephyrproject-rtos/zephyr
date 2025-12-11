# Copyright (c) 2025 Qingdao IotPi Ltd.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_DT_HAS_SIFLI_SF32LB58_HPSYS_RCC_ENABLED          clock_control_sf32lb58_hpsys_rcc.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_SIFLI_SF32LB58_HPSYS_AON_ENABLED          clock_control_sf32lb58_hpsys_aon.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_SIFLI_SF32LB58_HPSYS_PERI_CLK_ENABLED     clock_control_sf32lb58_hpsys_peri_clk.c)

zephyr_library_sources_ifdef(CONFIG_DT_HAS_SIFLI_SF32LB58_XT_CLK_ENABLED             clock_control_sf32lb58_xt_clk.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_SIFLI_SF32LB58_RC_CLK_ENABLED             clock_control_sf32lb58_rc_clk.c)
