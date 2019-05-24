# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_NATIVE_APPLICATION)
add_subdirectory(libc)
endif()
add_subdirectory_ifdef(CONFIG_POSIX_API            posix)
add_subdirectory_ifdef(CONFIG_CMSIS_RTOS_V1        cmsis_rtos_v1)
add_subdirectory_ifdef(CONFIG_CMSIS_RTOS_V2        cmsis_rtos_v2)
add_subdirectory(gui)
add_subdirectory(os)
