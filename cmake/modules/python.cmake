# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

message("Finding python interpreter using CMake's FindPython,
if the incorrect interpreter is found setting Python_EXECUTABLE
will explicitly specify the correct interpreter to use")

# On Windows, instruct Python to output UTF-8 even when not
# interacting with a terminal. This is required since Python scripts
# are invoked by CMake code and, on Windows, standard I/O encoding defaults
# to the current code page if not connected to a terminal, which is often
# not what we want.
if (WIN32)
  set(ENV{PYTHONIOENCODING} "utf-8")
  set(PYTHON_MINIMUM_REQUIRED 3.8)

  find_package(Deprecated COMPONENTS PYTHON_PREFER)

  if(NOT DEFINED Python3_EXECUTABLE AND DEFINED WEST_PYTHON)
    set(Python3_EXECUTABLE "${WEST_PYTHON}")
  endif()

  if(NOT Python3_EXECUTABLE)
    # We are using foreach here, instead of
    # find_program(PYTHON_EXECUTABLE_SYSTEM_DEFAULT "python" "python3")
    # cause just using find_program directly could result in a python2.7 as python,
    # and not finding a valid python3.
    foreach(candidate "python" "python3")
      find_program(Python3_EXECUTABLE ${candidate})
      if(Python3_EXECUTABLE)
          execute_process (COMMAND "${Python3_EXECUTABLE}" -c
                                   "import sys; sys.stdout.write('.'.join([str(x) for x in sys.version_info[:2]]))"
                           RESULT_VARIABLE result
                           OUTPUT_VARIABLE version
                           OUTPUT_STRIP_TRAILING_WHITESPACE)

         if(version VERSION_LESS PYTHON_MINIMUM_REQUIRED)
           set(Python3_EXECUTABLE "Python3_EXECUTABLE-NOTFOUND" CACHE INTERNAL "Path to a program")
         endif()
      endif()
    endforeach()
  endif()

  find_package(Python3 ${PYTHON_MINIMUM_REQUIRED} REQUIRED)

  # Zephyr internally used Python variable.
  set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})

else()
  # Set the strategy of the FindPython module to LOCATION to find the first found
  # python that matches the requirements rather than the most recent
  set(Python_FIND_STRATEGY "LOCATION")

  find_package(Python 3.8...3.12 COMPONENTS Interpreter)

  # Set the Zephyr internally used Python variable.
  set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
endif()
