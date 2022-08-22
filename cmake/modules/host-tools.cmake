# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

if(ZEPHYR_SDK_HOST_TOOLS)
  include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/host-tools.cmake)
endif()

# gperf is an optional dependency
find_program(
  GPERF
  gperf
  )

# openocd is an optional dependency
find_program(
  OPENOCD
  openocd
  )

# bossac is an optional dependency
find_program(
  BOSSAC
  bossac
  )

# imgtool is an optional dependency (the build may also fall back to
# scripts/imgtool.py in the mcuboot repository if that's present in
# some cases)
find_program(
  IMGTOOL
  imgtool
  )

# TODO: Should we instead find one qemu binary for each ARCH?
# TODO: This will probably need to be re-organized when there exists more than one SDK.
