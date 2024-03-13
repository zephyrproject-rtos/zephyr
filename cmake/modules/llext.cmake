# Copyright (c) 2024 Schneider Electric
# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

# Add a macro that add a llext extension.
# macro is based on zephyr_library template
#
# output an elf file usable by llext
#
# Code can be linked by 2 differents ways:
#     1. using partial link (-r flag in gcc and -fno-pic) (default)
#     2. using shared link (-shared flag in gcc and -fpic)
#
# The code will be compiled with mostly the same C compiler flags used
# in the Zephyr build, but with some important modifications.
# Toolchain flag are automatically inherited and filtered.
#
# NOPIC:
#     -mlong-calls flag is always added
#     libc is detected from zephyr configuration and libc function code is embedded in extension
# PIC:
#    code is compiled and linked with -fpic flag
#    libc is detected from zephyr configuration and libc function are called. They are external symbol for extension.
#
# Example usage:
#     zephyr_llext(hello_world)
#     zephyr_llext_sources(hello_world.c hello_world2.c)
#     zephyr_llext_include_directories(.)
# will compile the source file hello_world.c and hello_world2.c to
# ${PROJECT_BINARY_DIR}/hello_world.llext
#


macro(zephyr_llext name)
    set(LLEXT_IS_PIC no)
    set(ZEPHYR_CURRENT_LLEXT ${name})
    set(ZEPHYR_LLEXT_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/linker/llext.ld)

    # ARGN is not a variable: assign its value to a variable
    set(ExtraMacroArgs ${ARGN})
    # Get the length of the list
    list(LENGTH ExtraMacroArgs NumExtraMacroArgs)
    # Execute the following block only if the length is > 0
    if(NumExtraMacroArgs GREATER 0)
        foreach(ExtraArg ${ExtraMacroArgs})
            if(${ExtraArg} STREQUAL PIC)
                set(LLEXT_IS_PIC yes)
            endif()
        endforeach()
    endif()

    add_executable(${ZEPHYR_CURRENT_LLEXT})

    set_property(TARGET ${ZEPHYR_CURRENT_LLEXT} APPEND PROPERTY LINK_DEPENDS ${ZEPHYR_LLEXT_LINKER_SCRIPT})

    target_compile_definitions(${ZEPHYR_CURRENT_LLEXT}
        PRIVATE $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>
        PRIVATE ZEPHYR_LLEXT
    )

    target_compile_options(${ZEPHYR_CURRENT_LLEXT}
        PRIVATE $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>
    )

    if(LLEXT_IS_PIC)
        target_compile_options(${ZEPHYR_CURRENT_LLEXT} PRIVATE -fpic -fpie)
    else()
        if("${ARCH}" STREQUAL "arm")
            target_compile_options(${ZEPHYR_CURRENT_LLEXT} PRIVATE -mlong-calls)
        endif()
    endif()

    target_include_directories(${ZEPHYR_CURRENT_LLEXT}
        PUBLIC $<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_include_directories(${ZEPHYR_CURRENT_LLEXT} SYSTEM
    PUBLIC $<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
    )

    # copy toolchain link flag
    set(LD_FLAGS ${TOOLCHAIN_LD_FLAGS})
    list(REMOVE_ITEM LD_FLAGS NO_SPLIT)

    set_target_properties(${ZEPHYR_CURRENT_LLEXT} PROPERTIES LINK_DEPENDS ${ZEPHYR_LLEXT_LINKER_SCRIPT})
    get_property(LIBC_LINK_LIBRARIES TARGET zephyr_interface PROPERTY LIBC_LINK_LIBRARIES)

    if(LLEXT_IS_PIC)
        # to use -shared, we need libc build with -fpic or not used libc
        target_link_options(${ZEPHYR_CURRENT_LLEXT} PRIVATE -shared)
    else()
        target_link_options(${ZEPHYR_CURRENT_LLEXT} PRIVATE -r)
    endif()

    target_link_options(${ZEPHYR_CURRENT_LLEXT} PRIVATE
        -n # disable section alignment
        -e start # no entry point
        -T ${ZEPHYR_LLEXT_LINKER_SCRIPT}
        -Wl,-Map=$<TARGET_FILE_BASE_NAME:${ZEPHYR_CURRENT_LLEXT}>.map
        ${LD_FLAGS}
    )
    if(LLEXT_IS_PIC)
        # libc is not linked shared so we cannot link with it
        target_link_options(${ZEPHYR_CURRENT_LLEXT} PRIVATE -nolibc -nostartfiles -fpic -fpie)
        target_link_libraries(${ZEPHYR_CURRENT_LLEXT} PRIVATE gcc)
    else()
        # TODO add newlib and newlib nano support
        if(CONFIG_PICOLIBC)
            target_sources(${ZEPHYR_CURRENT_LLEXT} PRIVATE ${ZEPHYR_BASE}/lib/libc/picolibc/libc-hooks.c)
            target_link_options(${ZEPHYR_CURRENT_LLEXT} PRIVATE -DPICOLIBC_INTEGER_PRINTF_SCANF)
        endif()

        target_link_libraries(${ZEPHYR_CURRENT_LLEXT} PRIVATE ${LIBC_LINK_LIBRARIES})
    endif()

    # strip llext
    add_custom_target(${ZEPHYR_CURRENT_LLEXT}.llext ALL
        COMMAND ${CMAKE_OBJCOPY} --strip-unneeded $<TARGET_FILE:${ZEPHYR_CURRENT_LLEXT}> ${ZEPHYR_CURRENT_LLEXT}.llext
        DEPENDS ${ZEPHYR_CURRENT_LLEXT}
    )

    add_dependencies(${ZEPHYR_CURRENT_LLEXT}
        zephyr_interface
        zephyr_generated_headers
    )

endmacro()

function(zephyr_llext_sources source)
  target_sources(${ZEPHYR_CURRENT_LLEXT} PRIVATE ${source} ${ARGN})
endfunction()

function(zephyr_llext_include_directories)
    target_include_directories(${ZEPHYR_CURRENT_LLEXT} PRIVATE ${ARGN})
endfunction()
