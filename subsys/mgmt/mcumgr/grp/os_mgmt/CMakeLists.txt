#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# OS Management group public API is exposed through zephyr_interface,
# when OS Management is enabled.
add_library(mgmt_mcumgr_grp_os src/os_mgmt.c)

target_include_directories(mgmt_mcumgr_grp_os PUBLIC include)

target_link_libraries(mgmt_mcumgr_grp_os PRIVATE
  zephyr_interface mgmt_mcumgr_mgmt mgmt_mcumgr_util
)

if (CONFIG_REBOOT)
  target_link_libraries(mgmt_mcumgr_grp_os PRIVATE kernel)
endif()

target_link_libraries(mgmt_mcumgr INTERFACE mgmt_mcumgr_grp_os)

if(DEFINED CONFIG_MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME)
  set(MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME_DIR ${PROJECT_BINARY_DIR}/os_mgmt_auto)
  file(MAKE_DIRECTORY ${MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME_DIR})
  file(WRITE ${MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME_DIR}/os_mgmt_build_date.c "/* Auto generated file, do not edit */\n#include <stdint.h>\nuint8_t *MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME = __TIMESTAMP__;")
  zephyr_library_sources(${MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME_DIR}/os_mgmt_build_date.c)
endif()
