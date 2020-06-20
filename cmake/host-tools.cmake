# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/host-tools.cmake)

# west is an optional dependency
find_program(
  WEST
  west
  )
if(${WEST} STREQUAL WEST-NOTFOUND)
  unset(WEST)
else()
  # If west is found, make sure its version matches the minimum
  # required one.
  set(MIN_WEST_VERSION 0.7.1)
  execute_process(
    COMMAND
    ${PYTHON_EXECUTABLE}
    -c
    "import west.version; print(west.version.__version__, end='')"
    OUTPUT_VARIABLE west_version
    RESULT_VARIABLE west_version_output_result
    )

  if(west_version_output_result)
    message(FATAL_ERROR "Unable to import west.version from '${PYTHON_EXECUTABLE}'")
  endif()

  if(${west_version} VERSION_LESS ${MIN_WEST_VERSION})
    message(FATAL_ERROR "The detected west version is unsupported.\n\
  The version was found to be ${west_version}:\n\
    ${item}\n\
  But the minimum supported version is ${MIN_WEST_VERSION}\n\
  Please upgrade with:\n\
      pip3 install --upgrade west")
  endif()
  # Just output information for a single version. This will still work
  # even after output is one line.
  message(STATUS "Found west: ${WEST} (found suitable version \"${west_version}\", minimum required is \"${MIN_WEST_VERSION}\")")

    execute_process(
      COMMAND ${WEST}  topdir
      OUTPUT_VARIABLE  WEST_TOPDIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
      WORKING_DIRECTORY ${ZEPHYR_BASE}
      )
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
    string(REGEX MATCH "Version: DTC ([0-9]+[.][0-9]+[.][0-9]+).*" out_var ${dtc_version_output})

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

# TODO: Should we instead find one qemu binary for each ARCH?
# TODO: This will probably need to be re-organized when there exists more than one SDK.
