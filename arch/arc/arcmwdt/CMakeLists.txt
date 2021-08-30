# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARCMWDT_LIBC OR CONFIG_CPLUSPLUS)
  zephyr_sources(arcmwdt-dtr-stubs.c)
endif()
