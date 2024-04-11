# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Google LLC.

# FindThreads module for locating threads implementation.
#
# The module defines the following variables:
#
# 'Threads_FOUND'
# Indicates if threads are supported.
#
# 'CMAKE_THREAD_LIBS_INIT'
# The threads library to use. Zephyr provides threads implementation and no
# special flags are needed, so this will be empty.
#
# 'CMAKE_USE_PTHREADS_INIT'
# Indicates if threads are pthread compatible.
#
# This module is compatible with FindThreads module from CMake.
# The original implementation tries to find threads library using various
# methods (e.g. checking if pthread library is present or compiling example
# program to check if the implementation is provided by libc), but it's not
# able to detect pthread implementation provided by Zephyr.

include(FindPackageHandleStandardArgs)

set(Threads_FOUND FALSE)

if(DEFINED CONFIG_PTHREAD)
  set(Threads_FOUND TRUE)
  set(CMAKE_THREAD_LIBS_INIT )
  set(CMAKE_USE_PTHREADS_INIT 1)
endif()

find_package_handle_standard_args(Threads DEFAULT_MSG Threads_FOUND)

if(Threads_FOUND AND NOT TARGET Threads::Threads)
  # This is just an empty interface, because we don't need to provide any
  # options. Nevertheless this library must exist, because third-party modules
  # can link it to their own libraries.
  add_library(Threads::Threads INTERFACE IMPORTED)
endif()
