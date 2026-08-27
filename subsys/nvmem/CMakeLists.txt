# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(nvmem.c)
zephyr_library_sources_ifdef(CONFIG_NVMEM_PSA nvmem_psa.c)

if(CONFIG_NVMEM_PSA AND CONFIG_BUILD_WITH_TFM)
  zephyr_library_link_libraries(tfm_api)
endif()
