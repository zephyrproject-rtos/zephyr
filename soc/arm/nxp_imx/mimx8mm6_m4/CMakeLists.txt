#
# Copyright (c) 2020, Manivannan Sadhasivam <mani@kernel.org>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )

if(CONFIG_OPENAMP_RSC_TABLE)
  zephyr_linker_section(NAME .resource_table GROUP ROM_REGION NOINPUT)
  zephyr_linker_section_configure(SECTION .resource_table KEEP INPUT ".resource_table*")
endif()
