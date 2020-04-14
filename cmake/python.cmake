# SPDX-License-Identifier: Apache-2.0

# On Windows, instruct Python to output UTF-8 even when not
# interacting with a terminal. This is required since Python scripts
# are invoked by CMake code and, on Windows, standard I/O encoding defaults
# to the current code page if not connected to a terminal, which is often
# not what we want.
if (WIN32)
  set(ENV{PYTHONIOENCODING} "utf-8")
endif()

set(PYTHON_MINIMUM_REQUIRED 3.6)

# We are using foreach here, instead of find_program(PYTHON_EXECUTABLE_SYSTEM_DEFAULT "python" "python3")
# cause just using find_program directly could result in a python2.7 as python, and not finding a valid python3.
foreach(PYTHON_PREFER ${PYTHON_PREFER} "python" "python3")
  find_program(PYTHON_PREFER_EXECUTABLE ${PYTHON_PREFER})
  if(PYTHON_PREFER_EXECUTABLE)
      execute_process (COMMAND "${PYTHON_PREFER_EXECUTABLE}" -c
                               "import sys; sys.stdout.write('.'.join([str(x) for x in sys.version_info[:2]]))"
                       RESULT_VARIABLE result
                       OUTPUT_VARIABLE version
                       ERROR_QUIET
                       OUTPUT_STRIP_TRAILING_WHITESPACE)

     if(version VERSION_LESS PYTHON_MINIMUM_REQUIRED)
       set_property (CACHE PYTHON_PREFER_EXECUTABLE PROPERTY VALUE "PYTHON_PREFER_EXECUTABLE-NOTFOUND")
     else()
       set(PYTHON_MINIMUM_REQUIRED ${version})
       set(PYTHON_EXACT EXACT)
       break()
     endif()
  endif()
endforeach()

find_package(Python3 ${PYTHON_MINIMUM_REQUIRED} REQUIRED ${PYTHON_EXACT})
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
