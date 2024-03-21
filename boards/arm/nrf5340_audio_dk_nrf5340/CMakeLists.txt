# Copyright (c) 2021 Nordic Semiconductor ASA.
# SPDX-License-Identifier: Apache-2.0

if ((CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP OR CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP_NS)
    AND CONFIG_BOARD_ENABLE_CPUNET)
  zephyr_library()
  zephyr_library_sources(nrf5340_audio_dk_cpunet_reset.c)

  if (CONFIG_BUILD_WITH_TFM)
    zephyr_library_include_directories(
      $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/install/interface/include
    )
  endif()

endif()
