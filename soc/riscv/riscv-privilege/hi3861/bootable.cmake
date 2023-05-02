# Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

set(CMAKE_C_COMPILER_FORCED 1)
set(CMAKE_CXX_COMPILER_FORCED 1)

if(NOT PYTHON_EXECUTABLE)
  message(FATAL_ERROR "PYTHON_EXECUTABLE is not set")
endif()

if(NOT CROSS_COMPILE)
  message(FATAL_ERROR "CROSS_COMPILE is not set")
endif()

set(CMAKE_C_COMPILER ${CROSS_COMPILE}gcc)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++)
set(CMAKE_LINKER ${CROSS_COMPILE}ld)
set(CMAKE_OBJCOPY ${CROSS_COMPILE}objcopy)
set(CMAKE_SIZE ${CROSS_COMPILE}size)

set(SIGN_BIN_PY ${CMAKE_CURRENT_LIST_DIR}/sign_bin.py)

function(generate_binary target)
  add_custom_command(
    TARGET  ${target} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
    ARGS    -O binary
            -S
            -R .rom.text -R .rom.code.text
            -R .rom.data -R .rom.code.data
            -R .rom.bss  -R .rom.code.bss
            "$<TARGET_FILE:${target}>"
            "$<TARGET_NAME:${target}>.bin"
    COMMAND ${PYTHON_EXECUTABLE} ${SIGN_BIN_PY}
    ARGS    --binary "$<TARGET_NAME:${target}>.bin"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endfunction()

add_compile_options(
  -freorder-blocks-algorithm=simple -fno-schedule-insns -nostdinc
  -fno-aggressive-loop-optimizations -fno-builtin -fno-exceptions
  -fno-short-enums -mtune=size -msmall-data-limit=0 -Wall -Werror -Os -std=c99
  -falign-functions=2 -fdata-sections -ffunction-sections -fno-common
  -fstack-protector-strong
)

add_link_options(-nostdlib -nostartfiles)
add_link_options(-Wl,--gc-sections)
add_link_options(-Wl,--print-memory-usage)
