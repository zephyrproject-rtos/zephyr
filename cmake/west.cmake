# SPDX-License-Identifier: Apache-2.0

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

  if(WEST_PYTHON)
    get_filename_component(WEST_PYTHON ${WEST_PYTHON} ABSOLUTE)
    if(NOT (${WEST_PYTHON} STREQUAL ${PYTHON_EXECUTABLE}))
      set(PYTHON_EXECUTABLE_OUT_OF_SYNC "\nNote:\n\
  The Python version used by west is:  ${WEST_PYTHON}\n\
  The Python version used by CMake is: ${PYTHON_EXECUTABLE}\n\
  This might be correct, but please verify your installation.")
    endif()
  endif()

  if(west_version_output_result)
    message(FATAL_ERROR "Unable to import west.version from '${PYTHON_EXECUTABLE}'\n\
  Please install with:\n\
      ${PYTHON_EXECUTABLE} -m pip install west\
  ${PYTHON_EXECUTABLE_OUT_OF_SYNC}")
  endif()

  if(${west_version} VERSION_LESS ${MIN_WEST_VERSION})
    message(FATAL_ERROR "The detected west version is unsupported.\n\
  The version was found to be ${west_version}:\n\
    ${item}\n\
  But the minimum supported version is ${MIN_WEST_VERSION}\n\
  Please upgrade with:\n\
      ${PYTHON_EXECUTABLE} -m pip install --upgrade west\
  ${PYTHON_EXECUTABLE_OUT_OF_SYNC}\n")
  endif()

  # Just output information for a single version. This will still work
  # even after output is one line.
  message(STATUS "Found west: ${WEST} (found suitable version \"${west_version}\", minimum required is \"${MIN_WEST_VERSION}\")")

  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c
    "import pathlib; from west.util import west_topdir; print(pathlib.Path(west_topdir()).as_posix())"
    OUTPUT_VARIABLE  WEST_TOPDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY ${ZEPHYR_BASE}
    )
endif()
