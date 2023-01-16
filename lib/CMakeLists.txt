# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_EXTERNAL_LIBC)
add_subdirectory(libc)
add_subdirectory(posix)
endif()
add_subdirectory_ifdef(CONFIG_CPP                  cpp)
add_subdirectory(os)
add_subdirectory(util)
add_subdirectory_ifdef(CONFIG_SMF smf)
add_subdirectory_ifdef(CONFIG_OPENAMP              open-amp)
