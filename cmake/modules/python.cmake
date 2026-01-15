# SPDX-License-Identifier: Apache-2.0

#[=======================================================================[.rst:
python
######

Configure Python for use in Zephyr build system.

This module finds and configures Python for use in the Zephyr build system.
It ensures a compatible Python version is available and sets up necessary
environment variables and CMake variables.

Variables
*********

The :cmake:variable:`PYTHON_MINIMUM_REQUIRED` variable is used to specify the minimum required
Python version.

The following variables will be defined when this CMake module completes:

* :cmake:variable:`PYTHON_EXECUTABLE`

Search Process
==============

The module searches for Python in the following order:

1. Using the ``Python3_EXECUTABLE`` if already defined
2. Using the ``WEST_PYTHON`` if defined
3. Searching for "python" or "python3" in the system PATH and virtual environment
4. Using CMake's ``find_package(Python3)`` mechanism

The module verifies that any found Python meets the minimum version requirement.

Windows-Specific Configuration
==============================

On Windows systems, the module sets the ``PYTHONIOENCODING`` environment variable
to "utf-8" to ensure consistent encoding behavior when Python is invoked from CMake.

Example Usage
=============

.. code-block:: cmake

   include(python)

   # Now PYTHON_EXECUTABLE can be used to invoke Python scripts
   execute_process(COMMAND ${PYTHON_EXECUTABLE} my_script.py)

#]=======================================================================]

include_guard(GLOBAL)

# On Windows, instruct Python to output UTF-8 even when not
# interacting with a terminal. This is required since Python scripts
# are invoked by CMake code and, on Windows, standard I/O encoding defaults
# to the current code page if not connected to a terminal, which is often
# not what we want.
if(WIN32)
  set(ENV{PYTHONIOENCODING} "utf-8")
endif()

set(PYTHON_MINIMUM_REQUIRED 3.10)

if(NOT DEFINED Python3_EXECUTABLE AND DEFINED WEST_PYTHON)
  set(Python3_EXECUTABLE "${WEST_PYTHON}")
endif()

if(NOT Python3_EXECUTABLE)
  # We are using foreach here, instead of
  # find_program(PYTHON_EXECUTABLE_SYSTEM_DEFAULT "python" "python3")
  # cause just using find_program directly could result in a python2.7 as python,
  # and not finding a valid python3.
  foreach(candidate "python" "python3")
    find_program(Python3_EXECUTABLE ${candidate} PATHS ENV VIRTUAL_ENV NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH)
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
