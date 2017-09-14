include($ENV{ZEPHYR_BASE}/cmake/host-tools-${ZEPHYR_GCC_VARIANT}.cmake OPTIONAL)

# Search for the must-have program dtc on PATH and in
# TOOLCHAIN_HOME. Usually DTC will be provided by an SDK, but for
# SDK-less projects like gccarmemb, it is up to the user to install
# dtc.
find_program(
  DTC
  dtc
  PATHS ${TOOLCHAIN_HOME} ENV PATH
  PATH_SUFFIXES "usr/bin"
  )
if(${DTC} STREQUAL DTC-NOTFOUND)
  message(FATAL_ERROR "Unable to find dtc")
endif()

find_program(
  KCONFIG_CONF
  conf
  PATHS ${TOOLCHAIN_HOME} ${PREBUILT_HOST_TOOLS}/kconfig ENV PATH
  PATH_SUFFIXES "usr/bin"
  )
if(${KCONFIG_CONF} STREQUAL KCONFIG_CONF-NOTFOUND)
  message(FATAL_ERROR "Unable to find the Kconfig program 'conf'")
endif()

# conf is an optional dependency
find_program(
  KCONFIG_MCONF
  mconf
  PATHS ${TOOLCHAIN_HOME} ${PREBUILT_HOST_TOOLS}/kconfig ENV PATH
  PATH_SUFFIXES "usr/bin"
  )

# qemu is an optional dependency
if("${ARCH}" STREQUAL "x86")
  set(QEMU_binary_suffix i386)
else()
  set(QEMU_binary_suffix ${ARCH})
endif()
find_program(
  QEMU
  qemu-system-${QEMU_binary_suffix}
  PATHS ${TOOLCHAIN_HOME} ENV PATH
  PATH_SUFFIXES "usr/bin"
  )
# TODO: Should we instead find one qemu binary for each ARCH?
# TODO: This will probably need to be re-organized when there exists more than one SDK.
