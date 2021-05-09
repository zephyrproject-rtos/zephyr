# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_EXTERNAL_LIBC)
add_subdirectory(libc)
add_subdirectory(posix)
endif()
add_subdirectory_ifdef(CONFIG_CMSIS_RTOS_V1        cmsis_rtos_v1)
add_subdirectory_ifdef(CONFIG_CMSIS_RTOS_V2        cmsis_rtos_v2)
add_subdirectory(gui)
add_subdirectory(os)
add_subdirectory(util)
add_subdirectory_ifdef(CONFIG_OPENAMP              open-amp)
