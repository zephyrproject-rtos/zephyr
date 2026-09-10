# SPDX-License-Identifier: Apache-2.0

if(CONFIG_64BIT)
  message(FATAL_ERROR
    "CONFIG_64BIT=y while targeting a 32-bit ARM processor.\n"
    "If you were targeting native_sim/native/64, target native_sim instead.\n"
    "Otherwise, be sure to define CONFIG_64BIT appropriately.\n"
  )
endif()
