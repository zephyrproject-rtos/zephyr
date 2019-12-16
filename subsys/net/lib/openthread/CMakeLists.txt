# SPDX-License-Identifier: Apache-2.0

add_subdirectory(platform)

# Obtain OpenThread repository root directory
execute_process(
    COMMAND
    ${WEST} list -f {posixpath} openthread
    OUTPUT_VARIABLE ot_root_dir OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Include OpenThread headers
zephyr_system_include_directories(${ot_root_dir}/include)
zephyr_system_include_directories(${ot_root_dir}/examples/platforms)

# Determine which libs should be linked in
set(ot_libs "")

if(CONFIG_OPENTHREAD_FTD)
set(cli_lib openthread-cli-ftd)
elseif(CONFIG_OPENTHREAD_MTD)
set(cli_lib openthread-cli-mtd)
endif()

if(CONFIG_OPENTHREAD_SHELL)
list(APPEND ot_libs ${cli_lib})
endif()

if(CONFIG_OPENTHREAD_FTD)
list(APPEND ot_libs openthread-ftd)
elseif(CONFIG_OPENTHREAD_MTD)
list(APPEND ot_libs openthread-mtd)
endif()

list(APPEND ot_libs openthread-platform-utils-static)

zephyr_link_libraries(${ot_libs})
