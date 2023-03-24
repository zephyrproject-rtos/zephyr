# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

# On Windows, instruct Python to output UTF-8 even when not
# interacting with a terminal. This is required since Python scripts
# are invoked by CMake code and, on Windows, standard I/O encoding defaults
# to the current code page if not connected to a terminal, which is often
# not what we want.
if (WIN32)
  set(ENV{PYTHONIOENCODING} "utf-8")
endif()

set(PYTHON_MINIMUM_REQUIRED 3.8)

function(python_get_version interpreter)
  execute_process(COMMAND "${interpreter}" -c
    "import sys; sys.stdout.write('.'.join([str(x) for x in sys.version_info[:2]]))"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE version
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(PYTHON_VERSION "${version}" PARENT_SCOPE)
endfunction()

# If we're told exactly which Python executable to use, we should respect that,
# instead of using the default matching process below.
if(DEFINED PYTHON_EXECUTABLE)
  python_get_version("${PYTHON_EXECUTABLE}")
  if(PYTHON_VERSION VERSION_LESS PYTHON_MINIMUM_REQUIRED)
    message(FATAL_ERROR
      "Python executable ${PYTHON_EXECUTABLE} has version ${PYTHON_VERSION}, "
      "however, the minimum required version is ${PYTHON_MINIMUM_REQUIRED}.")
  endif()
  set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
else()
  # We are using foreach here, instead of find_program(PYTHON_EXECUTABLE_SYSTEM_DEFAULT "python" "python3")
  # cause just using find_program directly could result in a python2.7 as python, and not finding a valid python3.
  foreach(PYTHON_PREFER ${PYTHON_PREFER} ${WEST_PYTHON} "python" "python3")
    find_program(PYTHON_PREFER_EXECUTABLE ${PYTHON_PREFER})
    if(PYTHON_PREFER_EXECUTABLE)
      python_get_version("${PYTHON_PREFER_EXECUTABLE}")
      if(PYTHON_VERSION VERSION_LESS PYTHON_MINIMUM_REQUIRED)
        set(PYTHON_PREFER_EXECUTABLE "PYTHON_PREFER_EXECUTABLE-NOTFOUND")
      else()
        set(PYTHON_MINIMUM_REQUIRED "${PYTHON_VERSION}")
        set(PYTHON_EXACT EXACT)
        # Python3_ROOT_DIR ensures that location will be preferred by FindPython3.
        # On Linux, this has no impact as it will usually be /usr/bin
        # but on Windows it solve issues when both 32 and 64 bit versions are
        # installed, as version is not enough and FindPython3 might pick the
        # version not on %PATH%. Setting Python3_ROOT_DIR ensures we are using
        # the version we just tested.
        get_filename_component(PYTHON_PATH ${PYTHON_PREFER_EXECUTABLE} DIRECTORY)
        set(Python3_ROOT_DIR ${PYTHON_PATH})
        break()
      endif()
    endif()
  endforeach()

  find_package(Python3 ${PYTHON_MINIMUM_REQUIRED} REQUIRED ${PYTHON_EXACT})
  set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
endif()
