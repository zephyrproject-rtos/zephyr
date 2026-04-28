cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_C_COMPILER   arm-zephyr-eabi-gcc)
set(CMAKE_FIND_ROOT_PATH $ENV{ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

project(extension)

include($ENV{LLEXT_EDK_INSTALL_DIR}/cmake.cflags)

# Add LLEXT_CFLAGS to our flags
add_compile_options(${LLEXT_CFLAGS})
add_compile_options("-Werror")
add_compile_options("-c")

# Get flags from COMPILE_OPTIONS
get_property(COMPILE_OPTIONS_PROP DIRECTORY PROPERTY COMPILE_OPTIONS)

add_custom_command(
    OUTPUT
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.llext
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.inc
    COMMAND ${CMAKE_C_COMPILER} ${COMPILE_OPTIONS_PROP}
        -o ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.llext
        ${PROJECT_SOURCE_DIR}/src/main.c
    COMMAND
        ${PYTHON_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/file2hex.py
        --file ${PROJECT_NAME}.llext
        > ${PROJECT_NAME}.inc
)

add_custom_target(extension ALL DEPENDS ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.llext)
