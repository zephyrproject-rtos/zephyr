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
endif()

# Set the strategy of the FindPython module to LOCATION to find the first found
# python that matches the requirements rather than the most recent
set(Python_FIND_STRATEGY "LOCATION")

# Prefer PATH over Registry entries for Windows
set(Python_FIND_REGISTRY "LAST")

find_package(Python 3.8...3.12 COMPONENTS Interpreter)

# Set the Zephyr internally used Python variable.
set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
