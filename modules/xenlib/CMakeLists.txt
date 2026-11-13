# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 EPAM Systems
# Author: Volodymyr Babchuk <volodymyr_babchuk@epam.com>

if(CONFIG_XENLIB)

  set(ZEPHYR_XENLIB_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_include_directories(${ZEPHYR_XENLIB_DIR}/include)

  # Provide Xen headers to Xenlib through an OS-independent include path.
  # Xenlib can include them as <xen/*.h> instead of <zephyr/xen/*.h>.
  function(xenlib_include_zephyr_headers)
    zephyr_library_include_directories(${ZEPHYR_BASE}/include/zephyr)
  endfunction()

  add_subdirectory(osal)
  add_subdirectory_ifdef(CONFIG_XEN_STORE_COMMON xenstore-common)
  add_subdirectory_ifdef(CONFIG_XEN_STORE_SRV xenstore-srv)
  add_subdirectory_ifdef(CONFIG_XEN_DOMAIN_MANAGEMENT xen-dom-mgmt)
  add_subdirectory_ifdef(CONFIG_XEN_SHELL xen-shell-cmd)
  add_subdirectory_ifdef(CONFIG_XEN_CONSOLE_SRV xen-console-srv)
  add_subdirectory_ifdef(CONFIG_XEN_VCH vch)
  add_subdirectory_ifdef(CONFIG_XEN_LIBFDT libfdt)
  add_subdirectory_ifdef(CONFIG_XSTAT xstat)

endif()
