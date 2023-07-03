# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${CONFIG_SOC})

zephyr_sources(
  start.S
  soc_irq.S
  l2_cache.c
)

zephyr_sources_ifdef(CONFIG_SOC_ANDES_V5_PMA pma.c)

# Note: AndeStar V5 DSP needs custom Andes V5 toolchain
if(CONFIG_SOC_ANDES_V5_HWDSP)
  zephyr_cc_option(-mext-dsp)
endif()
