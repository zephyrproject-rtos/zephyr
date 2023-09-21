# SPDX-License-Identifier: Apache-2.0

# FIXME: SHADOW_VARS: Remove this once we have enabled -Wshadow globally.
add_compile_options($<TARGET_PROPERTY:compiler,warning_shadow_variables>)

add_subdirectory(crc)
if(NOT CONFIG_EXTERNAL_LIBC)
add_subdirectory(libc)
add_subdirectory(posix)
endif()
add_subdirectory_ifdef(CONFIG_CPP                  cpp)
add_subdirectory(hash)
add_subdirectory(os)
add_subdirectory_ifdef(CONFIG_SMF smf)
add_subdirectory_ifdef(CONFIG_OPENAMP              open-amp)
add_subdirectory_ifdef(CONFIG_ACPI acpi)
