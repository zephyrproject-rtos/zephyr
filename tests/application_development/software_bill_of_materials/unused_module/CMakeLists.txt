# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SBOM_UNUSED_MODULE)
  zephyr_library()
  zephyr_library_sources(src/unused.c)
  zephyr_include_directories(include)
endif()
