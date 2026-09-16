# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NVMEM_PROVIDER_PSA nvmem_provider_psa.c)

if(CONFIG_NVMEM_PROVIDER_PSA AND CONFIG_BUILD_WITH_TFM)
  zephyr_library_link_libraries(tfm_api)
endif()
