# Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

# The lora-basics-modem backend implementation depends on the include directories
# exposed by the lora-basics-modem library. Hence, if it exists then the source
# files are added to that otherwise a library with same name is created.
if(TARGET lora-basics-modem)
  set(ZEPHYR_CURRENT_LIBRARY lora-basics-modem)
else()
  zephyr_library_named(lora-basics-modem)
endif()

# Don't build the real implementation when the emulator is used
if(NOT CONFIG_LORAWAN_EMUL)
  zephyr_library_sources(lorawan.c)
  zephyr_library_sources(lw_priv.c)
endif()

add_subdirectory_ifdef(CONFIG_LORAWAN_NVM_SETTINGS nvm)
