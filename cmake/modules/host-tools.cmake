# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

if(ZEPHYR_SDK_HOST_TOOLS)
  include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/host-tools.cmake)
endif()

# dtc is an optional dependency
find_program(
  DTC
  dtc
  )

if(DTC)
  # Parse the 'dtc --version' output to find the installed version.
  set(MIN_DTC_VERSION 1.4.6)
  execute_process(
    COMMAND
    ${DTC} --version
    OUTPUT_VARIABLE dtc_version_output
    ERROR_VARIABLE  dtc_error_output
    RESULT_VARIABLE dtc_status
    )

  if(${dtc_status} EQUAL 0)
    string(REGEX MATCH "Version: DTC v?([0-9]+[.][0-9]+[.][0-9]+).*" out_var ${dtc_version_output})

    # Since it is optional, an outdated version is not an error. If an
    # outdated version is discovered, print a warning and proceed as if
    # DTC were not installed.
    if(${CMAKE_MATCH_1} VERSION_GREATER ${MIN_DTC_VERSION})
      message(STATUS "Found dtc: ${DTC} (found suitable version \"${CMAKE_MATCH_1}\", minimum required is \"${MIN_DTC_VERSION}\")")
    else()
      message(WARNING
        "Could NOT find dtc: Found unsuitable version \"${CMAKE_MATCH_1}\", but required is at least \"${MIN_DTC_VERSION}\" (found ${DTC}). Optional devicetree error checking with dtc will not be performed.")
      set(DTC DTC-NOTFOUND)
    endif()
  else()
    message(WARNING
      "Could NOT find working dtc: Found dtc (${DTC}), but failed to load with:\n ${dtc_error_output}")
    set(DTC DTC-NOTFOUND)
  endif()
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
