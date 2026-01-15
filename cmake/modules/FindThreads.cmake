# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Google LLC.

#[=======================================================================[.rst:
FindThreads
***********

Find the native threading library.

This module provides a unified interface for finding and linking against the native threading
library on the target platform.

Variables
=========

The following variables will be defined when this module completes:

.. cmake:variable:: Threads_FOUND

   Set to True if the threading library was found.

.. cmake:variable:: CMAKE_THREAD_LIBS_INIT

   The threading library to link against.

.. cmake:variable:: CMAKE_USE_WIN32_THREADS_INIT

   Set to True if using Windows threads.

.. cmake:variable:: CMAKE_USE_PTHREADS_INIT

   Set to True if using POSIX threads.

Usage
=====

This module is compatible with :cmake:module:`FindThreads <module:FindThreads>` module from CMake.
The original implementation tries to find threads library using various methods (e.g. checking if
pthread library is present or compiling example program to check if the implementation is provided
by libc), but it's not able to detect pthread implementation provided by Zephyr.

Example
-------

.. code-block:: cmake

   find_package(Threads REQUIRED)
   if(Threads_FOUND)
      target_link_libraries(my_target PRIVATE Threads::Threads)
   endif()
#]=======================================================================]

find_package_handle_standard_args(Threads
  DEFAULT_MSG
  Threads_FOUND
)

if(Threads_FOUND AND NOT TARGET Threads::Threads)
  # This is just an empty interface, because we don't need to provide any
  # options. Nevertheless this library must exist, because third-party modules
  # can link it to their own libraries.
  add_library(Threads::Threads INTERFACE IMPORTED)
endif()
