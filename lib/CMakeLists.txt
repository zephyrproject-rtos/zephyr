# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_NATIVE_APPLICATION)
add_subdirectory(libc)
endif()
add_subdirectory_ifdef(CONFIG_POSIX_API            posix)
add_subdirectory(cmsis_rtos)
add_subdirectory(gui)
add_subdirectory(os)
