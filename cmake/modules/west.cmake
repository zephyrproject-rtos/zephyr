# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

include(python)

# west is an optional dependency. We need to run west using the same
# Python interpreter as everything else, though, so we play some extra
# tricks.

# When west runs cmake, it sets WEST_PYTHON to its interpreter. If
# it's defined, we assume west is installed. We do these checks here
# instead of in the failure paths below to avoid CMake warnings about
# WEST_PYTHON not being used.
if(DEFINED WEST_PYTHON)
  # Cut out any symbolic links, e.g. python3.x -> python
  get_filename_component(west_realpath ${WEST_PYTHON} REALPATH)
  get_filename_component(python_realpath ${PYTHON_EXECUTABLE} REALPATH)

  # If realpaths differ from the variables we're using, add extra
  # diagnostics.
  if(NOT ("${west_realpath}" STREQUAL "${WEST_PYTHON}"))
    set(west_realpath_msg " (real path ${west_realpath})")
  else()
    set(west_realpath_msg "")
  endif()
  if(NOT ("${python_realpath}" STREQUAL "${PYTHON_EXECUTABLE}"))
    set(python_realpath_msg " (real path ${python_realpath})")
  else()
    set(python_realpath_msg "")
  endif()
endif()

execute_process(
  COMMAND
  ${PYTHON_EXECUTABLE}
  -c
  "import west.version; print(west.version.__version__, end='')"
  OUTPUT_VARIABLE west_version
  ERROR_VARIABLE west_version_err
  RESULT_VARIABLE west_version_output_result
  )

if(west_version_output_result)
  if(DEFINED WEST_PYTHON)
    if(NOT (${west_realpath} STREQUAL ${python_realpath}))
      set(PYTHON_EXECUTABLE_OUT_OF_SYNC "\nOr verify these installations:\n\
  The Python version used by west is:  ${WEST_PYTHON}${west_realpath_msg}\n\
  The Python version used by CMake is: ${PYTHON_EXECUTABLE}${python_realpath_msg}")
    endif()

    message(FATAL_ERROR "Unable to import west.version from '${PYTHON_EXECUTABLE}':\n${west_version_err}\
Please install with:\n\
    ${PYTHON_EXECUTABLE} -m pip install west\
${PYTHON_EXECUTABLE_OUT_OF_SYNC}")
  else()
    # WEST_PYTHON is undefined and we couldn't import west. That's
    # fine; it's optional.
    set(WEST WEST-NOTFOUND CACHE INTERNAL "West")
  endif()
else()
  # We can import west from PYTHON_EXECUTABLE and have its version.

  # Make sure its version matches the minimum required one.
  set(MIN_WEST_VERSION 0.7.1)
  if(${west_version} VERSION_LESS ${MIN_WEST_VERSION})
    message(FATAL_ERROR "The detected west version, ${west_version}, is unsupported.\n\
  The minimum supported version is ${MIN_WEST_VERSION}.\n\
  Please upgrade with:\n\
      ${PYTHON_EXECUTABLE} -m pip install --upgrade west\
  ${PYTHON_EXECUTABLE_OUT_OF_SYNC}\n")
  endif()

  # Set WEST to a COMMAND prefix as if it were a find_program()
  # result.
  #
  # From west 0.8 forward, you can run 'python -m west' to run
  # the command line application.
  set(WEST_MODULE west)
  if(${west_version} VERSION_LESS 0.8)
    # In west 0.7.x, this wasn't supported yet, but it happens to be
    # possible to run 'python -m west.app.main'.
    string(APPEND WEST_MODULE .app.main)
  endif()

  # Need to cache this so the Zephyr Eclipse plugin knows
  # how to invoke West.
  set(WEST ${PYTHON_EXECUTABLE} -m ${WEST_MODULE} CACHE INTERNAL "West")

  # Print information about the west module we're relying on. This
  # will still work even after output is one line.
  message(STATUS "Found west (found suitable version \"${west_version}\", minimum required is \"${MIN_WEST_VERSION}\")")

  execute_process(
    COMMAND ${WEST} topdir
    OUTPUT_VARIABLE WEST_TOPDIR
    ERROR_QUIET
    RESULT_VARIABLE west_topdir_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY ${ZEPHYR_BASE}
    )

  if(west_topdir_result)
    # west topdir is undefined.
    # That's fine; west is optional, so could be custom Zephyr project.
    set(WEST WEST-NOTFOUND CACHE INTERNAL "West")
  endif()
endif()
