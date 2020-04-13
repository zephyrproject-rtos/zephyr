# SPDX-License-Identifier: Apache-2.0

# On Windows, instruct Python to output UTF-8 even when not
# interacting with a terminal. This is required since Python scripts
# are invoked by CMake code and, on Windows, standard I/O encoding defaults
# to the current code page if not connected to a terminal, which is often
# not what we want.
if (WIN32)
  set(ENV{PYTHONIOENCODING} "utf-8")
endif()

find_package(Python3 3.6 REQUIRED)
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
