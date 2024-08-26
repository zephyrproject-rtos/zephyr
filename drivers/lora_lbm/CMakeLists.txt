# Copyright (c) 2024 Semtech Corporation - Félix Piédallu
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# Disable all warnings for Semtech code.
#
# Zephyr is compiled with a lot more warnings enabled then the basics modem.
# Since we copy the Basics Modem directly with no modifications, the warnings clog up
# the build output in our applications.
# zephyr_library_compile_options(-w)

if(CONFIG_SEMTECH_LR11XX)
  # Library flag that disables some warnings
  zephyr_library_compile_definitions(LR11XX_DISABLE_WARNINGS)

  zephyr_library_sources(lr11xx/lr11xx_board.c lr11xx/lr11xx_hal.c)

  zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_DRIVERS_RAL_RALF
    lr11xx/lr11xx_ral_bsp.c lr11xx/lr11xx_ral_bsp_calibration.c
  )
endif()

if(CONFIG_SEMTECH_SX126X)
  # Library flag that disables some warnings
  # zephyr_library_compile_definitions(LR11XX_DISABLE_WARNINGS)

  zephyr_library_sources(sx126x/sx126x_hal.c sx126x/sx126x_board.c)

  zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_DRIVERS_RAL_RALF
    sx126x/sx126x_ral_bsp.c
  )
endif()
