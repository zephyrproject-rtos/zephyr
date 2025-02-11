# SPDX-License-Identifier: Apache-2.0

if(CONFIG_TEMP_NRF5 AND NOT CONFIG_TEMP_NRF5_FORCE_ALT)
  zephyr_library()
  zephyr_library_sources(temp_nrf5.c)
elseif(CONFIG_TEMP_NRFS)
  zephyr_library()
  zephyr_library_sources(temp_nrfs.c)
endif()
