# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_EVENTFD eventfd)
add_subdirectory_ifdef(CONFIG_POSIX_C_LANG_SUPPORT_R c_lang_support_r)
add_subdirectory_ifdef(CONFIG_POSIX_C_LIB_EXT c_lib_ext)
add_subdirectory_ifdef(CONFIG_POSIX_SHELL shell)
add_subdirectory_ifdef(CONFIG_POSIX_SYSTEM_INTERFACES options)
# zephyr-keep-sorted-stop
