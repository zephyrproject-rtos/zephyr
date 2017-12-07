include($ENV{ZEPHYR_BASE}/cmake/host-tools-zephyr.cmake)

# Search for the must-have program dtc on PATH and in
# TOOLCHAIN_HOME. Usually DTC will be provided by an SDK, but for
# SDK-less projects like gccarmemb, it is up to the user to install
# dtc.
find_program(
  DTC
  dtc
  )
if(${DTC} STREQUAL DTC-NOTFOUND)
  message(FATAL_ERROR "Unable to find dtc")
endif()

find_program(
  KCONFIG_CONF
  conf
  )
if(${KCONFIG_CONF} STREQUAL KCONFIG_CONF-NOTFOUND)
  message(FATAL_ERROR "Unable to find the Kconfig program 'conf'")
endif()

find_program(
  GPERF
  gperf
  )
if(${GPERF} STREQUAL GPERF-NOTFOUND)
  message(FATAL_ERROR "Unable to find gperf")
endif()

# mconf is an optional dependency
find_program(
  KCONFIG_MCONF
  mconf
  )

# openocd is an optional dependency
find_program(
  OPENOCD
  openocd
  )

# TODO: Should we instead find one qemu binary for each ARCH?
# TODO: This will probably need to be re-organized when there exists more than one SDK.
