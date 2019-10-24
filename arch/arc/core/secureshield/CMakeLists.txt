#
# Copyright (c) 2018 Synopsys, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  arc_sjli.c
  arc_secure.S
  secure_sys_services.c
  )

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/arc/include
  )
