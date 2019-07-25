# SPDX-License-Identifier: Apache-2.0

# On Windows, instruct Python to output UTF-8 even when not
# interacting with a terminal. This is required since Python scripts
# are invoked by CMake code and, on Windows, standard I/O encoding defaults
# to the current code page if not connected to a terminal, which is often
# not what we want.
if (WIN32)
  set(ENV{PYTHONIOENCODING} "utf-8")
endif()

# The 'FindPythonInterp' that is distributed with CMake 3.8 has a bug
# that we need to work around until we upgrade to 3.13. Until then we
# maintain a patched copy in our repo. Bug:
# https://github.com/zephyrproject-rtos/zephyr/issues/11103
set(PythonInterp_FIND_VERSION 3.4)
set(PythonInterp_FIND_VERSION_COUNT 2)
set(PythonInterp_FIND_VERSION_MAJOR 3)
set(PythonInterp_FIND_VERSION_MINOR 4)
set(PythonInterp_FIND_VERSION_EXACT 0)
set(PythonInterp_FIND_REQUIRED 1)
include(${ZEPHYR_BASE}/cmake/backports/FindPythonInterp.cmake)
