# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/host-tools.cmake)

# west is optional
find_program(
  WEST
  west
  )
if(${WEST} STREQUAL WEST-NOTFOUND)
  unset(WEST)
else()
  # If west is found, make sure its version matches the minimum
  # required one.
  set(MIN_WEST_VERSION 0.6.0)
  execute_process(
    COMMAND
    ${WEST} --version
    OUTPUT_VARIABLE west_version_output
    )

  # The west --version output is currently multiline: bootstrapper
  # followed by installation version. Test both. This will still work
  # when .west/west is eliminated from the installation and obtained
  # via pypi, which will result in a single line of output.
  string(REGEX REPLACE "\n" ";" west_version_output "${west_version_output}")
  foreach(item ${west_version_output})
    if("${item}" MATCHES "^[^\/\\]*v([0-9]+[.][0-9]+[.][0-9]+)")
      set(west_version "${CMAKE_MATCH_1}")
      if(${west_version} VERSION_LESS ${MIN_WEST_VERSION})
        message(FATAL_ERROR "The detected west version is unsupported.\n\
  The version was found to be ${west_version}:\n\
    ${item}\n\
  But the minimum supported version is ${MIN_WEST_VERSION}\n\
  Please upgrade, e.g. with:\n\
      pip3 install --upgrade west\n\
  if the version from pip is too old, or:\n\
      west update\n\
  if the version in the installation is too old.")
      endif()
    endif()
  endforeach()
  # Just output information for a single version. This will still work
  # even after output is one line.
  message(STATUS "Found west: ${WEST} (found suitable version \"${west_version}\", minimum required is \"${MIN_WEST_VERSION}\")")
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
