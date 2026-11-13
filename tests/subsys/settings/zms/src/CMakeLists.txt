# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 BayLibre SAS

zephyr_include_directories(
  ${ZEPHYR_BASE}/subsys/settings/include
  ${ZEPHYR_BASE}/subsys/settings/src
  ${ZEPHYR_BASE}/tests/subsys/settings/zms/src
)

target_sources(app PRIVATE settings_test_zms.c)

if(CONFIG_FLASH_SIMULATOR_CALLBACKS)
  target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/kvss/zms)
  target_sources(app PRIVATE settings_test_zms_reset.c)
endif()

add_subdirectory(../../src settings_test_bindir)
