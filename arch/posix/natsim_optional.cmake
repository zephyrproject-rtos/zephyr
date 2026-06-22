# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Selection of optional components provided by the native simulator

if(CONFIG_NATIVE_USE_NSI_ERRNO)
  zephyr_library()

  zephyr_library_sources(${NSI_DIR}/common/src/nsi_errno.c)
endif()

if(CONFIG_NATIVE_USE_NSI_FCNTL)
  zephyr_library_named(nsi_fcntl)
  zephyr_library_sources(${NSI_DIR}/optional/src/nsi_fcntl.c)

  # We want to build this component using a fcntl.h which maps directly to the Zephyr VFS equivalent
  # macros. So we piggyback on the POSIX compatibility layer fcntl.h header
  zephyr_library_include_directories(${ZEPHYR_BASE}/include/zephyr/posix/)

  # Let's tell the native simulator build to build this optional component in the runner context too
  target_sources(native_simulator INTERFACE ${NSI_DIR}/optional/src/nsi_fcntl.c)
endif()
