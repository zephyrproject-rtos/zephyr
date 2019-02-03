include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/host-tools.cmake)

# west is optional
find_program(
  WEST
  west
  )
if(${WEST} STREQUAL WEST-NOTFOUND)
  unset(WEST)
endif()

# Search for the must-have program dtc on PATH and in
# TOOLCHAIN_HOME. Usually DTC will be provided by an SDK, but for
# SDK-less projects like gnuarmemb, it is up to the user to install
# dtc.
find_program(
  DTC
  dtc
  )
if(${DTC} STREQUAL DTC-NOTFOUND)
  message(FATAL_ERROR "Unable to find dtc")
endif()

# Parse the 'dtc --version' and make sure it is at least MIN_DTC_VERSION
set(MIN_DTC_VERSION 1.4.6)
execute_process(
  COMMAND
  ${DTC} --version
  OUTPUT_VARIABLE dtc_version_output
  )
string(REGEX MATCH "Version: DTC ([0-9]+\.[0-9]+.[0-9]+).*" out_var ${dtc_version_output})
if(${CMAKE_MATCH_1} VERSION_LESS ${MIN_DTC_VERSION})
  assert(0 "The detected dtc version is unsupported.                                 \n\
    The version was found to be ${CMAKE_MATCH_1}                                   \n\
    But the minimum supported version is ${MIN_DTC_VERSION}                        \n\
    See https://docs.zephyrproject.org/latest/getting_started/                     \n\
    for how to use the SDK's dtc alongside a custom toolchain."
  )
endif()

find_program(
  GPERF
  gperf
  )
if(${GPERF} STREQUAL GPERF-NOTFOUND)
  message(FATAL_ERROR "Unable to find gperf")
endif()

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

# TODO: Should we instead find one qemu binary for each ARCH?
# TODO: This will probably need to be re-organized when there exists more than one SDK.
