# The IronSide source directory can be overridden by setting
# IRONSIDE_SUPPORT_DIR before invoking the build system.
zephyr_get(IRONSIDE_SUPPORT_DIR SYSBUILD GLOBAL)
if(NOT DEFINED IRONSIDE_SUPPORT_DIR)
  set(IRONSIDE_SUPPORT_DIR
    ${ZEPHYR_HAL_NORDIC_MODULE_DIR}/ironside CACHE PATH "IronSide Support Directory"
  )
endif()
