cmake_minimum_required(VERSION 3.20.0)

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
    COMMAND ${CMAKE_C_COMPILER} ${COMPILE_OPTIONS_PROP}
        -o ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.llext
        ${PROJECT_SOURCE_DIR}/src/main.c
)

add_custom_target(extension ALL DEPENDS ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.llext)
